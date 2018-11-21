/*
 * dctext.cpp
 *
 *  Created on: Nov 8, 2018
 *      Author: wzk
 */
#include <FreetypeFontDC.hpp>
#include <dctext.hpp>

using namespace cr_osd;

DCTXT::DCTXT(const char* faceName, int fontSize)
:m_freetype(NULL)
{
	FreeTypeFont *font;
	char defaultface[] = "/usr/share/fonts/truetype/abyssinica/AbyssinicaSIL-R.ttf";
	//char defaultface[] = "/usr/share/fonts/truetype/abyssinica/simsun.ttc";
	font = new FreeTypeFont;
	if(faceName == NULL){
		faceName = defaultface;
	}
	if(font->create(faceName,fontSize) != true){
		printf("\n%s %d: font create fail!!!!!!", __func__, __LINE__);
	}
	m_freetype = font;
}
DCTXT::~DCTXT(void)
{
	FreeTypeFont *font = (FreeTypeFont *)m_freetype;
	delete font;
}

void DCTXT::putText(const cv::Mat& img, const wchar_t* text, const cv::Point& pt, const cv::Scalar& color)
{
	FreeTypeFont *font = (FreeTypeFont *)m_freetype;

	font->putText(img, text, pt, color);
}
