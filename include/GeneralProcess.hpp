/*
 * processbase.hpp
 *
 *  Created on: May 5, 2017
 *      Author: ubuntu
 */

#ifndef PROCESS_BASE_HPP_
#define PROCESS_BASE_HPP_

#include <glut.h>
#include "trackerProcess.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

typedef struct _TRKOSD_unit_info{
	bool bHasDraw;
	bool bNeedDraw;
	int iStyle;
	cv::Point pos;
	UTC_RECT_float rc;
}TRKOSDU_Info;

class CGeneralProc : public CTrackerProc
{
public:
	CGeneralProc(OSA_SemHndl *notifySem, IProcess *proc = NULL);
	virtual ~CGeneralProc();

	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnInit();
	virtual void OnConfig(int type, int iPrm, void* pPrm);
	virtual void OnRun();
	virtual void OnStop();
	virtual void Ontimer();
	virtual bool OnPreProcess(int chId, Mat &frame);
	virtual bool OnProcess(int chId, Mat &frame);
	virtual int OnOSD(int chId, Mat dc, CvScalar color);

	int WriteCalibAxisToFile();
	int ReadCalibAxisFromFile();
	int ReadCfgAvtFromFile();	

	bool m_bHide;

protected:
	void osd_cvdraw_cross(Mat &dc, int ix, int iy, float scalex, float scaley, bool bShow);
	void osd_cvdraw_trk(Mat &dc, UTC_RECT_float rcTrack, int iStat, bool bShow = true);

	enum{
		U_WIN = 0,
		U_AXIS,
		U_MAX
	};
	TRKOSDU_Info units[MAX_CHAN][2];
};

#endif /* PROCESSBASE_HPP_ */
