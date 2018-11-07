/*
 * glosd.hpp
 *
 *  Created on: Nov 6, 2018
 *      Author: wzk
 */

#ifndef GLOSD_HPP_
#define GLOSD_HPP_

#include <GLShaderManager.h>
#include <GLBatch.h>
#include <opencv2/opencv.hpp>
#include "osa_mutex.h"

extern GLShaderManager		glShaderManager;

class GLOSD;
class GLOSDUNITBase
{
protected:
	friend class GLOSD;
	GLOSD *m_factory;
	int m_index;
	bool m_bUpdate;
	GLfloat *vVertexPos;
	GLfloat *vColorPos;
	int m_nVert;
	int m_thickness;
	OSA_MutexHndl *m_mutexlock;
	GLOSDUNITBase(GLOSD *factory, int nVert);
	virtual ~GLOSDUNITBase(void);
public:
	cv::Size m_viewport;
	cv::Point2f m_center;
	inline cv::Point2f normalized(cv::Point pt)
	{
		cv::Point2f rpt(((float)pt.x-m_center.x)/m_center.x, (m_center.y - (float)pt.y)/m_center.y);
		//OSA_printf("%s %d: c(%f, %f) pt(%d, %d) rpt(%f, %f)", __func__, __LINE__,
		//		m_center.x, m_center.y, pt.x, pt.y, rpt.x, rpt.y);
		return rpt;
	}
};

class GLOSDLine : public GLOSDUNITBase
{
public:
	GLOSDLine(GLOSD *factory);
	virtual ~GLOSDLine(void);
	void line(cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness=1);
};

class GLOSDBase
{
public:
	virtual void Draw(void) = 0;
};

class GLOSDLine;
typedef std::vector<GLOSDUNITBase*> vUNIT;
typedef std::vector<GLOSDUNITBase*>::iterator viUNIT;
typedef std::vector<vUNIT> aUNIT;
class GLOSD : public GLOSDBase
{
	cv::Size m_viewport;
	cv::Point2f m_center;
	OSA_MutexHndl m_mutexlock;
	vUNIT vecUnits;
	aUNIT mapUnits;
	GLfloat *vVertexArray;
	GLfloat *vColorArray;
	int m_allocCnt;
	int m_iCur;
public:
	GLOSD(int vWidth = 1920, int vHeight = 1080);

	virtual ~GLOSD(void);

	virtual void Draw(void);

public:
	void Add(GLOSDUNITBase* unit);
	void Erase(GLOSDUNITBase* unit);
	void SetThickness(GLOSDUNITBase* unit, int thickness);

protected:
};

#endif /* GLOSD_HPP_ */
