#if 1
/*
 * mmtdProcess.hpp
 *
 *  Created on: Sep 19, 2018
 *      Author: wzk
 */

#ifndef MMTDPROCESS_HPP_
#define MMTDPROCESS_HPP_

#include "processBase.hpp"
#include "MMTD.h"

class CMMTDProcess : public CProcessBase
{
	CMMTD *m_mmtd;
	int m_curChId;
	int m_nCount;
	int m_nSelect;
	int m_nDrop;
	int m_fovId;
	int m_ezoomx;
	OSDU_Info m_units[MAX_CHAN][MAX_TGT_NUM];
	cv::Size m_imgSize[MAX_CHAN];
	int ReadCfgMmtFromFile();
public :
	enum{
		VP_CFG_MMTDEnable=VP_CFG_MMTD_BASE,
		VP_CFG_MMTDTargetCount,
		VP_CFG_MMTDMax
	};
	CMMTDProcess(IProcess *proc = NULL);
	virtual ~CMMTDProcess();
	virtual int process(int chId, int fovId, int ezoomx, Mat frame, uint64_t timestamp);
	virtual int dynamic_config(int type, int iPrm, void* pPrm = NULL, int prmSize = 0);
	virtual int OnOSD(int chId, int fovId, int ezoomx, Mat& dc, IDirectOSD *osd);

	bool m_bEnable;
	TARGETBOX m_target[MAX_TGT_NUM];
	bool m_bHide;
};



#endif /* MMTDPROCESS_HPP_ */

#endif


