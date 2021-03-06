/*
 * crCore.cpp
 *
 *  Created on: Sep 27, 2018
 *      Author: wzk
 */
#include "glosd.hpp"
#include "SceneProcess.hpp"
#include "SVMDetectProcess.hpp"
#include "blobDetectProcess.hpp"
#include "backgroundProcess.hpp"
#include "motionDetectProcess.hpp"
#include "mmtdProcess.hpp"
#include "GeneralProcess.hpp"
//#include "Displayer.hpp"
#include "gluVideoWindow.hpp"
#include "gluVideoWindowSecondScreen.hpp"
#include "encTrans.hpp"
#include "osa_image_queue.h"
#include "cuda_convert.cuh"
#include "thread.h"
#include "crCore.hpp"

namespace cr_osd
{
extern void put(wchar_t* s, const cv::Point& pos, const cv::Scalar& color, size_t n, const wchar_t* format, ...);
//extern void put(const wchar_t* s, const cv::Point& pos, const cv::Scalar& color);
//extern void clear(void);
extern GLShaderManagerMini		glShaderManager;
extern std::vector<cr_osd::GLOSDFactory *> vosdFactorys;
};
//using namespace cr_osd;
#define isEqual(a,b) (fabs((a)-(b))<=1.0e-6)

namespace cr_local
{
static cr_osd::GLOSDFactory *vOSDs[CORE_CHN_MAX];
static cr_osd::GLOSD *glosdFront = NULL;
static CEncTrans *enctran = NULL;
static CGluVideoWindow *render = NULL;
static IProcess *proc = NULL;
static CSceneProcess *scene = NULL;
static CSVMDetectProcess *svm = NULL;
static CBlobDetectProcess *blob = NULL;
static CBkgdDetectProcess *bkgd = NULL;
static CMotionDetectProcess *motion = NULL;
static CMMTDProcess *mmtd = NULL;
static CGeneralProc *general = NULL;
static int curChannelFlag = 0;
static int curSubChannelIdFlag = -1;
static int curFovIdFlag[CORE_CHN_MAX];
static bool curFixSizeFlag = false;
static bool enableTrackFlag = false;
static bool enableMMTDFlag = false;
static bool enableMotionDetectFlag = false;
static bool enableEnhFlag[CORE_CHN_MAX];
static bool enableBlobFlag = false;
static int bindBlendFlag[CORE_CHN_MAX];
static int bindBlendTFlag[CORE_CHN_MAX];
static cv::Matx44f blendMatric[CORE_CHN_MAX];
static bool enableEncoderFlag[CORE_CHN_MAX];
static bool enableOSDFlag = true;
static int ezoomxFlag[CORE_CHN_MAX];
static float scaleFlag[CORE_CHN_MAX];
static int colorYUVFlag = WHITECOLOR;
static cv::Scalar colorRGBAFlag = cv::Scalar::all(255);
static int curThicknessFlag = 2;
static int nValidChannels = CORE_CHN_MAX;
static cv::Size channelsImgSize[CORE_CHN_MAX];
static int channelsFormat[CORE_CHN_MAX];
static int channelsFPS[CORE_CHN_MAX];
static char fileNameFont[CORE_CHN_MAX][256];
static char fileNameFontRender[256];
static int fontSizeVideo[CORE_CHN_MAX];
static int fontSizeRender = 45;
static int renderFPS = 30;
static int curTransLevel = 1;
static void (*renderHook)(int displayId, int stepIdx, int stepSub, int context) = NULL;
static cv::Rect subRc;
static cv::Matx44f subMatric;

static int defaultEncParamTab0[CORE_CHN_MAX*3][8] = {
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{5600000,  -1,    -1,   -1,   -1,   -1,   -1, } //8M
};
static int defaultEncParamTab1[CORE_CHN_MAX*3][8] = {
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, }, //8M
	//bitrate; minQP; maxQP;minQI;maxQI;minQB;maxQB;
	{700000,  -1,    -1,   -1,   -1,   -1,   -1, },//2M
	{1400000,  -1,    -1,   -1,   -1,   -1,   -1, },//4M
	{2800000,  -1,    -1,   -1,   -1,   -1,   -1, } //8M
};

//static int *userEncParamTab[3] = {NULL, NULL, NULL};
static int *userEncParamTab0[CORE_CHN_MAX][3];
static int *userEncParamTab1[CORE_CHN_MAX][3];

static __inline__ int* getEncParamTab(int chId, int level)
{
	int nEnc = 0;
	for(int i = 0; i < nValidChannels; i++)
		nEnc += enableEncoderFlag[i];
	if(nEnc>1)
		return userEncParamTab1[chId][level];
	return userEncParamTab0[chId][level];
}

static int ReadCfgFile_OSD(int nChannels)
{
	int iret = -1;
	string cfgFile;
	cfgFile = "ConfigOSDFile.yml";
	FILE *fp = fopen(cfgFile.c_str(), "rt");
	if(fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);

		if(len > 10)
		{
			FileStorage fr(cfgFile, FileStorage::READ);
			if(fr.isOpened())
			{
				int ivalue;
				string szfile;
				for(int chId=0; chId<nChannels; chId++){
					string strkey;
					char keyStr[256];
					sprintf(keyStr, "ch%02d_font_filename", chId);
					strkey = keyStr;
					szfile = (string)fr[strkey];
					if(szfile.length()>0){
						strcpy(fileNameFont[chId], szfile.data());
					}
					sprintf(keyStr, "ch%02d_font_size", chId);
					strkey = keyStr;
					ivalue = (int)fr[strkey];
					if(ivalue>10 && ivalue<200){
						fontSizeVideo[chId] = ivalue;
					}
				}

				szfile = (string)fr["Front_font_filename"];
				if(szfile.length()>0){
					strcpy(fileNameFontRender, szfile.data());
				}
				ivalue = (int)fr["Front_font_size"];
				if(ivalue>10 && ivalue<200){
					fontSizeRender = ivalue;
				}

				iret = 0;
			}
		}
	}

	for(int chId=0; chId<nChannels; chId++)
		OSA_printf("OSD: video%02d fontSize = %d [%s]",chId, fontSizeVideo[chId], fileNameFont[chId]);
	OSA_printf("OSD: Front fontSize = %d [%s]\n\n",fontSizeRender, fileNameFontRender);
	return iret;
}

static void localInit(int nChannels, bool bEncoder)
{
	curChannelFlag = 0;
	memset(curFovIdFlag, 0, sizeof(curFovIdFlag));
	enableTrackFlag = false;
	enableMMTDFlag = false;
	renderHook = NULL;
	memset(enableEnhFlag, 0, sizeof(enableEnhFlag));
	memset(bindBlendFlag, 0, sizeof(bindBlendFlag));
	memset(userEncParamTab0, 0, sizeof(userEncParamTab0));
	memset(userEncParamTab1, 0, sizeof(userEncParamTab1));
	for(int i=0; i<CORE_CHN_MAX; i++){
		vOSDs[i] = NULL;
		enableEncoderFlag[i] = true;
		ezoomxFlag[i] = 1;
		scaleFlag[i] = 1.0f;
		bindBlendTFlag[i] = -1;
		blendMatric[i] = cv::Matx44f::eye();
		channelsFPS[i] = 30;
		userEncParamTab0[i][0] = defaultEncParamTab0[i*3+0];
		userEncParamTab0[i][1] = defaultEncParamTab0[i*3+1];
		userEncParamTab0[i][2] = defaultEncParamTab0[i*3+2];
		userEncParamTab1[i][0] = defaultEncParamTab1[i*3+0];
		userEncParamTab1[i][1] = defaultEncParamTab1[i*3+1];
		userEncParamTab1[i][2] = defaultEncParamTab1[i*3+2];
		//sprintf(fileNameFont[i], "/usr/share/fonts/truetype/abyssinica/AbyssinicaSIL-R.ttf");
		//sprintf(fileNameFont[i], "/usr/share/fonts/truetype/freefont/FreeMono.ttf");
		//sprintf(fileNameFont[i], "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf");
		//sprintf(fileNameFont[i], "/usr/share/fonts/truetype/freefont/FreeSans.ttf");
		//sprintf(fileNameFont[i], "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf");
		//sprintf(fileNameFont[i], "/usr/share/fonts/truetype/freefont/FreeSansBoldOblique.ttf");
		//sprintf(fileNameFont[i], "/usr/share/fonts/truetype/freefont/FreeSansOblique.ttf");
		//sprintf(fileNameFont[i], "/usr/share/fonts/truetype/freefont/FreeSerif.ttf");
		//sprintf(fileNameFont[i],"/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-L.ttf");
		//sprintf(fileNameFont[i],"/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-M.ttf");
		//sprintf(fileNameFont[i],"/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-R.ttf");
		//sprintf(fileNameFont[i],"/usr/share/fonts/truetype/fonts-japanese-gothic.ttf");
		//sprintf(fileNameFont[i],"/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf");
		sprintf(fileNameFont[i],"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
		//sprintf(fileNameFont[i],"/usr/share/fonts/truetype/liberation/LiberationSansNarrow-Regular.ttf");
		//sprintf(fileNameFont[i],"/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf");
		fontSizeVideo[i] = 45;
	}
	glosdFront = NULL;
	enableOSDFlag = true;
	colorYUVFlag = WHITECOLOR;
	colorRGBAFlag = cv::Scalar::all(255);
	curThicknessFlag = 2;
	if(bEncoder)
		curThicknessFlag = 1;
	//sprintf(fileNameFontRender, "/usr/share/fonts/truetype/abyssinica/simsun.ttc");
	sprintf(fileNameFontRender, "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
	fontSizeRender = 45;
	renderFPS = 30;
	curTransLevel = 1;
	nValidChannels = nChannels;
	ReadCfgFile_OSD(nChannels);
	OSA_assert(nChannels>0 && nChannels<=CORE_CHN_MAX);
}

/************************************************************
 *
 *
 */
static int setEncTransLevel(int iLevel)
{
	ENCTRAN_encPrm encPrm;
	int iret = OSA_SOK;

	if(enctran == NULL)
		return iret;

	enctran->dynamic_config(CEncTrans::CFG_TransLevel, iLevel, NULL);
	for(int chId=0; chId<nValidChannels; chId++){
		int* params = getEncParamTab(chId, iLevel);
		encPrm.bitrate = params[0];
		encPrm.minQP = params[1];
		encPrm.maxQP = params[2];
		encPrm.minQI = params[3];
		encPrm.maxQI = params[4];
		encPrm.minQB = params[5];
		encPrm.maxQB = params[6];

		enctran->dynamic_config(CEncTrans::CFG_EncPrm, chId, &encPrm);
	}
	curTransLevel = iLevel;

	return iret;
}

static int setMainChId(int chId, int fovId, int ndrop, UTC_SIZE acqSize)
{
	VPCFG_MainChPrm mcPrm;
	int iret = OSA_SOK;
	if(chId<0||chId>=nValidChannels)
		return OSA_EFAIL;
	if(fovId<0||fovId>=MAX_NFOV_PER_CHAN)
		return OSA_EFAIL;
	mcPrm.fovId = fovId;
	mcPrm.iIntervalFrames = ndrop;
	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, curFixSizeFlag, &acqSize, sizeof(acqSize));
	proc->dynamic_config(CProcessBase::VP_CFG_MainChId, chId, &mcPrm, sizeof(mcPrm));
	if(render!= NULL){
		render->dynamic_config(CGluVideoWindow::VWIN_CFG_ChId, 0, &chId);
	}
	curChannelFlag = chId;
	curFovIdFlag[chId] = fovId;
	return iret;
}

static int setSubChId(int chId)
{
	curSubChannelIdFlag = chId;
	if(render!= NULL){
		render->dynamic_config(CGluVideoWindow::VWIN_CFG_ChId, 1, &chId);
	}
	return OSA_SOK;
}

static int enableTrack(bool enable, UTC_SIZE winSize, bool bFixSize)
{
	int iret = OSA_SOK;
	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, bFixSize, &winSize, sizeof(winSize));
	proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, enable);
	enableTrackFlag = enable;
	curFixSizeFlag = bFixSize;
	return iret;
}

static int enableTrack(bool enable, UTC_RECT_float winRect, bool bFixSize)
{
	int iret = OSA_SOK;
	proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, enable, &winRect, sizeof(winRect));
	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, bFixSize);
	enableTrackFlag = enable;
	curFixSizeFlag = bFixSize;
	return iret;
}

static int enableMMTD(bool enable, int nTarget, int nSel)
{
#if 1
	int iret = OSA_SOK;
	if(nSel <= 0)
		nSel = nTarget;
	if(enable)
		proc->dynamic_config(CMMTDProcess::VP_CFG_MMTDTargetCount, nTarget, &nSel, sizeof(nSel));
	proc->dynamic_config(CMMTDProcess::VP_CFG_MMTDEnable, enable);
	enableMMTDFlag = enable;
	return iret;
#endif
}

static int enableMotionDetect(bool enable)
{
#if 1
	proc->dynamic_config(CMotionDetectProcess::VP_CFG_MONTIONEnable, enable);
	enableMotionDetectFlag = enable;
#else
	proc->dynamic_config(CSceneProcess::VP_CFG_SceneEnable, enable);
#endif
	return OSA_SOK;
}

static int enableTrackByMMTD(int index, cv::Size *winSize, bool bFixSize)
{
#if 0
	if(index<0 || index>=MAX_TGT_NUM)
		return OSA_EFAIL;

	if(!mmtd->m_target[index].valid)
		return OSA_EFAIL;

	UTC_RECT_float acqrc;
	acqrc.x = mmtd->m_target[index].Box.x;
	acqrc.y = mmtd->m_target[index].Box.y;
	acqrc.width = mmtd->m_target[index].Box.width;
	acqrc.height = mmtd->m_target[index].Box.height;

	if(winSize != NULL){
		acqrc.x += (acqrc.width*0.5);
		acqrc.y += (acqrc.height*0.5);
		acqrc.width = winSize->width;
		acqrc.height = winSize->height;
		acqrc.x -= (acqrc.width*0.5);
		acqrc.y -= (acqrc.height*0.5);
	}

	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, bFixSize);
	curFixSizeFlag = bFixSize;
	return proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, true, &acqrc, sizeof(acqrc));
#else
	PROC_TARGETINFO tgt;
	tgt.valid = 0;
	//OSA_printf("%s: CProcessBase::VP_CFG_GetTargetInfo index = %d", __func__, index);
	int ret = mmtd->dynamic_config(CProcessBase::VP_CFG_GetTargetInfo, index, &tgt, sizeof(tgt));
	if(ret != OSA_SOK || !tgt.valid){
		//OSA_printf("%s: CProcessBase::VP_CFG_GetTargetInfo index = %d ret %d valid %d", __func__, index, ret, tgt.valid);
		return OSA_EFAIL;
	}
	UTC_RECT_float acqrc;
	acqrc.x = tgt.Box.x;
	acqrc.y = tgt.Box.y;
	acqrc.width = tgt.Box.width;
	acqrc.height = tgt.Box.height;

	if(winSize != NULL){
		acqrc.x += (acqrc.width*0.5);
		acqrc.y += (acqrc.height*0.5);
		acqrc.width = winSize->width;
		acqrc.height = winSize->height;
		acqrc.x -= (acqrc.width*0.5);
		acqrc.y -= (acqrc.height*0.5);
	}

	proc->dynamic_config(CTrackerProc::VP_CFG_AcqWinSize, bFixSize);
	curFixSizeFlag = bFixSize;
	enableTrackFlag = true;
	return proc->dynamic_config(CTrackerProc::VP_CFG_TrkEnable, true, &acqrc, sizeof(acqrc));
#endif
}

static int enableEnh(bool enable)
{
	enableEnhFlag[curChannelFlag] = enable;
	return OSA_SOK;
}

static int enableEnh(int chId, bool enable)
{
	if(chId<0 || chId >=CORE_CHN_MAX)
		return OSA_EFAIL;
	enableEnhFlag[chId] = enable;
	return OSA_SOK;
}

static int enableBlob(bool enable)
{
	proc->dynamic_config(CBlobDetectProcess::VP_CFG_BLOBTargetCount, 1);
	proc->dynamic_config(CBlobDetectProcess::VP_CFG_BLOBEnable, enable);
	//proc->dynamic_config(CSVMDetectProcess::VP_CFG_SVMTargetCount, 8);
	//proc->dynamic_config(CSVMDetectProcess::VP_CFG_SVMEnable, enable);
	enableBlobFlag = enable;
	return OSA_SOK;
}

static int bindBlend(int chId, int blendchId, cv::Matx44f matric)
{
	int ret = OSA_SOK;
	if(chId<0 || chId >=CORE_CHN_MAX)
		return OSA_EFAIL;
	if(blendchId>=0 && blendchId < CORE_CHN_MAX){
		bindBlendFlag[blendchId] |= 1<<chId;
		bindBlendTFlag[chId] = blendchId;
	}else{
		int curBlendchId = bindBlendTFlag[chId];
		if(curBlendchId>=0)
			bindBlendFlag[curBlendchId] &= ~(1<<chId);
		bindBlendTFlag[chId] = -1;
	}
	blendMatric[chId] = matric;
	if(render!= NULL){
		if(blendchId>=0 && blendchId < CORE_CHN_MAX){
			ret = render->dynamic_config(CGluVideoWindow::VWIN_CFG_BlendTransMat, chId*VWIN_CHAN_MAX+blendchId, matric.val);
			ret = proc->dynamic_config(CBkgdDetectProcess::VP_CFG_BkgdDetectEnable, true, &blendchId, sizeof(blendchId));
		}else{
			ret = proc->dynamic_config(CBkgdDetectProcess::VP_CFG_BkgdDetectEnable, false);
		}
		ret = render->dynamic_config(CGluVideoWindow::VWIN_CFG_BlendChId, chId, &blendchId);
	}
	return ret;
}

static int bindBlend(int blendchId, cv::Matx44f matric)
{
	return bindBlend(curChannelFlag, blendchId, matric);
}

static int setWinPos(int winId, cv::Rect rc)
{
	int ret = OSA_SOK;
	if(render!= NULL){
		ret = render->dynamic_config(CGluVideoWindow::VWIN_CFG_ViewPos, winId, &rc);
		if(winId == 1){
			subRc = render->m_vvideoRenders[1]->m_viewPort;//render->m_renders[1].displayrect;
			subMatric = render->m_vvideoRenders[1]->m_matrix;//render->m_renders[1].transform;
		}
	}
	return ret;
}

static int setWinMatric(int winId, cv::Matx44f matric)
{
	int ret = OSA_SOK;
	if(render!= NULL){
		ret = render->dynamic_config(CGluVideoWindow::VWIN_CFG_ViewTransMat, winId, matric.val);
		if(winId == 1){
			subRc = render->m_vvideoRenders[1]->m_viewPort;//render->m_renders[1].displayrect;
			subMatric = render->m_vvideoRenders[1]->m_matrix;//render->m_renders[1].transform;
		}
	}
	return ret;
}

static int enableOSD(bool enable)
{
	enableOSDFlag = enable;
	return OSA_SOK;
}

static int enableEncoder(int chId, bool enable)
{
	int ret = OSA_SOK;
	if(enctran == NULL)
		return OSA_SOK;
	enableEncoderFlag[chId] = enable;
	ret = enctran->dynamic_config(CEncTrans::CFG_Enable, chId, &enable);

	for(int chId=0; chId<nValidChannels; chId++){
		ENCTRAN_encPrm encPrm;
		int* params = getEncParamTab(chId, curTransLevel);
		encPrm.bitrate = params[0];
		encPrm.minQP = params[1];
		encPrm.maxQP = params[2];
		encPrm.minQI = params[3];
		encPrm.maxQI = params[4];
		encPrm.minQB = params[5];
		encPrm.maxQB = params[6];

		ret |= enctran->dynamic_config(CEncTrans::CFG_EncPrm, chId, &encPrm);
	}
	return ret;
}

static int setAxisPos(cv::Point pos)
{
	cv::Point2f setPos(pos.x, pos.y);
	int ret = proc->dynamic_config(CTrackerProc::VP_CFG_Axis, 0, &setPos, sizeof(setPos));
	OSA_assert(ret == OSA_SOK);
	ret = proc->dynamic_config(CTrackerProc::VP_CFG_SaveAxisToArray, 0);
	return ret;
}

static int saveAxisPos()
{
	return proc->dynamic_config(CTrackerProc::VP_CFG_SaveAxisToFile, 0);
}

static int setEZoomx(int value)
{
	if(enctran != NULL){
		if(value != 1 && value != 2 && value != 4)
			return OSA_EFAIL;
	}
	ezoomxFlag[curChannelFlag] = value;
	return OSA_SOK;
}

static int setEZoomx(int chId, int value)
{
	if(enctran != NULL){
		if(value != 1 && value != 2 && value != 4)
			return OSA_EFAIL;
	}
	if(chId<0 || chId >=CORE_CHN_MAX)
		return OSA_EFAIL;
	ezoomxFlag[chId] = value;
	return OSA_SOK;
}

static int setTrackCoast(int nFrames)
{
	return proc->dynamic_config(CTrackerProc::VP_CFG_TrkCoast, nFrames);
}

static int setTrackPosRef(cv::Point2f ref)
{
	return proc->dynamic_config(CTrackerProc::VP_CFG_TrkPosRef, 0, &ref, sizeof(ref));
}

static int setOSDColor(int value, int thickness)
{
	int Y,U,V,R,G,B;
	Y = value & 0xff; U = (value>>8) & 0xff; V = (value>>16) & 0xff;
	R = Y+((360*(V-128))>>8);
	G = Y-((88*(U-128)+184*(V-128))>>8);
	B = Y+((455*(U-128))>>8);

	switch(value)
	{
	case WHITECOLOR:
		R = 255; G = 255; B = 255;
		break;
	case YELLOWCOLOR:
		R = 255; G = 255; B = 0;
		break;
	case CRAYCOLOR:
		break;
	case GREENCOLOR:
		R = 0; G = 255; B = 0;
		break;
	case MAGENTACOLOR:
		break;
	case REDCOLOR:
		R = 255; G = 0; B = 0;
		break;
	case BLUECOLOR:
		R = 0; G = 0; B = 255;
		break;
	case BLACKCOLOR:
		R = 0; G = 0; B = 0;
		break;
	case BLANKCOLOR:
		break;
	}

	colorYUVFlag = value;
	colorRGBAFlag = cv::Scalar(R,G,B,255);
	curThicknessFlag = thickness;
	return OSA_SOK;
}

static int setOSDColor(cv::Scalar rgba, int thickness)
{
	int Y,U,V,R,G,B;
	R = rgba.val[0];G = rgba.val[1];B = rgba.val[2];
	Y = (77*R+150*G+29*B)>>8;
	U = ((-44*R-87*G+131*B)>>8)+128;
	V = ((131*R-110*G-21*B)>>8)+128;
	colorYUVFlag = (Y&0xff)|((U&0xff)<<8)|((V&0xff)<<16);
	colorRGBAFlag = rgba;
	curThicknessFlag = thickness;
	return OSA_SOK;
}

static int setHideSysOsd(bool bHideOSD)
{
	svm->m_bHide = bHideOSD;
	blob->m_bHide = bHideOSD;
	motion->m_bHide = bHideOSD;
	mmtd->m_bHide = bHideOSD;
	general->m_bHide = bHideOSD;
	return OSA_SOK;
}


/************************************************************************
 *      process unit
 *
 */

class InputQueue
{
public:
	InputQueue(){};
	virtual ~InputQueue(){destroy();};
	int create(int nChannel, int nBuffer = 2)
	{
		int ret = OSA_SOK;
		OSA_assert(nChannel > 0);
		OSA_assert(queue.size() == 0);
		for(int chId=0; chId<nChannel; chId++){
			OSA_BufHndl* hndl = new OSA_BufHndl;
			OSA_assert(hndl != NULL);
			ret = image_queue_create(hndl, nBuffer, 0, memtype_null);
			OSA_assert(ret == OSA_SOK);
			for(int i=0; i<hndl->numBuf; i++)
			{
				OSA_BufInfo* bufInfo = &hndl->bufInfo[i];
				struct v4l2_buffer *vbuf = new struct v4l2_buffer;
				OSA_assert(vbuf != NULL);
				memset(vbuf, 0, sizeof(vbuf));
				bufInfo->resource = vbuf;
			}
			queue.push_back(hndl);
		}
		return ret;
	}
	int destroy()
	{
		for(int chId=0; chId<queue.size(); chId++){
			OSA_BufHndl* hndl = queue[chId];
			OSA_assert(hndl != NULL);
			for(int i=0; i<hndl->numBuf; i++){
				struct v4l2_buffer *vbuf = (struct v4l2_buffer *)hndl->bufInfo[i].resource;
				if(vbuf != NULL)
					delete vbuf;
				hndl->bufInfo[i].resource = NULL;
			}
			image_queue_delete(hndl);
			delete hndl;
			queue[chId] = NULL;
		}
		queue.clear();
		return OSA_SOK;
	}
	__inline__ uint64 getCurrentTime(void)
	{
		uint64 ret = 0l;
		//struct timeval now;
		//gettimeofday(&now, NULL);
		//ret = (uint64)now.tv_sec * 1000000000ul + (uint64)now.tv_usec*1000ul;
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ret = (uint64)ts.tv_sec * 1000000000ul + (uint64)ts.tv_nsec;
		return ret;
	}

	int input(int chId,unsigned char *data, const struct v4l2_buffer *vbuf, cv::Size vSize, int format, int channels)
	{
		int ret = OSA_SOK;
		//OSA_printf("%s %d: ch%d(%ld) %d x %d %d", __func__, __LINE__,
		//		chId, queue.size(), vSize.width, vSize.height, channels);
		if(chId>=queue.size()) return OSA_EFAIL;
		OSA_assert(data != NULL);
		OSA_assert(vbuf != NULL);

		OSA_BufHndl* hndl = queue[chId];
		OSA_assert(hndl != NULL);
		OSA_BufInfo* bufInfo = image_queue_getEmpty(hndl);
		if(bufInfo != NULL){
			memcpy(bufInfo->resource, vbuf, sizeof(struct v4l2_buffer));
			bufInfo->chId = chId;
			bufInfo->width = vSize.width;
			bufInfo->height = vSize.height;
			bufInfo->format = format;
			bufInfo->channels = channels;
			//OSA_assert(bufInfo->channels == 2);
			bufInfo->flags = (int)vbuf->flags;
			bufInfo->timestampCap = (uint64)vbuf->timestamp.tv_sec*1000000000ul
					+ (uint64)vbuf->timestamp.tv_usec*1000ul;
			bufInfo->timestamp = (uint64_t)getTickCount();
			//OSA_printf("bufInfo->timestampCap %ld %ld %ld",bufInfo->timestampCap, bufInfo->timestamp, getCurrentTime());
			bufInfo->virtAddr = data;
			image_queue_putFull(hndl, bufInfo);
			//OSA_printf("%s %d: %d x %d c%d", __func__, __LINE__, bufInfo->width, bufInfo->height, bufInfo->channels);
		}else{
			//if(chId != 1)
			OSA_printf("InputQueue %s %d: ch%d over flow", __func__, __LINE__, chId);
		}

		return ret;
	}
public:
	OSA_BufInfo* getFullQueue(int chId){
		if(chId>=queue.size())
			return NULL;
		return image_queue_getFull(queue[chId]);
	}
	int getFullQueueCount(int chId){
		if(chId>=queue.size())
			return -1;
		return image_queue_fullCount(queue[chId]);
	}
	int putEmptyQueue(OSA_BufInfo* info){
		OSA_assert(info->chId < queue.size());
		return image_queue_putEmpty(queue[info->chId], info);
	}
protected:
	std::vector<OSA_BufHndl*> queue;
};

static InputQueue *inputQ = NULL;
static OSA_BufHndl *imgQRender[CORE_CHN_MAX] = {NULL,};
static OSA_BufHndl *imgQEnc[CORE_CHN_MAX] = {NULL,};
static OSA_SemHndl *imgQEncSem[CORE_CHN_MAX] = {NULL,};
static unsigned char *memsI420[CORE_CHN_MAX] = {NULL,};
static cv::Mat imgOsd[CORE_CHN_MAX];
static OSA_MutexHndl *cumutex = NULL;
static void glosdInit(void);
static void renderCall(int displayId, int stepIdx, int stepSub, int context);

static int init(CORE1001_INIT_PARAM *initParam, OSA_SemHndl *notify = NULL, CGluVideoWindow *videoWindow = NULL)
{
	int ret = OSA_SOK;
	int chId;
	unsigned char *mem = NULL;
	OSA_assert(initParam != NULL);
	int channels = initParam->nChannels;
	bool bEncoder = initParam->bEncoder;
	bool bRender = initParam->bRender;
	bool bHideOSD = initParam->bHideOSD;
	localInit(channels, bEncoder);
	renderHook = initParam->renderHook;
	if(initParam->encoderParamTab[0]!=NULL){
		for(chId=0; chId<channels; chId++){
			userEncParamTab0[chId][0] = initParam->encoderParamTab[0];
			userEncParamTab1[chId][0] = initParam->encoderParamTab[0];
		}
	}
	if(initParam->encoderParamTab[1]!=NULL){
		for(chId=0; chId<channels; chId++){
			userEncParamTab0[chId][1] = initParam->encoderParamTab[1];
			userEncParamTab1[chId][1] = initParam->encoderParamTab[1];
		}
	}
	if(initParam->encoderParamTab[2]!=NULL){
		for(chId=0; chId<channels; chId++){
			userEncParamTab0[chId][2] = initParam->encoderParamTab[2];
			userEncParamTab1[chId][2] = initParam->encoderParamTab[2];
		}
	}
	for(chId=0; chId<channels; chId++){
		if(initParam->encoderParamTabMulti[chId][0]!=NULL)
			userEncParamTab1[chId][0] = initParam->encoderParamTabMulti[chId][0];
		if(initParam->encoderParamTabMulti[chId][1]!=NULL)
			userEncParamTab1[chId][1] = initParam->encoderParamTabMulti[chId][1];
		if(initParam->encoderParamTabMulti[chId][2]!=NULL)
			userEncParamTab1[chId][2] = initParam->encoderParamTabMulti[chId][2];
	}

	cumutex = new OSA_MutexHndl;
	OSA_mutexCreate(cumutex);
	cuConvertInit(channels, cumutex);

	for(chId=0; chId<channels; chId++){
		CORE1001_CHN_INIT_PARAM *chInf = &initParam->chnInfo[chId];
		int width = chInf->imgSize.width;
		int height = chInf->imgSize.height;
		channelsImgSize[chId] = chInf->imgSize;
		channelsFormat[chId] = chInf->format;
		channelsFPS[chId] = chInf->fps;
		ret = cudaHostAlloc((void**)&memsI420[chId], width*height*3/2, cudaHostAllocDefault);
		ret = cudaHostAlloc((void**)&mem, width*height, cudaHostAllocDefault);
		imgOsd[chId] = Mat(height, width, CV_8UC1, mem);
		memset(imgOsd[chId].data, 0, imgOsd[chId].cols*imgOsd[chId].rows*imgOsd[chId].channels());
		if(bEncoder){
			vOSDs[chId] = new cr_osd::DCOSD(&imgOsd[chId], fontSizeVideo[chId], fileNameFont[chId]);
			cr_osd::vosdFactorys.push_back(vOSDs[chId]);
		}
	}

	if(bEncoder)
	{
		int iLevel = curTransLevel;
		ENCTRAN_InitPrm enctranInit;
		memset(&enctranInit, 0, sizeof(enctranInit));
		enctranInit.iTransLevel = iLevel;
		enctranInit.nChannels = channels;
		for(chId=0; chId<channels; chId++){
			CORE1001_CHN_INIT_PARAM *chInf = &initParam->chnInfo[chId];
			int* params = getEncParamTab(chId, iLevel);
			enctranInit.defaultEnable[chId] = enableEncoderFlag[chId];
			enctranInit.imgSize[chId] = channelsImgSize[chId];
			enctranInit.inputFPS[chId] = chInf->fps;
			enctranInit.encPrm[chId].fps = chInf->fps;
			enctranInit.encPrm[chId].bitrate = params[0];
			enctranInit.encPrm[chId].minQP = params[1];
			enctranInit.encPrm[chId].maxQP = params[2];
			enctranInit.encPrm[chId].minQI = params[3];
			enctranInit.encPrm[chId].maxQI = params[4];
			enctranInit.encPrm[chId].minQB = params[5];
			enctranInit.encPrm[chId].maxQB = params[6];
			enctranInit.srcType[chId] = APPSRC;
		}
		if(initParam->encStreamIpaddr != NULL){
			enctranInit.bRtp = true;
			strcpy(enctranInit.destIpaddr, initParam->encStreamIpaddr);
		}

		enctran = new CEncTrans;
		OSA_assert(enctran != NULL);

		enctran->create();
		enctran->init(&enctranInit);
		enctran->run();
		for(chId=0; chId<channels; chId++){
			imgQEnc[chId] = enctran->m_bufQue[chId];
			imgQEncSem[chId] = enctran->m_bufSem[chId];
		}
	}

	if(bRender)
	{
		renderFPS = initParam->renderFPS;
		if(renderFPS<=0)
			renderFPS = 30;

		if(videoWindow == NULL){
			VWIND_Prm dsInit;
			memset(&dsInit, 0, sizeof(dsInit));
			dsInit.disSched = initParam->renderSched;
			if(dsInit.disSched<=0.00001)
				dsInit.disSched = 3.5;
			dsInit.bFullScreen = true;
			dsInit.nChannels = channels;
			dsInit.memType = memtype_glpbo;//memtype_glpbo;//memtype_cuhost;//memtype_cudev;
			dsInit.nQueueSize = 4;
			dsInit.disFPS = initParam->renderFPS;
			dsInit.renderfunc = renderCall;
			//dsInit.winWidth = initParam->renderSize.width;
			//dsInit.winHeight = initParam->renderSize.height;
			for(chId=0; chId<channels; chId++){
				dsInit.channelInfo[chId].w = channelsImgSize[chId].width;
				dsInit.channelInfo[chId].h = channelsImgSize[chId].height;
				dsInit.channelInfo[chId].fps = initParam->chnInfo[chId].fps;
				if(!bEncoder && channelsFormat[chId] == V4L2_PIX_FMT_GREY)
					dsInit.channelInfo[chId].c = 1;
				else
					dsInit.channelInfo[chId].c = 3;
			}
			render = new CGluVideoWindow(initParam->renderRC);
			render->Create(dsInit);
		}else{
			videoWindow->m_initPrm.renderfunc = renderCall;
			render = videoWindow;
		}
		for(chId=0; chId<channels; chId++){
			imgQRender[chId] = render->m_bufQue[chId];
		}
		inputQ = new InputQueue;
		inputQ->create(channels, 4);
		glosdInit();

		subRc = render->m_vvideoRenders[1]->m_viewPort;//render->m_renders[1].displayrect;
		subMatric = render->m_vvideoRenders[1]->m_matrix;//render->m_renders[1].transform;
	}

	scene  = new CSceneProcess();
	svm = new CSVMDetectProcess(scene);
	blob = new CBlobDetectProcess(svm);
	bkgd = new CBkgdDetectProcess(blob);
	motion = new CMotionDetectProcess(bkgd);
	mmtd = new CMMTDProcess(motion);
	general = new CGeneralProc(notify,mmtd);

	svm->m_bHide = bHideOSD;
	blob->m_bHide = bHideOSD;
	motion->m_bHide = bHideOSD;
	mmtd->m_bHide = bHideOSD;
	general->m_bHide = bHideOSD;

	for(chId=0; chId<channels; chId++){
		if(!bEncoder && bRender)
			general->m_dc[chId] = cv::Mat(initParam->renderRC.height, initParam->renderRC.width, CV_8UC1);
		else
			general->m_dc[chId] = imgOsd[chId];

		general->m_vosds[chId] = vOSDs[chId];
		general->m_imgSize[chId] = channelsImgSize[chId];
	}

	proc = general;
	general->creat();
	general->init();
	general->run();

	return ret;
}

static int uninit()
{
	general->stop();
	general->destroy();
	enctran->stop();
	enctran->destroy();
	if(render != NULL){
		render->Destroy();
		delete render;
		render = NULL;
	}

	if(enctran == NULL){
		for(int i=0; i<cr_osd::vosdFactorys.size(); i++)
			delete cr_osd::vosdFactorys[i];
	}
	cr_osd::vosdFactorys.clear();

	delete enctran;
	delete general;
	delete mmtd;
	delete motion;
	delete bkgd;
	delete blob;
	delete svm;
	delete scene;
	for(int chId=0; chId<nValidChannels; chId++){
		cudaFreeHost(imgOsd[chId].data);
		cudaFreeHost(memsI420[chId]);
		if(vOSDs[chId] != NULL)
			delete vOSDs[chId];
		vOSDs[chId] = NULL;
	}

	cuConvertUinit();
	if(cumutex != NULL){
		OSA_mutexUnlock(cumutex);
		OSA_mutexLock(cumutex);
		OSA_mutexDelete(cumutex);
		delete cumutex;
		cumutex = NULL;
	}
	if(inputQ != NULL)
		delete inputQ;
}
#define ZeroCpy	(0)
static void encTranFrame(int chId, const Mat& img, const struct v4l2_buffer& bufInfo, bool bEnc)
{
	Mat i420;
	OSA_BufInfo* info = NULL;
	unsigned char *mem = NULL;

	enctran->scheduler(chId);

	if(bEnc){
		if(ZeroCpy){
			if(imgQEnc[chId] != NULL)
				info = image_queue_getEmpty(imgQEnc[chId]);
		}
		if(info != NULL){
			info->chId = chId;
			info->timestampCap = (uint64)bufInfo.timestamp.tv_sec*1000000000ul
					+ (uint64)bufInfo.timestamp.tv_usec*1000ul;
			info->timestamp = (uint64_t)getTickCount();
			mem = (unsigned char *)info->virtAddr;
		}
	}

	if(mem == NULL)
		mem = memsI420[chId];
	i420 = Mat((int)(img.rows+img.rows/2), img.cols, CV_8UC1, mem);
	if(enableOSDFlag){
		if(enableEnhFlag[chId])
			cuConvertEnh_async(chId, img, imgOsd[chId], i420, ezoomxFlag[chId], colorYUVFlag);
		else
			cuConvert_async(chId, img, imgOsd[chId], i420, ezoomxFlag[chId], colorYUVFlag);
	}else{
		if(enableEnhFlag[chId])
			cuConvertEnh_async(chId, img, i420, ezoomxFlag[chId]);
		else
			cuConvert_async(chId, img, i420, ezoomxFlag[chId]);
	}

	if(bEnc){
		if(!ZeroCpy)
			enctran->pushData(i420, chId, V4L2_PIX_FMT_YUV420M);
		if(info != NULL){
			info->channels = i420.channels();
			info->width = i420.cols;
			info->height = i420.rows;
			info->format = V4L2_PIX_FMT_YUV420M;
			image_queue_putFull(imgQEnc[chId], info);
			OSA_semSignal(imgQEncSem[chId]);
		}
	}
}
#undef ZeroCpy

static void renderFrameAfterEnc(int chId, const Mat& img, const struct v4l2_buffer& bufInfo)
{
	Mat bgr;
	OSA_BufInfo* info = image_queue_getEmpty(imgQRender[chId]);
	if(info != NULL)
	{
		bgr = Mat(img.rows,img.cols,CV_8UC3, info->physAddr);
		cuConvertConn_yuv2bgr_i420(chId, bgr, CUT_FLAG_devAlloc);
		info->chId = chId;
		info->channels = bgr.channels();
		info->width = bgr.cols;
		info->height = bgr.rows;
		info->format = V4L2_PIX_FMT_BGR24;
		info->timestampCap = (uint64)bufInfo.timestamp.tv_sec*1000000000ul
				+ (uint64)bufInfo.timestamp.tv_usec*1000ul;
		info->timestamp = (uint64_t)getTickCount();
		image_queue_putFull(imgQRender[chId], info);
	}else{
		//OSA_printf("%s %d: overflow!", __func__, __LINE__);
		//render->m_timerRun = false;
	}
}

static unsigned long render_i[CORE_CHN_MAX] = {0, 0, 0, 0};
static unsigned long render_to[CORE_CHN_MAX] = {0, 0, 0, 0};
static void renderFrame(int chId, const Mat& img, const struct v4l2_buffer& bufInfo, int format)
{
	bool bRender = (chId == curChannelFlag || chId == curSubChannelIdFlag || bindBlendFlag[chId] != 0);

	if(render_i[chId]==render_to[chId])
	{
		if(bRender && imgQRender[chId] != NULL)
		{
			Mat frame;
			OSA_BufInfo* info = image_queue_getEmpty(imgQRender[chId]);
			if(info != NULL)
			{
				if(format==V4L2_PIX_FMT_YUYV){
					frame = Mat(img.rows,img.cols,CV_8UC3, info->physAddr);
					if(enableEnhFlag[chId])
						cuConvertEnh_yuv2bgr_yuyv_async(chId, img, frame, CUT_FLAG_devAlloc);
					else{
						cuConvert_yuv2bgr_yuyv_async(chId, img, frame, CUT_FLAG_devAlloc);
					}
					info->format = V4L2_PIX_FMT_BGR24;
				}
				else if(format==V4L2_PIX_FMT_BGR24){
					frame = Mat(img.rows,img.cols,CV_8UC3, info->physAddr);
					cudaMemcpy(info->physAddr, img.data, frame.rows*frame.cols*frame.channels(), cudaMemcpyHostToHost);
					info->format = V4L2_PIX_FMT_BGR24;
					//OSA_printf("%s %d: %d x %d", __func__, __LINE__, frame.cols, frame.rows);
				}
				else//if(format==V4L2_PIX_FMT_GREY)
				{
					frame = Mat(img.rows,img.cols,CV_8UC1, info->physAddr);
					if(enableEnhFlag[chId])
						cuConvertEnh_gray(chId, img, frame, CUT_FLAG_devAlloc);
					else
						cudaMemcpy(info->physAddr, img.data, frame.rows*frame.cols*frame.channels(), cudaMemcpyHostToHost);
					info->format = format;//V4L2_PIX_FMT_GREY;
				}
				info->chId = chId;
				info->channels = frame.channels();
				info->width = frame.cols;
				info->height = frame.rows;
				info->timestampCap = (uint64)bufInfo.timestamp.tv_sec*1000000000ul
						+ (uint64)bufInfo.timestamp.tv_usec*1000ul;
				info->timestamp = (uint64_t)getTickCount();
				image_queue_putFull(imgQRender[chId], info);
			}
			else{
				OSA_printf("core %s %d: ch%d overflow!", __func__, __LINE__, chId);
				image_queue_switchEmpty(imgQRender[chId]);
			}
		}
		int inc = channelsFPS[chId]/renderFPS;
		if(inc<=0)inc = 1;
		render_to[chId]=render_i[chId]+inc;
	}
	render_i[chId]++;
}

static void processFrameAtOnce(int cap_chid, unsigned char *src, const struct v4l2_buffer& capInfo, int format)
{
	if(capInfo.flags & V4L2_BUF_FLAG_ERROR)
		return;

	uint64_t timestamp = (uint64_t)getTickCount();

	if(curChannelFlag == cap_chid /*|| curSubChannelIdFlag == cap_chid*/ || bindBlendFlag[cap_chid] != 0){
		Mat img;
		if(format==V4L2_PIX_FMT_YUYV)
		{
			img	= Mat(channelsImgSize[cap_chid].height,channelsImgSize[cap_chid].width,CV_8UC2, src);
		}
		else if(format==V4L2_PIX_FMT_GREY)
		{
			img = Mat(channelsImgSize[cap_chid].height,channelsImgSize[cap_chid].width,CV_8UC1,src);
		}
		else if(format==V4L2_PIX_FMT_BGR24){
			img = Mat(channelsImgSize[cap_chid].height,channelsImgSize[cap_chid].width,CV_8UC3,src);
		}else{
			OSA_assert(0);
		}
		proc->process(cap_chid, curFovIdFlag[cap_chid], ezoomxFlag[cap_chid], img, timestamp);
	}

}

static void processFrame(int cap_chid, unsigned char *src, const struct v4l2_buffer& capInfo, int format)
{
	Mat img;

	//if(capInfo.flags & V4L2_BUF_FLAG_ERROR)
	//	return;
	//struct timespec ns0;
	//clock_gettime(CLOCK_MONOTONIC_RAW, &ns0);
	if(format==V4L2_PIX_FMT_YUYV)
	{
		//OSA_printf("%s ch%d %d", __func__, cap_chid, OSA_getCurTimeInMsec());
		//OSA_printf("%s: %d ch%d %p", __func__, __LINE__, cap_chid, src);
		img	= Mat(channelsImgSize[cap_chid].height,channelsImgSize[cap_chid].width,CV_8UC2, src);
	}
	else if(format==V4L2_PIX_FMT_GREY)
	{
		//OSA_printf("%s ch%d %d", __func__, cap_chid, OSA_getCurTimeInMsec());
		//OSA_printf("%s: %d ch%d %p", __func__, __LINE__, cap_chid, src);
		img = Mat(channelsImgSize[cap_chid].height,channelsImgSize[cap_chid].width,CV_8UC1,src);
	}
	else if(format==V4L2_PIX_FMT_BGR24){
		img = Mat(channelsImgSize[cap_chid].height,channelsImgSize[cap_chid].width,CV_8UC3,src);
		//OSA_printf("%s %d: %d x %d", __func__, __LINE__, img.cols, img.rows);
	}else{
		OSA_assert(0);
	}

	//if(imgQEnc[cap_chid] != NULL && vOSDs[cap_chid] != NULL){
	//	OSA_assert(enctran != NULL);
	if(vOSDs[cap_chid] != NULL){
		vOSDs[cap_chid]->begin(colorRGBAFlag, curThicknessFlag);
	}

	if(curChannelFlag == cap_chid /*|| curSubChannelIdFlag == cap_chid*/ || bindBlendFlag[cap_chid] != 0)
		proc->OnOSD(cap_chid, curFovIdFlag[cap_chid], ezoomxFlag[cap_chid], general->m_dc[cap_chid], general->m_vosds[cap_chid]);

	if(bindBlendFlag[cap_chid] != 0 && render != NULL && bkgd != NULL){
		GLV_BlendPrm prm;
		prm.fAlpha = bkgd->m_alpha;
		prm.thr0Min = bkgd->m_thr0Min;
		prm.thr0Max = bkgd->m_thr0Max;
		prm.thr1Min = bkgd->m_thr1Min;
		prm.thr1Max = bkgd->m_thr1Max;
		for(int chId=0; chId<VWIN_CHAN_MAX; chId++){
			if((bindBlendFlag[cap_chid] & (1<<chId))!=0)
				render->dynamic_config(CGluVideoWindow::VWIN_CFG_BlendPrm, chId*VWIN_CHAN_MAX+cap_chid, &prm);
		}
	}

	if(imgQEnc[cap_chid] != NULL)
	{
		if(vOSDs[cap_chid] != NULL && enableOSDFlag)
			vOSDs[cap_chid]->Draw();
		encTranFrame(cap_chid, img, capInfo, enableEncoderFlag[cap_chid]);
		if(imgQRender[cap_chid] != NULL)
			renderFrameAfterEnc(cap_chid, img, capInfo);
		if(vOSDs[cap_chid] != NULL)
			vOSDs[cap_chid]->end();
	}
	else if(imgQRender[cap_chid] != NULL)
	{
		/*if(!isEqual(ezoomxFlag[cap_chid]*1.0, scaleFlag[cap_chid])){
			scaleFlag[cap_chid] = ezoomxFlag[cap_chid]*1.0;
			GLfloat fscale = scaleFlag[cap_chid];
			GLfloat mat4[16] ={
				fscale, 0.0f, 0.0f, 0.0f,
				0.0f, fscale, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};
			render->dynamic_config(CGluVideoWindow::DS_CFG_VideoTransMat, cap_chid, mat4);
		}*/
		if(curChannelFlag == cap_chid && !isEqual(ezoomxFlag[cap_chid]*1.0, scaleFlag[0])){
			scaleFlag[0] = ezoomxFlag[cap_chid]*1.0;
			GLfloat fscale = scaleFlag[0];
			GLfloat mat4[16] ={
				fscale, 0.0f, 0.0f, 0.0f,
				0.0f, fscale, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};
			render->dynamic_config(CGluVideoWindow::VWIN_CFG_ViewTransMat, 0, mat4);
		}
		//render->m_bOsd = enableOSDFlag;
		renderFrame(cap_chid, img, capInfo, format);
	}

	//struct timespec ns1;
	//clock_gettime(CLOCK_MONOTONIC_RAW, &ns1);
	//printf("[%ld.%ld] ch%d timestamp %ld.%ld flags %08X\n", ns1.tv_sec, ns1.tv_nsec/1000000,
	//		cap_chid, ns0.tv_sec, ns0.tv_nsec/1000000, info.flags);
}

static void videoInput(int cap_chid, unsigned char *src, const struct v4l2_buffer& capInfo, int format)
{
	if(capInfo.flags & V4L2_BUF_FLAG_ERROR){
		//OSA_printf("%s %d: ch%d V4L2_BUF_FLAG_ERROR(0x%x)", __func__, __LINE__, cap_chid, capInfo.flags);
		return;
	}
	//if(capInfo.flags & V4L2_BUF_FLAG_ERROR)
	//	return;
	processFrameAtOnce(cap_chid,src, capInfo, format);
	if(inputQ != NULL){
		int channels = ( (format == V4L2_PIX_FMT_GREY) ? 1 : ((format == V4L2_PIX_FMT_BGR24 || format == V4L2_PIX_FMT_RGB24) ? 3 : 2) );
		inputQ->input(cap_chid, src, &capInfo, channelsImgSize[cap_chid], format, channels);
		OSA_assert(render != NULL);
	}else{
		processFrame(cap_chid,src, capInfo, format);
	}
}

static void glosdInit(void)
{
#if 1
	cr_osd::glShaderManager.InitializeStockShaders();
	for(int chId=0; chId<nValidChannels; chId++){
		if(vOSDs[chId] == NULL){
			vOSDs[chId] = new cr_osd::GLOSD(render->m_rc.width, render->m_rc.height, fontSizeVideo[chId], fileNameFont[chId]);
		}
	}
	if(enctran == NULL){
		glosdFront = new cr_osd::GLOSD(render->m_rc.width, render->m_rc.height, fontSizeRender, fileNameFontRender);
		cr_osd::vosdFactorys.push_back(glosdFront);
	}
#else
	if(enctran == NULL){
		cr_osd::glShaderManager.InitializeStockShaders();
		glosdFront = new cr_osd::GLOSD(render->m_mainWinWidth, render->m_mainWinHeight, fontSizeRender, fileNameFontRender);
		for(int chId=0; chId<nValidChannels; chId++){
			if(vOSDs[chId] == NULL)
				vOSDs[chId] = glosdFront;
		}
		cr_osd::vosdFactorys.push_back(glosdFront);
	}
#endif
}

static void renderCall(int displayId, int stepIdx, int stepSub, int context)
{
	if(renderHook != NULL)
		renderHook(displayId, stepIdx, stepSub, context);

	if(enctran == NULL)
	{
		OSA_assert(imgQEnc[0] == NULL);
		/*if(stepIdx == CGluVideoWindow::RUN_ENTER)
		{
			for(int chId =0; chId<nValidChannels; chId++){
				if(vOSDs[chId] != NULL)
					vOSDs[chId]->begin(colorRGBAFlag, curThicknessFlag);
			}
		}*/
		if(stepIdx == CGluVideoWindow::RUN_WIN)
		{
			if(enableOSDFlag && stepSub == 0 && vOSDs[context] != NULL){
				vOSDs[context]->Draw();
			}
		}
		if(stepIdx == CGluVideoWindow::RUN_SWAP)
		{
			for(int chId =0; chId<nValidChannels; chId++){
				if(vOSDs[chId] != NULL)
					vOSDs[chId]->end();
			}
			if(glosdFront != NULL)
				glosdFront->Draw();
		}
	}

	if(stepIdx == CGluVideoWindow::RUN_ENTER)
	{
		if(inputQ == NULL)
			return ;
	#if 0
		for(int chId = 0; chId < nValidChannels; chId++){
			OSA_BufInfo* info = NULL;
			info = inputQ->getFullQueue(chId);
			if(info == NULL)
				continue;
			OSA_assert(info->chId == chId);
			processFrame(chId, (unsigned char*)info->virtAddr, *(struct v4l2_buffer*)info->resource, info->format);
			inputQ->putEmptyQueue(info);
		}
	#endif
	#if 0
		for(int chId = 0; chId < nValidChannels; chId++){
			OSA_BufInfo* info = NULL;
			int nFull = inputQ->getFullQueueCount(chId);
			for(int i=0; i<nFull-1; i++){
				info = inputQ->getFullQueue(chId);
				inputQ->putEmptyQueue(info);
			}
			info = inputQ->getFullQueue(chId);
			if(info == NULL)
				continue;
			OSA_assert(info->chId == chId);
			processFrame(chId, (unsigned char*)info->virtAddr, *(struct v4l2_buffer*)info->resource, info->format);
			inputQ->putEmptyQueue(info);
		}
	#endif
	#if 1
		for(int chId = 0; chId < nValidChannels; chId++){
			OSA_BufInfo* info = NULL;
			int nFull = inputQ->getFullQueueCount(chId);
			for(int i=0; i<nFull; i++){
				info = inputQ->getFullQueue(chId);
				OSA_assert(info->chId == chId);
				processFrame(chId, (unsigned char*)info->virtAddr, *(struct v4l2_buffer*)info->resource, info->format);
				inputQ->putEmptyQueue(info);
			}
		}
	#endif
	}
}//void renderCall(int displayId, int stepIdx, int stepSub, int context)

};//namespace cr_local

class Core_1001 : public ICore_1001
{
	Core_1001():m_notifySem(NULL),m_bRun(false){
		coreId = ID;
		memset(&m_stats, 0, sizeof(m_stats));
	};
	virtual ~Core_1001(){uninit();};
	friend ICore* ICore::Qury(int coreID);
	OSA_SemHndl m_updateSem;
	OSA_SemHndl *m_notifySem;
	bool m_bRun;
	void update();
	static void *thrdhndl_update(void *context)
	{
		Core_1001 *core = (Core_1001 *)context;
		while(core->m_bRun){
			OSA_semWait(&core->m_updateSem, OSA_TIMEOUT_FOREVER);
			if(!core->m_bRun)
				break;
			core->update();
			if(core->m_notifySem != NULL)
				OSA_semSignal(core->m_notifySem);
		}
		return NULL;
	}
public:
	static char m_version[16];
	static unsigned int ID;
	static wchar_t sztest[128];
	virtual int init(void *pParam, int paramSize)
	{
		printf("\r\n crCore(ID:%08X) v%s--------------Build date: %s %s \r\n",
				ID, m_version, __DATE__, __TIME__);

		int ret = OSA_SOK;
		int nChannels = 0;
		OSA_semCreate(&m_updateSem, 1, 0);
		if(sizeof(CORE1001_INIT_PARAM) == paramSize){
			CORE1001_INIT_PARAM *initParam = (CORE1001_INIT_PARAM*)pParam;
			m_notifySem = initParam->notify;
			ret = cr_local::init(initParam, &m_updateSem);
			nChannels = initParam->nChannels;
		}else if(sizeof(CORE1001_INIT_PARAM2) == paramSize){
			CORE1001_INIT_PARAM2 *initParam2 = (CORE1001_INIT_PARAM2*)pParam;
			CORE1001_INIT_PARAM initParam;
			memset(&initParam, 0, sizeof(initParam));
			memcpy(initParam.chnInfo, initParam2->chnInfo, sizeof(initParam.chnInfo));
			initParam.nChannels = initParam2->nChannels;
			initParam.notify = initParam2->notify;
			initParam.bEncoder = initParam2->bEncoder;
			initParam.bHideOSD = initParam2->bHideOSD;
			initParam.encStreamIpaddr = initParam2->encStreamIpaddr;
			memcpy(initParam.encoderParamTab, initParam2->encoderParamTab, sizeof(initParam.encoderParamTab));
			memcpy(initParam.encoderParamTabMulti, initParam2->encoderParamTabMulti, sizeof(initParam.encoderParamTabMulti));
			if(initParam2->videoWindow != NULL){
				initParam.bRender = true;
				initParam.renderRC = initParam2->videoWindow->m_rc;
				initParam.renderFPS = initParam2->videoWindow->m_initPrm.disFPS;
				initParam.renderSched = initParam2->videoWindow->m_initPrm.disSched;
				initParam.renderHook = initParam2->videoWindow->m_initPrm.renderfunc;
			}
			ret = cr_local::init(&initParam, &m_updateSem, initParam2->videoWindow);
			nChannels = initParam.nChannels;
		}else{
			OSA_assert(0);
		}
		memset(&m_stats, 0, sizeof(m_stats));
		update();
		for(int chId=0; chId<nChannels; chId++)
			m_dc[chId] = cr_local::general->m_dc[chId];
		m_bRun = true;
		start_thread(thrdhndl_update, this);
		cr_osd::put(sztest, cv::Point(800, 30), cv::Scalar(80, 80, 80, 10), 128, L"== ID:%x v%s ==", ID, m_version);
		return ret;
	}
	virtual int uninit()
	{
		m_bRun = false;
		OSA_semSignal(&m_updateSem);
		int ret = cr_local::uninit();
		cr_osd::clear();
		memset(&m_stats, 0, sizeof(m_stats));
		OSA_semDelete(&m_updateSem);
		return ret;
	}
	virtual void processFrame(int chId, unsigned char *data, struct v4l2_buffer capInfo, int format)
	{
		static unsigned int cnt = 10;
		if(capInfo.flags & V4L2_BUF_FLAG_ERROR){
			swprintf(sztest, 128, L"== ID:%x v%s ==", ID, m_version);
			cnt = 10;
		}else if(cnt==0){
			swprintf(sztest, 128, L"");
		}
		cnt = (cnt > 0) ? (cnt-1) : 0;
		cr_local::videoInput(chId, data, capInfo, format);
	}

	virtual int setMainChId(int chId, int fovId, int ndrop, cv::Size acqSize)
	{
		UTC_SIZE sz;
		sz.width = acqSize.width; sz.height = acqSize.height;
		int ret = cr_local::setMainChId(chId, fovId, ndrop, sz);
		update();
		return ret;
	}
	virtual int setSubChId(int chId)
	{
		int ret = cr_local::setSubChId(chId);
		update();
		return ret;
	}
	virtual int enableTrack(bool enable, cv::Size winSize, bool bFixSize)
	{
		UTC_SIZE sz;
		sz.width = winSize.width; sz.height = winSize.height;
		int ret = cr_local::enableTrack(enable, sz, bFixSize);
		update();
		return ret;
	}
	virtual int enableTrack(bool enable, Rect2f winRect, bool bFixSize)
	{
		UTC_RECT_float rc;
		rc.x = winRect.x; rc.y = winRect.y;
		rc.width = winRect.width; rc.height = winRect.height;
		int ret = cr_local::enableTrack(enable, rc, bFixSize);
		update();
		return ret;
	}
	virtual int enableMMTD(bool enable, int nTarget, int nSel = 0)
	{
		int ret = cr_local::enableMMTD(enable, nTarget, nSel);
		update();
		return ret;
	}
	virtual int enableMotionDetect(bool enable)
	{
		int ret = cr_local::enableMotionDetect(enable);
		update();
		return ret;
	}
	virtual int enableTrackByMMTD(int index, cv::Size *winSize, bool bFixSize)
	{
		int ret = cr_local::enableTrackByMMTD(index, winSize, bFixSize);
		update();
		return ret;
	}
	virtual int enableEnh(bool enable)
	{
		int ret = cr_local::enableEnh(enable);
		update();
		return ret;
	}
	virtual int enableEnh(int chId, bool enable)
	{
		int ret = cr_local::enableEnh(chId, enable);
		update();
		return ret;
	}
	virtual int enableBlob(bool enable)
	{
		int ret = cr_local::enableBlob(enable);
		update();
		return ret;
	}
	virtual int bindBlend(int blendchId, const cv::Matx44f& matric)
	{
		int ret = cr_local::bindBlend(blendchId, matric);
		update();
		return ret;
	}
	virtual int bindBlend(int chId, int blendchId, const cv::Matx44f& matric)
	{
		int ret = cr_local::bindBlend(chId, blendchId, matric);
		update();
		return ret;
	}
	virtual int enableOSD(bool enable)
	{
		int ret = cr_local::enableOSD(enable);
		update();
		return ret;
	}
	virtual int enableEncoder(int chId, bool enable)
	{
		int ret = cr_local::enableEncoder(chId, enable);
	}
	virtual int setAxisPos(cv::Point pos)
	{
		int ret = cr_local::setAxisPos(pos);
		update();
		return ret;
	}
	virtual int saveAxisPos()
	{
		int ret = cr_local::saveAxisPos();
		update();
		return ret;
	}
	virtual int setTrackPosRef(cv::Point2f ref)
	{
		int ret = cr_local::setTrackPosRef(ref);
		update();
		return ret;
	}
	virtual int setTrackCoast(int nFrames)
	{
		int ret = cr_local::setTrackCoast(nFrames);
		update();
		return ret;
	}
	virtual int setEZoomx(int value)
	{
		int ret = cr_local::setEZoomx(value);
		update();
		return ret;
	}
	virtual int setEZoomx(int chId, int value)
	{
		int ret = cr_local::setEZoomx(chId, value);
		update();
		return ret;
	}
	virtual int setWinPos(int winId, const cv::Rect& rc)
	{
		int ret = cr_local::setWinPos(winId, rc);
		update();
		return ret;
	}
	virtual int setWinMatric(int winId, const cv::Matx44f& matric)
	{
		int ret = cr_local::setWinMatric(winId, matric);
		update();
		return ret;
	}

	virtual int setOSDColor(int yuv, int thickness)
	{
		int ret = cr_local::setOSDColor(yuv, thickness);
		update();
		return ret;
	}
	virtual int setOSDColor(cv::Scalar color, int thickness)
	{
		int ret = cr_local::setOSDColor(color, thickness);
		update();
		return ret;
	}
	virtual int setEncTransLevel(int iLevel)
	{
		int ret = cr_local::setEncTransLevel(iLevel);
		update();
		return ret;
	}
	virtual int setHideSysOsd(bool bHideOSD)
	{
		int ret = cr_local::setHideSysOsd(bHideOSD);
		update();
		return ret;
	}
};
unsigned int Core_1001::ID = COREID_1001;
char Core_1001::m_version[16] = CORE_1001_VERSION_;
wchar_t Core_1001::sztest[128] = L"";

void Core_1001::update()
{
	//OSA_printf("%s %d: enter.", __func__, __LINE__);
	m_stats.mainChId = cr_local::curChannelFlag;
	m_stats.subChId = cr_local::curSubChannelIdFlag;
	m_stats.acqWinSize.width = cr_local::general->m_sizeAcqWin.width;
	m_stats.acqWinSize.height = cr_local::general->m_sizeAcqWin.height;
	m_stats.enableTrack = cr_local::enableTrackFlag;
	m_stats.enableMMTD = cr_local::enableMMTDFlag;
	m_stats.enableMotionDetect = cr_local::enableMotionDetectFlag;
	m_stats.enableBlob = cr_local::enableBlobFlag;
	m_stats.enableOSD = cr_local::enableOSDFlag;
	m_stats.iTrackorStat = cr_local::general->m_iTrackStat;
	UTC_RECT_float curRC = cr_local::general->m_rcTrk;
	m_stats.trackPos.x = curRC.x+curRC.width/2;
	m_stats.trackPos.y = curRC.y+curRC.height/2;
	m_stats.trackWinSize.width = curRC.width;
	m_stats.trackWinSize.height = curRC.height;
	m_stats.lossCoastFrames = cr_local::general->m_iTrackLostCnt;
	m_stats.lossCoastTelapse = cr_local::general->m_telapseLost;
	m_stats.subRc = cr_local::subRc;
	m_stats.subMatric = cr_local::subMatric;
	m_stats.colorYUV = cr_local::colorYUVFlag;
	m_stats.transLevel = cr_local::curTransLevel;
	if(cr_local::mmtd->m_bEnable){
		int cnt = min(MAX_TGT_NUM, CORE_TGT_NUM_MAX);
		for(int i=0; i<cnt; i++)
		{
			m_stats.tgts[i].valid = cr_local::mmtd->m_target[i].valid;
			m_stats.tgts[i].index = i;
			m_stats.tgts[i].Box = cr_local::mmtd->m_target[i].Box;
			m_stats.tgts[i].pos = tRectCenter(m_stats.tgts[i].Box);
		}
	}else if(cr_local::motion->m_bEnable){
		int chId = cr_local::motion->m_curChId;
		int cnt = cr_local::motion->m_targets[chId].size();
		int i;
		cnt = min(cnt, CORE_TGT_NUM_MAX);
		for(i=0; i<cnt; i++)
		{
			m_stats.tgts[i].valid = true;
			m_stats.tgts[i].index = cr_local::motion->m_targets[chId][i].index;
			m_stats.tgts[i].Box = cr_local::motion->m_targets[chId][i].targetRect;
			m_stats.tgts[i].pos = tRectCenter(m_stats.tgts[i].Box);
		}
		for(;i<CORE_TGT_NUM_MAX; i++)
			m_stats.tgts[i].valid = false;
	}
	m_stats.blob.valid = cr_local::blob->m_bValid;
	m_stats.blob.pos = cr_local::blob->m_pos;
	m_stats.blob.Box = cr_local::blob->m_rc;

	for(int chId = 0; chId < cr_local::nValidChannels; chId++){
		CORE1001_CHN_STATS *chn = &m_stats.chn[chId];
		chn->imgSize = cr_local::general->m_imgSize[chId];
		chn->fovId = cr_local::curFovIdFlag[chId];
		chn->axis.x = cr_local::general->m_AxisCalibX[chId][chn->fovId];
		chn->axis.y = cr_local::general->m_AxisCalibY[chId][chn->fovId];
		chn->enableEnh = cr_local::enableEnhFlag[chId];
		chn->iEZoomx = cr_local::ezoomxFlag[chId];
		chn->enableEncoder = cr_local::enableEncoderFlag[chId];//enctran->m_enable[chId];
		chn->frameTimestamp = cr_local::general->m_frameTimestamp[chId];
		chn->blendBindId = cr_local::bindBlendTFlag[chId];
		chn->blendMatric = cr_local::blendMatric[chId];
	}
}

static int rafcnt = 0;
static ICore *coreRaf = NULL;
ICore* ICore::Qury(int coreID)
{
	if(coreRaf != NULL){
		rafcnt ++;
		return coreRaf;
	}
	if(Core_1001::ID == coreID){
		coreRaf = new Core_1001;
		rafcnt =1;
	}

	return coreRaf;
}
void ICore::Release(ICore* core)
{
	if(core != NULL && coreRaf != NULL && core == coreRaf){
		rafcnt --;
		if(rafcnt == 0){
			delete coreRaf;
			coreRaf = NULL;
		}
	}
}

/*********************************************************************
 *
 *
 */
#include "crcoreSecondScreen.hpp"

typedef struct _secondScreen_ct{
	unsigned int teg;
	CGluVideoWindowSecond *videoWin;
	ICore_1001 *core;
	cr_osd::GLOSD *glosdFront;
	cv::Rect rc;
	int fps;
	int parent;
	bool bFull;
	int fontSize;
	char fontFile[256];
}SECONDSCREEN_CT;
#define SSCT_TEG	0x55ff00AA

static CSecondScreenBase *gSecondScreen = NULL;
static void SecondScreen_renderCall(int winId, int stepIdx, int stepSub, int context)
{
	OSA_assert(gSecondScreen != NULL);
	gSecondScreen->renderCall(stepIdx, stepSub, context);
}

CSecondScreenBase::CSecondScreenBase(const cv::Rect& rc, int fps, bool bFull, int fontSize, const char* fontFile)
{
	OSA_assert(gSecondScreen == NULL);
	gSecondScreen = this;
	SECONDSCREEN_CT *ctx = (SECONDSCREEN_CT*)malloc(sizeof(SECONDSCREEN_CT));
	memset(ctx, 0, sizeof(SECONDSCREEN_CT));
	ctx->teg = SSCT_TEG;
	ctx->rc = rc;
	ctx->fps = fps;
	ctx->parent = 0;
	ctx->bFull = bFull;
	ctx->videoWin = new CGluVideoWindowSecond(ctx->rc, ctx->parent);
	ctx->core = (ICore_1001 *)ICore::Qury(COREID_1001);
	ctx->glosdFront = NULL;
	ctx->fontSize = fontSize;
	if(fontFile != NULL)
		strcpy(ctx->fontFile, fontFile);
	else
		memset(ctx->fontFile, 0, sizeof(ctx->fontFile));

	OSA_printf("%s %d: %s rc(%d,%d,%d,%d)", __FILE__, __LINE__, __func__, rc.x, rc.y, rc.width, rc.height);

	CGluVideoWindowSecond *videoWin = ctx->videoWin;
    VWIND_Prm vwinPrm;
    memset(&vwinPrm, 0, sizeof(vwinPrm));
    vwinPrm.disFPS = ctx->fps;
    vwinPrm.bFullScreen = ctx->bFull;
	vwinPrm.renderfunc = SecondScreen_renderCall;
    int iRet = ctx->videoWin->Create(vwinPrm);
    OSA_assert(iRet == OSA_SOK);
	ctx->glosdFront = new cr_osd::GLOSD(videoWin->m_rc.width, videoWin->m_rc.height, fontSize, fontFile);
	OSA_assert(ctx->glosdFront != NULL);
	cr_osd::vosdFactorys.push_back(ctx->glosdFront);

	m_context = ctx;
	OSA_printf("[%d]%s %d %s", OSA_getCurTimeInMsec(), __FILE__, __LINE__, __func__);
}

CSecondScreenBase::~CSecondScreenBase()
{
	SECONDSCREEN_CT *ctx = (SECONDSCREEN_CT*)m_context;
	if(ctx!=NULL){
		if(ctx->glosdFront != NULL)
			delete ctx->glosdFront;
		delete ctx->videoWin;
		ICore::Release(ctx->core);
		free(m_context);
	}
	m_context = NULL;
}

void CSecondScreenBase::renderCall(int stepIdx, int stepSub, int context)
{
	SECONDSCREEN_CT *ctx = (SECONDSCREEN_CT*)m_context;
	OSA_assert(ctx->teg == SSCT_TEG);
	CGluVideoWindowSecond *videoWin = ctx->videoWin;

	OnRender(stepIdx, stepSub, context);
	if(stepIdx == CGluVideoWindow::RUN_WIN)
	{
		//if(cr_local::enableOSDFlag && stepSub == 0 && context > 0 && cr_local::vOSDs[context] != NULL){
		//	cr_local::vOSDs[context]->Draw();
		//}
	}
	if(stepIdx == CGluVideoWindow::RUN_SWAP)
	{
#if(0)
		using namespace cr_osd;
		OSA_assert(ctx->glosdFront != NULL);
		int width = ctx->glosdFront->m_viewport.width;
		int height = ctx->glosdFront->m_viewport.height;
		GLOSDLine line1(ctx->glosdFront);
		GLOSDLine line2(ctx->glosdFront);
		GLOSDRectangle rectangle1(ctx->glosdFront);
		GLOSDPolygon ploygon0(ctx->glosdFront, 3);
		GLOSDRect rect0(ctx->glosdFront);
		static Point ct(0, 0);
		static int incx = 1;
		static int incy = 1;
		if(ct.x<-(width/2-100) || ct.x>width/2-100)
			incx *= -1;
		if(ct.y<-(height/2-100) || ct.y>height/2-100)
			incy *= -1;
		ct.x += incx;
		ct.y += incy;
		Point center(width/2+ct.x, height/2+ct.y);
		line1.line(Point(center.x-100, center.y), Point(center.x+100, center.y), Scalar(0, 255, 0, 255), 2);
		line2.line(Point(center.x, center.y-100), Point(center.x, center.y+100), Scalar(255, 255, 0, 255), 2);
		rectangle1.rectangle(Rect(center.x-50, center.y-50, 100, 100), Scalar(255, 0, 0, 255), 1);
		cv::Point pts[] = {cv::Point(center.x, center.y-80),cv::Point(center.x-75, center.y+38),cv::Point(center.x+75, center.y+38)};
		ploygon0.polygon(pts, Scalar(0, 0, 255, 255), 3);
		rect0.rect(Rect(center.x-50, center.y-50, 100, 100), Scalar(28, 28, 28, 255), 6);
		//GLOSDLine line3(&glosd);
		//GLOSDLine line4(&glosd);
		//line3.line(Point(width/2-50, height/2), Point(width/2+50, height/2), Scalar(0, 255, 0, 255), 2);
		//line4.line(Point(width/2, height/2-50), Point(width/2, height/2+50), Scalar(255, 255, 0, 255), 2);
		GLOSDCross cross(ctx->glosdFront);
		cross.cross(Point(width/2, height/2), Size(50, 50), Scalar(255, 255, 0, 255), 1);
		GLOSDNumberedBox box(ctx->glosdFront);
		box.numbox(128, Rect(width/2-30, height/2-20, 60, 40), Scalar(255, 255, 0, 255), 1);
		GLOSDTxt txt1(ctx->glosdFront);
		GLOSDTxt txt2(ctx->glosdFront);
		static wchar_t strTxt1[128] = L"0";
		txt1.txt(Point(center.x-5, center.y-txt1.m_fontSize+10), strTxt1, Scalar(255, 0, 255, 128));
		static wchar_t strTxt2[128];
		swprintf(strTxt2, 128, L"%d, %d", center.x, center.y);
		txt2.txt(Point(center.x+10, center.y-txt1.m_fontSize-10), strTxt2, Scalar(255, 255, 255, 200));
#endif
		OSA_assert(ctx->glosdFront != NULL);
		ctx->glosdFront->Draw();
	}
}

int CSecondScreenBase::set(int winId, int chId, const cv::Rect& rc, const cv::Matx44f& matric)
{
	SECONDSCREEN_CT *ctx = (SECONDSCREEN_CT*)m_context;
	OSA_assert(ctx->teg == SSCT_TEG);
	CGluVideoWindowSecond *videoWin = ctx->videoWin;
	int iRet = videoWin->dynamic_config(CGluVideoWindow::VWIN_CFG_ChId, winId, &chId);
	if(iRet != OSA_SOK)
		return iRet;
	cv::Rect rcNew = rc;
	iRet = videoWin->dynamic_config(CGluVideoWindow::VWIN_CFG_ViewPos, winId, &rcNew);
	if(iRet != OSA_SOK)
		return iRet;
	cv::Matx44f matricNew = matric;
	return videoWin->dynamic_config(CGluVideoWindow::VWIN_CFG_ViewTransMat, winId, matricNew.val);
}

int CSecondScreenBase::set(int winId, int chId)
{
	SECONDSCREEN_CT *ctx = (SECONDSCREEN_CT*)m_context;
	OSA_assert(ctx->teg == SSCT_TEG);
	CGluVideoWindowSecond *videoWin = ctx->videoWin;
	return videoWin->dynamic_config(CGluVideoWindow::VWIN_CFG_ChId, winId, &chId);;
}

int CSecondScreenBase::set(int winId, const cv::Rect& rc)
{
	int iRet = OSA_SOK;
	cv::Rect rcNew = rc;
	SECONDSCREEN_CT *ctx = (SECONDSCREEN_CT*)m_context;
	OSA_assert(ctx->teg == SSCT_TEG);
	CGluVideoWindowSecond *videoWin = ctx->videoWin;
	iRet = videoWin->dynamic_config(CGluVideoWindow::VWIN_CFG_ViewPos, winId, &rcNew);
	return iRet;
}

int CSecondScreenBase::set(int winId, const cv::Matx44f& matric)
{
	int iRet = OSA_SOK;
	cv::Matx44f matricNew = matric;
	SECONDSCREEN_CT *ctx = (SECONDSCREEN_CT*)m_context;
	OSA_assert(ctx->teg == SSCT_TEG);
	CGluVideoWindowSecond *videoWin = ctx->videoWin;
	iRet = videoWin->dynamic_config(CGluVideoWindow::VWIN_CFG_ViewTransMat, winId, matricNew.val);
	return iRet;
}

