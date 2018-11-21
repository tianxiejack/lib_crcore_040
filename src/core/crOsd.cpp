/*
 * crOsd.cpp
 *
 *  Created on: Nov 16, 2018
 *      Author: wzk
 */
# include "glosd.hpp"
# include <opencv2/opencv.hpp>
# include <stdio.h>
# include <stdarg.h>
# include <stdlib.h>
# include <string.h>
# include <signal.h>
# include <iostream>
# include <osa.h>
//# include <typeinfo>
//# include <type_traits>

using namespace cr_osd;
using namespace std;

extern std::vector<GLOSDFactory *> vosdFactorys;

namespace cr_osd
{
static std::vector<GLOSDTxt *> vtxts;
void put(wchar_t* s, const cv::Point& pos, const cv::Scalar& color, size_t n, const wchar_t* format, ...)
{
	va_list args;
	va_start(args, format);
	//unsigned int ID = va_arg(args, unsigned int);
	//vwprintf(__format, args);
	vswprintf(s, n, format, args);
	va_end(args);

	int nFacts = vosdFactorys.size();
	for(int i=0; i<nFacts; i++){
		cv::Size2f scale((float)vosdFactorys[i]->m_viewport.width/vosdFactorys[0]->m_viewport.width, (float)vosdFactorys[i]->m_viewport.height/vosdFactorys[0]->m_viewport.height);
		GLOSDTxt * txt = new GLOSDTxt(vosdFactorys[i], scale);
		vtxts.push_back(txt);
		txt->txt(pos, s, color);
	}
}

void put(const wchar_t* s, const cv::Point& pos, const cv::Scalar& color)
{
	OSA_assert(s != NULL);
	int nFacts = vosdFactorys.size();
	for(int i=0; i<nFacts; i++){
		cv::Size2f scale((float)vosdFactorys[i]->m_viewport.width/vosdFactorys[0]->m_viewport.width, (float)vosdFactorys[i]->m_viewport.height/vosdFactorys[0]->m_viewport.height);
		GLOSDTxt * txt = new GLOSDTxt(vosdFactorys[i], scale);
		vtxts.push_back(txt);
		txt->txt(pos, s, color);
	}
}

void put(const int* pValue, const wchar_t* format, const cv::Point& pos, const cv::Scalar& color)
{
	OSA_assert(pValue != NULL);
	OSA_assert(format != NULL);
	int nFacts = vosdFactorys.size();
	for(int i=0; i<nFacts; i++){
		cv::Size2f scale((float)vosdFactorys[i]->m_viewport.width/vosdFactorys[0]->m_viewport.width, (float)vosdFactorys[i]->m_viewport.height/vosdFactorys[0]->m_viewport.height);
		GLOSDTxt * txt = new GLOSDTxt(vosdFactorys[i], scale);
		vtxts.push_back(txt);
		txt->txt(pos, pValue, format, color);
	}
}

void put(const unsigned int* pValue, const wchar_t* format, const cv::Point& pos, const cv::Scalar& color)
{
	OSA_assert(pValue != NULL);
	OSA_assert(format != NULL);
	int nFacts = vosdFactorys.size();
	for(int i=0; i<nFacts; i++){
		cv::Size2f scale((float)vosdFactorys[i]->m_viewport.width/vosdFactorys[0]->m_viewport.width, (float)vosdFactorys[i]->m_viewport.height/vosdFactorys[0]->m_viewport.height);
		GLOSDTxt * txt = new GLOSDTxt(vosdFactorys[i], scale);
		vtxts.push_back(txt);
		txt->txt(pos, pValue, format, color);
	}
}

void put(const unsigned char* pValue, const wchar_t* format, const cv::Point& pos, const cv::Scalar& color)
{
	OSA_assert(pValue != NULL);
	OSA_assert(format != NULL);
	int nFacts = vosdFactorys.size();
	for(int i=0; i<nFacts; i++){
		cv::Size2f scale((float)vosdFactorys[i]->m_viewport.width/vosdFactorys[0]->m_viewport.width, (float)vosdFactorys[i]->m_viewport.height/vosdFactorys[0]->m_viewport.height);
		GLOSDTxt * txt = new GLOSDTxt(vosdFactorys[i], scale);
		vtxts.push_back(txt);
		txt->txt(pos, pValue, format, color);
	}
}

void put(const float* pValue, const wchar_t* format, const cv::Point& pos, const cv::Scalar& color)
{
	OSA_assert(pValue != NULL);
	OSA_assert(format != NULL);
	int nFacts = vosdFactorys.size();
	for(int i=0; i<nFacts; i++){
		cv::Size2f scale((float)vosdFactorys[i]->m_viewport.width/vosdFactorys[0]->m_viewport.width, (float)vosdFactorys[i]->m_viewport.height/vosdFactorys[0]->m_viewport.height);
		GLOSDTxt * txt = new GLOSDTxt(vosdFactorys[i], scale);
		vtxts.push_back(txt);
		txt->txt(pos, pValue, format, color);
	}
}

void put(const int *statValue, const int nStat, const cv::Point& pos, const cv::Scalar& color, ...)
{
	OSA_assert(statValue != NULL);
	va_list args;
	va_start(args, color);
	int nFacts = vosdFactorys.size();
	for(int i=0; i<nFacts; i++){
		cv::Size2f scale((float)vosdFactorys[i]->m_viewport.width/vosdFactorys[0]->m_viewport.width, (float)vosdFactorys[i]->m_viewport.height/vosdFactorys[0]->m_viewport.height);
		GLOSDTxt * txt = new GLOSDTxt(vosdFactorys[i], scale);
		vtxts.push_back(txt);
		txt->txt(pos, color, statValue, nStat, args);
	}
	va_end(args);
}

void erase(const void *share)
{
	std::vector<GLOSDTxt*>::iterator it;
	for(it=vtxts.begin();it!=vtxts.end();){
		if((*it)->m_share == share){
			GLOSDTxt * txt = (*it);
			delete txt;
			vtxts.erase(it);
		}else{
			++it;
		}
	}
}

void clear(void)
{
	int nTxts = vtxts.size();
	for(int i=0; i<nTxts; i++)
		delete vtxts[i];
	vtxts.clear();
}

void set(const void *share, const cv::Scalar& color)
{
	std::vector<GLOSDTxt*>::iterator it;
	for(it=vtxts.begin();it!=vtxts.end(); it++){
		if((*it)->m_share == share){
			GLOSDTxt * txt = (*it);
			txt->setcolor(color);
		}
	}
}

void set(const void *share, const int yuv)
{
	int Y,U,V,R,G,B;
	Y = yuv & 0xff; U = (yuv>>8) & 0xff; V = (yuv>>16) & 0xff;
	R = Y+((360*(V-128))>>8);
	G = Y-((88*(U-128)+184*(V-128))>>8);
	B = Y+((455*(U-128))>>8);
	cv::Scalar color = cv::Scalar(R,G,B,255);
	set(share, color);
}

void set(const cv::Scalar& color)
{
	std::vector<GLOSDTxt*>::iterator it;
	for(it=vtxts.begin();it!=vtxts.end(); it++){
		GLOSDTxt * txt = (*it);
		txt->setcolor(color);
	}
}

void set(const int yuv)
{
	int Y,U,V,R,G,B;
	Y = yuv & 0xff; U = (yuv>>8) & 0xff; V = (yuv>>16) & 0xff;
	R = Y+((360*(V-128))>>8);
	G = Y-((88*(U-128)+184*(V-128))>>8);
	B = Y+((455*(U-128))>>8);
	cv::Scalar color = cv::Scalar(R,G,B,255);
	set(color);
}

void set(const void *share, const cv::Point& pos)
{
	std::vector<GLOSDTxt*>::iterator it;
	for(it=vtxts.begin();it!=vtxts.end(); it++){
		if((*it)->m_share == share){
			GLOSDTxt * txt = (*it);
			txt->setpos(pos);
		}
	}
}

/******************************************************
 *
 *
 */

class Pattern
{
	std::vector<void *> vbases;
protected:
	Pattern(int nVert, GLenum primitive = GL_LINES);
	virtual ~Pattern(void);
	virtual int update(const std::vector<cv::Point>& vpts, const cv::Scalar& color = cv::Scalar::all(255), int thickness = 1);
	virtual void set(const cv::Scalar& color);
	virtual void set(const int yuv);
};

Pattern::Pattern(int nVert, GLenum primitive)
{
	int nFacts = vosdFactorys.size();
	for(int i=0; i<nFacts; i++){
		GLOSDUNITBase *base = new GLOSDUNITBase(vosdFactorys[i], nVert, primitive);
		vbases.push_back(base);
	}
}
Pattern::~Pattern(void)
{
	std::vector<void *>::iterator it;
	for(it=vbases.begin();it!=vbases.end(); it++){
		GLOSDUNITBase *base = (GLOSDUNITBase *)(*it);
		delete base;
	}
	vbases.clear();
}
int Pattern::update(const std::vector<cv::Point>& vpts, const cv::Scalar& color, int thickness)
{
	std::vector<void *>::iterator it;
	for(it=vbases.begin();it!=vbases.end(); it++){
		GLOSDUNITBase *base = (GLOSDUNITBase *)(*it);
		if(vpts.size() != base->m_nVert)
			return -1;
		cv::Size2f scale((float)base->m_viewport.width/((GLOSDUNITBase *)vbases[0])->m_viewport.width, (float)base->m_viewport.height/((GLOSDUNITBase *)vbases[0])->m_viewport.height);
		cv::Point2f pos;
		float *vVertex, *vColor;
		cv::Scalar norColor(color.val[0]*0.003921569, color.val[1]*0.003921569, color.val[2]*0.003921569, color.val[3]*0.003921569);
		OSA_mutexLock(base->m_mutexlock);
		vVertex = base->vVertexPos;
		vColor = base->vColorPos;
		for(int i=0; i<base->m_nVert; i++){
			base->m_vtps[i] = cv::Point(vpts[i].x*scale.width, vpts[i].y*scale.height);
			pos = base->normalized(base->m_vtps[i]);
			vVertex[0] = pos.x; vVertex[1] = pos.y;
			vVertex += 3;
			vColor[0] = norColor.val[0]; vColor[1] = norColor.val[1];
			vColor[2] = norColor.val[2]; vColor[3] = norColor.val[3];
			vColor += 4;
		}
		base->m_bUpdate = true;
		if(base->m_thickness != thickness)
			base->m_factory->SetThickness(base, thickness);
		OSA_mutexUnlock(base->m_mutexlock);
	}
	return 0;
}

void Pattern::set(const cv::Scalar& color)
{
	std::vector<void *>::iterator it;
	float *vColor;
	cv::Scalar norColor(color.val[0]*0.003921569, color.val[1]*0.003921569, color.val[2]*0.003921569, color.val[3]*0.003921569);
	for(it=vbases.begin();it!=vbases.end(); it++){
		GLOSDUNITBase *base = (GLOSDUNITBase *)(*it);
		OSA_mutexLock(base->m_mutexlock);
		vColor = base->vColorPos;
		for(int i=0; i<base->m_nVert; i++){
			vColor[0] = norColor.val[0]; vColor[1] = norColor.val[1];
			vColor[2] = norColor.val[2]; vColor[3] = norColor.val[3];
			vColor += 4;
		}
		OSA_mutexUnlock(base->m_mutexlock);
	}
}
void Pattern::set(const int yuv)
{
	int Y,U,V,R,G,B;
	Y = yuv & 0xff; U = (yuv>>8) & 0xff; V = (yuv>>16) & 0xff;
	R = Y+((360*(V-128))>>8);
	G = Y-((88*(U-128)+184*(V-128))>>8);
	B = Y+((455*(U-128))>>8);
	cv::Scalar color = cv::Scalar(R,G,B,255);
	set(color);
}


};//namespace cr_osd
