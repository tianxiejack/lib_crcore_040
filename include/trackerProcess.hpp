/*
 * TackerProcess.hpp
 *
 *  Created on:
 *      Author: sh
 */

#ifndef VIDEOPROCESS_HPP_
#define VIDEOPROCESS_HPP_
#include "processBase.hpp"
#include "UtcTrack.h"

typedef struct _main_thr_obj_cxt{
	int 	chId;
	int     fovId;
	int     ezoomx;
}MAIN_ProcThrObj_cxt;

typedef struct _main_thr_obj{
	MAIN_ProcThrObj_cxt	cxt[2];
	OSA_ThrHndl		thrHandleProc;
	//OSA_SemHndl		procNotifySem;
	OSA_MsgqHndl hMsgQ;
	int			pp;
	volatile bool		exitProcThread;
	bool		initFlag;
	void		*pParent;
	OSA_MutexHndl m_mutex;
}MAIN_ProcThrObj;

typedef struct _Track_info{
	UTC_RECT_float trackrect;
	UTC_RECT_float reAcqRect;
	unsigned int  trackfov;
	unsigned int TrkStat;

}Track_InfoObj;

typedef struct _VP_cfg_mainChId_prm{
	int fovId;
	int iIntervalFrames;
}VPCFG_MainChPrm;

class CTrackerProc : public CProcessBase
{
	MAIN_ProcThrObj	mainProcThrObj;
	unsigned char *m_mainMem[2];
	Mat mainFramegray[2];
	Mat mainFrame[2];

public://close
	CTrackerProc(OSA_SemHndl *notifySem, IProcess *proc);
	virtual ~CTrackerProc();
	int creat();
	int destroy();
	int init();
	enum{
		VP_CFG_MainChId=VP_CFG_TRK_BASE,
		VP_CFG_MainFov,
		VP_CFG_AcqWinSize,
		VP_CFG_TrkEnable,
		VP_CFG_TrkPosRef,
		VP_CFG_TrkCoast,
		VP_CFG_Axis,
		VP_CFG_SaveAxisToArray,
		VP_CFG_SaveAxisToFile,
		VP_CFG_NewData,
		VP_CFG_Quit,
		VP_CFG_Max
	};
	virtual int dynamic_config(int type, int iPrm, void* pPrm = NULL, int prmSize = 0);
	int run();
	int stop();
	virtual int process(int chId, int fovId, int ezoomx, Mat frame);

	cv::Size m_imgSize[MAX_CHAN];
	float m_AxisCalibX[MAX_CHAN][MAX_NFOV_PER_CHAN];
	float m_AxisCalibY[MAX_CHAN][MAX_NFOV_PER_CHAN];
	Mat m_dc[MAX_CHAN];
	int	m_curChId;
	int m_curFovId;
	int m_curEZoomx[MAX_CHAN];
	bool m_bTrack;
	int m_iTrackStat;
	int m_iTrackLostCnt;
	UTC_SIZE m_sizeAcqWin;
	UTC_RECT_float m_rcAcq;
	UTC_RECT_float m_rcTrk;
	cv::Point2f m_axis;

public://open
	virtual void OnCreate(){};
	virtual void OnDestroy(){};
	virtual void OnInit(){};
	virtual void OnConfig(int type, int iPrm, void* pPrm){};
	virtual void OnRun(){};
	virtual void OnStop(){};
	virtual void Ontimer(){};
	virtual bool OnPreProcess(int chId, Mat &frame){return true;}
	virtual bool OnProcess(int chId, Mat &frame){
		if(m_usrNotifySem != NULL && m_curChId == chId)
			OSA_semSignal(m_usrNotifySem);
		OnOSD(chId, m_dc[chId], m_color);
		return true;
	}
	virtual int OnOSD(int chId, Mat dc, CvScalar color){
		return CProcessBase::OnOSD(chId, dc, color);
	};

	bool algOsdRect;
	bool TrkAim43;
	bool moveStat;
	CvScalar m_color;
protected:
	//CRender m_render;
	//CEncTrans m_render;
	//typedef enum{
	//	DS_DC_ALG = 0,
	//	DS_DC_OSD,
	//}DS_DC_IDX;
	UTCTRACK_HANDLE m_track;

	Track_InfoObj *trackinfo_obj;
	int process_track(int trackStatus, Mat frame_gray, Mat frame_dis, UTC_RECT_float &rcResult);
	virtual int ReAcqTarget();

	void Track_reacq(UTC_RECT_float & m_rcTrack,int acqinterval);
	//void Track_fovreacq(short fov,int sensor,int sensorchange);
	int dynamic_config_(int type, int iPrm, void* pPrm = NULL);
	void update_acqRc();

private:
	char m_strDisplay[128];
	char m_strDisplay1[128];

	void printfInfo(Mat mframe,UTC_RECT_float &result,bool show);
	
	void main_proc_func();
	int MAIN_threadCreate(void);
	int MAIN_threadDestroy(void);
	static void *mainProcTsk(void *context)
	{
		//OSA_waitMsecs(200);
		OSA_printf("%s:%d context=%p\n", __func__, __LINE__, context);
		MAIN_ProcThrObj  * pObj= (MAIN_ProcThrObj*) context;
		CTrackerProc *ctxHdl = (CTrackerProc *) pObj->pParent;

		ctxHdl->main_proc_func();
		return NULL;
	}

	//static void extractYUYV2Gray(Mat src, Mat dst);

	static int64 tstart;

protected:

	bool m_bFixSize;
	int m_intervalFrame;
	int m_reTrackStat;
	UTC_RECT_float m_rcTrackSelf;
	int	rcMoveX, rcMoveY;	

	//int	adaptiveThred;
	OSA_SemHndl		*m_usrNotifySem;
};


__inline__ UTC_RECT_float tRectScale(UTC_RECT_float rc, cv::Size imgSize, float fscale)
{
	cv::Point2f point(rc.x, rc.y);
	UTC_RECT_float urc;
	point = tPosScale(point, imgSize, fscale);
	urc.x = point.x; urc.y = point.y;
	urc.width = rc.width*fscale;
	urc.height = rc.height*fscale;
	return urc;
}

#endif /* VIDEOPROCESS_HPP_ */
