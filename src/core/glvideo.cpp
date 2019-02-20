/*
 * glvideo.cpp
 *
 *  Created on: Feb 13, 2019
 *      Author: ubuntu
 */

#include <opencv2/opencv.hpp>
#include <osa_buf.h>
#include <glew.h>
#include <glut.h>
#include <freeglut_ext.h>
#include "osa_image_queue.h"
#include <cuda.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime_api.h>
#include "cuda_mem.hpp"
#include <linux/videodev2.h>
#include "glvideo.hpp"

__inline__ int GetChannels(int format)
{
	return (format==V4L2_PIX_FMT_GREY) ? 1 : 3;
}

CGLVideo::CGLVideo(int idx, const cv::Size& imgSize, int format, int fps, int nDrop, int memType)
:m_idx(idx),m_nDrop(nDrop), m_size(imgSize),m_format(format),m_fps(fps),m_memType(memType),textureId(0),nCnt(0ul),m_pbo(-1)
{
	memset(&m_bufQue, 0, sizeof(m_bufQue));
	m_matrix = cv::Matx44f::eye();
	//m_nDrop = m_disFPS/m_fps;
	int channels = GetChannels(format);
	int iRet = image_queue_create(&m_bufQue, 3, m_size.width*m_size.height*channels,m_memType);
	OSA_assert(iRet == OSA_SOK);
	//OSA_printf("%s %d: textureId = %d", __func__, __LINE__, textureId);
	glGenTextures(1, &textureId);
	//OSA_printf("%s %d: textureId = %d", __func__, __LINE__, textureId);
	//assert(glIsTexture(textureId));
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);//GL_NEAREST);//GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);//GL_NEAREST);//GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//GL_CLAMP);//GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);//GL_CLAMP);//GL_CLAMP_TO_EDGE);

	if(channels == 1)
		glTexImage2D(GL_TEXTURE_2D, 0, channels, m_size.width, m_size.height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, channels, m_size.width, m_size.height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, NULL);

	if(memtype_glpbo != m_memType)
	{
		glGenBuffers(1, &m_pbo);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, m_size.width*m_size.height*channels, NULL, GL_DYNAMIC_COPY);//GL_STATIC_DRAW);//GL_DYNAMIC_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}
	m_channels = channels;
}

CGLVideo::~CGLVideo()
{
	image_queue_delete(&m_bufQue);
	glDeleteTextures(1, &textureId);
	if(m_pbo>0)
		glDeleteBuffers(1, &m_pbo);
}

void CGLVideo::update()
{
	if(m_bufQue.bMap){
		for(int i=0; i<m_bufQue.numBuf; i++){
			cuMap(&m_bufQue.bufInfo[i]);
			image_queue_putEmpty(&m_bufQue, &m_bufQue.bufInfo[i]);
		}
		m_bufQue.bMap = false;
	}

	bool bDevMem = false;
	OSA_BufInfo* info = NULL;
	cv::Mat img;
	nCnt ++;
	if(nCnt > 300){
		int count = OSA_bufGetFullCount(&m_bufQue);
		nCnt = 1;
		if(count>1){
			OSA_printf("[%d]%s: ch%d queue count = %d, sync", OSA_getCurTimeInMsec(), __func__, m_idx, count);
			while(count>1){
				//image_queue_switchEmpty(&m_bufQue[chId]);
				info = image_queue_getFull(&m_bufQue);
				OSA_assert(info != NULL);
				image_queue_putEmpty(&m_bufQue, info);
				count = OSA_bufGetFullCount(&m_bufQue);
			}
		}
	}

	if(m_nDrop>1 && (nCnt%m_nDrop)!=1)
		return;

	info = image_queue_getFull(&m_bufQue);
	if(info != NULL)
	{
		GLuint pbo = 0;
		bDevMem = (info->memtype == memtype_cudev || info->memtype == memtype_cumap);
		void *data = (bDevMem) ? info->physAddr : info->virtAddr;
		if(info->channels == 1){
			img = cv::Mat(info->height, info->width, CV_8UC1, data);
		}else{
			img = cv::Mat(info->height, info->width, CV_8UC3, data);
		}
		m_tmBak = info->timestampCap;

		OSA_assert(img.cols > 0 && img.rows > 0);
		if(bDevMem)
		{
			unsigned int byteCount = img.cols*img.rows*img.channels();
			unsigned char *dev_pbo = NULL;
			size_t tmpSize;
			pbo = m_pbo;
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
			cudaResource_RegisterBuffer(m_idx, pbo, byteCount);
			cudaResource_mapBuffer(m_idx, (void **)&dev_pbo, &tmpSize);
			assert(tmpSize == byteCount);
			cudaMemcpy(dev_pbo, img.data, byteCount, cudaMemcpyDeviceToDevice);
			cudaDeviceSynchronize();
			cudaResource_unmapBuffer(m_idx);
			cudaResource_UnregisterBuffer(m_idx);
			img.data = NULL;
		}else if(info->memtype == memtype_glpbo){
			cuUnmap(info);
			pbo = info->pbo;
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
		}else{
			unsigned int byteCount = img.cols*img.rows*img.channels();
			unsigned char *dev_pbo = NULL;
			size_t tmpSize;
			pbo = m_pbo;
			OSA_assert(img.data != NULL);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
			cudaResource_RegisterBuffer(m_idx, pbo, byteCount);
			cudaResource_mapBuffer(m_idx, (void **)&dev_pbo, &tmpSize);
			assert(tmpSize == byteCount);
			cudaMemcpy(dev_pbo, img.data, byteCount, cudaMemcpyHostToDevice);
			cudaDeviceSynchronize();
			cudaResource_unmapBuffer(m_idx);
			cudaResource_UnregisterBuffer(m_idx);
		}
		glBindTexture(GL_TEXTURE_2D, textureId);
		if(img.channels() == 1)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, GL_RED, GL_UNSIGNED_BYTE, NULL);
		else //if(img.channels() == 3)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, GL_BGR_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		if(info->memtype == memtype_glpbo){
			cuMap(info);
		}
		image_queue_putEmpty(&m_bufQue, info);
	}
}
