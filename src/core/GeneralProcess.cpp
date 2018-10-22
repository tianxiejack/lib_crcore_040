/*
 * processbase.cpp
 *
 *  Created on: May 5, 2017
 *      Author: ubuntu
 */
#include <glut.h>
#include "GeneralProcess.hpp"
//#include "vmath.h"

#define CALIB_AXIS_FILE		"CalibAxisFile.yml"
#define CONFIG_AVT_FILE		"ConfigAvtFile.yml"

static int ScalerLarge,ScalerMid,ScalerSmall;

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

CGeneralProc::CGeneralProc(OSA_SemHndl *notifySem, IProcess *proc)
	:CTrackerProc(notifySem, proc),m_bHide(false)
{
	memset(units, 0, sizeof(units));
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

int CGeneralProc::OnOSD(int chId, Mat dc, CvScalar color)
{
	int ret = CProcessBase::OnOSD(chId, dc, color);
	int curChId = m_curChId;
	bool curTrack = m_bTrack;
	int curStat = m_iTrackStat;
	float scalex = dc.cols/1920.0;
	float scaley = dc.rows/1080.0;
	Point2f tmpPoint = tPosScale(m_axis, m_imgSize[chId], (float)m_curEZoomx[chId]);
	cv::Point axis((int)tmpPoint.x, (int)tmpPoint.y);
	//cv::Point axis((int)(m_axis.x+0.5), (int)(m_axis.y+0.5));
	UTC_RECT_float curRC = m_rcTrk;//tRectScale(m_rcTrack, m_imgSize[chId], (float)m_curEZoomx[chId]);

	if(units[chId][U_WIN].bHasDraw)
		osd_cvdraw_trk(dc, units[chId][U_WIN].rc, 0, false);
	units[chId][U_WIN].bHasDraw = false;
	if(units[chId][U_AXIS].bHasDraw)
		osd_cvdraw_cross(dc, units[chId][U_AXIS].pos.x, units[chId][U_AXIS].pos.y, scalex, scaley, false);
	units[chId][U_AXIS].bHasDraw = false;

	units[chId][U_AXIS].bNeedDraw = (curChId == chId && !m_bHide);
	if(units[chId][U_AXIS].bNeedDraw){
		units[chId][U_AXIS].pos = axis;
		osd_cvdraw_cross(dc, units[chId][U_AXIS].pos.x, units[chId][U_AXIS].pos.y, scalex, scaley, true);
		units[chId][U_AXIS].bHasDraw = true;
	}
	units[chId][U_WIN].bNeedDraw = (curChId == chId && curStat != 0 && !m_bHide);
	if(units[chId][U_WIN].bNeedDraw){
		units[chId][U_WIN].rc = curRC;
		units[chId][U_WIN].iStyle = curStat;
		osd_cvdraw_trk(dc, units[chId][U_WIN].rc, units[chId][U_WIN].iStyle, true);
		units[chId][U_WIN].bHasDraw = true;
	}
	return ret;
}

void CGeneralProc::osd_cvdraw_cross(Mat &dc, int ix, int iy, float scalex, float scaley, bool bShow)
{
	Point pt1,pt2,center;
	CvScalar lineColor;
	UInt32 width=0, height=0, widthGap=0, heightGap=0;
	int linePixels = 2;

	if(ix == 0 && iy == 0)
		return ;

	width = 120*scalex;
	height = 100*scaley;
	widthGap = 40*scalex;
	heightGap = 30*scaley;

	center.x = ix;
	center.y = iy;

	if(bShow)
	{
		lineColor = m_color;
	}
	else
	{
		lineColor = cvScalar(0);
	}

	if(0)
	{
		// only cross
		pt1.x = center.x-width/2;
		pt1.y = center.y;
		pt2.x = center.x+width/2;
		pt2.y = center.y;
		line(dc, pt1, pt2, lineColor, linePixels, 8);
		pt1.y += linePixels;
		pt2.y += linePixels;
		//line(dc, pt1, pt2, lineColor2, linePixels, 8);

		pt1.x = center.x;
		pt1.y = center.y-height/2;
		pt2.x = center.x;
		pt2.y = center.y+height/2;
		line(dc, pt1, pt2, lineColor, linePixels, 8);
		pt1.x += linePixels;
		pt2.x += linePixels;
		//line(dc, pt1, pt2, lineColor2, linePixels, 8);
	}
	else
	{
		// with center point
		//left horizonal line
		pt1.x = center.x-width/2;
		pt1.y = center.y;
		pt2.x = center.x-widthGap/2;
		pt2.y = center.y;
		line(dc, pt1, pt2, lineColor, linePixels, 8);
		pt1.y += linePixels+1;
		pt2.y += linePixels+1;
		//line(dc, pt1, pt2, lineColor2, linePixels, 8);
		//middle horizonal line
		pt1.x = center.x-1;
		pt1.y = center.y;
		pt2.x = center.x+linePixels+linePixels-1;
		pt2.y = center.y;
		line(dc, pt1, pt2, lineColor, linePixels, 8);
		pt1.y += linePixels+1;
		pt2.y += linePixels+1;
		//line(dc, pt1, pt2, lineColor2, linePixels, 8);
		//right horizonal line
		pt1.x = center.x+width/2;
		pt1.y = center.y;
		pt2.x = center.x+widthGap/2;
		pt2.y = center.y;
		line(dc, pt1, pt2, lineColor, linePixels, 8);
		pt1.y += linePixels+1;
		pt2.y += linePixels+1;
		//line(dc, pt1, pt2, lineColor2, linePixels, 8);
		//top vertical line
		pt1.x = center.x;
		pt1.y = center.y-height/2;
		pt2.x = center.x;
		pt2.y = center.y-heightGap/2;
		line(dc, pt1, pt2, lineColor, linePixels, 8);
		pt1.x += linePixels+1;
		pt2.x += linePixels+1;
		//line(dc, pt1, pt2, lineColor2, linePixels, 8);
		//bottom vertical line
		pt1.x = center.x;
		pt1.y = center.y+height/2;
		pt2.x = center.x;
		pt2.y = center.y+heightGap/2;
		line(dc, pt1, pt2, lineColor, linePixels, 8);
		pt1.x += linePixels+1;
		pt2.x += linePixels+1;
		//line(dc, pt1, pt2, lineColor2, linePixels, 8);
	}
}

void CGeneralProc::osd_cvdraw_trk(Mat &dc, UTC_RECT_float rcTrack, int iStat, bool bShow)
{
	UTC_RECT_float rcResult = rcTrack;
    CvScalar lineColor;
	Point pt1,pt2;
	int linePixels = 2;

    if(rcTrack.width == 0 || rcTrack.height == 0)
        return ;

	if(bShow)
		lineColor = m_color;
	else
		lineColor = cv::Scalar(0);//GetcvColour(ecolor_Default);

	if(bShow == false)
	{
		rectangle( dc,
			Point( rcResult.x, rcResult.y ),
			Point( rcResult.x+rcResult.width, rcResult.y+rcResult.height),
			lineColor, linePixels, 8 );
		return ;
	}

	if(iStat == 1)	// trk
		rectangle( dc,
			Point( rcResult.x, rcResult.y ),
			Point( rcResult.x+rcResult.width, rcResult.y+rcResult.height),
			lineColor, linePixels, 8 );
	else					// assi
	{
		//zuo shang jiao 
		pt1.x = rcResult.x;
		pt1.y = rcResult.y;
		pt2.x = rcResult.x+rcResult.width/4;
		pt2.y = rcResult.y;
		line(dc, pt1, pt2, lineColor, linePixels, 8);

		pt1.x = rcResult.x;
		pt1.y = rcResult.y;
		pt2.x = rcResult.x;
		pt2.y = rcResult.y+rcResult.height/4;
		line(dc, pt1, pt2, lineColor, linePixels, 8);

		//you shang jiao
		pt1.x = rcResult.x+rcResult.width*3/4;
		pt1.y = rcResult.y;
		pt2.x = rcResult.x+rcResult.width;
		pt2.y = rcResult.y;
		line(dc, pt1, pt2, lineColor, linePixels, 8);

		pt1.x = rcResult.x+rcResult.width;
		pt1.y = rcResult.y;
		pt2.x = rcResult.x+rcResult.width;
		pt2.y = rcResult.y+rcResult.height/4;
		line(dc, pt1, pt2, lineColor, linePixels, 8);

		//zuo xia jiao
		pt1.x = rcResult.x;
		pt1.y = rcResult.y+rcResult.height*3/4;
		pt2.x = rcResult.x;
		pt2.y = rcResult.y+rcResult.height;
		line(dc, pt1, pt2, lineColor, linePixels, 8);

		pt1.x = rcResult.x;
		pt1.y = rcResult.y+rcResult.height;
		pt2.x = rcResult.x+rcResult.width/4;
		pt2.y = rcResult.y+rcResult.height;
		line(dc, pt1, pt2, lineColor, linePixels, 8);

		//you xia jiao 
		pt1.x = rcResult.x+rcResult.width*3/4;
		pt1.y = rcResult.y+rcResult.height;
		pt2.x = rcResult.x+rcResult.width;
		pt2.y = rcResult.y+rcResult.height;
		line(dc, pt1, pt2, lineColor, linePixels, 8);

		pt1.x = rcResult.x+rcResult.width;
		pt1.y = rcResult.y+rcResult.height*3/4;
		pt2.x = rcResult.x+rcResult.width;
		pt2.y = rcResult.y+rcResult.height;
		line(dc, pt1, pt2, lineColor, linePixels, 8);
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
		return 0;

	}
	else
		return -1;
}



