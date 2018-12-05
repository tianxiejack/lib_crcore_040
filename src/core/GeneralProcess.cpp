/*
 * processbase.cpp
 *
 *  Created on: May 5, 2017
 *      Author: ubuntu
 */
#include "GeneralProcess.hpp"
//#include "vmath.h"

#define CALIB_AXIS_FILE		"CalibAxisFile.yml"
#define CONFIG_AVT_FILE		"ConfigAvtFile.yml"

/*
static inline void my_rotate(GLfloat result[16], float theta)
{
	float rads = float(theta/180.0f) * CV_PI;
	const float c = cosf(rads);
	const float s = sinf(rads);

	memset(result, 0, sizeof(GLfloat)*16);

	result[0] = c;
	result[1] = -s;
	result[4] = s;
	result[5] = c;
	result[10] = 1.0f;
	result[15] = 1.0f;
}
*/
CGeneralProc::CGeneralProc(OSA_SemHndl *notifySem, IProcess *proc)
	:CTrackerProc(notifySem, proc),m_bHide(false)
{
}

CGeneralProc::~CGeneralProc()
{
}

void CGeneralProc::OnCreate(){}
void CGeneralProc::OnDestroy(){}
void CGeneralProc::OnInit()
{
	if(ReadCalibAxisFromFile() != 0)
	{
		for(int i=0; i<MAX_CHAN; i++){
			for(int j=0; j<MAX_NFOV_PER_CHAN; j++){
				if(m_AxisCalibX[i][j]<=0 || m_AxisCalibX[i][j]>1920)
				m_AxisCalibX[i][j] = 1920/2;
				if(m_AxisCalibY[i][j]<=0 || m_AxisCalibY[i][j]>1080)
				m_AxisCalibY[i][j] = 1080/2;
			}
		}
	}

	if(ReadCfgAvtFromFile() != 0)
	{
		UTC_DYN_PARAM dynamicParam;
		dynamicParam.occlusion_thred = 0.28;
		dynamicParam.retry_acq_thred = 0.38;
		UtcSetDynParam(m_track, dynamicParam);
		float up_factor = 0.0125;
		UtcSetUpFactor(m_track, up_factor);
		TRK_SECH_RESTRAINT resTraint;
		resTraint.res_distance = 80;
		resTraint.res_area = 5000;
		UtcSetRestraint(m_track, resTraint);
		UtcSetPLT_BS(m_track, tPLT_WRK, BoreSight_Mid);
	}
}
void CGeneralProc::OnConfig(int type, int iPrm, void* pPrm)
{
	switch(type)
	{
	case VP_CFG_SaveAxisToFile:
		WriteCalibAxisToFile();
		break;
	default:
		break;
	}
}
void CGeneralProc::OnRun()
{

}
void CGeneralProc::OnStop()
{

}
void CGeneralProc::Ontimer(){}
bool CGeneralProc::OnPreProcess(int chId, Mat &frame)
{
	return true;
}

__inline__ UTC_RECT_float tRectScale(UTC_RECT_float rc, cv::Size orgSize, cv::Size scaleSize)
{
	UTC_RECT_float ret;
	cv::Point2f fscale(scaleSize.width/orgSize.width, scaleSize.height/orgSize.height);
	ret.x = scaleSize.width/2.0f - (orgSize.width/2.0f - rc.x)*fscale.x;
	ret.y = scaleSize.height/2.0f - (orgSize.height/2.0f - rc.y)*fscale.y;
	ret.width = rc.width*fscale.x;
	ret.height = rc.height*fscale.y;
	return ret;
}

int CGeneralProc::OnOSD(int chId, int fovId, int ezoomx, Mat& dc, IDirectOSD *osd)
{
	int ret = CProcessBase::OnOSD(chId, fovId, ezoomx, dc, osd);
	float scalex = dc.cols/1920.0;
	float scaley = dc.rows/1080.0;

	int curChId = m_curChId;
	bool curTrack = m_bTrack;
	int curStat = m_iTrackStat;
	Point2f curAxis;
	curAxis.x = m_AxisCalibX[chId][fovId];
	curAxis.y = m_AxisCalibY[chId][fovId];
	Point2f tmpPoint = tPosScale(curAxis, m_imgSize[chId], (float)ezoomx);
	tmpPoint = tPosScale(tmpPoint, m_imgSize[chId], cv::Size(dc.cols, dc.rows));
	cv::Point axis((int)(tmpPoint.x+0.5), (int)(tmpPoint.y+0.5));
	UTC_RECT_float curRC = tRectScale(m_rcTrk, m_imgSize[chId], cv::Size(dc.cols, dc.rows));//m_rcTrk;//tRectScale(m_rcTrack, m_imgSize[chId], (float)m_curEZoomx[chId]);

	if((curChId == chId && !m_bHide)){
		osd->cross(axis, cv::Size2f(scalex, scaley), 0);
	}
	if((curChId == chId && curTrack && !m_bHide)){
		osd_cvdraw_trk(dc, osd, curRC, curStat, true);
	}

	return ret;
}

void CGeneralProc::osd_cvdraw_trk(Mat &dc, IDirectOSD *osd, UTC_RECT_float rcTrack, int iStat, bool bShow)
{
	UTC_RECT_float rcResult = rcTrack;

    if(rcTrack.width == 0 || rcTrack.height == 0)
        return ;

	if(iStat == 0 || iStat == 1){
		osd->rectangle(cv::Rect(rcResult.x, rcResult.y, rcResult.width, rcResult.height), !bShow);
	}else{
		std::vector<cv::Point> vPts;
		vPts.resize(16);
		vPts[0].x = rcResult.x;
		vPts[0].y = rcResult.y;
		vPts[1].x = rcResult.x+rcResult.width/4;
		vPts[1].y = rcResult.y;
		vPts[2].x = rcResult.x;
		vPts[2].y = rcResult.y;
		vPts[3].x = rcResult.x;
		vPts[3].y = rcResult.y+rcResult.height/4;
		vPts[4].x = rcResult.x+rcResult.width*3/4;
		vPts[4].y = rcResult.y;
		vPts[5].x = rcResult.x+rcResult.width;
		vPts[5].y = rcResult.y;
		vPts[6].x = rcResult.x+rcResult.width;
		vPts[6].y = rcResult.y;
		vPts[7].x = rcResult.x+rcResult.width;
		vPts[7].y = rcResult.y+rcResult.height/4;
		vPts[8].x = rcResult.x;
		vPts[8].y = rcResult.y+rcResult.height*3/4;
		vPts[9].x = rcResult.x;
		vPts[9].y = rcResult.y+rcResult.height;
		vPts[10].x = rcResult.x;
		vPts[10].y = rcResult.y+rcResult.height;
		vPts[11].x = rcResult.x+rcResult.width/4;
		vPts[11].y = rcResult.y+rcResult.height;
		vPts[12].x = rcResult.x+rcResult.width*3/4;
		vPts[12].y = rcResult.y+rcResult.height;
		vPts[13].x = rcResult.x+rcResult.width;
		vPts[13].y = rcResult.y+rcResult.height;
		vPts[14].x = rcResult.x+rcResult.width;
		vPts[14].y = rcResult.y+rcResult.height*3/4;
		vPts[15].x = rcResult.x+rcResult.width;
		vPts[15].y = rcResult.y+rcResult.height;
		osd->line(vPts, !bShow);
	}
}

bool CGeneralProc::OnProcess(int chId, Mat &frame)
{
	//if(m_bTrack && m_iTrackStat == 2 && m_iTrackLostCnt > 30*5){
	//	m_bTrack = false;
	//	m_iTrackStat = 0;
	//}
	CTrackerProc::OnProcess(chId, frame);
	return true;
}

int CGeneralProc::WriteCalibAxisToFile()
{
	string CalibFile;
	CalibFile = CALIB_AXIS_FILE;
	char calib_x[64] = "calib_x";
	char calib_y[64] = "calib_y";

	FILE *fp = fopen(CalibFile.c_str(), "wt");
	if(fp !=NULL)
	{
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);
		if(len == 0)
		{
			FileStorage fw(CalibFile,FileStorage::WRITE);
			if(fw.isOpened())
			{
				for(int i=0; i<MAX_CHAN; i++){
					for(int j=0; j<MAX_NFOV_PER_CHAN; j++){
						sprintf(calib_x, "calib_x_%d-%d", i,j);
						sprintf(calib_y, "calib_y_%d-%d", i,j);
						fw <<calib_x<< (float)m_AxisCalibX[i][j];
						fw <<calib_y<< (float)m_AxisCalibY[i][j];
					}
				}
			}
		}
	}

	FILE *fpp = fopen(CalibFile.c_str(), "rt");
	if(fpp !=NULL)
	{
		fseek(fpp, 0, SEEK_END);
		int len = ftell(fpp);
		fclose(fpp);
		if(len > 10)
			return 0;
	}else
		return -1;
}

int CGeneralProc::ReadCalibAxisFromFile()
{
	string CalibFile;
	CalibFile = CALIB_AXIS_FILE;

	char calib_x[64] = "calib_x";
	char calib_y[64] = "calib_y";

	FILE *fp = fopen(CalibFile.c_str(), "rt");
	if(fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);

		if(len < 10)
			return -1;
		else
		{
			FileStorage fr(CalibFile, FileStorage::READ);
			if(fr.isOpened())
			{
				for(int i=0; i<MAX_CHAN; i++){
					for(int j=0; j<MAX_NFOV_PER_CHAN; j++){
						sprintf(calib_x, "calib_x_%d-%d", i,j);
						sprintf(calib_y, "calib_y_%d-%d", i,j);
						m_AxisCalibX[i][j] = (float)fr[calib_x];
						m_AxisCalibY[i][j] = (float)fr[calib_y];
					}
				}
				return 0;
			}else
				return -1;
		}
	}else
		return -1;
}

int CGeneralProc::ReadCfgAvtFromFile()
{
	string cfgAvtFile;
	cfgAvtFile = CONFIG_AVT_FILE;

	char cfg_avt[16] = "cfg_avt_";
    int configId_Max=128;
    float cfg_blk_val[128];

	FILE *fp = fopen(cfgAvtFile.c_str(), "rt");
	if(fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		int len = ftell(fp);
		fclose(fp);

		if(len < 10)
			return -1;
		else
		{
			FileStorage fr(cfgAvtFile, FileStorage::READ);
			if(fr.isOpened())
			{
				for(int i=0; i<configId_Max; i++){
					sprintf(cfg_avt, "cfg_avt_%d", i);
					cfg_blk_val[i] = (float)fr[cfg_avt];
					//printf(" update cfg [%d] %f \n", i, cfg_blk_val[i]);
				}
			}else
				return -1;
		}

		UTC_DYN_PARAM dynamicParam;
		if(cfg_blk_val[0] > 0)
			dynamicParam.occlusion_thred = cfg_blk_val[0];
		else
			dynamicParam.occlusion_thred = 0.28;
		if(cfg_blk_val[1] > 0)
			dynamicParam.retry_acq_thred = cfg_blk_val[1];
		else
			dynamicParam.retry_acq_thred = 0.38;
		//UtcSetDynParam(sThis->m_track, dynamicParam);
		float up_factor;
		if(cfg_blk_val[2] > 0)
			up_factor = cfg_blk_val[2];
		else
			up_factor = 0.0125;
		UtcSetUpFactor(m_track, up_factor);
		TRK_SECH_RESTRAINT resTraint;
		if(cfg_blk_val[3] > 0)
			resTraint.res_distance = (int)(cfg_blk_val[3]);
		else
			resTraint.res_distance = 80;
		if(cfg_blk_val[4] > 0)
			resTraint.res_area = (int)(cfg_blk_val[4]);
		else
			resTraint.res_area = 5000;
		printf("UtcSetRestraint: distance=%d area=%d \n", resTraint.res_distance, resTraint.res_area);
		UtcSetRestraint(m_track, resTraint);

		int gapframe;
		if(cfg_blk_val[5] > 0)
			gapframe = (int)cfg_blk_val[5];
		else
			gapframe = 10;
		UtcSetIntervalFrame(m_track, gapframe);

        bool enhEnable;
		if(cfg_blk_val[6] > -1)
			enhEnable = (bool)cfg_blk_val[6];
		else
			enhEnable = 1;
		UtcSetEnhance(m_track, enhEnable);

		float cliplimit;
		if(cfg_blk_val[7] > 0)
			cliplimit = cfg_blk_val[7];
		else
			cliplimit = 4.0;
		UtcSetEnhfClip(m_track, cliplimit);

		bool dictEnable;

		dictEnable = (bool)cfg_blk_val[8];

		UtcSetPredict(m_track, dictEnable);

		
		int moveX,moveY;

		moveX = (int)cfg_blk_val[9];
		moveY = (int)cfg_blk_val[10];

		UtcSetMvPixel(m_track,moveX,moveY);

		int moveX2,moveY2;
		
		moveX2 = (int)cfg_blk_val[11];
		moveY2 = (int)cfg_blk_val[12];		

		UtcSetMvPixel2(m_track,moveX2,moveY2);


		int segPixelX,segPixelY;

		segPixelX = (int)cfg_blk_val[13];
		segPixelY = (int)cfg_blk_val[14];		
        UtcSetSegPixelThred(m_track,segPixelX,segPixelY);
		
		if(cfg_blk_val[15] == 1)
			algOsdRect = true;
		else
			algOsdRect = false;


		int ScalerLarge,ScalerMid,ScalerSmall;
		ScalerLarge = (int)cfg_blk_val[16];
		ScalerMid = (int)cfg_blk_val[17];		
		ScalerSmall = (int)cfg_blk_val[18];
		UtcSetSalientScaler(m_track, ScalerLarge, ScalerMid, ScalerSmall);

		int Scatter;
		Scatter = (int)cfg_blk_val[19];
		UtcSetSalientScatter(m_track, Scatter);

		float ratio;
		ratio = cfg_blk_val[20];
		UtcSetSRAcqRatio(m_track, ratio);

		bool FilterEnable;
		FilterEnable = (bool)cfg_blk_val[21];
		UtcSetBlurFilter(m_track,FilterEnable);

		bool BigSecEnable;
		BigSecEnable = (bool)cfg_blk_val[22];
		UtcSetBigSearch(m_track, BigSecEnable);

		int SalientThred;
		SalientThred = (int)cfg_blk_val[23];
		UtcSetSalientThred(m_track,SalientThred);

		int ScalerEnable;
		ScalerEnable = (int)cfg_blk_val[24];
		UtcSetMultScaler(m_track, ScalerEnable);

		bool DynamicRatioEnable;
		DynamicRatioEnable = (bool)cfg_blk_val[25];
		UtcSetDynamicRatio(m_track, DynamicRatioEnable);


		UTC_SIZE acqSize;
		acqSize.width = (int)cfg_blk_val[26];
		acqSize.height = (int)cfg_blk_val[27];		
		UtcSetSRMinAcqSize(m_track,acqSize);

		if(cfg_blk_val[28] == 1)
			TrkAim43 = true;
		else
			TrkAim43 = false;

		bool SceneMVEnable;
		SceneMVEnable = (bool)cfg_blk_val[29];
		UtcSetSceneMV(m_track, SceneMVEnable);

		bool BackTrackEnable;
		BackTrackEnable = (bool)cfg_blk_val[30];
		UtcSetBackTrack(m_track, BackTrackEnable);

		bool  bAveTrkPos;
		bAveTrkPos = (bool)cfg_blk_val[31];
		UtcSetAveTrkPos(m_track, bAveTrkPos);


		float fTau;
		fTau = cfg_blk_val[32];
		UtcSetDetectftau(m_track, fTau);


		int  buildFrms;
		buildFrms = (int)cfg_blk_val[33];
		UtcSetDetectBuildFrms(m_track, buildFrms);
		
		int  LostFrmThred;
		LostFrmThred = (int)cfg_blk_val[34];
		UtcSetLostFrmThred(m_track, LostFrmThred);

		float  histMvThred;
		histMvThred = cfg_blk_val[35];
		UtcSetHistMVThred(m_track, histMvThred);

		int  detectFrms;
		detectFrms = (int)cfg_blk_val[36];
		UtcSetDetectFrmsThred(m_track, detectFrms);

		int  stillFrms;
		stillFrms = (int)cfg_blk_val[37];
		UtcSetStillFrmsThred(m_track, stillFrms);

		float  stillThred;
		stillThred = cfg_blk_val[38];
		UtcSetStillPixThred(m_track, stillThred);


		bool  bKalmanFilter;
		bKalmanFilter = (bool)cfg_blk_val[39];
		UtcSetKalmanFilter(m_track, bKalmanFilter);

		float xMVThred, yMVThred;
		xMVThred = cfg_blk_val[40];
		yMVThred = cfg_blk_val[41];
		UtcSetKFMVThred(m_track, xMVThred, yMVThred);

		float xStillThred, yStillThred;
		xStillThred = cfg_blk_val[42];
		yStillThred = cfg_blk_val[43];
		UtcSetKFStillThred(m_track, xStillThred, yStillThred);

		float slopeThred;
		slopeThred = cfg_blk_val[44];
		UtcSetKFSlopeThred(m_track, slopeThred);

		float kalmanHistThred;
		kalmanHistThred = cfg_blk_val[45];
		UtcSetKFHistThred(m_track, kalmanHistThred);

		float kalmanCoefQ, kalmanCoefR;
		kalmanCoefQ = cfg_blk_val[46];
		kalmanCoefR = cfg_blk_val[47];
		UtcSetKFCoefQR(m_track, kalmanCoefQ, kalmanCoefR);

		bool  bSceneMVRecord;
		bSceneMVRecord = (bool)cfg_blk_val[48];

		
		UtcSetSceneMVRecord(m_track, bSceneMVRecord);


		bool bEnhScene;
		bEnhScene = (bool)cfg_blk_val[51];
		UtcSetSceneEnh(m_track, bEnhScene);

		int nrxScene,nryScene;
		nrxScene = (int)cfg_blk_val[52];
		nryScene = (int)cfg_blk_val[53];
		UtcSetSceneEnhMacro(m_track, nrxScene, nryScene);
		
		OSA_printf("stillFrms = %d,stillThred=%f---------------------------------\n",stillFrms,stillThred);
		
		//if(cfg_blk_val[36] > 0)
			UtcSetRoiMaxWidth(m_track, 200);

		UtcSetPLT_BS(m_track, tPLT_WRK, BoreSight_Mid);

		m_lossCoastTelapseMax = (int)cfg_blk_val[54];
		OSA_printf("[General]%s %d: lossCoastTelapseMax = %d ms", __func__, __LINE__, m_lossCoastTelapseMax);

		return 0;

	}
	else
		return -1;
}



