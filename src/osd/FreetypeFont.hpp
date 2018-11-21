//#pragma once

//#include "CELLMath.hpp"
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

//using namespace CELL;

struct  FontVertex
{
    float       x,y,z;
    float       u,v;
    //Rgba4Byte   color;
};


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
    /**
    *   ?��??????????????????��??
    *   ????1024??��??????????��??????(16????)
    */
    unsigned int   x0:10;
    unsigned int   y0:10;
    unsigned int   x1:10;
    unsigned int   y1:10;
    //! ????????x????
    unsigned int   offsetX:8;
    //! ????????y????
    unsigned int   offsetY:8;
    //! ????????��????256??
    unsigned int   texIndex:8;
};

class   FreeTypeFont
{
public:
    FT_Library      _library;
    FT_Face         _face; 
    unsigned        _sysFontTexture;
    Character       _character[1<<16];
    int             _textureWidth;
    int             _textureHeight;
    int             _viewWidth;
    int             _viewHeight;
    int             _yStart;
    int             _xStart;
    int             _fontSize;  
    int             _fontPixelX;
    int             _fontPixelY;
    bool            _isSymbol;
    char            _vertexBuffer[1024 * 64];
public:
    
    FreeTypeFont()
    {
        memset(_character,0,sizeof(_character));

        _library        =   0;
        _face           =   0; 
        _yStart         =   0;
        _xStart         =   0;
        _sysFontTexture =   0;
        _fontSize       =   0;
        _fontPixelX     =   0;
        _fontPixelY     =   0;
        _isSymbol       =   false;

        FT_Error    error = FT_Init_FreeType( (FT_Library*)&_library );
        if(error  != FT_Err_Ok)
        {
        	printf("FT_Init_FreeType Error !!! %d\n",error);
        }
    }

    ~FreeTypeFont()
    {
        destroy();
    }

    void    destroy()
    {
        glDeleteTextures(1,&_sysFontTexture);
        FT_Done_Face(FT_Face(_face));
        FT_Done_FreeType(_library);
        _xStart     =   0;
        _yStart     =   0;
        _face       =   0;
        _library    =   0;
        memset(_character,0,sizeof(_character));
    }
   

    unsigned    getTextureHandle()
    {
        return  _sysFontTexture;
    }

    int     getWidth()
    {
        return  _textureWidth;
    }

    int     getHeight()
    {
        return  _textureHeight;
    }
    void    begin(int width = 0,int height = 0)
    {
    	//glColor3f(1.0,0.0,0.0);
    	#if 0
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0,width,height,0,-1,1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_LIGHTING);
       // glDisable(GL_DEPTH_TEST);
	  #endif
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D,_sysFontTexture);
    }

     void    bindtexture()
    {
      glBindTexture(GL_TEXTURE_2D,_sysFontTexture);
    }

    void    end()
    {
        glDisable(GL_BLEND);
      //  glEnable(GL_DEPTH_TEST);
       // glEnable(GL_LIGHTING);
        glBindTexture(GL_TEXTURE_2D,0);
    }
    /**
    *   ????????
    */
    bool    create(const char* faceName,int fontSize,int w ,int h)
    {
    	printf("\n%s %d: fontSize = %d viewsize(%dx%d) _library(%p)",
    			__func__, __LINE__, fontSize,w,h, _library);
        _textureWidth   =   fontSize*32;
        _textureHeight  =   fontSize*16;
        _viewWidth = w;
        _viewHeight = h;
        /**
        *   ??????????��
        */
        _fontSize   =   fontSize;
        /**
        *   ??????????????????
        *   ????��???
        */
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
        
        /**
        *   ??????????????
        */

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
        /**
        *   ???????
        */
        if (_sysFontTexture != 0)
        {
            glDeleteTextures(1,&_sysFontTexture);
        }

        glGenTextures(1,&_sysFontTexture);
        /**
        *   ??????????id,????��?(????)
        */
        glBindTexture( GL_TEXTURE_2D, _sysFontTexture );
        /**
        *   ??????????,??��?????????????????????????????????? 
        */
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	#if 0
	GLint iWidth, iHeight, iComponents;
	GLbyte *pBytes;
	GLenum eFormat;
	 pBytes = gltReadTGABits("1.tga",&iWidth, &iHeight,
                              &iComponents, &eFormat);
	  glTexImage2D( 
            GL_TEXTURE_2D,      //! ?????????
            0,                  //! ????????????????????mipmap,??lod,???????��???????????y?��??????
            iComponents,           //! ?????????��???
            _textureWidth,
            _textureHeight,
            0,                  //! ?????
            GL_RED,           //! ????????bmp?��?windows,???????�՛�???????bgr???
            GL_UNSIGNED_BYTE,   //! ?????8bit???
           pBytes
            );
	  glGenerateMipmap(GL_TEXTURE_2D);
	   free(pBytes);
	#else

        glTexImage2D( 
            GL_TEXTURE_2D,      //! ?????????
            0,                  //! ????????????????????mipmap,??lod,???????��???????????y?��??????
            GL_ALPHA,           //! ?????????��???
            _textureWidth,
            _textureHeight,
            0,                  //! ?????
            GL_ALPHA,           //! ????????bmp?��?windows,???????�՛�???????bgr???
            GL_UNSIGNED_BYTE,   //! ?????8bit???
            0
            );
	#endif
        return  true;
    }

    
    Character getCharacter( int ch )
    {
        if (_character[ch].x0 == 0 &&
            _character[ch].x0 == 0 &&
            _character[ch].x1 == 0 &&
            _character[ch].y1 == 0
            )
        {

            glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

		//printf("_++++++_fontPixelX = %d  _fontSize = %d  _xStart = %d _textureWidth = %d\n",_fontPixelX,_fontSize,_xStart,_textureWidth);
            if (_xStart + std::max(_fontPixelX,_fontSize) > _textureWidth)
            {
                /**
                *   ��?????,??????
                */
                _xStart =   0;
                /**
                *   y???��???????
                */
                _yStart +=  std::max(_fontPixelY,_fontSize);
            }
            FT_Load_Glyph((FT_Face) _face, FT_Get_Char_Index( (FT_Face)_face, ch ), FT_LOAD_DEFAULT );
            FT_Glyph glyph;
            FT_Glyph glyphSrc;
            FT_Get_Glyph( FT_Face(_face)->glyph, &glyphSrc );
			
            glyph   =   glyphSrc;

            /**
            *   ?????????��?????????��?????????
            *   ????????��????????????ft_render_mode_mono
            *   ???????????????12????????????ft_render_mode_normal??
            */
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

            /**
            *   ????????????��???????
            */
            if (pBitMap->width == 0 || pBitMap->rows == 0)
            {
                char    mem[1024 * 32];
                memset(mem,0,sizeof(mem));

                _character[ch].x0       =   _xStart;
                _character[ch].y0       =   _yStart;
                _character[ch].x1       =   _xStart + _fontSize/2;
                _character[ch].y1       =   _yStart + _fontSize - 1;
                _character[ch].offsetY  =   _fontSize - 1;
                _character[ch].offsetX  =   0;

                

                glBindTexture(GL_TEXTURE_2D,_sysFontTexture);

                glTexSubImage2D (
                    GL_TEXTURE_2D,
                    0,
                    _xStart,
                    _yStart,
                    _fontSize/2,
                    _fontSize,
                    GL_ALPHA,
                    GL_UNSIGNED_BYTE,
                    mem
                    );
                _xStart +=  _fontSize/2;


            }
            else
            {
                glBindTexture(GL_TEXTURE_2D,_sysFontTexture);

                _character[ch].x0       =   _xStart;
                _character[ch].y0       =   _yStart;
                _character[ch].x1       =   _xStart + pBitMap->width;
                _character[ch].y1       =   _yStart + pBitMap->rows;

                _character[ch].offsetY  =   std::max(bitmap_glyph->top,0);
                _character[ch].offsetX  =   std::max(bitmap_glyph->left,0);
                //printf("\n%s: %d, %d", __func__, _character[ch].offsetY, bitmap_glyph->top);

                glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
                glTexSubImage2D (
                    GL_TEXTURE_2D,
                    0,
                    _xStart,
                    _yStart,
                    std::max<unsigned int>(1,pBitMap->width),
                    std::max<unsigned int>(1,pBitMap->rows),
                    GL_ALPHA,
                    GL_UNSIGNED_BYTE,
                    pBitMap->buffer
                    );
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
	inline float norx(int x)
	{
		int c = _viewWidth>>1;
		return ((float)x-c)/c;
	}
	inline float nory(int y)
	{
		int c = _viewHeight>>1;
		return (((float)c - y)/c);
	}
    //float  drawText(int x,int y,int z,const wchar_t* text, size_t length, const float *fcolor, FontVertex** vertexs = NULL,size_t*vertexLength = NULL)
    int putText(const cv::Size& viewSize, const wchar_t* text, const cv::Point& pt, const cv::Scalar& color)
    {
    	//Rgba color;
        float           texWidth    =   float(_textureWidth);
        float           texHeight   =   float(_textureHeight);
        float           xStart      =   float(pt.x);
        float           yStart      =   float(pt.y + _fontSize);
        //float           zStart      =   float(z);
        unsigned        index       =   0;
        unsigned        size        =   wcslen(text);
        //float          vSize(0,0);
        FontVertex*     vertex      =   (FontVertex*)_vertexBuffer;

        _viewWidth = viewSize.width;
        _viewHeight = viewSize.height;

        //printf("\n%s %d: (%d %d %p) size = %d", __func__, __LINE__, x, y, fcolor, size);

        if (size == 0)
        {
            return  -1;
        }
        for (unsigned i = 0 ;i <  size; ++ i )
        {
            //printf("\n%s %d: i = %d text[i] = %d", __func__, __LINE__,i, text[i]);
            Character   ch          =   getCharacter(text[i]);
            //printf("\n%s %d: text[i] = %d(%d,%d)(%d,%d)(%d,%d)", __func__, __LINE__,text[i], ch.x0, ch.y0, ch.x1, ch.y1, ch.offsetX, ch.offsetY);

            float       h           =   float(ch.y1 - ch.y0);
            float       w           =   float(ch.x1 - ch.x0);
            float       offsetY     =   (float(h) - float(ch.offsetY));
            /**
            *   ???????
            */
            vertex[index + 0].x       =   norx(xStart);
            vertex[index + 0].y       =   nory(yStart - h + offsetY);
            vertex[index + 0].u       =   ch.x0/texWidth;
            vertex[index + 0].v       =   ch.y0/texHeight;
            /**
            *   ???????
            */
            vertex[index + 1].x       =   norx(xStart + w);
            vertex[index + 1].y       =   nory(yStart - h + offsetY);
            vertex[index + 1].u       =   ch.x1/texWidth;
            vertex[index + 1].v       =   ch.y0/texHeight;
            /**
            *   ???????
            */
            vertex[index + 2].x       =   norx(xStart + w);
            vertex[index + 2].y       =   nory(yStart  + offsetY);
            vertex[index + 2].u       =   ch.x1/texWidth;
            vertex[index + 2].v       =   ch.y1/texHeight;
            /**
            *   ???????
            */
            vertex[index + 3].x       =   norx(xStart);
            vertex[index + 3].y       =   nory(yStart - h + offsetY);
            vertex[index + 3].u       =   ch.x0/texWidth;
            vertex[index + 3].v       =   ch.y0/texHeight;
            /**
            *   ???????
            */
            vertex[index + 4].x       =   norx(xStart + w);
            vertex[index + 4].y       =   nory(yStart  + offsetY);
            vertex[index + 4].u       =   ch.x1/texWidth;
            vertex[index + 4].v       =   ch.y1/texHeight;
            /**
            *   ???????
            */
            vertex[index + 5].x       =   norx(xStart);
            vertex[index + 5].y       =   nory(yStart  + offsetY);
            vertex[index + 5].u       =   ch.x0/texWidth;
            vertex[index + 5].v       =   ch.y1/texHeight;

            index   +=  6;
            xStart  +=  w + (ch.offsetX + 1);

            //vSize.x +=  w + (ch.offsetX + 1);
            //vSize.y =   tmax<float>(h + offsetY,vSize.y);

        }
        //printf("\n%s %d: (%d %d %p) size = %d", __func__, __LINE__, x, y, fcolor, size);


#if 0
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        {
            glVertexPointer(3,GL_FLOAT,         sizeof(FontVertex),     vertex);
            glTexCoordPointer(2,GL_FLOAT,       sizeof(FontVertex),     &vertex[0].u);
            glColorPointer(4,GL_UNSIGNED_BYTE,  sizeof(FontVertex),     &vertex[0].color);
            glDrawArrays(GL_TRIANGLES,0,index);
        }
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
#else
	//printf("the index=%d\n",index);
	GLBatch	squareBatch;
	squareBatch.Begin(GL_TRIANGLES, index,1);
	//float norm=512.0;
	for(int k=0;k<index;k++)
		{
			 squareBatch.MultiTexCoord2f(0, vertex[k].u,vertex[k].v);
			 squareBatch.Color4f(color.val[0]*0.003921569, color.val[1]*0.003921569, color.val[2]*0.003921569, color.val[3]*0.003921569);
			 squareBatch.Vertex3f(vertex[k].x, vertex[k].y, 0);
			//printf("the size=%d  index=%d (%f,%f)   (%f    %f     %f)\n",size,index,vertex[k].u,vertex[k].v,vertex[k].x,vertex[k].y,vertex[k].z);
	        //printf("\n%s %d: %d %d (%f %f %f %f %f)", __func__, __LINE__,size,index,vertex[k].u,vertex[k].v,vertex[k].x,vertex[k].y,vertex[k].z);
		}
	squareBatch.End();
	squareBatch.Draw();

#endif

#if 0

	GLBatch				ceilingBatch;
	GLfloat zp=0;
	float a=1;
       ceilingBatch.Begin(GL_TRIANGLE_STRIP, 4, 1);

        ceilingBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        ceilingBatch.Vertex3f(-a, a, zp);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        ceilingBatch.Vertex3f(a, a, zp);
        
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        ceilingBatch.Vertex3f(-a, -a, zp);

        ceilingBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        ceilingBatch.Vertex3f(a, -a, zp);

        ceilingBatch.End();

           //  glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_FLOOR]);
     //   freetype->bindtexture();
        ceilingBatch.Draw();


#endif
        return  0;
    }
};
