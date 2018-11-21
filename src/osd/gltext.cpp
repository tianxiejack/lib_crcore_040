/*
 * gltext.cpp
 *
 *  Created on: Nov 8, 2018
 *      Author: wzk
 */
#include <FreetypeFont.hpp>
#include <gltext.hpp>

using namespace cr_osd;

GLTXT::GLTXT(const char* faceName, int fontSize, int nWidth, int nHeight)
:m_freetype(NULL)
{
	FreeTypeFont *font;
	char defaultface[] = "/usr/share/fonts/truetype/abyssinica/AbyssinicaSIL-R.ttf";
	//char defaultface[] = "/usr/share/fonts/truetype/abyssinica/simsun.ttc";
	font = new FreeTypeFont;
	if(faceName == NULL){
		faceName = defaultface;
	}
	if(font->create(faceName,fontSize,nWidth,nHeight) != true){
		printf("\n%s %d: font create fail!!!!!!", __func__, __LINE__);
	}
	m_freetype = font;
}
GLTXT::~GLTXT(void)
{
	FreeTypeFont *font = (FreeTypeFont *)m_freetype;
	delete font;
}

#include <GLShaderManager.h>
extern GLShaderManager		glShaderManager;
//void GLTXT::drawtxt(int x, int y, const wchar_t* text, const float* color)
void GLTXT::putText(const cv::Size& viewSize, const wchar_t* text, const cv::Point& pt, const cv::Scalar& color)
{
	FreeTypeFont *font = (FreeTypeFont *)m_freetype;
	font->begin();
	font->putText(viewSize, text, pt, color);
	font->end();
}
