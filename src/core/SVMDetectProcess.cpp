#if 1
#include "SVMDetectProcess.hpp"

#define CONFIG_SVM_FILE		"ConfigSVMDetectFile.yml"

CSVMDetectProcess::CSVMDetectProcess(IProcess *proc)
	:CProcessBase(proc),m_nCount(1),m_curChId(0), m_bEnable(false),
	 m_fovId(0), m_ezoomx(1), m_bHide(false)
{
	memset(m_units, 0, sizeof(m_units));
	memset(m_cnt, 0, sizeof(m_cnt));
	for(int chId=0; chId<MAX_CHAN; chId++){
		m_imgSize[chId].width = 1920;
		m_imgSize[chId].height = 1080;
	}
	m_threshold = 0.1;
	m_dSize.width = 1920/2;
	m_dSize.height = 1080/2;
	m_minArea = 80*80;
	m_maxArea = m_dSize.area();
	m_minRatioHW = 0.5;
	m_maxRatioHW = 2.0;
	ReadCfgFile();
	hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
}

CSVMDetectProcess::~CSVMDetectProcess()
{
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

int CSVMDetectProcess::process(int chId, int fovId, int ezoomx, Mat frame)
{
	int iRet = CProcessBase::process(chId, fovId, ezoomx, frame);

	if(m_curChId != chId || !m_bEnable)
		return iRet;

	m_imgSize[chId].width = frame.cols;
	m_imgSize[chId].height = frame.rows;

	if(m_fovId!= fovId || m_ezoomx!=ezoomx){
		m_fovId= fovId;
		m_ezoomx=ezoomx;
		OSA_mutexLock(&m_mutexlock);
		m_targets[m_curChId].clear();
		for(int i=0; i<MAX_SVM_TGT_NUM; i++)
			m_units[m_curChId][i].bNeedDraw = false;
		OSA_mutexUnlock(&m_mutexlock);
		m_cnt[chId] = 0;
	}

	if(m_bEnable && m_curChId == chId)
	{
		m_cnt[chId]++;

		if(m_ezoomx<=1){
			detect(chId, frame);
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
			detect(chId, rsMat);
		}
		update(chId);
	}

	return iRet;
}

int CSVMDetectProcess::detect(int chId, Mat frame)
{
    vector<Rect> found, found_filtered;
    double t = (double)getTickCount();
    // run the detector with default parameters. to get a higher hit-rate
    // (and more false alarms, respectively), decrease the hitThreshold and
    // groupThreshold (set groupThreshold to 0 to turn off the grouping completely).
    hog.detectMultiScale(frame, found, 0, Size(8,8), Size(32,32), 1.05, 2);
    t = (double)getTickCount() - t;
    printf("tdetection time = %gms\n", t*1000./cv::getTickFrequency());

    size_t i, j;
    for( i = 0; i < found.size(); i++ )
    {
        Rect r = found[i];
        for( j = 0; j < found.size(); j++ )
            if( j != i && (r & found[j]) == r)
                break;
        if( j == found.size() )
            found_filtered.push_back(r);
    }
    m_targets[chId].clear();
    for( i = 0; i < found_filtered.size(); i++ )
    {
    	SVM_TGT_INFO tgt;
        Rect r = found_filtered[i];
        // the HOG detector returns slightly larger rectangles than the real objects.
        // so we slightly shrink the rectangles to get a nicer output.
        r.x += cvRound(r.width*0.1);
        r.width = cvRound(r.width*0.8);
        r.y += cvRound(r.height*0.07);
        r.height = cvRound(r.height*0.8);
        tgt.targetRect = r;
        tgt.index = i;
        m_targets[chId].push_back(tgt);
    }
}

void CSVMDetectProcess::update(int chId)
{
	//OSA_printf("%s %d: ch%d size = %ld", __func__, __LINE__, chId, m_targets[chId].size());
	//OSA_mutexLock(&m_mutexlock);
	int cnt = 0;
	if(!m_bHide){
		int i=0;
		if(m_bEnable&& m_curChId == chId){
			cnt = m_targets[chId].size();
			cnt = m_nCount < cnt ? m_nCount : cnt;
			for(i=0; i<cnt; i++)
			{
				m_units[chId][i].bNeedDraw = true;
				m_units[chId][i].orgPos =
						Point(m_targets[chId][i].targetRect.x + m_targets[chId][i].targetRect.width/2,
								m_targets[chId][i].targetRect.y + m_targets[chId][i].targetRect.height/2);
				m_units[chId][i].orgRC = m_targets[chId][i].targetRect;
				m_units[chId][i].orgValue = m_targets[chId][i].index;
			}
		}
		for(;i<MAX_SVM_TGT_NUM; i++)
			m_units[chId][i].bNeedDraw = false;
	}
	//OSA_mutexUnlock(&m_mutexlock);
}

int CSVMDetectProcess::OnOSD(int chId, int fovId, int ezoomx, Mat& dc, IDirectOSD *osd)
{
	int ret = CProcessBase::OnOSD(chId, fovId, ezoomx, dc, osd);

	float scalex = dc.cols/1920.0;
	float scaley = dc.rows/1080.0;
	int winWidth = 40*scalex;
	int winHeight = 40*scaley;
	bool bFixSize = false;

	//OSA_mutexLock(&m_mutexlock);
	if(m_curChId == chId){
		for(int i=0; i<MAX_SVM_TGT_NUM; i++){
			if(m_units[chId][i].bNeedDraw){
				if(!bFixSize){
					winWidth = m_units[chId][i].orgRC.width;
					winHeight = m_units[chId][i].orgRC.height;
				}
				Rect rc(m_units[chId][i].orgPos.x-winWidth/2, m_units[chId][i].orgPos.y-winHeight/2, winWidth, winHeight);
				rc = tRectScale(rc, m_imgSize[chId], cv::Size(dc.cols, dc.rows));
				m_units[chId][i].drawRC = rc;
				osd->numberedBox(m_units[chId][i].drawRC, i+1, 0);
			}
		}
	}
	//OSA_mutexUnlock(&m_mutexlock);
}

int CSVMDetectProcess::dynamic_config(int type, int iPrm, void* pPrm, int prmSize)
{
	int iret = OSA_SOK;

	iret = CProcessBase::dynamic_config(type, iPrm, pPrm, prmSize);

	if(type<VP_CFG_BASE || type>VP_CFG_Max)
		return iret;

	//cout << "CSVMDetectProcess::dynamic_config type " << type << " iPrm " << iPrm << endl;
	OSA_mutexLock(&m_mutexlock);
	switch(type)
	{
	case VP_CFG_SVMTargetCount:
		m_nCount = iPrm;
		iret = OSA_SOK;
		break;
	case VP_CFG_SVMEnable:
		m_bEnable = iPrm;
		m_targets[m_curChId].clear();
		for(int i=0; i<MAX_SVM_TGT_NUM; i++)
			m_units[m_curChId][i].bNeedDraw = false;
		m_cnt[m_curChId] = 0;
		iret = OSA_SOK;
		break;
	case VP_CFG_MainChId:
		if(m_bEnable){
			m_targets[m_curChId].clear();
			for(int i=0; i<MAX_SVM_TGT_NUM; i++)
				m_units[m_curChId][i].bNeedDraw = false;
			m_curChId = iPrm;
			m_targets[m_curChId].clear();
			for(int i=0; i<MAX_SVM_TGT_NUM; i++)
				m_units[m_curChId][i].bNeedDraw = false;
		}else{
			m_curChId = iPrm;
		}
		m_cnt[m_curChId] = 0;
		break;
	default:
		break;
	}
	OSA_mutexUnlock(&m_mutexlock);
	return iret;
}

int CSVMDetectProcess::ReadCfgFile()
{
	int iret = -1;
	string cfgFile;
	cfgFile = CONFIG_SVM_FILE;
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
				float fvalue;
				int ivalue;
				fvalue = (float)fr["cfg_SVM_threshold"];
				if(fvalue>0.001 && fvalue<1.001)
					m_threshold = fvalue;

				ivalue = (int)fr["cfg_SVM_roi_width"];
				if(ivalue > 0)
					m_dSize.width = ivalue;
				ivalue = (int)fr["cfg_SVM_roi_height"];
				if(ivalue > 0)
					m_dSize.height = ivalue;

				ivalue = (int)fr["cfg_SVM_area_min"];
				if(ivalue > 0)
					m_minArea = ivalue;
				ivalue = (int)fr["cfg_SVM_area_max"];
				if(ivalue > 0)
					m_maxArea = ivalue;

				fvalue = (float)fr["cfg_SVM_ratio_min"];
				if(fvalue>0.001 && fvalue<1.001)
					m_minRatioHW = fvalue;
				fvalue = (float)fr["cfg_SVM_ratio_max"];
				if(fvalue>0.001 && fvalue<10000.001)
					m_maxRatioHW = fvalue;

				iret = 0;
			}
		}
	}
	//OSA_printf("SVMDetect %s: thr %f roi(%dx%d) area(%d,%d) ratio(%f,%f)\n", __func__,
	//		m_threshold, m_dSize.width, m_dSize.height, m_minArea, m_maxArea, m_minRatioHW, m_maxRatioHW);
	return iret;
}
#endif
