/*
 * glosd.cpp
 *
 *  Created on: Nov 6, 2018
 *      Author: wzk
 */
#include "glosd.hpp"
# include <stdarg.h>

using namespace cr_osd;

GLShaderManager		glShaderManager;

GLOSDTxt::GLOSDTxt(GLOSDFactory *factory, const cv::Size2f& posScale)
: m_factory(factory),m_index(-1), m_bUpdate(false), m_text(NULL), m_memSize(0),m_textLen(0), m_bAlloc(false),
  m_share(NULL), m_sValue(NULL), m_iValue(NULL), m_fValue(NULL), m_statValue(NULL), m_byValue(NULL), m_uiValue(NULL),
  m_posScale(posScale)
{
	m_pos.x = 0;
	m_pos.y = 0;
	memset(m_vColors, 0, sizeof(m_vColors));
	m_factory->Add(this);
	m_format.clear();
}
GLOSDTxt::~GLOSDTxt(void)
{
	if(m_factory != NULL)
		m_factory->Erase(this);
	if(m_text != NULL)
		delete [] m_text;
	int n = m_format.size();
	for(int i=0; i<n; i++){
		delete [] m_format[i];
	}
	m_format.clear();
}

void GLOSDTxt::setcolor(const cv::Scalar& color)
{
	OSA_mutexLock(m_mutexlock);
	m_vColors[0] = color.val[0];
	m_vColors[1] = color.val[1];
	m_vColors[2] = color.val[2];
	m_vColors[3] = color.val[3];
	OSA_mutexUnlock(m_mutexlock);
}
void GLOSDTxt::setpos(const cv::Point& pt)
{
	OSA_mutexLock(m_mutexlock);
	m_pos = cv::Point(pt.x*m_posScale.width, pt.y*m_posScale.height);
	OSA_mutexUnlock(m_mutexlock);
}

void GLOSDTxt::txt(const cv::Point& pt, const wchar_t* text, const cv::Scalar& color)
{
	OSA_mutexLock(m_mutexlock);
	m_pos = cv::Point(pt.x*m_posScale.width, pt.y*m_posScale.height);
	m_vColors[0] = color.val[0];
	m_vColors[1] = color.val[1];
	m_vColors[2] = color.val[2];
	m_vColors[3] = color.val[3];
	m_sValue = text;
	m_share = text;
	m_textLen = wcslen(m_sValue);
	if((m_textLen)*sizeof(wchar_t)>m_memSize || m_memSize == 0){
		if(m_text != NULL)
			delete [] m_text;
		m_memSize = (m_textLen+16)*sizeof(wchar_t);
		m_text = new wchar_t[m_textLen+16];
	}
	memset(m_text, 0, m_memSize);
	memcpy(m_text, m_sValue, m_textLen*sizeof(wchar_t));
	OSA_mutexUnlock(m_mutexlock);
}

void GLOSDTxt::txt(const cv::Point& pt, const int *iValue, const wchar_t* format, const cv::Scalar& color)
{
	OSA_mutexLock(m_mutexlock);
	m_pos = cv::Point(pt.x*m_posScale.width, pt.y*m_posScale.height);
	m_vColors[0] = color.val[0];
	m_vColors[1] = color.val[1];
	m_vColors[2] = color.val[2];
	m_vColors[3] = color.val[3];
	m_iValue = iValue;
	m_share = iValue;
	size_t len = wcslen(format)+16;
	if(len*sizeof(wchar_t)>m_memSize){
		if(m_text != NULL)
			delete [] m_text;
		m_memSize = (len)*sizeof(wchar_t);
		m_text = new wchar_t[len];
		memset(m_text, 0, m_memSize);
	}
	if(m_format.size()== 0)
		m_format.resize(1);
	if(m_format[0] != NULL)
		delete [] m_format[0];
	m_format[0] = new wchar_t[len];
	memset(m_format[0], 0, m_memSize);
	memcpy(m_format[0], format, wcslen(format)*sizeof(wchar_t));
	swprintf(m_text, len, m_format[0], *m_iValue);
	OSA_mutexUnlock(m_mutexlock);
}

void GLOSDTxt::txt(const cv::Point& pt, const unsigned int *uiValue, const wchar_t* format, const cv::Scalar& color)
{
	OSA_mutexLock(m_mutexlock);
	m_pos = cv::Point(pt.x*m_posScale.width, pt.y*m_posScale.height);
	m_vColors[0] = color.val[0];
	m_vColors[1] = color.val[1];
	m_vColors[2] = color.val[2];
	m_vColors[3] = color.val[3];
	m_uiValue = uiValue;
	m_share = uiValue;
	size_t len = wcslen(format)+16;
	if(len*sizeof(wchar_t)>m_memSize){
		if(m_text != NULL)
			delete [] m_text;
		m_memSize = (len)*sizeof(wchar_t);
		m_text = new wchar_t[len];
		memset(m_text, 0, m_memSize);
	}
	if(m_format.size()== 0)
		m_format.resize(1);
	if(m_format[0] != NULL)
		delete [] m_format[0];
	m_format[0] = new wchar_t[len];
	memset(m_format[0], 0, m_memSize);
	memcpy(m_format[0], format, wcslen(format)*sizeof(wchar_t));
	swprintf(m_text, len, m_format[0], *m_uiValue);
	OSA_mutexUnlock(m_mutexlock);
}

void GLOSDTxt::txt(const cv::Point& pt, const unsigned char *byValue, const wchar_t* format, const cv::Scalar& color)
{
	OSA_mutexLock(m_mutexlock);
	m_pos = cv::Point(pt.x*m_posScale.width, pt.y*m_posScale.height);
	m_vColors[0] = color.val[0];
	m_vColors[1] = color.val[1];
	m_vColors[2] = color.val[2];
	m_vColors[3] = color.val[3];
	m_byValue = byValue;
	m_share = byValue;
	size_t len = wcslen(format)+16;
	if(len*sizeof(wchar_t)>m_memSize){
		if(m_text != NULL)
			delete [] m_text;
		m_memSize = (len)*sizeof(wchar_t);
		m_text = new wchar_t[len];
		memset(m_text, 0, m_memSize);
	}
	if(m_format.size()== 0)
		m_format.resize(1);
	if(m_format[0] != NULL)
		delete [] m_format[0];
	m_format[0] = new wchar_t[len];
	memset(m_format[0], 0, m_memSize);
	memcpy(m_format[0], format, wcslen(format)*sizeof(wchar_t));
	swprintf(m_text, len, m_format[0], *byValue);
	OSA_mutexUnlock(m_mutexlock);
}

void GLOSDTxt::txt(const cv::Point& pt, const float *fValue, const wchar_t* format, const cv::Scalar& color)
{
	OSA_mutexLock(m_mutexlock);
	m_pos = cv::Point(pt.x*m_posScale.width, pt.y*m_posScale.height);
	m_vColors[0] = color.val[0];
	m_vColors[1] = color.val[1];
	m_vColors[2] = color.val[2];
	m_vColors[3] = color.val[3];
	m_fValue = fValue;
	m_share = fValue;
	size_t len = wcslen(format)+16;
	if(len*sizeof(wchar_t)>m_memSize){
		if(m_text != NULL)
			delete [] m_text;
		m_memSize = (len)*sizeof(wchar_t);
		m_text = new wchar_t[len];
		memset(m_text, 0, m_memSize);
	}
	if(m_format.size()== 0)
		m_format.resize(1);
	if(m_format[0] != NULL)
		delete [] m_format[0];
	m_format[0] = new wchar_t[len];
	memset(m_format[0], 0, m_memSize);
	memcpy(m_format[0], format, wcslen(format)*sizeof(wchar_t));
	swprintf(m_text, len, m_format[0], *m_fValue);
	OSA_mutexUnlock(m_mutexlock);
}

void GLOSDTxt::txt(const cv::Point& pt, const cv::Scalar& color, const int *statValue, const int nStat, __gnuc_va_list __arg)
{
	OSA_mutexLock(m_mutexlock);
	m_pos = cv::Point(pt.x*m_posScale.width, pt.y*m_posScale.height);
	m_vColors[0] = color.val[0];
	m_vColors[1] = color.val[1];
	m_vColors[2] = color.val[2];
	m_vColors[3] = color.val[3];
	m_statValue = statValue;
	m_share = statValue;

	size_t len = 0;
	if(m_format.size() != nStat)
		m_format.resize(nStat);
	for(int i=0; i<nStat; i++){
		if(m_format[i] != NULL)
			delete [] m_format[0];
		const wchar_t* format = va_arg(__arg, wchar_t*);
		if(format == NULL)
			break;
		m_format[i] = new wchar_t[wcslen(format)+1];
		memset(m_format[i], 0, (wcslen(format)+1)*sizeof(wchar_t));
		memcpy(m_format[i], format, wcslen(format)*sizeof(wchar_t));
		len = std::max(len, wcslen(format)+16);
	}

	if(len*sizeof(wchar_t)>m_memSize){
		if(m_text != NULL)
			delete [] m_text;
		m_memSize = (len)*sizeof(wchar_t);
		m_text = new wchar_t[len];
	}

	int stat = *m_statValue;
	if(stat<m_format.size())
		swprintf(m_text, len, m_format[stat]);
	OSA_mutexUnlock(m_mutexlock);
}

void GLOSDTxt::update(void)
{
	int len = m_memSize/sizeof(wchar_t);
	if(m_byValue != NULL){
		swprintf(m_text, len, m_format[0], *m_byValue);
	}
	if(m_uiValue != NULL){
		swprintf(m_text, len, m_format[0], *m_uiValue);
	}
	if(m_iValue != NULL){
		swprintf(m_text, len, m_format[0], *m_iValue);
	}
	if(m_fValue != NULL){
		swprintf(m_text, len, m_format[0], *m_fValue);
	}
	if(m_statValue != NULL){
		int stat = *m_statValue;
		if(stat<m_format.size())
			swprintf(m_text, len, m_format[stat]);
	}
	if(m_sValue != NULL){
		m_textLen = wcslen(m_sValue);
		if((m_textLen)*sizeof(wchar_t)>m_memSize || m_memSize == 0){
			if(m_text != NULL)
				delete [] m_text;
			m_memSize = (m_textLen+16)*sizeof(wchar_t);
			m_text = new wchar_t[m_textLen+16];
		}
		memset(m_text, 0, m_memSize);
		memcpy(m_text, m_sValue, m_textLen*sizeof(wchar_t));
	}
	m_textLen = wcslen(m_text);
}

#define VSize (m_nVert*3*sizeof(GLfloat))
#define CSize (m_nVert*4*sizeof(GLfloat))
#define DType(p, t) (((t-1)<<4) | ((p)&0x0f))
#define DType_primitive(t) ((t)&0x0f)
#define DType_thickness(t) (((t)>>4)+1)
GLOSDUNITBase::GLOSDUNITBase(GLOSDFactory *factory, int nVert, GLenum primitive): m_factory(factory),
		m_nVert(nVert), m_primitive(primitive), m_index(-1), m_thickness(1), m_bUpdate(false),m_bRect(false)
{
	m_dtype = DType(m_primitive, m_thickness);
	OSA_assert(m_factory != NULL);
	vVertexPos = NULL;
	vColorPos = NULL;
	m_factory->Add(this);
	memset(vVertexPos, 0, VSize);
	memset(vColorPos, 0, CSize);
	m_vtps.resize(m_nVert);
}
GLOSDUNITBase::~GLOSDUNITBase(void)
{
	if(m_factory != NULL)
		m_factory->Erase(this);
	m_vtps.clear();
}

GLOSDLine::GLOSDLine(GLOSDFactory *factory):GLOSDUNITBase(factory, 2)
{
}
GLOSDLine::~GLOSDLine(void)
{
}

void GLOSDLine::line(const cv::Point& pt1, const cv::Point& pt2, const cv::Scalar& color, int thickness)
{
	cv::Point2f pos;
	float *vVertex, *vColor;
	cv::Scalar norColor(color.val[0]*0.003921569, color.val[1]*0.003921569, color.val[2]*0.003921569, color.val[3]*0.003921569);
	OSA_mutexLock(m_mutexlock);
	m_vtps[0] = pt1;
	m_vtps[1] = pt2;
	pos = normalized(pt1);
	vVertex = vVertexPos; vVertex[0] = pos.x; vVertex[1] = pos.y;
	pos = normalized(pt2);
	vVertex = vVertexPos+3; vVertex[0] = pos.x; vVertex[1] = pos.y;
	vColor = vColorPos;
	vColor[0] = norColor.val[0]; vColor[1] = norColor.val[1];
	vColor[2] = norColor.val[2]; vColor[3] = norColor.val[3];
	vColor = vColorPos + 4;
	vColor[0] = norColor.val[0]; vColor[1] = norColor.val[1];
	vColor[2] = norColor.val[2]; vColor[3] = norColor.val[3];
	m_bUpdate = true;
	if(m_thickness != thickness)
		m_factory->SetThickness(this, thickness);
	OSA_mutexUnlock(m_mutexlock);
}

GLOSDPolygon::GLOSDPolygon(GLOSDFactory *factory, int npts): GLOSDUNITBase(factory, npts, GL_LINE_LOOP/*GL_POLYGON*/)
{
}
GLOSDPolygon::~GLOSDPolygon(void)
{
}
void GLOSDPolygon::polygon(const cv::Point* pts, const cv::Scalar& color, int thickness)
{
	cv::Point2f pos;
	float *vVertex, *vColor;
	cv::Scalar norColor(color.val[0]*0.003921569, color.val[1]*0.003921569, color.val[2]*0.003921569, color.val[3]*0.003921569);
	OSA_mutexLock(m_mutexlock);
	vVertex = vVertexPos;
	vColor = vColorPos;
	for(int i=0; i<m_nVert; i++){
		m_vtps[i] = pts[i];
		pos = normalized(pts[i]);
		vVertex[0] = pos.x; vVertex[1] = pos.y;
		vVertex += 3;
		vColor[0] = norColor.val[0]; vColor[1] = norColor.val[1];
		vColor[2] = norColor.val[2]; vColor[3] = norColor.val[3];
		vColor += 4;
	}
	m_bUpdate = true;
	if(m_thickness != thickness)
		m_factory->SetThickness(this, thickness);
	OSA_mutexUnlock(m_mutexlock);
}

GLOSDRect::GLOSDRect(GLOSDFactory *factory) : GLOSDPolygon(factory, 4)
{
	m_bRect = true;
}
GLOSDRect::~GLOSDRect(void)
{
}
void GLOSDRect::rect(const cv::Rect& rec, const cv::Scalar& color, int thickness)
{
	cv::Point pt1, pt2, pts[4];
	pt1 = rec.tl(); pt2 = rec.br();
	pts[0] = cvPoint(pt1.x, pt1.y);
	pts[1] = cvPoint(pt2.x, pt1.y);
	pts[2] = cvPoint(pt2.x, pt2.y);
	pts[3] = cvPoint(pt1.x, pt2.y);
	polygon(pts, color, thickness);
}

GLOSDRectangle::GLOSDRectangle(GLOSDFactory *factory): GLOSDUNITBase(factory, 8)
{
}
GLOSDRectangle::~GLOSDRectangle(void)
{
}
void GLOSDRectangle::rectangle(const cv::Rect& rec, const cv::Scalar& color, int thickness)
{
	cv::Point pt[8], pt1, pt2;
	cv::Point2f pos[8];
	float *vVertex, *vColor;
	cv::Scalar norColor(color.val[0]*0.003921569, color.val[1]*0.003921569, color.val[2]*0.003921569, color.val[3]*0.003921569);
	OSA_mutexLock(m_mutexlock);
	pt1 = rec.tl(); pt2 = rec.br();
	int wdf = ((thickness)>>2);
	pt[0] = cvPoint(pt1.x-(wdf), pt1.y);
	pt[1] = cvPoint(pt2.x+((wdf<<1)), pt1.y);
	pt[2] = cvPoint(pt2.x, pt1.y);
	pt[3] = cvPoint(pt2.x, pt2.y);
	pt[4] = cvPoint(pt2.x+(wdf), pt2.y);
	pt[5] = cvPoint(pt1.x-((wdf<<1)), pt2.y);
	pt[6] = cvPoint(pt1.x, pt2.y);
	pt[7] = cvPoint(pt1.x, pt1.y);
	vVertex = vVertexPos;
	vColor = vColorPos;
	for(int i=0; i<m_nVert; i++){
		m_vtps[i] = pt[i];
		pos[i] = normalized(pt[i]);
		vVertex[0] = pos[i].x; vVertex[1] = pos[i].y;
		vVertex += 3;
		vColor[0] = norColor.val[0]; vColor[1] = norColor.val[1];
		vColor[2] = norColor.val[2]; vColor[3] = norColor.val[3];
		vColor += 4;
	}
	m_bUpdate = true;
	if(m_thickness != thickness)
		m_factory->SetThickness(this, thickness);
	OSA_mutexUnlock(m_mutexlock);
}


/*******************************************************
 *
 *
 *
 */
GLOSDFactoryBase::GLOSDFactoryBase(int vWidth, int vHeight, int fontSize)
:m_allocCnt(0), m_iCur(0), vVertexArray(NULL), vColorArray(NULL), m_fontSize(fontSize)
{
	m_viewport = cv::Size(vWidth, vHeight);
	m_center.x = (m_viewport.width * 0.5f);
	m_center.y = (m_viewport.height * 0.5f);
	mapUnits.resize(8);
	vecCnt.resize(8);
	vecCnt[0] = 0;
	vecCnt[1] = 0;
	m_color = cv::Scalar::all(1.0);
	m_thickness = 1;
	OSA_mutexCreate(&m_mutexlock);
}

GLOSDFactoryBase::~GLOSDFactoryBase(void)
{
	OSA_mutexLock(&m_mutexlock);
	int len;

	len = vecTxts.size();
	for(int i=0; i<len; i++){
		GLOSDTxt *txt = vecTxts[i];
		txt->m_factory = NULL;
	}
	vecTxts.clear();

	len = vecUnits.size();
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
	vecCnt.clear();

	OSA_mutexUnlock(&m_mutexlock);
	OSA_mutexLock(&m_mutexlock);
	OSA_mutexDelete(&m_mutexlock);
}

void GLOSDFactoryBase::Add(GLOSDTxt* txt)
{
	int idx = vecTxts.size();
	txt->m_index = idx;
	txt->m_viewport = m_viewport;
	txt->m_center = m_center;
	txt->m_fontSize = m_fontSize;
	txt->m_mutexlock = &m_mutexlock;
	vecTxts.push_back(txt);
}
void GLOSDFactoryBase::Erase(GLOSDTxt* txt)
{
	viTXT it;
	for(it=vecTxts.begin();it!=vecTxts.end(); it++){
		if(*it == txt){
			vecTxts.erase(it);
			break;
		}
	}
	int i=0;
	for(it=vecTxts.begin();it!=vecTxts.end(); it++){
		(*it)->m_index = i++;
	}
}

void GLOSDFactoryBase::Add(GLOSDUNITBase* unit)
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
	mapUnits[unit->m_dtype].push_back(unit);
	vecCnt[unit->m_dtype] += unit->m_nVert;
	m_iCur+=unit->m_nVert;
	//OSA_printf("%s : %d %p", __func__, unit->m_index, unit);
	OSA_mutexUnlock(&m_mutexlock);
}

void GLOSDFactoryBase::Erase(GLOSDUNITBase* unit)
{
	if(unit == NULL || unit->m_factory != this || unit->m_index<0 ||unit != vecUnits[unit->m_index]){
		OSA_printf("%s: %d ", __func__, unit->m_index);
	}
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
			vecUnits[i]->m_index -= 1;//vecUnits[i]->m_index-1;
			vecUnits[i]->vVertexPos = vPos;
			vecUnits[i]->vColorPos = cPos;
			vPos += vecUnits[i]->m_nVert*3;
			cPos += vecUnits[i]->m_nVert*4;
			//printf("; idx = %d %d %p", idx, vecUnits[i]->m_index, vecUnits[i]);
		}
	}
	m_iCur -= unit->m_nVert;
	unit->m_factory = NULL;
	unit->m_index = -1;
	//vecUnits.pop_back();
	for(viUNIT it=vecUnits.begin();it!=vecUnits.end(); it++){
		if(*it == unit){
			vecUnits.erase(it);
			break;
		}
	}

	for(viUNIT it=mapUnits[unit->m_dtype].begin();it!=mapUnits[unit->m_dtype].end(); it++){
		if(*it == unit){
			mapUnits[unit->m_dtype].erase(it);
			vecCnt[unit->m_dtype]-=unit->m_nVert;
			break;
		}
	}

	OSA_mutexUnlock(&m_mutexlock);
}

void GLOSDFactoryBase::SetThickness(GLOSDUNITBase* unit, int thickness)
{
	viUNIT it;
	for(it=mapUnits[unit->m_dtype].begin();it!=mapUnits[unit->m_dtype].end(); it++){
		if(*it == unit){
			mapUnits[unit->m_dtype].erase(it);
			vecCnt[unit->m_dtype]-=unit->m_nVert;
			break;
		}
	}
	unit->m_thickness = thickness;
	unit->m_dtype = DType(unit->m_primitive, unit->m_thickness);
	if(unit->m_dtype+1 > mapUnits.size()){
		mapUnits.resize(unit->m_dtype+1);
		vecCnt.resize(unit->m_dtype+1);
	}
	mapUnits[unit->m_dtype].push_back(unit);
	vecCnt[unit->m_dtype]+=unit->m_nVert;
}

/*********************************************************
 *
 *
 */
GLOSD::GLOSD(int vWidth, int vHeight, int fontSize, const char* faceName)
: GLOSDFactoryBase(vWidth, vHeight, fontSize),m_gltxt(NULL)
{
	m_gltxt = new GLTXT(faceName, m_fontSize, vWidth, vHeight);
}
GLOSD::~GLOSD(void)
{
	delete m_gltxt;
}
void GLOSD::directDraw(const cv::Scalar& norColor, int  thickness)
{
	int nPts = m_vPts.size();
	if(nPts>0)
	{
		M3DVector4f  vColor;
		vColor[0] = norColor.val[0];vColor[1] = norColor.val[1];
		vColor[2] = norColor.val[2];vColor[3] = norColor.val[3];
		glShaderManager.UseStockShader(GLT_SHADER_IDENTITY, vColor);
		GLBatch lineBatch;
		lineBatch.Reset();
		lineBatch.Begin(GL_LINES, nPts);
		for(int i=0; i<nPts; i++)
		{
			lineBatch.Vertex3f(m_vPts[i].x,m_vPts[i].y, 0);
		}
		lineBatch.End();
		glLineWidth(thickness);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		lineBatch.Draw();
		glDisable(GL_BLEND);
		glLineWidth(1.0);
	}

	glShaderManager.UseStockShader(GLT_SHADER_TEXTURE_SHADED, 0);
	int nNums = m_vNumPts.size();
	cv::Scalar color(norColor.val[0]*255.0,norColor.val[1]*255.0,norColor.val[2]*255.0,norColor.val[3]*255.0);
	for(int i=0; i<nNums; i++){
		wchar_t wsNumber[16];
		memset(wsNumber, 0, sizeof(wsNumber));
		swprintf(wsNumber, 16, L"%d", m_vNums[i]);
		cv::Point pt(m_vNumPts[i].x+((thickness-m_thickness)>>1), m_vNumPts[i].y+((thickness-m_thickness)>>1));
		m_gltxt->putText(m_viewport, wsNumber, pt, color);
	}

}
void GLOSD::Draw(void)
{
	OSA_mutexLock(&m_mutexlock);
	glViewport(0, 0, m_viewport.width, m_viewport.height);

	directDraw(cv::Scalar(fabs(m_color.val[0]-0.5), fabs(m_color.val[1]-0.5), fabs(m_color.val[2]-0.5), 1.0), m_thickness+2);
	directDraw(m_color, m_thickness);

	int alen = mapUnits.size();
	if(alen>0)
	{
		GLfloat *vVer, *vColor;
		static M3DMatrix44f	identity = { 1.0f, 0.0f, 0.0f, 0.0f,
										 0.0f, 1.0f, 0.0f, 0.0f,
										 0.0f, 0.0f, 1.0f, 0.0f,
										 0.0f, 0.0f, 0.0f, 1.0f };
		glShaderManager.UseStockShader(GLT_SHADER_SHADED, identity);
		for(int type=alen-1; type>=0; --type)
		{
			int len = mapUnits[type].size();
			if(len<=0)
				continue;
			int thickness = DType_thickness(type);
			GLenum primitive = DType_primitive(type);
			GLBatch             floorBatch;
			floorBatch.Reset();
			floorBatch.Begin(primitive, vecCnt[type]);

			for(int i=0; i<len; i++){
				GLOSDUNITBase* unit = mapUnits[type][i];
				vVer = unit->vVertexPos;
				vColor = unit->vColorPos;
				int nVert = unit->m_nVert;
				for(int j=0; j<nVert; j++){
					floorBatch.Color4f(vColor[0],vColor[1],vColor[2],vColor[3]);
					floorBatch.Vertex3f(vVer[0],vVer[1],vVer[2]);
					vVer += 3;
					vColor += 4;
				}
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

	glShaderManager.UseStockShader(GLT_SHADER_TEXTURE_SHADED, 0);
	int cnt = vecTxts.size();
	for(int idx = 0; idx<cnt; idx++){
		GLOSDTxt* txt = vecTxts[idx];
		OSA_assert(txt != NULL);
		OSA_assert(m_gltxt != NULL);
		txt->update();
		if(txt->m_text != NULL)
			txt->m_textLen = wcslen(txt->m_text);
		if(txt->m_textLen>0){
			cv::Point pos2(txt->m_pos.x+1, txt->m_pos.y+1);
			cv::Scalar color2(fabs(txt->m_vColors[0]-128.0),fabs(txt->m_vColors[1]-128.0),fabs(txt->m_vColors[2]-128.0),txt->m_vColors[3]);
			m_gltxt->putText(m_viewport, txt->m_text, pos2, color2);
			cv::Point pos1(txt->m_pos.x, txt->m_pos.y);
			cv::Scalar color1(txt->m_vColors[0],txt->m_vColors[1],txt->m_vColors[2],txt->m_vColors[3]);
			m_gltxt->putText(m_viewport, txt->m_text, pos1, color1);
		}
	}

	OSA_mutexUnlock(&m_mutexlock);
}

void GLOSD::begin(cv::Scalar& color, int thickness)
{
	OSA_mutexLock(&m_mutexlock);
	m_color = cv::Scalar(color.val[0]*0.003921569,color.val[1]*0.003921569,color.val[2]*0.003921569,color.val[3]*0.003921569);
	m_thickness = thickness;
	m_vPts.clear();
	m_vNumPts.clear();
	m_vNums.clear();
	OSA_mutexUnlock(&m_mutexlock);
}

void GLOSD::end(void)
{
	OSA_mutexLock(&m_mutexlock);
	//m_vPts.clear();
	//m_vNumPts.clear();
	//m_vNums.clear();
	OSA_mutexUnlock(&m_mutexlock);
}

void GLOSD::line(const std::vector<cv::Point>& pts, int flag)
{
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	int nPts = pts.size();
	for(int i=0; i<nPts; i++){
		m_vPts.push_back(normalized(pts[i]));
	}
	OSA_mutexUnlock(&m_mutexlock);
}
void GLOSD::polygon(const std::vector<cv::Point>& pts, int flag)
{
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	int nPts = pts.size();
	if(nPts>0){
		m_vPts.push_back(normalized(pts[0]));
		for(int i=0; i<nPts; i++){
			m_vPts.push_back(normalized(pts[i]));
			m_vPts.push_back(normalized(pts[i]));
		}
		m_vPts.push_back(normalized(pts[0]));
	}
	OSA_mutexUnlock(&m_mutexlock);
}
void GLOSD::rectangle(const cv::Rect& rec, int flag)
{
	cv::Point pt1, pt2;
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	pt1 = rec.tl(); pt2 = rec.br();
	int wdf = ((m_thickness)>>2);
	m_vPts.push_back(normalized(cvPoint(pt1.x-(wdf), pt1.y)));
	m_vPts.push_back(normalized(cvPoint(pt2.x+((wdf<<1)), pt1.y)));
	m_vPts.push_back(normalized(cvPoint(pt2.x, pt1.y)));
	m_vPts.push_back(normalized(cvPoint(pt2.x, pt2.y)));
	m_vPts.push_back(normalized(cvPoint(pt2.x+(wdf), pt2.y)));
	m_vPts.push_back(normalized(cvPoint(pt1.x-((wdf<<1)), pt2.y)));
	m_vPts.push_back(normalized(cvPoint(pt1.x, pt2.y)));
	m_vPts.push_back(normalized(cvPoint(pt1.x, pt1.y)));
	OSA_mutexUnlock(&m_mutexlock);
}
void GLOSD::cross(const cv::Point& center, const cv::Size2f& scale, int flag)
{
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	UInt32 width=0, height=0, widthGap=0, heightGap=0;
	width = 120*scale.width;
	height = 100*scale.height;
	widthGap = 40*scale.width;
	heightGap = 30*scale.height;
	// with center point
	//left horizonal line
	m_vPts.push_back(normalized(cvPoint(center.x-(width>>1),center.y)));
	m_vPts.push_back(normalized(cvPoint(center.x-(widthGap>>1),center.y)));
	//m_vPts.push_back(normalized(cvPoint(center.x-(width>>1),center.y+thickness+1)));
	//m_vPts.push_back(normalized(cvPoint(center.x-(widthGap>>1),center.y+thickness+1)));
	//middle horizonal line
	m_vPts.push_back(normalized(cvPoint(center.x-(m_thickness>>1)-1,center.y)));
	m_vPts.push_back(normalized(cvPoint(center.x+(m_thickness>>1)+1,center.y)));
	//right horizonal line
	m_vPts.push_back(normalized(cvPoint(center.x+(width>>1),center.y)));
	m_vPts.push_back(normalized(cvPoint(center.x+(widthGap>>1),center.y)));
	//top vertical line
	m_vPts.push_back(normalized(cvPoint(center.x, center.y-(height>>1))));
	m_vPts.push_back(normalized(cvPoint(center.x, center.y-(heightGap>>1))));
	//bottom vertical line
	m_vPts.push_back(normalized(cvPoint(center.x, center.y+(height>>1))));
	m_vPts.push_back(normalized(cvPoint(center.x, center.y+(heightGap>>1))));
	OSA_mutexUnlock(&m_mutexlock);
}

void GLOSD::numberedBox(const cv::Rect& rec, int number, int flag)
{
	cv::Point pt1, pt2;
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	pt1 = rec.tl(); pt2 = rec.br();
	int wdf = ((m_thickness)>>2);
	m_vPts.push_back(normalized(cvPoint(pt1.x-(wdf), pt1.y)));
	m_vPts.push_back(normalized(cvPoint(pt2.x+((wdf<<1)), pt1.y)));
	m_vPts.push_back(normalized(cvPoint(pt2.x, pt1.y)));
	m_vPts.push_back(normalized(cvPoint(pt2.x, pt2.y)));
	m_vPts.push_back(normalized(cvPoint(pt2.x+(wdf), pt2.y)));
	m_vPts.push_back(normalized(cvPoint(pt1.x-((wdf<<1)), pt2.y)));
	m_vPts.push_back(normalized(cvPoint(pt1.x, pt2.y)));
	m_vPts.push_back(normalized(cvPoint(pt1.x, pt1.y)));
	m_vNumPts.push_back((cvPoint(rec.x+rec.width+2, rec.y-m_fontSize-2)));
	m_vNums.push_back(number);
	OSA_mutexUnlock(&m_mutexlock);
}
enum { XY_SHIFT = 16, XY_ONE = 1 << XY_SHIFT, DRAWING_STORAGE_BLOCK = (1<<12) - 256 };
void GLOSD::ellipse(const cv::RotatedRect& box, int number, int flag)
{
	if(flag == 1)return ;
	OSA_mutexLock(&m_mutexlock);
    int _angle = cvRound(box.angle);
    cv::Point center(cvRound(box.center.x*(1 << XY_SHIFT)),
                 cvRound(box.center.y*(1 << XY_SHIFT)));
    //cv::Point center(cvRound(500*(1 << XY_SHIFT)),
    //             cvRound(400*(1 << XY_SHIFT)));
    cv::Size axes(cvRound(box.size.width*(1 << (XY_SHIFT - 1))),
              cvRound(box.size.height*(1 << (XY_SHIFT - 1))));

    axes.width = std::abs(axes.width), axes.height = std::abs(axes.height);
    int delta = (std::max(axes.width,axes.height)+(XY_ONE>>1))>>XY_SHIFT;
    delta = delta < 3 ? 90 : delta < 10 ? 30 : delta < 15 ? 18 : 5;

    std::vector<cv::Point> v;
    cv::ellipse2Poly( center, axes, _angle, 0, 360, delta, v );

    int nPts = v.size();
    if(nPts>1){
    	int i = 0;
    	cv::Point2f bpt = normalized( cv::Point((v[i].x + (XY_ONE>>1)) >> XY_SHIFT, (v[i].y+ (XY_ONE>>1)) >> XY_SHIFT) );
		m_vPts.push_back( bpt );
		for(i=1; i<nPts; i++){
			cv::Point2f pt = normalized( cv::Point((v[i].x + (XY_ONE>>1)) >> XY_SHIFT, (v[i].y+ (XY_ONE>>1)) >> XY_SHIFT) );
			m_vPts.push_back( pt );
			m_vPts.push_back( pt );
		}
		m_vPts.push_back( bpt );

		if(flag & 4)
		{
			cv::Point pt = ( cv::Point((v[(nPts>>3)*5].x + (XY_ONE>>1)) >> XY_SHIFT, (v[(nPts>>3)*5].y+ (XY_ONE>>1)) >> XY_SHIFT) );
			m_vNumPts.push_back((cvPoint(pt.x+2, pt.y-m_fontSize-2)));
			m_vNums.push_back(number);
		}
    }
	if(flag & 2)
	{
		m_vPts.push_back( normalized(cv::Point(box.center.x-4, box.center.y-4)) );
		m_vPts.push_back( normalized(cv::Point(box.center.x+4, box.center.y+4)) );
		m_vPts.push_back( normalized(cv::Point(box.center.x-4, box.center.y+4)) );
		m_vPts.push_back( normalized(cv::Point(box.center.x+4, box.center.y-4)) );
	}

	OSA_mutexUnlock(&m_mutexlock);
}

/*************************************************************
 *
 *
 *
 */

DCOSD::DCOSD(cv::Mat *dc, int fontSize, const char* faceName)
:GLOSDFactoryBase(dc->cols, dc->rows, fontSize),m_dc(dc), m_dcuMem(NULL),m_idcu(0)
{
	m_dcuMem = new DCU[1024];
	memset(m_dcuMem, 0, sizeof(DCU)*1024);
	int ndcu = m_vdcus[0].size();
	for(int i=0; i<ndcu; i++){
		m_vdcus[0][i]->vdrawPos.clear();
	}
	m_vdcus[0].clear();
	ndcu = m_vdcus[1].size();
	for(int i=0; i<ndcu; i++){
		m_vdcus[1][i]->vdrawPos.clear();
	}
	m_vdcus[1].clear();

	m_dctxt = new DCTXT(faceName, m_fontSize);
}
DCOSD::~DCOSD(void)
{
	delete [] m_dcuMem;
	delete m_dctxt;
}
void DCOSD::dcuDraw(const DCU& dcu, const cv::Scalar& color)
{
	int cnt = dcu.vdrawPos.size();
	//OSA_printf("%dx%d %d %d", m_dc->cols, m_dc->rows, cnt, dcu.bTxt);

	if(dcu.bTxt){
		m_dctxt->putText(*m_dc, dcu.txt, dcu.vdrawPos[0], color);
		//cv::putText(*m_dc, dcu.txt, dcu.vdrawPos[0], CV_FONT_HERSHEY_COMPLEX, 1.0,color);
		return ;
	}
	if(dcu.primitive == GL_LINES){
		for(int i=0; i<(cnt>>1); i++){
			//OSA_printf("%dx%d (%d, %d)(%d %d) color(%f %f %f %f) %d %d", m_dc->cols, m_dc->rows,
			//		dcu.vdrawPos[i*2].x, dcu.vdrawPos[i*2].y,
			//		dcu.vdrawPos[i*2+1].x, dcu.vdrawPos[i*2+1].y,
			//		color.val[0],color.val[1],color.val[2],color.val[2],
			//		dcu.thickness, dcu.lineType);
			cv::line(*m_dc, dcu.vdrawPos[i*2], dcu.vdrawPos[i*2+1], color, dcu.thickness, dcu.lineType);
		}
		return ;
	}
	if(dcu.primitive == GL_LINE_LOOP && cnt>0){
		if(dcu.bRect){
			cv::rectangle(*m_dc, dcu.vdrawPos[0], dcu.vdrawPos[2], color, dcu.thickness, dcu.lineType);
		}
		else{
			int i;
			for(i=0; i<cnt-1; i++){
				cv::line(*m_dc, dcu.vdrawPos[i], dcu.vdrawPos[i+1], color, dcu.thickness, dcu.lineType);
			}
			cv::line(*m_dc, dcu.vdrawPos[i], dcu.vdrawPos[0], color, dcu.thickness, dcu.lineType);
		}
		return ;
	}
}
void DCOSD::HideDCU(vDCU& vdcus)
{
	int ndcu = vdcus.size();
	if(ndcu<=0)
		return ;
	cv::Scalar color = cv::Scalar::all(0);
	for(int i=0; i<ndcu; i++){
		dcuDraw(*vdcus[i], color);
		vdcus[i]->vdrawPos.clear();
	}
	vdcus.clear();
}
void DCOSD::HideDCU(vDCU& vdcus, const void *context)
{
	int ndcu = vdcus.size();
	if(ndcu<=0)
		return ;
	cv::Scalar color = cv::Scalar::all(0);
	for(viDCU it=vdcus.begin();it!=vdcus.end(); it++){
		if((*it)->context == context){
			dcuDraw(**it, color);
			(*it)->vdrawPos.clear();
			vdcus.erase(it);
			break;
		}
	}
}
void DCOSD::directDraw(void)
{
	int nLines = (m_vPts.size()>>1);
	for(int i=0; i<nLines; i++){
		if(m_dc->channels() == 1)
			cv::line(*m_dc, m_vPts[i*2], m_vPts[i*2+1], cv::Scalar(m_color.val[3]), m_thickness);
		else
			cv::line(*m_dc, m_vPts[i*2], m_vPts[i*2+1], m_color, m_thickness);
	}
	int nNums = m_vNumPts.size();
	for(int i=0; i<nNums; i++){
		wchar_t wsNumber[16];
		memset(wsNumber, 0, sizeof(wsNumber));
		swprintf(wsNumber, 16, L"%d", m_vNums[i]);
		m_dctxt->putText(*m_dc, wsNumber, m_vNumPts[i], m_color);
	}
}
void DCOSD::Draw(void)
{
	OSA_mutexLock(&m_mutexlock);

	directDraw();

	int hdcu = (m_idcu ^ 1);
	DCU *dcuPos = m_dcuMem;

	HideDCU(m_vdcus[hdcu]);

	GLfloat *vVer, *vColor;
	int alen = mapUnits.size();
	//OSA_printf("alen = %d", alen);
	for(int type=alen-1; type>=0; --type)
	{
		//OSA_printf("alen = %d type = %d", alen, type);
		int len = mapUnits[type].size();
		//OSA_printf("alen = %d type = %d len = %d", alen, type, len);
		if(len<=0)
			continue;
		int thickness = DType_thickness(type);
		GLenum primitive = DType_primitive(type);
		//OSA_printf("alen = %d type = %d len = %d thickness = %d primitive = %d",alen, type, len, thickness, primitive);

		for(int i=0; i<len; i++){
			GLOSDUNITBase* unit = mapUnits[type][i];
			dcuPos->context = unit;
			dcuPos->primitive = primitive;
			dcuPos->bRect = unit->m_bRect;
			dcuPos->thickness = thickness;
			dcuPos->lineType = 8;//CV_AA;
			dcuPos->bTxt = false;
			vVer = unit->vVertexPos;
			vColor = unit->vColorPos;
			int nVert = unit->m_nVert;
			dcuPos->vdrawPos.clear();
			OSA_assert(m_dc != NULL);
			for(int j=0; j<nVert; j++){
				//cv::Point *pt = new cv::Point(vVer[0]*m_dc->cols+0.5, vVer[1]*m_dc->rows+0.5);
				dcuPos->vdrawPos.push_back(unit->m_vtps[j]);
				vVer += 3;
				//vColor += 4;
			}
			cv::Scalar color;
			if(m_dc->channels() == 1)
				color = cv::Scalar(vColor[3]*255.0);
			else
				color = cv::Scalar(vColor[0]*255.0, vColor[1]*255.0, vColor[2]*255.0, vColor[3]*255.0);
			dcuDraw(*dcuPos, color);
			m_vdcus[m_idcu].push_back(dcuPos);
			dcuPos+=1;
		}
	}

	int cnt = vecTxts.size();
	for(int idx = 0; idx<cnt; idx++){
		GLOSDTxt* txt = vecTxts[idx];
		OSA_assert(txt != NULL);
		txt->update();
		if(txt->m_text != NULL)
			txt->m_textLen = wcslen(txt->m_text);
		if(txt->m_textLen>0){
			dcuPos->bTxt = true;
			dcuPos->vdrawPos.clear();
			dcuPos->vdrawPos.push_back(txt->m_pos);
			memset(dcuPos->txt, 0, sizeof(dcuPos->txt));
			memcpy(dcuPos->txt, txt->m_text, txt->m_textLen*sizeof(wchar_t));
			cv::Scalar color(txt->m_vColors[0],txt->m_vColors[1],txt->m_vColors[2],txt->m_vColors[3]);
			dcuDraw(*dcuPos, color);
			m_vdcus[m_idcu].push_back(dcuPos);
			dcuPos+=1;
		}
	}

	m_idcu ^= 1;

	OSA_mutexUnlock(&m_mutexlock);
}

void DCOSD::begin(cv::Scalar& color, int thickness)
{
	OSA_mutexLock(&m_mutexlock);
	m_color = cv::Scalar(color.val[0],color.val[1],color.val[2],color.val[3]);
	m_thickness = thickness;
	m_vPts.clear();
	m_vNumPts.clear();
	m_vNums.clear();
	OSA_mutexUnlock(&m_mutexlock);
}

void DCOSD::end(void)
{
	OSA_mutexLock(&m_mutexlock);
	m_color = cv::Scalar::all(0);
	directDraw();
	m_vPts.clear();
	m_vNumPts.clear();
	m_vNums.clear();
	OSA_mutexUnlock(&m_mutexlock);
}

void DCOSD::line(const std::vector<cv::Point>& pts, int flag)
{
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	int nPts = pts.size();
	for(int i=0; i<nPts; i++){
		m_vPts.push_back(pts[i]);
	}
	OSA_mutexUnlock(&m_mutexlock);
}
void DCOSD::polygon(const std::vector<cv::Point>& pts, int flag)
{
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	int nPts = pts.size();
	if(nPts>0){
		m_vPts.push_back(pts[0]);
		for(int i=0; i<nPts; i++){
			m_vPts.push_back(pts[i]);
			m_vPts.push_back(pts[i]);
		}
		m_vPts.push_back(pts[0]);
	}
	OSA_mutexUnlock(&m_mutexlock);
}
void DCOSD::rectangle(const cv::Rect& rec, int flag)
{
	cv::Point pt1, pt2;
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	pt1 = rec.tl(); pt2 = rec.br();
	int wdf = ((m_thickness)>>2);
	m_vPts.push_back((cvPoint(pt1.x-(wdf), pt1.y)));
	m_vPts.push_back((cvPoint(pt2.x+((wdf<<1)), pt1.y)));
	m_vPts.push_back((cvPoint(pt2.x, pt1.y)));
	m_vPts.push_back((cvPoint(pt2.x, pt2.y)));
	m_vPts.push_back((cvPoint(pt2.x+(wdf), pt2.y)));
	m_vPts.push_back((cvPoint(pt1.x-((wdf<<1)), pt2.y)));
	m_vPts.push_back((cvPoint(pt1.x, pt2.y)));
	m_vPts.push_back((cvPoint(pt1.x, pt1.y)));
	OSA_mutexUnlock(&m_mutexlock);
}
void DCOSD::cross(const cv::Point& center, const cv::Size2f& scale, int flag)
{
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	UInt32 width=0, height=0, widthGap=0, heightGap=0;
	width = 120*scale.width;
	height = 100*scale.height;
	widthGap = 40*scale.width;
	heightGap = 30*scale.height;
	// with center point
	//left horizonal line
	m_vPts.push_back((cvPoint(center.x-(width>>1),center.y)));
	m_vPts.push_back((cvPoint(center.x-(widthGap>>1),center.y)));
	//m_vPts.push_back((cvPoint(center.x-(width>>1),center.y+thickness+1)));
	//m_vPts.push_back((cvPoint(center.x-(widthGap>>1),center.y+thickness+1)));
	//middle horizonal line
	m_vPts.push_back((cvPoint(center.x-(m_thickness>>1)-1,center.y)));
	m_vPts.push_back((cvPoint(center.x+(m_thickness>>1)+1,center.y)));
	//right horizonal line
	m_vPts.push_back((cvPoint(center.x+(width>>1),center.y)));
	m_vPts.push_back((cvPoint(center.x+(widthGap>>1),center.y)));
	//top vertical line
	m_vPts.push_back((cvPoint(center.x, center.y-(height>>1))));
	m_vPts.push_back((cvPoint(center.x, center.y-(heightGap>>1))));
	//bottom vertical line
	m_vPts.push_back((cvPoint(center.x, center.y+(height>>1))));
	m_vPts.push_back((cvPoint(center.x, center.y+(heightGap>>1))));
	OSA_mutexUnlock(&m_mutexlock);
}

void DCOSD::numberedBox(const cv::Rect& rec, int number, int flag)
{
	cv::Point pt1, pt2;
	if(flag != 0)return ;
	OSA_mutexLock(&m_mutexlock);
	pt1 = rec.tl(); pt2 = rec.br();
	int wdf = ((m_thickness)>>2);
	m_vPts.push_back((cvPoint(pt1.x-(wdf), pt1.y)));
	m_vPts.push_back((cvPoint(pt2.x+((wdf<<1)), pt1.y)));
	m_vPts.push_back((cvPoint(pt2.x, pt1.y)));
	m_vPts.push_back((cvPoint(pt2.x, pt2.y)));
	m_vPts.push_back((cvPoint(pt2.x+(wdf), pt2.y)));
	m_vPts.push_back((cvPoint(pt1.x-((wdf<<1)), pt2.y)));
	m_vPts.push_back((cvPoint(pt1.x, pt2.y)));
	m_vPts.push_back((cvPoint(pt1.x, pt1.y)));
	m_vNumPts.push_back((cvPoint(rec.x+rec.width+2, rec.y-m_fontSize-2)));
	m_vNums.push_back(number);
	OSA_mutexUnlock(&m_mutexlock);
}
//enum { XY_SHIFT = 16, XY_ONE = 1 << XY_SHIFT, DRAWING_STORAGE_BLOCK = (1<<12) - 256 };
void DCOSD::ellipse(const cv::RotatedRect& box, int number, int flag)
{
	if(flag == 1)return ;
	OSA_mutexLock(&m_mutexlock);
    int _angle = cvRound(box.angle);
    cv::Point center(cvRound(box.center.x*(1 << XY_SHIFT)),
                 cvRound(box.center.y*(1 << XY_SHIFT)));
    //cv::Point center(cvRound(500*(1 << XY_SHIFT)),
    //             cvRound(400*(1 << XY_SHIFT)));
    cv::Size axes(cvRound(box.size.width*(1 << (XY_SHIFT - 1))),
              cvRound(box.size.height*(1 << (XY_SHIFT - 1))));

    axes.width = std::abs(axes.width), axes.height = std::abs(axes.height);
    int delta = (std::max(axes.width,axes.height)+(XY_ONE>>1))>>XY_SHIFT;
    delta = delta < 3 ? 90 : delta < 10 ? 30 : delta < 15 ? 18 : 5;

    std::vector<cv::Point> v;
    cv::ellipse2Poly( center, axes, _angle, 0, 360, delta, v );

    int nPts = v.size();
    if(nPts>1){
    	int i = 0;
    	cv::Point bpt = ( cv::Point((v[i].x + (XY_ONE>>1)) >> XY_SHIFT, (v[i].y+ (XY_ONE>>1)) >> XY_SHIFT) );
		m_vPts.push_back( bpt );
		for(i=1; i<nPts; i++){
			cv::Point pt = ( cv::Point((v[i].x + (XY_ONE>>1)) >> XY_SHIFT, (v[i].y+ (XY_ONE>>1)) >> XY_SHIFT) );
			m_vPts.push_back( pt );
			m_vPts.push_back( pt );
		}
		m_vPts.push_back( bpt );

		if(flag & 4)
		{
			cv::Point pt = ( cv::Point((v[(nPts>>3)*5].x + (XY_ONE>>1)) >> XY_SHIFT, (v[(nPts>>3)*5].y+ (XY_ONE>>1)) >> XY_SHIFT) );
			m_vNumPts.push_back((cvPoint(pt.x+2, pt.y-m_fontSize-2)));
			m_vNums.push_back(number);
		}
    }
	if(flag & 2)
	{
		m_vPts.push_back( (cv::Point(box.center.x-4, box.center.y-4)) );
		m_vPts.push_back( (cv::Point(box.center.x+4, box.center.y+4)) );
		m_vPts.push_back( (cv::Point(box.center.x-4, box.center.y+4)) );
		m_vPts.push_back( (cv::Point(box.center.x+4, box.center.y-4)) );
	}

	OSA_mutexUnlock(&m_mutexlock);
}

