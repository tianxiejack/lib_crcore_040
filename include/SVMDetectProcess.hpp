/*
 * SVMDetectProcess.hpp
 *
 *  Created on: Sep 19, 2018
 *      Author: wzk
 */
#if 1
#ifndef SVMDETECTPROCESS_HPP_
#define SVMDETECTPROCESS_HPP_

#include "processBase.hpp"

typedef	struct	 _SVM_tgt_info{
	cv::Rect						targetRect;
	int									index;
}SVM_TGT_INFO;

typedef std::vector<SVM_TGT_INFO> vSVMTarget;
#define MAX_SVM_TGT_NUM		(16)
class CSVMDetectProcess : public CProcessBase
{
	int m_curChId;
	int m_nCount;
	int m_fovId;
	int m_ezoomx;
	OSDU_Info m_units[MAX_CHAN][MAX_SVM_TGT_NUM];
	cv::Size m_imgSize[MAX_CHAN];
	float m_threshold;
	unsigned long m_cnt[MAX_CHAN];
	int ReadCfgFile();
	int detect(int chId, Mat frame);
	void update(int chId);
public :
	enum{
		VP_CFG_SVMEnable=VP_CFG_SVM_BASE,
		VP_CFG_SVMTargetCount,
		VP_CFG_Max
	};
	CSVMDetectProcess(IProcess *proc = NULL);
	virtual ~CSVMDetectProcess();
	virtual int process(int chId, int fovId, int ezoomx, Mat frame);
	virtual int dynamic_config(int type, int iPrm, void* pPrm = NULL, int prmSize = 0);
	virtual int OnOSD(int chId, int fovId, int ezoomx, Mat& dc, IDirectOSD *osd);

	bool m_bEnable;
	vSVMTarget m_targets[MAX_CHAN];
	bool m_bHide;

protected:
	cv::Size m_dSize;
	int m_minArea;
	int m_maxArea;
	float m_minRatioHW;
	float m_maxRatioHW;
	HOGDescriptor hog;
};



#endif /* SVMDETECTPROCESS_HPP_ */
#endif
