/*
 * glosd.cpp
 *
 *  Created on: Nov 6, 2018
 *      Author: wzk
 */
#include "glosd.hpp"

GLShaderManager		glShaderManager;

#define VSize (m_nVert*3*sizeof(GLfloat))
#define CSize (m_nVert*4*sizeof(GLfloat))
GLOSDUNITBase::GLOSDUNITBase(GLOSD *factory, int nVert): m_factory(factory), m_nVert(nVert)
{
	m_index = -1;
	m_bUpdate = false;
	m_thickness = 1;
	OSA_assert(m_factory != NULL);
	vVertexPos = NULL;
	vColorPos = NULL;
	m_factory->Add(this);
	memset(vVertexPos, 0, VSize);
	memset(vColorPos, 0, CSize);
}
GLOSDUNITBase::~GLOSDUNITBase(void)
{
	if(m_factory != NULL)
		m_factory->Erase(this);
}

GLOSDLine::GLOSDLine(GLOSD *factory):GLOSDUNITBase(factory, 2)
{
}
GLOSDLine::~GLOSDLine(void)
{
}

void GLOSDLine::line(cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness)
{
	cv::Point2f pos;
	float *vVertex, *vColor;
	OSA_mutexLock(m_mutexlock);
	pos = normalized(pt1);
	vVertex = vVertexPos; vVertex[0] = pos.x; vVertex[1] = pos.y;
	pos = normalized(pt2);
	vVertex = vVertexPos+3; vVertex[0] = pos.x; vVertex[1] = pos.y;
	vColor = vColorPos;
	vColor[0] = color.val[0]/255.0; vColor[1] = color.val[1]/255.0;
	vColor[2] = color.val[2]/255.0; vColor[3] = color.val[3]/255.0;
	vColor = vColorPos + 4;
	vColor[0] = color.val[0]/255.0; vColor[1] = color.val[1]/255.0;
	vColor[2] = color.val[2]/255.0; vColor[3] = color.val[3]/255.0;
	m_bUpdate = true;
	if(m_thickness != thickness)
		m_factory->SetThickness(this, thickness);
	OSA_mutexUnlock(m_mutexlock);
}


GLOSD::GLOSD(int vWidth, int vHeight):m_allocCnt(0), m_iCur(0), vVertexArray(NULL), vColorArray(NULL)
{
	m_viewport = cv::Size(vWidth, vHeight);
	m_center.x = (m_viewport.width * 0.5f);
	m_center.y = (m_viewport.height * 0.5f);
	mapUnits.resize(2);
	OSA_mutexCreate(&m_mutexlock);
}

GLOSD::~GLOSD(void)
{
	OSA_mutexLock(&m_mutexlock);
	int len = vecUnits.size();
	for(int i=0; i<len; i++){
		GLOSDUNITBase *line = vecUnits[i];
		line->m_factory = NULL;
	}
	vecUnits.clear();

	if(vVertexArray != NULL)
		delete [] vVertexArray;
	vVertexArray = NULL;
	if(vColorArray != NULL)
		delete [] vColorArray;
	vColorArray = NULL;
	m_allocCnt = 0;

	mapUnits.clear();

	OSA_mutexUnlock(&m_mutexlock);
	OSA_mutexLock(&m_mutexlock);
	OSA_mutexDelete(&m_mutexlock);
}

void GLOSD::Add(GLOSDUNITBase* unit)
{
	OSA_mutexLock(&m_mutexlock);
	if(m_iCur+unit->m_nVert>m_allocCnt){
		float *remem;

		remem = new float[(m_allocCnt+50)*3*2];
		OSA_assert(remem != NULL);
		if(vVertexArray != NULL){
			memcpy(remem, vVertexArray, m_allocCnt*3*2);
			delete [] vVertexArray;
		}
		vVertexArray = remem;

		remem = new float[(m_allocCnt+50)*4*2];
		OSA_assert(remem != NULL);
		if(vColorArray != NULL){
			memcpy(remem, vColorArray, m_allocCnt*4*2);
			delete [] vColorArray;
		}
		vColorArray = remem;

		GLfloat *vPos = vVertexArray;
		GLfloat *cPos = vColorArray;
		for(int i=0; i<vecUnits.size(); i++){
			vecUnits[i]->vVertexPos = vPos;
			vecUnits[i]->vColorPos = cPos;
			vPos += vecUnits[i]->m_nVert*3;
			cPos += vecUnits[i]->m_nVert*4;
		}

		m_allocCnt = m_allocCnt+50;
	}
	int idx = vecUnits.size();
	unit->m_index = idx;
	unit->m_viewport = m_viewport;
	unit->m_center = m_center;
	unit->m_mutexlock = &m_mutexlock;
	unit->vVertexPos = vVertexArray+m_iCur*3;
	unit->vColorPos = vColorArray+m_iCur*4;
	vecUnits.push_back(unit);
	//line->m_iter = vecUnits.end();
	mapUnits[unit->m_thickness].push_back(unit);
	m_iCur+=unit->m_nVert;
	OSA_mutexUnlock(&m_mutexlock);
}

void GLOSD::Erase(GLOSDUNITBase* unit)
{
	OSA_mutexLock(&m_mutexlock);
	OSA_assert(unit == vecUnits[unit->m_index]);
	int len = vecUnits.size();
	int idx = unit->m_index;
	if(len>idx+1){
		GLfloat *vPos = unit->vVertexPos;
		GLfloat *cPos = unit->vColorPos;
		GLfloat *vNextPos = vPos + unit->m_nVert*3;
		GLfloat *cNextPos = cPos + unit->m_nVert*4;
		GLfloat *vCur = vVertexArray+m_iCur*3;
		GLfloat *cCur = vColorArray+m_iCur*4;
		memcpy(vPos, vNextPos, vCur-vNextPos);
		memcpy(cPos, cNextPos, cCur-cNextPos);
		for(int i=idx+1; i<len; i++)
		{
			vecUnits[i]->m_index = vecUnits[i]->m_index-1;
			vecUnits[i]->vVertexPos = vPos;
			vecUnits[i]->vColorPos = cPos;
			vPos += vecUnits[i]->m_nVert*3;
			cPos += vecUnits[i]->m_nVert*4;
		}
	}
	m_iCur -= unit->m_nVert;
	unit->m_factory = NULL;
	vecUnits.pop_back();
	//vecUnits.erase(line->m_iter);
	viUNIT it;
	for(it=mapUnits[unit->m_thickness].begin();it!=mapUnits[unit->m_thickness].end(); it++){
		if(*it == unit){
			mapUnits[unit->m_thickness].erase(it);
			break;
		}
	}
	OSA_mutexUnlock(&m_mutexlock);
}

void GLOSD::SetThickness(GLOSDUNITBase* unit, int thickness)
{
	viUNIT it;
	for(it=mapUnits[unit->m_thickness].begin();it!=mapUnits[unit->m_thickness].end(); it++){
		if(*it == unit){
			mapUnits[unit->m_thickness].erase(it);
			break;
		}
	}
	unit->m_thickness = thickness;
	if(unit->m_thickness+1 > mapUnits.size())
		mapUnits.resize(unit->m_thickness+1);
	mapUnits[unit->m_thickness].push_back(unit);
}

void GLOSD::Draw(void)
{
	OSA_mutexLock(&m_mutexlock);
	{
		static M3DMatrix44f	identity = { 1.0f, 0.0f, 0.0f, 0.0f,
										 0.0f, 1.0f, 0.0f, 0.0f,
										 0.0f, 0.0f, 1.0f, 0.0f,
										 0.0f, 0.0f, 0.0f, 1.0f };
		glShaderManager.UseStockShader(GLT_SHADER_SHADED, identity);
		GLBatch             floorBatch;
		GLfloat *vVer, *vColor;

		int alen = mapUnits.size();
		for(int thickness=1; thickness<alen; thickness++)
		{
			int len = mapUnits[thickness].size();
			if(len<=0)
				continue;
			floorBatch.Reset();
			floorBatch.Begin(GL_LINES, len*2);

			for(int i=0; i<len; i++){
				vVer = mapUnits[thickness][i]->vVertexPos;
				vColor = mapUnits[thickness][i]->vColorPos;
				floorBatch.Color4f(vColor[0],vColor[1],vColor[2],vColor[3]);
				floorBatch.Vertex3f(vVer[0],vVer[1],vVer[2]);
				vVer += 3;
				vColor += 4;
				floorBatch.Color4f(vColor[0],vColor[1],vColor[2],vColor[3]);
				floorBatch.Vertex3f(vVer[0],vVer[1],vVer[2]);
			}

			floorBatch.End();
			glLineWidth(thickness);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			floorBatch.Draw();
			glDisable(GL_BLEND);
			glLineWidth(1.0);
		}
	}
	OSA_mutexUnlock(&m_mutexlock);
}



