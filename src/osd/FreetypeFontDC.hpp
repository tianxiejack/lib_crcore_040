//#pragma once
#include <opencv2/opencv.hpp>
#include "freetype/ftglyph.h"
//#include "ftglyph.h"
//#pragma comment(lib,"freetype.lib")
#include <GLBatch.h>
#include <wchar.h>
#include FT_GLYPH_H
#include FT_TRUETYPE_TABLES_H
#include FT_BITMAP_H
#include FT_WINFONTS_H

namespace cr_osd
{

class   Character
{
public:
    Character()
    {
        x0          =   0;
        y0          =   0;
        x1          =   0;
        y1          =   0;
        offsetX     =   0;
        offsetY     =   0;
    }
    unsigned int   x0:10;
    unsigned int   y0:10;
    unsigned int   x1:10;
    unsigned int   y1:10;
    unsigned int   offsetX:8;
    unsigned int   offsetY:8;
    unsigned int   texIndex:8;
};

class   FreeTypeFont
{
public:
    FT_Library      _library;
    FT_Face         _face; 
    cv::Mat         _sysFontImg;
    cv::Mat         _sysZeroImg;
    Character       _character[1<<16];

    int             _textureWidth;
    int             _textureHeight;
    int             _yStart;
    int             _xStart;
    int             _fontSize;  
    int             _fontPixelX;
    int             _fontPixelY;
    bool            _isSymbol;
public:
    
    FreeTypeFont()
    {
        memset(_character,0,sizeof(_character));

        _library        =   0;
        _face           =   0; 
        _yStart         =   0;
        _xStart         =   0;
        _fontSize       =   0;
        _fontPixelX     =   0;
        _fontPixelY     =   0;
        _isSymbol       =   false;

        FT_Init_FreeType( (FT_Library*)&_library );
    }

    ~FreeTypeFont()
    {
        destroy();
    }

    void    destroy()
    {
        FT_Done_Face(FT_Face(_face));
        _xStart     =   0;
        _yStart     =   0;
        _face       =   0;
        memset(_character,0,sizeof(_character));
    }

    int     getWidth()
    {
        return  _textureWidth;
    }

    int     getHeight()
    {
        return  _textureHeight;
    }

    bool    create(const char* faceName,int fontSize)
    {
        _textureWidth   =   fontSize*32;
        _textureHeight  =   fontSize*16;
        _fontSize   =   fontSize;
        if (_face)
        {
            FT_Done_Face(FT_Face(_face));
            _xStart         =   0;
            _yStart         =   0;
            memset(_character,0,sizeof(_character));
        }
        
        
        FT_Face     ftFace  =   0;
        FT_Error    error;
        if((error   =   FT_New_Face( (FT_Library)_library, faceName, 0, &ftFace )) != FT_Err_Ok)
        {
        	printf("FT_New_Face Error %d\n",error);
        }
        _face   =   ftFace;
       
        if ( error != 0 ) 
        {
            return  false;
        }
        
        if ( ftFace->charmaps != 0 && ftFace->num_charmaps > 0 )
        {
            FT_Select_Charmap( ftFace, ftFace->charmaps[0]->encoding );
        }

        FT_Select_Charmap( ftFace, FT_ENCODING_UNICODE );
        FT_F26Dot6 ftSize = (FT_F26Dot6)(fontSize * (1 << 6));
        
     
        if((error   =   FT_Set_Char_Size(ftFace, ftSize, 0, 0, fontSize)) != FT_Err_Ok)
        {
        	printf("FT_Set_Char_Size Error %d\n",error);
        }
        _sysFontImg = cv::Mat(_textureHeight, _textureWidth, CV_8UC1);
        _sysZeroImg = cv::Mat(_textureHeight, _textureWidth, CV_8UC1);
        memset(_sysFontImg.data, 0, _sysFontImg.cols*_sysFontImg.rows*_sysFontImg.channels());
        memset(_sysZeroImg.data, 0, _sysZeroImg.cols*_sysZeroImg.rows*_sysZeroImg.channels());

        printf("%s %d: fontSize=%d _sysFontImg(%dx%d)\n", __FILE__, __LINE__,
        		fontSize, _sysFontImg.cols,_sysFontImg.rows);

        return  true;
    }

    inline void SubImage2D(cv::Mat dst, cv::Mat src, int xOffset, int yOffset)
    {
    	for(int i=0; i<src.rows; i++){
    		memcpy(dst.data + ((yOffset+i)*dst.cols+xOffset), src.data + i*src.cols, src.cols);
    	}
    }
    
    inline void SubImage2D(cv::Mat dst, cv::Mat src, int dxOffset, int dyOffset, int sxOffset, int syOffset, int width, int height)
    {
    	for(int i=0; i<height; i++){
    		memcpy(dst.data + ((dyOffset+i)*dst.cols+dxOffset), src.data + ((syOffset+i)*src.cols+sxOffset), width);
    	}
    }

    Character getCharacter( int ch )
    {
        if (_character[ch].x0 == 0 &&
            _character[ch].x0 == 0 &&
            _character[ch].x1 == 0 &&
            _character[ch].y1 == 0
            )
        {

            if (_xStart + std::max(_fontPixelX,_fontSize) > _textureWidth)
            {
                _xStart =   0;
                _yStart +=  std::max(_fontPixelY,_fontSize);
            }
            FT_Load_Glyph((FT_Face) _face, FT_Get_Char_Index( (FT_Face)_face, ch ), FT_LOAD_DEFAULT );
            FT_Glyph glyph;
            FT_Glyph glyphSrc;
            FT_Get_Glyph( FT_Face(_face)->glyph, &glyphSrc );
			
            glyph   =   glyphSrc;

            FT_Error    err =   -1;
            if (_fontSize <= 16)
            {
                err =   FT_Glyph_To_Bitmap( &glyph, ft_render_mode_mono, 0, 1 );
            }
            else
            {
                if ( ch >= 0 && ch <=256 && !_isSymbol)
                {

                    err =   FT_Glyph_To_Bitmap( &glyph, ft_render_mode_mono, 0, 1 );
                }
                else
                {
                    err =   FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );
                }
            }
            if(err != 0)
            {
            	printf("%s %d: %c[%d] err = %d", __func__, __LINE__, ch, ch, err);
            }
 
            FT_BitmapGlyph  bitmap_glyph    =   (FT_BitmapGlyph)glyph;
            FT_Bitmap&      bitmap          =   bitmap_glyph->bitmap;
            FT_Bitmap       ftBitmap;

            FT_Bitmap*      pBitMap         =   &bitmap;
           
            FT_Bitmap_New(&ftBitmap);

            if(bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
            {
                if (FT_Bitmap_Convert((FT_Library)_library, &bitmap, &ftBitmap, 1) == 0)
                {
                    /**
                    *   Go through the bitmap and convert all of the nonzero values to 0xFF (white).
                    */
                    for (unsigned char* p = ftBitmap.buffer, * endP = p + ftBitmap.width * ftBitmap.rows; p != endP; ++p)
                        *p ^= -*p ^ *p;
                    pBitMap  =   &ftBitmap;
                }
            }

            if (pBitMap->width == 0 || pBitMap->rows == 0)
            {
                unsigned char    mem[1024 * 32];
                memset(mem,0,sizeof(mem));

                _character[ch].x0       =   _xStart;
                _character[ch].y0       =   _yStart;
                _character[ch].x1       =   _xStart + _fontSize/2;
                _character[ch].y1       =   _yStart + _fontSize - 1;
                _character[ch].offsetY  =   _fontSize - 1;
                _character[ch].offsetX  =   0;
                cv::Mat tmp(_fontSize, _fontSize/2, CV_8UC1, mem);
                SubImage2D(_sysFontImg, tmp, _xStart, _yStart);
                _xStart +=  _fontSize/2;
            }
            else
            {
                _character[ch].x0       =   _xStart;
                _character[ch].y0       =   _yStart;
                _character[ch].x1       =   _xStart + pBitMap->width;
                _character[ch].y1       =   _yStart + pBitMap->rows;
                _character[ch].offsetY  =   std::max(bitmap_glyph->top,0);
                _character[ch].offsetX  =   std::max(bitmap_glyph->left,0);
                cv::Mat tmp(std::max<unsigned int>(1, pBitMap->rows), std::max<unsigned int>(1,pBitMap->width), CV_8UC1, pBitMap->buffer);
                SubImage2D(_sysFontImg, tmp, _xStart, _yStart);
                _xStart     +=  (pBitMap->width + 1);
                _fontPixelY =   std::max<unsigned int>(_fontPixelY,pBitMap->rows);
                _fontPixelX =   std::max<unsigned int>(_fontPixelX,pBitMap->width);
            }

            if (glyph != glyphSrc)
            {
                FT_Done_Glyph(glyph);
            }
            else
            {
                FT_Done_Glyph(glyphSrc);
            }

            FT_Bitmap_Done((FT_Library)_library,&ftBitmap);

        }
        return  _character[ch];
    }

    void    selectCharmap( unsigned charMap )
    {
        FT_Select_Charmap( FT_Face(_face), (FT_Encoding)charMap);
    }

    int  putText(const cv::Mat& img, const wchar_t* text, const cv::Point& pt, const cv::Scalar& color)
    {
        float           xStart      =   float(pt.x);
        float           yStart      =   float(pt.y);
        int        		size        =   wcslen(text);

        if (size == 0)
        {
            return  (-1);
        }
        for (int i = 0 ;i <  size; ++ i )
        {
            Character   ch          =   getCharacter(text[i]);

            int       h           =   (ch.y1 - ch.y0);
            int       w           =   (ch.x1 - ch.x0);
            int       offsetY     =   ((h) - (ch.offsetY));

            if(color.val[3]<= 0.001)
            {
            	SubImage2D(img, _sysZeroImg, xStart, yStart + _fontSize - (ch.offsetY), ch.x0, ch.y0, w, h);
            }
        	else
            {
            	SubImage2D(img, _sysFontImg, xStart, yStart + _fontSize - (ch.offsetY), ch.x0, ch.y0, w, h);
            }
            xStart  +=  w + (ch.offsetX + 1);
        }
        return  size;
    }
};

};//namespace cr_osd
