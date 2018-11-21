/*
 * CTrackerProc.cpp
 *
 *  Created on: 2017
 *      Author: sh
 */
#include <glut.h>
#include "trackerProcess.hpp"
//#include "vmath.h"
//#include "arm_neon.h"


//using namespace vmath;

int64 CTrackerProc::tstart = 0;

extern void cutColor(cv::Mat src, cv::Mat &dst, int code);

int CTrackerProc::MAIN_threadCreate(void)
{
	int iRet = OSA_SOK;
	//iRet = OSA_semCreate(&mainProcThrObj.procNotifySem ,1,0) ;
	OSA_assert(iRet == OSA_SOK);
	iRet = OSA_msgqCreate(&mainProcThrObj.hMsgQ);
	OSA_assert(iRet == OSA_SOK);
	iRet = OSA_mutexCreate(&mainProcThrObj.m_mutex);
	OSA_assert(iRet == OSA_SOK);

	mainProcThrObj.exitProcThread = false;

	mainProcThrObj.initFlag = true;

	mainProcThrObj.pParent = (void*)this;

	m_mainMem[0]= new unsigned char[1080*1920];
	m_mainMem[1]= new unsigned char[1080*1920];

	iRet = OSA_thrCreate(&mainProcThrObj.thrHandleProc, mainProcTsk, 0, 0, &mainProcThrObj);

	return iRet;
}

void CTrackerProc::printfInfo(Mat mframe,UTC_RECT_float &result,bool show)
{
	char strFrame[128];
	cv::Scalar RBG = show?cvScalar(255,255,0,255):cvScalar(0,0,0,0);
	
	sprintf(strFrame, "resultXY=(%0.1f,%0.1f) ", result.x, result.y) ;
	putText(mframe,  strFrame, Point(80, 80),FONT_HERSHEY_DUPLEX, 1, RBG,1);

}

static void extractYUYV2Gray(Mat src, Mat dst)
{
	int ImgHeight, ImgWidth;

	ImgWidth = src.cols;
	ImgHeight = src.rows;
	uint8_t  *  pDst8_t;
	uint8_t *  pSrc8_t;

	pSrc8_t = (uint8_t*)(src.data);
	pDst8_t = (uint8_t*)(dst.data);

	for(int y = 0; y < ImgHeight*ImgWidth; y++)
	{
		pDst8_t[y] = pSrc8_t[y*2];
	}
}

int CTrackerProc::process(int chId, int fovId, int ezoomx, Mat frame)
{
	static int ppBak = 1;
	if(frame.cols<=0 || frame.rows<=0)
		return 0;

	m_imgSize[chId].width = frame.cols;
	m_imgSize[chId].height = frame.rows;

//	tstart = getTickCount();
	//if(chId == m_curChId)
	{
		int pp;

		if(!OnPreProcess(chId, frame)){
			return 0;
		}

		OSA_mutexLock(&mainProcThrObj.m_mutex);
		pp = mainProcThrObj.pp;
		mainProcThrObj.cxt[pp].chId = chId;
		mainProcThrObj.cxt[pp].fovId = fovId;
		mainProcThrObj.cxt[pp].ezoomx = ezoomx;
		mainFrame[pp] = frame;
		int channel = frame.channels();
		//OSA_printf("%s: %d ch%d frame %dx%d", __func__, __LINE__, chId, frame.cols, frame.rows);
		Mat frame_gray = Mat(frame.rows, frame.cols, CV_8UC1, m_mainMem[pp]);
		//OSA_printf("%s: %d ", __func__, __LINE__);
		if(channel == 2)
		{
			//OSA_printf("%s: %d %p", __func__, __LINE__, frame.data);
			OSA_assert(frame.data != NULL && frame_gray.data != NULL);
			extractYUYV2Gray(frame, frame_gray);
			//OSA_printf("%s: %d ", __func__, __LINE__);
		}
		else if(channel == 3){
			cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
		}else{
			memcpy(frame_gray.data, frame.data, frame.cols*frame.rows*channel*sizeof(unsigned char));
		}
		mainFramegray[pp] = frame_gray;
		if(ppBak != pp){
			//OSA_printf("%s %d: ch%d(%d) fov%d(%d)", __func__, __LINE__, chId, m_curChId, fovId, m_curFovId[m_curChId]);
			OSA_msgqSendMsg(&mainProcThrObj.hMsgQ, NULL, VP_CFG_NewData, NULL, 0, NULL);
			ppBak = pp;
		}else{
			//OnProcess(chId, frame);
		}
		OSA_mutexUnlock(&mainProcThrObj.m_mutex);
	}

	//OnOSD(chId, fovId, ezoomx, m_dc[chId], m_vosds[chId]);
	//	OSA_printf("process_frame: chId = %d, time = %f sec \n",chId,  ( (getTickCount() - tstart)/getTickFrequency()) );

	return 0;
}

void CTrackerProc::main_proc_func()
{
	int ret = OSA_SOK;
	OSA_MsgHndl msgRcv;
	OSA_printf("%s: Main Proc Tsk Is Entering..[%d].\n",__func__, mainProcThrObj.exitProcThread);
	unsigned int framecount=0;

	while(mainProcThrObj.exitProcThread ==  false)
	{
		/******************************************/
		//OSA_semWait(&mainProcThrObj.procNotifySem, OSA_TIMEOUT_FOREVER);
		ret = OSA_msgqRecvMsgEx(&mainProcThrObj.hMsgQ, &msgRcv, OSA_TIMEOUT_FOREVER);
		OSA_assert(ret == OSA_SOK);

		if(msgRcv.cmd == VP_CFG_Quit){
			OSA_printf("%s %d: ... break", __func__, __LINE__);
			break;
		}
		if(msgRcv.cmd != VP_CFG_NewData){
			dynamic_config_(msgRcv.cmd, msgRcv.flags, msgRcv.pPrm);
			if(msgRcv.pPrm != NULL)
				OSA_memFree(msgRcv.pPrm);
			continue;
		}

		//OSA_printf("%s:pp = %d ,\n",__func__, mainProcThrObj.pp);
		OSA_mutexLock(&mainProcThrObj.m_mutex);
		int pp = mainProcThrObj.pp;
		mainProcThrObj.pp ^=1;
		OSA_mutexUnlock(&mainProcThrObj.m_mutex);
		Mat frame = mainFrame[pp];
		Mat frame_gray = mainFramegray[pp];
		int chId = mainProcThrObj.cxt[pp].chId;
		int fovId = mainProcThrObj.cxt[pp].fovId;
		int ezoomx = mainProcThrObj.cxt[pp].ezoomx;

		CProcessBase::process(chId, fovId, ezoomx, frame_gray);

		int iTrackStat = ReAcqTarget();
		m_curEZoomx[chId] = ezoomx;
		if(!m_bTrack || chId != m_curChId || fovId != m_curFovId[m_curChId]){
			OnProcess(chId, frame);
			continue;
		}

		/******************************************/
		if(iTrackStat >= 0)
		{
			UTC_RECT_float tmpRc;
			if(iTrackStat == 0)
				tmpRc = m_rcAcq;
			else
				tmpRc = m_rcTrackSelf;
			m_iTrackStat = process_track(iTrackStat, frame_gray, frame_gray, tmpRc);
			if(m_bFixSize){
				m_rcTrackSelf.x = tmpRc.x+(tmpRc.width-m_rcAcq.width)/2;
				m_rcTrackSelf.y = tmpRc.y+(tmpRc.height-m_rcAcq.height)/2;
				m_rcTrackSelf.width = m_rcAcq.width;
				m_rcTrackSelf.height = m_rcAcq.height;
			}else{
				m_rcTrackSelf = tmpRc;
			}
		}
		else
		{
			m_iTrackStat = 2;
			m_rcTrackSelf = m_rcAcq;
		}
		m_rcTrk = tRectScale(m_rcTrackSelf, m_imgSize[chId], (float)m_curEZoomx[chId]);

		if(m_iTrackStat == 2)
			m_iTrackLostCnt++;
		else
			m_iTrackLostCnt = 0;

		OnProcess(chId, frame);
		framecount++;

	/************************* while ********************************/
	}
	OSA_printf("%s: Main Proc Tsk Is Exit.\n",__func__);
}

int CTrackerProc::MAIN_threadDestroy(void)
{
	int iRet = OSA_SOK;

	OSA_mutexLock(&mainProcThrObj.m_mutex);

	mainProcThrObj.exitProcThread = true;
	//OSA_semSignal(&mainProcThrObj.procNotifySem);
	OSA_msgqSendMsg(&mainProcThrObj.hMsgQ, NULL, VP_CFG_Quit, NULL, 0, NULL);

	iRet = OSA_thrDelete(&mainProcThrObj.thrHandleProc);

	mainProcThrObj.initFlag = false;
	//OSA_semDelete(&mainProcThrObj.procNotifySem);
	OSA_msgqDelete(&mainProcThrObj.hMsgQ);
	OSA_mutexDelete(&mainProcThrObj.m_mutex);
	delete [] m_mainMem[0];
	delete [] m_mainMem[1];

	return iRet;
}

CTrackerProc::CTrackerProc(OSA_SemHndl *notifySem, IProcess *proc)
	:CProcessBase(proc),m_track(NULL),m_curChId(0),m_usrNotifySem(notifySem),
	 m_bFixSize(false)//, adaptiveThred(180)
{
	memset(&mainProcThrObj, 0, sizeof(MAIN_ProcThrObj));
	memset(&m_rcTrk, 0, sizeof(m_rcTrk));
	memset(m_mainMem, 0, sizeof(m_mainMem));
	memset(m_curFovId, 0, sizeof(m_curFovId));
	memset(m_vosds, 0, sizeof(m_vosds));
	m_bTrack = false;
	m_iTrackStat = 0;
	m_iTrackLostCnt = 0;
	m_sizeAcqWin.width = 80;
	m_sizeAcqWin.height = 60;
	m_axis.x=960;
	m_axis.y=540;
	m_intervalFrame = 0;
	m_reTrackStat = 0;
	rcMoveX = 0;
	rcMoveY = 0;

	algOsdRect = false;

	memset(m_AxisCalibX, 0, sizeof(m_AxisCalibX));
	memset(m_AxisCalibY, 0, sizeof(m_AxisCalibY));

	for(int i=0; i<MAX_CHAN; i++){
		m_curEZoomx[i] = 1;
		m_imgSize[i].width = 1920;
		m_imgSize[i].height = 1080;
		for(int j=0; j<MAX_NFOV_PER_CHAN; j++){
			m_AxisCalibX[i][j] = m_imgSize[i].width/2;
			m_AxisCalibY[i][j] = m_imgSize[i].height/2;
		}
	}
}

CTrackerProc::~CTrackerProc()
{
	destroy();
}

int CTrackerProc::creat()
{
	int i = 0;

	trackinfo_obj=(Track_InfoObj *)malloc(sizeof(Track_InfoObj));

	MAIN_threadCreate();

	OnCreate();

	return 0;
}

int CTrackerProc::destroy()
{
	stop();

	MAIN_threadDestroy();

	OnDestroy();
	return 0;
}

int CTrackerProc::init()
{

	OnInit();
	for(int i=0; i<MAX_CHAN; i++){
		for(int j=0; j<MAX_NFOV_PER_CHAN; j++){
			if(m_AxisCalibX[i][j]<=0 || m_AxisCalibX[i][j]>1920)
			m_AxisCalibX[i][j] = 1920/2;
			if(m_AxisCalibY[i][j]<=0 || m_AxisCalibY[i][j]>1080)
			m_AxisCalibY[i][j] = 1080/2;
		}
	}
	m_axis.x = m_AxisCalibX[m_curChId][m_curFovId[m_curChId]];
	m_axis.y = m_AxisCalibY[m_curChId][m_curFovId[m_curChId]];

	return 0;
}

int CTrackerProc::dynamic_config(int type, int iPrm, void* pPrm, int prmSize)
{
	void *memPrm = NULL;

	//OSA_printf("%s %i: type %d iPrm %d pPrm = %p prmSize = %d", __func__, __LINE__,
	//		type, iPrm, pPrm, prmSize);
	if(prmSize>0 && pPrm!=NULL){
		memPrm = OSA_memAlloc(prmSize);
		OSA_assert(memPrm != NULL);
		memcpy(memPrm, pPrm, prmSize);
	}
	int iRet = OSA_msgqSendMsg(&mainProcThrObj.hMsgQ, NULL, type, memPrm, iPrm, NULL);

	OnConfig(type, iPrm, pPrm);

	return iRet;
}

int CTrackerProc::dynamic_config_(int type, int iPrm, void* pPrm)
{
	int iret = OSA_SOK;
	int bakChId;
	float fwidth, fheight;

	iret = CProcessBase::dynamic_config(type, iPrm, pPrm);

	if(type<VP_CFG_BASE || type>VP_CFG_Max)
		return iret;

	switch(type)
	{
	case VP_CFG_MainChId:
		bakChId = m_curChId;
		m_curChId = iPrm;
		//if(m_curChId != bakChId)
		//	OnOSD(bakChId, m_curFovId[bakChId], m_curEZoomx[bakChId], m_dc[bakChId], m_vosds[bakChId]);
		m_iTrackStat = 0;
		m_iTrackLostCnt = 0;
		if(pPrm != NULL){
			VPCFG_MainChPrm *mchPrm = (VPCFG_MainChPrm*)pPrm;
			m_intervalFrame = mchPrm->iIntervalFrames;
			m_reTrackStat = 0;
			if(mchPrm->fovId>=0 && mchPrm->fovId<MAX_NFOV_PER_CHAN)
				m_curFovId[m_curChId] = mchPrm->fovId;
		}else{
			m_intervalFrame = 0;
			m_reTrackStat = 0;
		}
		m_axis.x = m_AxisCalibX[m_curChId][m_curFovId[m_curChId]];
		m_axis.y = m_AxisCalibY[m_curChId][m_curFovId[m_curChId]];
		//fwidth = (float)m_imgSize[m_curChId].width;
		//fheight = (float)m_imgSize[m_curChId].height;
		//m_axis.x = fwidth/2.0f - (fwidth/2.0f - m_axis.x)*(float)m_curEZoomx;
		//m_axis.y = fheight/2.0f - (fheight/2.0f - m_axis.y)*(float)m_curEZoomx;
		update_acqRc();
		break;
	case VP_CFG_MainFov:
		if(iPrm>=0 && iPrm<MAX_NFOV_PER_CHAN){
			m_curFovId[m_curChId] = iPrm;
			m_iTrackStat = 0;
			m_iTrackLostCnt = 0;
			m_intervalFrame = (pPrm == NULL) ? 1: *(int*)pPrm;
			m_reTrackStat = 0;
			m_axis.x = m_AxisCalibX[m_curChId][m_curFovId[m_curChId]];
			m_axis.y = m_AxisCalibY[m_curChId][m_curFovId[m_curChId]];
			//fwidth = (float)m_imgSize[m_curChId].width;
			//fheight = (float)m_imgSize[m_curChId].height;
			//m_axis.x = fwidth/2.0f - (fwidth/2.0f - m_axis.x)*(float)m_curEZoomx;
			//m_axis.y = fheight/2.0f - (fheight/2.0f - m_axis.y)*(float)m_curEZoomx;
			update_acqRc();
		}
		break;
	case VP_CFG_Axis:
		{
			cv::Point2f *axisPrm = (cv::Point2f*)pPrm;
			m_axis.x = axisPrm->x;
			m_axis.y = axisPrm->y;
			update_acqRc();
		}
		break;
	case VP_CFG_SaveAxisToArray:
		m_AxisCalibX[m_curChId][m_curFovId[m_curChId]] = m_axis.x;
		m_AxisCalibY[m_curChId][m_curFovId[m_curChId]] = m_axis.y;
		break;
	case VP_CFG_AcqWinSize:
		if(pPrm != NULL){
			memcpy(&m_sizeAcqWin, pPrm, sizeof(m_sizeAcqWin));
			//OSA_printf("%s: m_sizeAcqWin(%d,%d)", __func__, m_sizeAcqWin.width, m_sizeAcqWin.height);
			update_acqRc();
			m_iTrackStat = 0;
			m_iTrackLostCnt = 0;
			m_intervalFrame = 0;
			m_reTrackStat = 0;
		}
		m_bFixSize = iPrm;
		break;
	case VP_CFG_TrkEnable:
		if(pPrm == NULL)
		{
			update_acqRc();
		}
		else
		{
			UTC_RECT_float urTmp;
			memcpy(&urTmp, pPrm, sizeof(urTmp));
			m_rcAcq = tRectScale(urTmp, m_imgSize[m_curChId], 1.0/m_curEZoomx[m_curChId]);
			m_rcTrackSelf = urTmp;
			OSA_printf(" m_rcAcq update by assign %f %f %f %f\n", m_rcAcq.x, m_rcAcq.y, m_rcAcq.width, m_rcAcq.height);
		}

		if(iPrm == 0)		// exit trk
		{
			m_bTrack = false;
			m_iTrackStat = 0;
			m_iTrackLostCnt = 0;
			m_intervalFrame = 0;
			m_reTrackStat = 0;
			m_rcTrackSelf.width = 0;
			m_rcTrackSelf.height = 0;
			m_rcTrackSelf.x = m_axis.x;
			m_rcTrackSelf.y = m_axis.y;
		}
		else				// start or reset trk
		{
			m_iTrackStat = 0;
			m_iTrackLostCnt = 0;
			m_intervalFrame = 0;
			m_reTrackStat = 0;
			m_bTrack = true;
		}
		break;
	case VP_CFG_TrkPosRef:
		if(m_bTrack && pPrm != NULL)
		{
			cv::Point2f ref = *(cv::Point2f*)pPrm;
			cv::Rect_<float> rc(m_rcTrackSelf.x, m_rcTrackSelf.y, m_rcTrackSelf.width, m_rcTrackSelf.height);
			if(ref.x>0.1) rc.x = m_rcTrackSelf.x+0.5;
			if(ref.x<-0.1) rc.x = m_rcTrackSelf.x-0.5;
			if(ref.y>0.1) rc.y = m_rcTrackSelf.y+0.5;
			if(ref.y<-0.1) rc.y = m_rcTrackSelf.y-0.5;
			if(rc.width<3) rc.width = 3;
			if(rc.height<3) rc.height = 3;

			m_rcAcq.width = rc.width;m_rcAcq.height = rc.height;
			m_rcAcq.x = rc.x + ref.x;
			m_rcAcq.y = rc.y + ref.y;
			m_iTrackStat = 0;
			m_iTrackLostCnt = 0;
			m_intervalFrame = 0;
			m_reTrackStat = 0;
			//OSA_printf("ref(%f, %f)",ref.x, ref.y);
			//OSA_printf("tk(%f,%f,%f,%f)",m_rcTrackSelf.x, m_rcTrackSelf.y, m_rcTrackSelf.width, m_rcTrackSelf.height);
			//OSA_printf("aq(%f,%f,%f,%f)\n\r",m_rcAcq.x, m_rcAcq.y, m_rcAcq.width, m_rcAcq.height);
		}
		break;
	case VP_CFG_TrkCoast:
		if(m_bTrack)
		{
			m_intervalFrame = iPrm;
			m_reTrackStat = m_iTrackStat;
		}
		break;

	default:
		iret = OSA_EFAIL;
		break;
	}
	//OSA_printf("%s %i: ", __func__, __LINE__);

	return iret;
}

void CTrackerProc::update_acqRc()
{
	UTC_RECT_float rc;
	rc.width  = m_sizeAcqWin.width/m_curEZoomx[m_curChId];
	rc.height = m_sizeAcqWin.height/m_curEZoomx[m_curChId];
	rc.x = m_axis.x - rc.width/2;
	rc.y = m_axis.y - rc.height/2;
	m_rcAcq = rc;
	m_rcTrk = tRectScale(m_rcAcq, m_imgSize[m_curChId], (float)m_curEZoomx[m_curChId]);
	m_rcTrackSelf = rc;
	//OSA_printf("%s: curCh%d m_rcAcq(%.1f %.1f %.1f %.1f)",__func__,m_curChId,m_rcAcq.x, m_rcAcq.y, m_rcAcq.width, m_rcAcq.height);
}

int CTrackerProc::run()
{
	m_track = CreateUtcTrk();

	OSA_printf(" %d:%s end\n", OSA_getCurTimeInMsec(),__func__);

	OnRun();

	return 0;
}

int CTrackerProc::stop()
{
	OnStop();

	OSA_printf(" %d:%s enter\n", OSA_getCurTimeInMsec(),__func__);

	if(m_track != NULL)
		DestroyUtcTrk(m_track);
	m_track = NULL;

	return 0;
}

/*
void CTrackerProc::Track_fovreacq(short fov,int sensor,int sensorchange)
{
	//UTC_RECT_float rect;
	unsigned int currentx=0;
	unsigned int currenty=0;
	unsigned int TvFov[5] = {120,50,16,5,2};//fovSize[0][n]*5%
	unsigned int FrFov[5] = {200,50,16,4,2};//fovSize[1][n]*5%

	if(sensorchange == 1){
		currentx = m_axis.x;
		currenty = m_axis.y;
	}
	else
	{		
		currentx=trackinfo_obj->trackrect.x+trackinfo_obj->trackrect.width/2;
		currenty=trackinfo_obj->trackrect.y+trackinfo_obj->trackrect.height/2;
		//OSA_printf("m_ImageAxisxy(%d,%d)\n",m_axis.x,m_axis.y);
		if(sensor==0){
			
			if((abs(currentx-m_axis.x)>24)||(abs(currenty-m_axis.y)>24)){
				currentx=m_axis.x;
				currenty=m_axis.y;
			}

		}
		else if(sensor==1){
			if((abs(currentx-m_axis.x)>12)||(abs(currenty-m_axis.y)>12)){
				currentx=m_axis.x;
				currenty=m_axis.y;
			}
		}
	}
	int prifov=trackinfo_obj->trackfov;
	
	double ratio=tan(3.1415926*fov/36000)/tan(3.1415926*prifov/36000);
	
	unsigned int w=trackinfo_obj->trackrect.width/ratio;
	unsigned int h=trackinfo_obj->trackrect.height/ratio;
	if(sensorchange)
	{
		w=w*vcapWH[sensor^1][0]/vcapWH[sensor][0];
		h=h*vcapWH[sensor^1][1]/vcapWH[sensor][1];

	}
	
	//OSA_printf("prifov=%d fov=%d  the w=%d  h=%d ratio=%f wt=%f,\n",prifov,fov,w,h,ratio,trackinfo_obj->trackrect.width);

	{
		if(w>trkWinWH[sensor][4][0])
			w=trkWinWH[sensor][4][0];
		else if(w<trkWinWH[sensor][0][0])
			w=trkWinWH[sensor][0][0];

		if(h>trkWinWH[sensor][4][1])
			h=trkWinWH[sensor][4][1];
		else if(h<trkWinWH[sensor][0][1])
			h=trkWinWH[sensor][0][1];

	}
	
	trackinfo_obj->trackfov=fov;

	//OSA_printf("fov=%d,trackinfo_obj->TrkStat=%d,sensor=%d\n",fov,trackinfo_obj->TrkStat,sensor);
	if(trackinfo_obj->TrkStat){
		if(sensor == 0){
			if((fov>((fovSize[sensor][0])-TvFov[0]))&&(fov<((fovSize[sensor][0])+TvFov[0]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("TV Big chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
			else if((fov>((fovSize[sensor][1])-TvFov[1]))&&(fov<((fovSize[sensor][1])+TvFov[1]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("TV Mid chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
			else if((fov>((fovSize[sensor][2])-TvFov[2]))&&(fov<((fovSize[sensor][2])+TvFov[2]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("TV Sml chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
			else if((fov>((fovSize[sensor][3])-TvFov[3]))&&(fov<((fovSize[sensor][3])+TvFov[3]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("TV 1 du chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
			else if((fov>((fovSize[sensor][4])-TvFov[4]))&&(fov<((fovSize[sensor][4])+TvFov[4]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("TV Zoom chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}			
		}
		else if(sensor == 1){
			if((fov>((fovSize[sensor][0])-FrFov[0]))&&(fov<((fovSize[sensor][0])+FrFov[0]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("FR Big chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
			else if((fov>((fovSize[sensor][1])-FrFov[1]))&&(fov<((fovSize[sensor][1])+FrFov[1]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("FR Mid chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
			else if((fov>((fovSize[sensor][2])-FrFov[2]))&&(fov<((fovSize[sensor][2])+FrFov[2]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("FR Sml chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
			else if((fov>((fovSize[sensor][3])-FrFov[3]))&&(fov<((fovSize[sensor][3])+FrFov[3]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("FR 1 du chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
			else if((fov>((fovSize[sensor][4])-FrFov[4]))&&(fov<((fovSize[sensor][4])+FrFov[4]))){
				trackinfo_obj->reAcqRect.width=w;
				trackinfo_obj->reAcqRect.height=h;
				trackinfo_obj->reAcqRect.x=currentx-w/2;
				trackinfo_obj->reAcqRect.y=currenty-h/2;
				Track_reacq(trackinfo_obj->reAcqRect,3);
				//OSA_printf("FR Zoom chId%d  fov = %d,xy(%f,%f),WH(%f,%f) ratio=%f\n",sensor,fov,trackinfo_obj->reAcqRect.x,trackinfo_obj->reAcqRect.y,trackinfo_obj->reAcqRect.width,trackinfo_obj->reAcqRect.height,ratio);
			}
		}
	}	
}*/
void CTrackerProc::Track_reacq(UTC_RECT_float & rcTrack,int acqinterval)
{
	m_intervalFrame=acqinterval;
	m_rcAcq=rcTrack;
}

int CTrackerProc::ReAcqTarget()
{
	int iRet = m_iTrackStat;

	if(m_intervalFrame > 0){
		m_intervalFrame--;
		if(m_intervalFrame == 0){
			iRet = m_reTrackStat;	// need acq
			m_iTrackLostCnt = 0;
		}
		else
			iRet = -1;	// need pause
	}
	if(m_intervalFrame<0)
		return -1;
	return iRet;
}

int CTrackerProc::process_track(int trackStatus, Mat frame_gray, Mat frame_dis, UTC_RECT_float &rcResult)
{
	IMG_MAT image;
	cv::Size imgSize(frame_gray.cols, frame_gray.rows);

	image.data_u8 	= frame_gray.data;
	image.width 	= frame_gray.cols;
	image.height 	= frame_gray.rows;
	image.channels 	= 1;
	image.step[0] 	= image.width;
	image.dtype 	= 0;
	image.size	= frame_gray.cols*frame_gray.rows;

	if(trackStatus != 0)
	{
		rcResult = UtcTrkProc(m_track, image, &trackStatus);
	}
	else
	{
		UTC_ACQ_param acq;
		OSA_printf("ACQ (%f, %f, %f, %f)",
				rcResult.x, rcResult.y, rcResult.width, rcResult.height);
		acq.axisX = m_axis.x;// image.width/2;
		acq.axisY = m_axis.y;//image.height/2;
		acq.rcWin.x = (int)(rcResult.x);
		acq.rcWin.y = (int)(rcResult.y);
		acq.rcWin.width = (int)(rcResult.width);
		acq.rcWin.height = (int)(rcResult.height);
		if(acq.rcWin.width<0)
		{
			acq.rcWin.width=0;
		}
		else if(acq.rcWin.width>= image.width)
		{
			acq.rcWin.width=80;
		}
		if(acq.rcWin.height<0)
		{
			acq.rcWin.height=0;
		}
		else if(acq.rcWin.height>= image.height)
		{
			acq.rcWin.height=60;
		}
		if(acq.rcWin.x<0)
		{
			acq.rcWin.x=0;
		}
		else if(acq.rcWin.x>image.width-acq.rcWin.width)
		{
			acq.rcWin.x=image.width-acq.rcWin.width;
		}
		if(acq.rcWin.y<0)
		{
			acq.rcWin.y=0;
		}
		else if(acq.rcWin.y>image.height-acq.rcWin.height)
		{
			acq.rcWin.y=image.height-acq.rcWin.height;
		}

		UtcTrkAcq(m_track, image, acq);

		trackStatus = 1;
	}

	return trackStatus;
}

