#if 1
#include "motionDetectProcess.hpp"

#define CONFIG_MTD_FILE		"ConfigMotionDetectFile.yml"

CMotionDetectProcess::CMotionDetectProcess(IProcess *proc)
	:CProcessBase(proc),m_nCount(MAX_TARGET_COUNT),m_curChId(0), m_bEnable(false),
	 m_fovId(0), m_ezoomx(1), m_bHide(false), m_inter(NULL), m_bOpen(false),m_curMode(WARN_WARN_MODE)
{
	memset(m_units, 0, sizeof(m_units));
	memset(m_cnt, 0, sizeof(m_cnt));
	for(int chId=0; chId<MAX_CHAN; chId++){
		m_imgSize[chId].width = 1920;
		m_imgSize[chId].height = 1080;
	}
	m_inter = MvDetector_Create();
	OSA_assert(m_inter != NULL);
	m_accuracy = 2;
	m_inputMinArea = 40*40;
	m_inputMaxArea = 400*400;
	m_threshold = 20;
	ReadCfgFile();
	m_inter->init(notifyHdl, this);
	m_roi = Rect(300, 100, 1920-300, 1080-200);
}

CMotionDetectProcess::~CMotionDetectProcess()
{
	if(m_inter != NULL)
		m_inter->destroy();
	m_inter = NULL;
}

inline Rect tRectCropScale(cv::Size imgSize, float fs)
{
	float fscaled = 1.0/fs;
	cv::Size2f rdsize(imgSize.width/2.0, imgSize.height/2.0);
	cv::Point point((int)(rdsize.width-rdsize.width*fscaled+0.5), (int)(rdsize.height-rdsize.height*fscaled+0.5));
	cv::Size sz((int)(imgSize.width*fscaled+0.5), (int)(imgSize.height*fscaled+0.5));
	return Rect(point, sz);
}

inline int meancr(unsigned char *img, int width)
{
	return ((img[0]+img[1]+img[width]+img[width+1])>>2);
}
static void mResize(Mat src, Mat dst, int zoomx)
{
	int width = src.cols;
	int height = src.rows;
	int zoomxStep = zoomx>>1;
	uint8_t  *  pDst8_t;
	uint8_t *  pSrc8_t;

	pSrc8_t = (uint8_t*)(src.data)+(height/2-(height/(zoomx<<1)))*width+(width/2-(width/(zoomx<<1)));
	pDst8_t = (uint8_t*)(dst.data);

	for(int y = 0; y < height; y++)
	{
		int halfIy = y>>zoomxStep;
		for(int x = 0; x < width; x++){
			pDst8_t[y*width+x] = meancr(pSrc8_t+halfIy*width+(x>>zoomxStep), width);
		}
	}
}

inline void mResizeX2(Mat src, Mat dst)
{
	int width = src.cols;
	int height = src.rows;
	uint8_t *  pDst8_t;
	uint8_t *  pSrc8_t;

	pSrc8_t = (uint8_t*)(src.data)+(height>>2)*width+(width>>2);
	pDst8_t = (uint8_t*)(dst.data);

	for(int y = 0; y < (height>>1); y++)
	{
		for(int x = 0; x < (width>>1); x++){
			pDst8_t[y*2*width+x*2] = pSrc8_t[y*width+x];
			pDst8_t[y*2*width+x*2+1] = (pSrc8_t[y*width+x]+pSrc8_t[y*width+x+1])>>1;
			pDst8_t[(y*2+1)*width+x*2] = (pSrc8_t[y*width+x]+pSrc8_t[(y+1)*width+x])>>1;
			pDst8_t[(y*2+1)*width+x*2+1] = (pSrc8_t[y*width+x]+pSrc8_t[y*width+x+1]+pSrc8_t[(y+1)*width+x]+pSrc8_t[(y+1)*width+x+1])>>2;
		}
	}
}

int CMotionDetectProcess::process(int chId, int fovId, int ezoomx, Mat frame)
{
	int iRet = OSA_SOK;
	iRet= CProcessBase::process(chId, fovId, ezoomx, frame);

	m_imgSize[chId].width = frame.cols;
	m_imgSize[chId].height = frame.rows;

	OSA_mutexLock(&m_mutexlock);
	if(m_curChId != chId || !m_bEnable){
		OSA_mutexUnlock(&m_mutexlock);
		return iRet;
	}

	if(m_fovId!= fovId || m_ezoomx!=ezoomx){
		m_inter->mvClose(m_curChId);
		m_bOpen = false;
		m_fovId= fovId;
		m_ezoomx=ezoomx;
		m_targets[m_curChId].clear();
		for(int i=0; i<MAX_MOTION_TGT_NUM; i++)
			m_units[m_curChId][i].bNeedDraw = false;
		m_cnt[chId] = 0;
	}

	OSA_assert(m_curChId == chId);
	{
		if(m_cnt[chId] == 20){
			m_inter->setUpdateFactor(1, chId);
		}
		if(m_cnt[chId] == 220)
			m_inter->setUpdateFactor(16, chId);
		m_cnt[chId]++;

		m_roi = Rect(0, 0, frame.cols, frame.rows);

		if(m_ezoomx<=1){
			//OSA_printf("%s %d: frame(%dx%d)", __func__, __LINE__, frame.cols, frame.rows);
			/*static int Once = 0;
			static Mat cFrame;
			if(!Once){
				Once = 1;
				frame.copyTo(cFrame);
			}
			cFrame.copyTo(frame);*/
			if(0){
				static float fcnt = 0.0;
				fcnt += 0.12;
				if(fcnt>300-1 || m_cnt[chId]==1)
					fcnt = 0.0;
				m_roi = Rect(300-fcnt, 100, frame.cols-600, frame.rows-200);
				Mat frameROI(frame, m_roi);
				m_inter->setFrame(frameROI, chId, m_accuracy, m_inputMinArea, m_inputMaxArea, m_threshold);
			}else{
				m_inter->setFrame(frame, chId, m_accuracy, m_inputMinArea, m_inputMaxArea, m_threshold);
			}
		}else{
			//int64 tks = getTickCount();
			cv::Mat rsMat = cv::Mat(frame.rows, frame.cols, CV_8UC1);
			if(m_ezoomx == 4){
				cv::Mat rsMat0 = cv::Mat(frame.rows, frame.cols, CV_8UC1);
				mResizeX2(frame, rsMat0);
				mResizeX2(rsMat0, rsMat);
			}
			else if(m_ezoomx == 8){
				cv::Mat rsMat0 = cv::Mat(frame.rows, frame.cols, CV_8UC1);
				mResizeX2(frame, rsMat);
				mResizeX2(rsMat, rsMat0);
				mResizeX2(rsMat0, rsMat);
			}else{
				mResizeX2(frame, rsMat);
			}
			m_inter->setFrame(rsMat, chId, m_accuracy, m_inputMinArea, m_inputMaxArea, m_threshold);
		}
		if(!m_bOpen && m_cnt[chId]>1){
			m_inter->mvOpen(m_curChId);
			m_bOpen = true;
		}
	}

	OSA_mutexUnlock(&m_mutexlock);

	return iRet;
}

void CMotionDetectProcess::update(int chId)
{
	//OSA_printf("%s %d: ch%d size = %ld", __func__, __LINE__, chId, m_targets[chId].size());
	if(m_curMode == WARN_MOVEDETECT_MODE)
		m_inter->getMoveTarget(m_targets[chId], chId);
	else
		m_inter->getWarnTarget(m_targets[chId], chId);
	if(!m_bHide){
		OSA_mutexLock(&m_mutexlock);
		int i=0;
		if(m_bEnable && m_bOpen && m_cnt[chId]>20 && m_curChId == chId){
			for(i=0; i<m_targets[chId].size(); i++)
			{
				m_units[chId][i].bNeedDraw = true;
				m_units[chId][i].orgPos = Point(m_targets[chId][i].targetRect.x + m_targets[chId][i].targetRect.width/2, m_targets[chId][i].targetRect.y + m_targets[chId][i].targetRect.height/2);
				m_units[chId][i].orgRC = m_targets[chId][i].targetRect;
				m_units[chId][i].orgValue = m_targets[chId][i].index;
			}
		}
		for(;i<MAX_MOTION_TGT_NUM; i++)
			m_units[chId][i].bNeedDraw = false;
		OSA_mutexUnlock(&m_mutexlock);
	}
}

int CMotionDetectProcess::OnOSD(int chId, int fovId, int ezoomx, Mat& dc, IDirectOSD *osd)
{
	int ret = CProcessBase::OnOSD(chId, fovId, ezoomx, dc, osd);


	/*static bool bFlag = false;
	if(chId == 0 && !bFlag){
	m_inter->setDrawOSD(dc, chId);
	m_inter->enableSelfDraw(true, chId);
	bFlag = true;
	}*/
	//return 0;


	float scalex = dc.cols/1920.0;
	float scaley = dc.rows/1080.0;
	int winWidth = 40*scalex;
	int winHeight = 40*scaley;
	bool bFixSize = false;

	OSA_mutexLock(&m_mutexlock);
	if(m_curChId == chId){
		for(int i=0; i<MAX_MOTION_TGT_NUM; i++){
			if(m_units[chId][i].bNeedDraw){
				if(!bFixSize){
					winWidth = m_units[chId][i].orgRC.width;
					winHeight = m_units[chId][i].orgRC.height;
				}
				//Rect rc(m_units[chId][i].orgPos.x-winWidth/2, m_units[chId][i].orgPos.y-winHeight/2, winWidth, winHeight);
				Rect rc(m_units[chId][i].orgPos.x-winWidth/2+m_roi.x, m_units[chId][i].orgPos.y-winHeight/2+m_roi.y, winWidth, winHeight);
				rc = tRectScale(rc, m_imgSize[chId], cv::Size(dc.cols, dc.rows));
				m_units[chId][i].drawRC = rc;
				osd->numberedBox(m_units[chId][i].drawRC, m_units[chId][i].orgValue+1, 0);
			}
		}
	}
	OSA_mutexUnlock(&m_mutexlock);
}

int CMotionDetectProcess::dynamic_config(int type, int iPrm, void* pPrm, int prmSize)
{
	int iret = OSA_SOK;

	iret = CProcessBase::dynamic_config(type, iPrm, pPrm, prmSize);

	if(type<VP_CFG_BASE || type>VP_CFG_Max)
		return iret;

	//cout << "CMotionDetectProcess::dynamic_config type " << type << " iPrm " << iPrm << endl;
	OSA_mutexLock(&m_mutexlock);
	switch(type)
	{
	case VP_CFG_MONTIONTargetCount:
		m_nCount = iPrm;
		//m_mmtd->SetTargetNum(m_nCount);
		break;
	case VP_CFG_MONTIONEnable:
		m_inter->setWarnMode(m_curMode, m_curChId);
		m_bEnable = iPrm;
		if(m_bEnable)
			m_inter->mvOpen(m_curChId);
		else
			m_inter->mvClose(m_curChId);
		m_targets[m_curChId].clear();
		for(int i=0; i<MAX_MOTION_TGT_NUM; i++)
			m_units[m_curChId][i].bNeedDraw = false;
		m_cnt[m_curChId] = 0;
		m_bOpen = m_bEnable;
		break;
	case VP_CFG_MainChId:
		if(m_bEnable){
			m_inter->mvClose(m_curChId);
			m_targets[m_curChId].clear();
			for(int i=0; i<MAX_MOTION_TGT_NUM; i++)
				m_units[m_curChId][i].bNeedDraw = false;
			m_curChId = iPrm;
			m_targets[m_curChId].clear();
			for(int i=0; i<MAX_MOTION_TGT_NUM; i++)
				m_units[m_curChId][i].bNeedDraw = false;
			m_inter->setWarnMode(m_curMode, m_curChId);
			//m_inter->mvOpen();
			m_bOpen = false;
		}else{
			m_curChId = iPrm;
		}
		m_cnt[m_curChId] = 0;
		break;
	default:
		iret = OSA_EFAIL;
		break;
	}
	OSA_mutexUnlock(&m_mutexlock);
	return iret;
}

int CMotionDetectProcess::ReadCfgFile()
{
	int iret = -1;
	string cfgFile;
	cfgFile = CONFIG_MTD_FILE;
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
				int value;
				value = (int)fr["cfg_mtd_accuracy"];
				if(value>0 && value<10)
					m_accuracy = value;
				value = (int)fr["cfg_mtd_minArea"];
				if(value>20 && value<1000*1000)
					m_inputMinArea = value;
				value = (int)fr["cfg_mtd_maxArea"];
				if(value>20 && value<1000*1000)
					m_inputMaxArea = value;
				value = (int)fr["cfg_mtd_threshold"];
				if(value>0 && value<10000)
					m_threshold = value;
				iret = 0;
			}
		}
	}
	OSA_printf("MotionDetect %s: acc %d, minA %d maxA %d thr %d\n",
			__func__, m_accuracy, m_inputMinArea, m_inputMaxArea, m_threshold);
	return iret;
}
#endif
