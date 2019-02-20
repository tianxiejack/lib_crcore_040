/*
 * gluVideoWindowSecondScreen.cpp
 *
 *  Created on: Feb 18, 2019
 *      Author: ubuntu
 */

#include <linux/videodev2.h>
#include "osa_image_queue.h"
#include "gluVideoWindowSecondScreen.hpp"
#include "glvideo.hpp"

CGluVideoWindowSecond::CGluVideoWindowSecond(const cv::Rect& rc, int parent, int nRender)
:CGluVideoWindow(rc, parent, nRender)
{

}

CGluVideoWindowSecond::~CGluVideoWindowSecond()
{

}

void CGluVideoWindowSecond::Display()
{
	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(1, RUN_ENTER, 0, 0);

	OSA_mutexLock(&m_mutex);
	OSA_assert(m_nRender == vvideoRenders.size());
	for(int winId=0; winId<m_nRender; winId++){
		CGLVideoBlendRender *render = vvideoRenders[winId];
		OSA_assert(render != NULL);
		if(render->m_video == NULL)
			continue;
		int chId = render->m_video->m_idx;
		if(chId < 0 || chId >= VWIND_CHAN_MAX)
			continue;
		int blend_chId = m_blendMap[chId];
		if(blend_chId>=0){
			render->blend(m_vvideos[blend_chId]);
			render->matrix(m_glmat44fBlend[chId*VWIND_CHAN_MAX+blend_chId]);
			render->params(m_glBlendPrm[chId*VWIND_CHAN_MAX+blend_chId]);
		}
		render->render();
		if(m_initPrm.renderfunc != NULL)
			m_initPrm.renderfunc(0, RUN_WIN, winId, chId);
	}
	OSA_mutexUnlock(&m_mutex);

	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(1, RUN_SWAP, 0, 0);

	CGluWindow::Display();

	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(1, RUN_LEAVE, 0, 0);
}

