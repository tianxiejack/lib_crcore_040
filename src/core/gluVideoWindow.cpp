/*
 * gluVideoWindow.cpp
 *
 *  Created on: Feb 14, 2019
 *      Author: ubuntu
 */

#include <linux/videodev2.h>
#include "osa_image_queue.h"
#include "gluVideoWindow.hpp"
#include "glvideo.hpp"

using namespace cv;
using namespace cr_osa;

static std::vector<CGLVideo *> glvvideos;
static int raf = 0;

CGluVideoWindow::CGluVideoWindow(const cv::Rect& rc, int parent, int nRender):CGluWindow(rc, parent),m_nRender(nRender),
		m_tmRender(0ul),m_telapse(5.0),m_nSwapTimeOut(0)
{
	memset(m_bufQue, 0, sizeof(m_bufQue));
	memset(&m_initPrm, 0, sizeof(m_initPrm));
	memset(m_tmBak, 0, sizeof(m_tmBak));
	m_initPrm.disSched = 3.5;
	for(int chId = 0; chId<VWIND_CHAN_MAX; chId++){
		m_blendMap[chId] = -1;
		m_maskMap[chId] = -1;
	}

	for(int i=0; i<VWIND_CHAN_MAX*VWIND_CHAN_MAX; i++){
		m_glmat44fBlend[i] = cv::Matx44f::eye();
		m_glBlendPrm[i].fAlpha = 0.5f;
		m_glBlendPrm[i].thr0Min = 0;
		m_glBlendPrm[i].thr0Max = 0;
		m_glBlendPrm[i].thr1Min = 0;
		m_glBlendPrm[i].thr1Max = 0;
	}
	OSA_mutexCreate(&m_mutex);
}

CGluVideoWindow::~CGluVideoWindow()
{
	OSA_mutexDelete(&m_mutex);
}

int CGluVideoWindow::setFPS(int fps)
{
    if (fps == 0)
    {
        OSA_printf("[Render]Fps 0 is not allowed. Not changing fps");
        return -1;
    }
    pthread_mutex_lock(&render_lock);
    m_initPrm.disFPS = fps;
    m_interval = (1000000000ul)/(uint64)m_initPrm.disFPS;
    render_time_sec = m_interval / 1000000000ul;
    render_time_nsec = (m_interval % 1000000000ul);
    memset(&last_render_time, 0, sizeof(last_render_time));
    pthread_mutex_unlock(&render_lock);
    return 0;
}

int CGluVideoWindow::Create(const VWIND_Prm& param)
{
	int iRet = CGluWindow::Create(param.bFullScreen);

	memcpy(&m_initPrm, &param, sizeof(VWIND_Prm));
    pthread_mutex_init(&render_lock, NULL);
    pthread_cond_init(&render_cond, NULL);
    setFPS(m_initPrm.disFPS);

	OSA_mutexLock(&m_mutex);
	if(glvvideos.size()==0){
		for(int chId=0; chId<param.nChannels; chId++){
			int nDrop = (param.channelInfo[chId].fps == 0) ? 0 : param.disFPS/param.channelInfo[chId].fps;
			CGLVideo *pVideo = new CGLVideo(chId, cv::Size(param.channelInfo[chId].w, param.channelInfo[chId].h),
					(param.channelInfo[chId].c == 1) ? V4L2_PIX_FMT_GREY : V4L2_PIX_FMT_RGB24,
					param.channelInfo[chId].fps, nDrop, param.memType);
			glvvideos.push_back(pVideo);
			m_vvideos.push_back(pVideo);
			m_bufQue[chId] = &pVideo->m_bufQue;
		}
	}else{
		for(int chId=0; chId<glvvideos.size(); chId++){
			m_vvideos.push_back(glvvideos[chId]);
			m_bufQue[chId] = &glvvideos[chId]->m_bufQue;
		}
	}
	raf ++;
	OSA_mutexUnlock(&m_mutex);

	for(int i=0; i<m_nRender; i++){
		CGLVideoBlendRender *render = CreateVedioRender(i);
		OSA_assert(render != NULL);
		vvideoRenders.push_back(render);
	}

	if(!CGLVideoRender::m_bLoadProgram)
		iRet = CGLVideoRender::gl_loadProgram();

	OSA_assert(iRet == OSA_SOK);

	return iRet;
}

void CGluVideoWindow::Destroy()
{
	OSA_printf("[%d] %s %d: GLU%d enter", OSA_getCurTimeInMsec(), __func__, __LINE__, m_winId);
	CGluWindow::Destroy();
	OSA_mutexLock(&m_mutex);
	raf --;
	if(raf == 0)
	{
		int nChannels = glvvideos.size();
		for(int chId=0; chId<nChannels; chId++){
			CGLVideo *pVideo = glvvideos[chId];
			delete pVideo;
		}
		glvvideos.clear();
	}
	m_vvideos.clear();
	for(int i=0; i<m_nRender; i++){
		CGLVideoBlendRender *render = vvideoRenders[i];
		delete render;
	}
	vvideoRenders.clear();
	OSA_mutexUnlock(&m_mutex);
    pthread_mutex_lock(&render_lock);
    pthread_cond_broadcast(&render_cond);
    pthread_mutex_unlock(&render_lock);
    pthread_mutex_destroy(&render_lock);
    pthread_cond_destroy(&render_cond);
}

void CGluVideoWindow::Display()
{
	//OSA_printf("[%d] %s %d: GLU%d enter", OSA_getCurTimeInMsec(), __func__, __LINE__, m_winId);
	OSA_mutexLock(&m_mutex);
	int chId, winId;
	for(chId = 0; chId<VWIND_CHAN_MAX; chId++){
		if(m_bufQue[chId] != NULL && m_bufQue[chId]->bMap){
			for(int i=0; i<m_bufQue[chId]->numBuf; i++){
				cuMap(&m_bufQue[chId]->bufInfo[i]);
				image_queue_putEmpty(m_bufQue[chId], &m_bufQue[chId]->bufInfo[i]);
			}
			m_bufQue[chId]->bMap = false;
		}
	}
	OSA_mutexUnlock(&m_mutex);

	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_MULTISAMPLE);

	int64 tStamp[10];
	tStamp[0] = getTickCount();
	static int64 tstart = 0ul, tend = 0ul;
	tstart = tStamp[0];
	if(tend == 0ul)
		tend = tstart;

	if(1)
	{
		uint64_t telapse_ns = (uint64_t)(m_telapse*1000000.f);
		uint64_t sleep_ns = render_time_nsec - telapse_ns;
		if (last_render_time.tv_sec != 0 && render_time_nsec>telapse_ns)
		{
			pthread_mutex_lock(&render_lock);
			struct timespec waittime = last_render_time;
			waittime.tv_nsec += sleep_ns;
			waittime.tv_sec += waittime.tv_nsec / 1000000000UL;
			waittime.tv_nsec %= 1000000000UL;
			pthread_cond_timedwait(&render_cond, &render_lock,&waittime);
			pthread_mutex_unlock(&render_lock);
		}
		else{
			//OSA_printf("%s %d: last_render_time.tv_sec=%ld render_time_nsec=%ld telapse_ns=%ld sleep_ns=%ld",
			//		__func__, __LINE__, last_render_time.tv_sec, render_time_nsec, telapse_ns, sleep_ns);
		}
	}

	tStamp[1] = getTickCount();

	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(0, RUN_ENTER, 0, 0);
	tStamp[2] = getTickCount();

	OSA_mutexLock(&m_mutex);
	int nChannels = m_vvideos.size();
	for(chId=0; chId<nChannels; chId++){
		CGLVideo *pVideo = m_vvideos[chId];
		pVideo->update();
	}
	OSA_mutexUnlock(&m_mutex);
	tStamp[3] = getTickCount();

	tStamp[4] = getTickCount();

	OSA_mutexLock(&m_mutex);
	OSA_assert(m_nRender == vvideoRenders.size());
	for(winId=0; winId<m_nRender; winId++){
		CGLVideoBlendRender *render = vvideoRenders[winId];
		OSA_assert(render != NULL);
		if(render->m_video == NULL)
			continue;
		chId = render->m_video->m_idx;
		if(chId < 0 || chId >= VWIND_CHAN_MAX)
			continue;
		int blend_chId = m_blendMap[chId];
		if(blend_chId>=0){
			render->blend(m_vvideos[blend_chId]);
			render->matrix(m_glmat44fBlend[chId*VWIND_CHAN_MAX+blend_chId]);
			render->params(m_glBlendPrm[chId*VWIND_CHAN_MAX+blend_chId]);
		}
		render->render();
		if(m_initPrm.renderfunc != NULL)
			m_initPrm.renderfunc(0, RUN_WIN, winId, chId);
	}
	OSA_mutexUnlock(&m_mutex);

	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(0, RUN_SWAP, 0, 0);

	tStamp[5] = getTickCount();
	int64 tcur = tStamp[5];
	m_telapse = (tStamp[5] - tStamp[1])*0.000001f + m_initPrm.disSched;

	if (last_render_time.tv_sec != 0)
	{
		pthread_mutex_lock(&render_lock);
		struct timespec waittime = last_render_time;
		last_render_time.tv_sec += render_time_sec;
		last_render_time.tv_nsec += render_time_nsec;
		last_render_time.tv_sec += last_render_time.tv_nsec / 1000000000UL;
		last_render_time.tv_nsec %= 1000000000UL;
        struct timeval now;
        gettimeofday(&now, NULL);
        __suseconds_t cur_us, rd_us;
        cur_us = (now.tv_sec * 1000000.0 + now.tv_usec);
        rd_us = (last_render_time.tv_sec * 1000000.0 + last_render_time.tv_nsec / 1000.0);
        if (rd_us>cur_us)
        {
    		waittime.tv_sec += render_time_sec;
    		waittime.tv_nsec += render_time_nsec-500000UL;
    		waittime.tv_sec += waittime.tv_nsec / 1000000000UL;
    		waittime.tv_nsec %= 1000000000UL;
    		pthread_cond_timedwait(&render_cond, &render_lock,&waittime);
        }
        else if(rd_us+10000UL<cur_us)
        {
            OSA_printf("%s %d: win%d frame_is_late(%ld us)", __func__, __LINE__, glutGetWindow(), cur_us - rd_us);
            if(cur_us - rd_us > 10000UL)
            	memset(&last_render_time, 0, sizeof(last_render_time));
        }
		pthread_mutex_unlock(&render_lock);
	}

	int64 tSwap = getTickCount();
	CGluWindow::Display();
	tStamp[6] = getTickCount();

	if(tStamp[6]-tSwap>5000000UL)
		m_nSwapTimeOut++;
	else
		m_nSwapTimeOut = 0;
	if (last_render_time.tv_sec == 0 || m_nSwapTimeOut>=3)
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		last_render_time.tv_sec = now.tv_sec;
		last_render_time.tv_nsec = now.tv_usec * 1000L;
		printf("\r\nReset render timer. fps(%d) swp(%ld ns)", m_initPrm.disFPS, tStamp[6]-tSwap);
		fflush(stdout);
	}

	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(0, RUN_LEAVE, 0, 0);
	tend = tStamp[6];
	float renderIntv = (tend - m_tmRender)/getTickFrequency();

#if 1
	static unsigned long rCount = 0;
	if(rCount%(m_initPrm.disFPS*100) == 0)
	{
		printf("\r\n[%d] %.4f (ws%.4f,cu%.4f,tv%.4f,to%.4f,rd%.4f,wp%.4f) %.4f(%.4f)",
			OSA_getCurTimeInMsec(),renderIntv,
			(tStamp[1]-tStamp[0])/getTickFrequency(),
			(tStamp[2]-tStamp[1])/getTickFrequency(),
			(tStamp[3]-tStamp[2])/getTickFrequency(),
			(tStamp[4]-tStamp[3])/getTickFrequency(),
			(tStamp[5]-tStamp[4])/getTickFrequency(),
			(tStamp[6]-tStamp[5])/getTickFrequency(),
			m_telapse, m_initPrm.disSched
			);
		fflush(stdout);
	}
	rCount ++;
#endif
	m_tmRender = tend;
}

void CGluVideoWindow::Reshape(int width, int height)
{
	//OSA_printf("[%d] %s %d: GLU%d %d x %d", OSA_getCurTimeInMsec(), __func__, __LINE__, m_winId, width, height);
	CGluWindow::Reshape(width, height);
	for(int i=0; i<vvideoRenders.size(); i++){
		CGLVideoBlendRender *render = vvideoRenders[i];
		if(render != NULL){
			cv::Rect viewPort = cv::Rect(0, 0, m_rcReal.width, m_rcReal.height);
			if(i!=0)
				viewPort = cv::Rect(m_rcReal.width*2/3, m_rcReal.height*2/3, m_rcReal.width/3, m_rcReal.height/3);
			render->set(viewPort);
		}
	}
}

CGLVideoBlendRender* CGluVideoWindow::CreateVedioRender(int index)
{
	CGLVideo *video = (m_vvideos.size() > index) ? m_vvideos[index] : NULL;
	GLMatx44f matrix = cv::Matx44f::eye();
	cv::Rect viewPort = cv::Rect(0, 0, m_rcReal.width, m_rcReal.height);
	CGLVideo *blend = NULL;
	GLMatx44f blendMatrix = cv::Matx44f::eye();
	GLV_BlendPrm prm;
	memset(&prm, 0, sizeof(prm));
	prm.fAlpha = 0.5f;
	if(index != 0){
		//video = NULL;
		viewPort = cv::Rect(m_rcReal.width*2/3, m_rcReal.height*2/3, m_rcReal.width/3, m_rcReal.height/3);
	}
	CGLVideoBlendRender *render = new CGLVideoBlendRender(video, matrix, viewPort, blend, blendMatrix, prm);
	return render;
}

int CGluVideoWindow::dynamic_config(VWIN_CFG type, int iPrm, void* pPrm)
{
	int iRet = OSA_SOK;
	int chId, renderId;
	bool bEnable;
	cv::Rect *rc;
	int renderCount = vvideoRenders.size();
	int videoCount = m_vvideos.size();
	CGLVideoBlendRender *render;
	CGLVideo *pVideo;

	if(type == VWIN_CFG_ChId){
		renderId = iPrm;
		if(renderId >= renderCount || renderId < 0 || vvideoRenders[renderId] == NULL)
			return -1;
		if(pPrm == NULL)
			return -2;
		chId = *(int*)pPrm;

		OSA_mutexLock(&m_mutex);
		render = vvideoRenders[renderId];
		int curId = (render->m_video != NULL) ? render->m_video->m_idx : -1;
		if(curId >= 0){
			int count = OSA_bufGetFullCount(m_bufQue[curId]);
			while(count>0){
				image_queue_switchEmpty(m_bufQue[curId]);
				count = OSA_bufGetFullCount(m_bufQue[curId]);
			}
		}
		pVideo = (chId>=0 && chId<videoCount) ? m_vvideos[chId] : NULL;
		render->set(pVideo);

		if(chId>=0 && chId<videoCount){
			int count = OSA_bufGetFullCount(m_bufQue[chId]);
			for(int i=0; i<count; i++){
				image_queue_switchEmpty(m_bufQue[chId]);
			}
			OSA_printf("%s %d: switchEmpty %d frames", __func__, __LINE__, count);
		}

		m_telapse = 5.0f;
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == VWIN_CFG_VideoTransMat){
		if(iPrm >= videoCount || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		pVideo = m_vvideos[chId];
		memcpy(pVideo->m_matrix.val, pPrm, sizeof(float)*16);
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == VWIN_CFG_ViewTransMat){
		if(iPrm >= renderCount || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		render = vvideoRenders[iPrm];
		GLMatx44f matrix;
		memcpy(matrix.val , pPrm, sizeof(float)*16);
		render->set(matrix);
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == VWIN_CFG_ViewPos){
		if(iPrm >= renderCount || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		render = vvideoRenders[iPrm];
		rc = (cv::Rect*)pPrm;
		render->set(*rc);
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == VWIN_CFG_BlendChId){
		if(iPrm >= VWIND_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		m_blendMap[iPrm] = *(int*)pPrm;
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == VWIN_CFG_MaskChId){
		if(iPrm >= VWIND_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		m_maskMap[iPrm] = *(int*)pPrm;
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == VWIN_CFG_BlendTransMat){
		if(iPrm >= VWIND_CHAN_MAX*VWIND_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		memcpy(m_glmat44fBlend[iPrm].val, pPrm, sizeof(float)*16);
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == VWIN_CFG_BlendPrm){
		if(iPrm >= VWIND_CHAN_MAX*VWIND_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		memcpy(&m_glBlendPrm[iPrm], pPrm, sizeof(GLV_BlendPrm));
		OSA_mutexUnlock(&m_mutex);
	}

	return iRet;
}

