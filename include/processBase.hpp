/*
 * processBase.hpp
 *
 *  Created on: Sep 19, 2018
 *      Author: wzk
 */

#ifndef PROCESSBASE_HPP_
#define PROCESSBASE_HPP_

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.hpp>
#include <opencv2/opencv.hpp>
#include "osa.h"
#include "osa_thr.h"
#include "osa_buf.h"
#include "osa_sem.h"
#include "osa_msgq.h"
#include "osa_mutex.h"

using namespace std;
using namespace cv;

#define MAX_CHAN 4
#define MAX_NFOV_PER_CHAN	16
#define MAX_NFOV	(MAX_CHAN*MAX_NFOV_PER_CHAN)

#define MAX_MMTD_TARGET_COUNT		(5)
#define VP_CFG_BASE			(0x0000)
#define VP_CFG_TRK_BASE		(0x0000 + VP_CFG_BASE)
#define VP_CFG_MMTD_BASE	(0x1000 + VP_CFG_BASE)

typedef struct _OSD_unit_info{
	bool bHasDraw;
	bool bNeedDraw;
	int iStyle;
	int thickness;
	int lineType;
	cv::Point txtPos;
	char txt[128];
	cv::Point rcPos;
	cv::Rect rc;
}OSDU_Info;

class IProcess
{
public:
	virtual int process(int chId, int fovId, int ezoomx, Mat frame) = 0;
	virtual int dynamic_config(int type, int iPrm, void* pPrm = NULL, int prmSize = 0) = 0;
	virtual int OnOSD(int chId, Mat dc, CvScalar color) = 0;
};

class CProcessBase : public IProcess
{
	IProcess *m_proc;
public:
	OSA_MutexHndl m_mutexlock;
	CProcessBase(IProcess *proc):m_proc(proc){
		OSA_mutexCreate(&m_mutexlock);
	};
	virtual ~CProcessBase(){
		OSA_mutexUnlock(&m_mutexlock);
		OSA_mutexLock(&m_mutexlock);
		OSA_mutexDelete(&m_mutexlock);
	};
	virtual int process(int chId, int fovId, int ezoomx, Mat frame){
		//cout<<"CProcessBase process"<<endl;

		if(m_proc != NULL)
			return m_proc->process(chId, fovId, ezoomx, frame);
		return 0;
	};
	virtual int dynamic_config(int type, int iPrm, void* pPrm = NULL, int prmSize = 0)
	{
		//cout<<"CProcessBase dynamic_config"<<endl;
		if(m_proc != NULL)
			return m_proc->dynamic_config(type, iPrm, pPrm, prmSize);
		return 0;
	};
	virtual int OnOSD(int chId, Mat dc, CvScalar color)
	{
		if(m_proc != NULL)
			return m_proc->OnOSD(chId, dc, color);
	}
};

__inline__ cv::Point tPosScale(cv::Point pos, cv::Size imgSize, float fscale)
{
	float x = imgSize.width/2.0f - (imgSize.width/2.0f - pos.x)*fscale;
	float y = imgSize.height/2.0f - (imgSize.height/2.0f - pos.y)*fscale;
	return cv::Point(int(x+0.5), int(y+0.5));
}

__inline__ cv::Point2f tPosScale(cv::Point2f pos, cv::Size imgSize, float fscale)
{
	float x = imgSize.width/2.0f - (imgSize.width/2.0f - pos.x)*fscale;
	float y = imgSize.height/2.0f - (imgSize.height/2.0f - pos.y)*fscale;
	return cv::Point2f(x, y);
}

__inline__ Rect tRectScale(Rect rc, cv::Size imgSize, float fscale)
{
	cv::Point2f point(rc.x, rc.y);
	cv::Size size((int)(rc.width*fscale+0.5), (int)(rc.height*fscale+0.5));
	point = tPosScale(point, imgSize, fscale);
	return Rect((int)point.x+0.5, (int)point.y+0.5, size.width, size.height);
}

typedef cv::Rect_<float> Rect2f;
__inline__ Rect2f tRectScale(Rect2f rc, cv::Size imgSize, float fscale)
{
	cv::Point2f point(rc.x, rc.y);
	cv::Size2f size(rc.width*fscale, rc.height*fscale);
	return Rect2f(tPosScale(point, imgSize, fscale), size);
}



#endif /* PROCESSBASE_HPP_ */
