
#include "Displayer.hpp"
//#include "glosd.hpp"
//#include <GLShaderManager.h>
//#include <gl.h>
#include <cuda.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime_api.h>
#include <X11/Xlib.h>
#include "osa_image_queue.h"
#include "cuda_mem.cpp"
#include "cuda_convert.cuh"

#define RENDMOD_TIME_ON		(0)
#define TAG_VALUE   (0x10001000)

#ifdef _WIN32
#pragma comment (lib, "glew32.lib")
#endif

using namespace cv;
using namespace cr_osa;

static CRender *gThis = NULL;

static GLfloat m_glvVertsDefault[8] = {-1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f};
static GLfloat m_glvTexCoordsDefault[8] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

CRender* CRender::createObject()
{
	if(gThis == NULL)
		gThis = new CRender();
	return gThis;
}
void CRender::destroyObject(CRender* obj)
{
	if(gThis != NULL)
		delete gThis;
	gThis = NULL;
}

CRender::CRender()
:m_mainWinWidth(1920),m_mainWinHeight(1080),m_renderCount(0),
m_bRun(false),m_bFullScreen(false),m_bDC(false),
 m_bUpdateVertex(false), m_timerRun(false),m_tmRender(0ul),m_waitSync(false),
 m_telapse(5.0)
{
	tag = TAG_VALUE;
	gThis = this;
	memset(&m_initPrm, 0, sizeof(m_initPrm));
	memset(m_bufQue, 0, sizeof(m_bufQue));
	memset(m_tmBak, 0, sizeof(m_tmBak));
	memset(m_glProgram, 0, sizeof(m_glProgram));
	for(int chId = 0; chId<DS_CHAN_MAX; chId++){
		m_blendMap[chId] = -1;
		m_maskMap[chId] = -1;
	}
	m_dcColor = cv::Scalar_<float>(255, 255, 255, 255);
	//m_thickness = 2;
}

CRender::~CRender()
{
	//destroy();
	gThis = NULL;
}

int CRender::create()
{
	memset(m_renders, 0, sizeof(m_renders));
	memset(m_curMap, 0, sizeof(m_curMap));
	for(int i=0; i<DS_RENDER_MAX; i++){
		m_renders[i].transform = cv::Matx44f::eye();
	}
	memset(&m_videoSize[0], 0, sizeof(DS_Size)*DS_CHAN_MAX);

	gl_create();

	OSA_mutexCreate(&m_mutex);

	return 0;
}

int CRender::destroy()
{
	int i;
	cudaError_t et;

	stop();

	gl_destroy();

	uninitRender();

	for(int chId=0; chId<m_initPrm.nChannels; chId++)
		image_queue_delete(&m_bufQue[chId]);

	OSA_mutexDelete(&m_mutex);
	for(i=0; i<DS_DC_CNT; i++){
		if(m_imgDC[i].data != NULL)
			cudaFreeHost(m_imgDC[i].data);
		m_imgDC[i].data = NULL;
	}

	return 0;
}

int CRender::initRender(bool updateMap)
{
	int i=0;

	if(updateMap){
		m_renders[i].video_chId    = 0;
	}
	m_renders[i].displayrect.x = 0;
	m_renders[i].displayrect.y = 0;
	m_renders[i].displayrect.width = m_mainWinWidth;
	m_renders[i].displayrect.height = m_mainWinHeight;
	i++;

	if(updateMap){
		m_renders[i].video_chId    = -1;
	}
	m_renders[i].displayrect.x = m_mainWinWidth*2/3;
	m_renders[i].displayrect.y = m_mainWinHeight*2/3;
	m_renders[i].displayrect.width = m_mainWinWidth/3;
	m_renders[i].displayrect.height = m_mainWinHeight/3;
	i++;

	if(updateMap){
		m_renders[i].video_chId    = -1;
	}
	m_renders[i].displayrect.x = m_mainWinWidth*2/3;
	m_renders[i].displayrect.y = m_mainWinHeight*2/3;
	m_renders[i].displayrect.width = m_mainWinWidth/3;
	m_renders[i].displayrect.height = m_mainWinHeight/3;
	i++;

	if(updateMap){
		m_renders[i].video_chId    = -1;
	}
	m_renders[i].displayrect.x = m_mainWinWidth*2/3;
	m_renders[i].displayrect.y = m_mainWinHeight*2/3;
	m_renders[i].displayrect.width = m_mainWinWidth/3;
	m_renders[i].displayrect.height = m_mainWinHeight/3;
	i++;

	m_renderCount = i;

	for(i=0; i<m_renderCount; i++){
		m_curMap[i] = m_renders[i].video_chId;
	}

	return 0;
}

void CRender::uninitRender()
{
	m_renderCount = 0;
}

void CRender::_display(void)
{
	OSA_assert(gThis->tag == TAG_VALUE);
	gThis->gl_display();
}

void CRender::_timeFunc(int value)
{
	if(!gThis->m_bRun){
		gThis->m_timerRun = false;
		return ;
	}
	gThis->m_timerRun = true;;
	glutTimerFunc((gThis->m_interval/1000000), _timeFunc, value);
#if RENDMOD_TIME_ON
	//gThis->_display();
	glutPostRedisplay();
#endif
	if(gThis->m_initPrm.timerfunc != NULL)
		gThis->m_initPrm.timerfunc(value);
}

void CRender::disp_fps(){
    static GLint frames = 0;
    static GLint t0 = 0;
    static char  fps_str[20] = {'\0'};
    GLint t = glutGet(GLUT_ELAPSED_TIME);
    sprintf(fps_str, "%6.1f FPS\n", 0.0f);
    if (t - t0 >= 200) {
        GLfloat seconds = (t - t0) / 1000.0;
        GLfloat fps = frames / seconds;
        sprintf(fps_str, "%6.1f FPS\n", fps);
        printf("%6.1f FPS\n", fps);
        t0 = t;
        frames = 0;
    }
    glColor3f(0.0, 0.0, 1.0);
    glRasterPos2f(0.5, 0.5);
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char *)fps_str);
    frames++;
}

void CRender::_reshape(int width, int height)
{
	assert(gThis != NULL);
	glViewport(0, 0, width, height);
	gThis->m_mainWinWidth = width;
	gThis->m_mainWinHeight = height;
	gThis->initRender(false);
	gThis->gl_updateVertex();
	gThis->gl_resize();
}
void CRender::gl_resize()
{
	//glGenBuffers(1, pixBuffObjs);
	//glBindBuffer(GL_PIXEL_PACK_BUFFER, pixBuffObjs[0]);
	//glBufferData(GL_PIXEL_PACK_BUFFER,
	//	m_mainWinWidth*m_mainWinHeight*3,
	//	NULL, GL_DYNAMIC_COPY);
	//glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void CRender::_close(void)
{
	if(gThis->m_initPrm.closefunc != NULL)
		gThis->m_initPrm.closefunc();
}

int CRender::init(DS_InitPrm *pPrm)
{
	OSA_assert(this == gThis);
	if(pPrm != NULL)
		memcpy(&m_initPrm, pPrm, sizeof(DS_InitPrm));

	if(m_initPrm.winWidth > 0)
		m_mainWinWidth = m_initPrm.winWidth;
	if(m_initPrm.winHeight > 0)
		m_mainWinHeight = m_initPrm.winHeight;
	if(m_initPrm.disFPS<=0)
		m_initPrm.disFPS = 25;
	if(m_initPrm.nQueueSize < 2)
		m_initPrm.nQueueSize = 2;
	m_interval = (1000000000ul)/(uint64)m_initPrm.disFPS;
	//memcpy(m_videoSize, m_initPrm.channelsSize, sizeof(m_videoSize));

    //glutInitWindowPosition(m_initPrm.winPosX, m_initPrm.winPosY);
    glutInitWindowSize(m_mainWinWidth, m_mainWinHeight);
    int winId = glutCreateWindow("DSS");
    OSA_assert(winId > 0);
    //int subId = glutCreateSubWindow( winId, 0, 0, m_mainWinWidth, m_mainWinHeight );
    //OSA_assert(subId > 0);
    //glutSetWindow(subId);
	glutDisplayFunc(_display);
	if(m_initPrm.idlefunc != NULL)
		glutIdleFunc(m_initPrm.idlefunc);
	glutReshapeFunc(_reshape);

	if(m_initPrm.keyboardfunc != NULL)
		glutKeyboardFunc(m_initPrm.keyboardfunc);
	if(m_initPrm.keySpecialfunc != NULL)
		glutSpecialFunc(m_initPrm.keySpecialfunc);

	//mouse event:
	if(m_initPrm.mousefunc != NULL)
		glutMouseFunc(m_initPrm.mousefunc);//GLUT_LEFT_BUTTON GLUT_MIDDLE_BUTTON GLUT_RIGHT_BUTTON; GLUT_DOWN GLUT_UP
	//glutMotionFunc();//button down
	//glutPassiveMotionFunc();//button up
	//glutEntryFunc();//state GLUT_LEFT, GLUT_ENTERED

	if(m_initPrm.visibilityfunc != NULL)
		glutVisibilityFunc(m_initPrm.visibilityfunc);

	//setFullScreen(true);
	if(m_initPrm.bFullScreen){
		glutFullScreen();
		m_bFullScreen = true;
		setFullScreen(m_bFullScreen);
	}
	glutCloseFunc(_close);

	cv::Size screenSize(m_mainWinWidth, m_mainWinHeight);// = getScreenSize();
	for(int i=0; i<DS_DC_CNT; i++){
		unsigned char* mem = NULL;
		cudaError_t et;
		int nChannel = 1;
		et = cudaHostAlloc((void**)&mem, screenSize.width*screenSize.height*nChannel, cudaHostAllocDefault);
		//et=cudaMallocManaged ((void**)&mem, screenSize.width*screenSize.height*4, cudaMemAttachGlobal);
		OSA_assert(et == cudaSuccess);
		memset(mem, 0, screenSize.width*screenSize.height*nChannel);
		if(nChannel == 1)
			m_imgDC[i] = cv::Mat(screenSize.height, screenSize.width, CV_8UC1, mem);
		else
			m_imgDC[i] = cv::Mat(screenSize.height, screenSize.width, CV_8UC4, mem);
	}

	initRender();
	gl_updateVertex();
	gl_init();
	for(int chId=0; chId<m_initPrm.nChannels; chId++){
		image_queue_create(&m_bufQue[chId], m_initPrm.nQueueSize,
				m_initPrm.channelsSize[chId].w*m_initPrm.channelsSize[chId].h*m_initPrm.channelsSize[chId].c,
				m_initPrm.memType);
		for(int i=0; i<m_bufQue[chId].numBuf; i++){
			m_bufQue[chId].bufInfo[i].width = m_initPrm.channelsSize[chId].w;
			m_bufQue[chId].bufInfo[i].height = m_initPrm.channelsSize[chId].h;
			m_bufQue[chId].bufInfo[i].channels = m_initPrm.channelsSize[chId].c;
		}
	}

	return 0;
}

int CRender::get_videoSize(int chId, DS_Size &size)
{
	if(chId < 0 || chId >= DS_CHAN_MAX)
		return -1;
	size = m_videoSize[chId];

	return 0;
}
int CRender::dynamic_config(DS_CFG type, int iPrm, void* pPrm)
{
	int iRet = 0;
	int chId;
	bool bEnable;
	DS_Rect *rc;

	if(type == DS_CFG_ChId){
		if(iPrm >= m_renderCount || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		chId = *(int*)pPrm;

		OSA_mutexLock(&m_mutex);
		int curId = m_renders[iPrm].video_chId;
		if(curId >= 0){
			int count = OSA_bufGetFullCount(&m_bufQue[curId]);
			while(count>0){
				image_queue_switchEmpty(&m_bufQue[curId]);
				count = OSA_bufGetFullCount(&m_bufQue[curId]);
			}
		}
		m_renders[iPrm].video_chId = chId;
		//if(winId == 0)
		{
			int toId = chId;
			if(toId >= 0 && toId<DS_CHAN_MAX){
				int count = OSA_bufGetFullCount(&m_bufQue[toId]);
				for(int i=0; i<count; i++){
					image_queue_switchEmpty(&m_bufQue[toId]);
				}
				OSA_printf("%s %d: switchEmpty %d frames", __func__, __LINE__, count);
			}
		}
		m_telapse = 5.0f;
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == DS_CFG_BlendChId){
		if(iPrm >= DS_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		m_blendMap[iPrm] = *(int*)pPrm;
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == DS_CFG_MaskChId){
		if(iPrm >= DS_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		OSA_mutexLock(&m_mutex);
		m_maskMap[iPrm] = *(int*)pPrm;
		OSA_mutexUnlock(&m_mutex);
	}

	if(type == DS_CFG_CropEnable){
		if(iPrm >= m_renderCount || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		bEnable = *(bool*)pPrm;
		m_renders[iPrm].bCrop = bEnable;
	}

	if(type == DS_CFG_CropRect){
		if(iPrm >= m_renderCount || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		rc = (DS_Rect*)pPrm;
		m_renders[iPrm].croprect = *rc;
		gl_updateVertex();
	}

	if(type == DS_CFG_VideoTransMat){
		if(iPrm >= DS_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		memcpy(m_glmat44fTrans[iPrm].val, pPrm, sizeof(float)*16);
	}

	if(type == DS_CFG_ViewTransMat){
		if(iPrm >= m_renderCount || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		memcpy(m_renders[iPrm].transform.val , pPrm, sizeof(float)*16);
		gl_updateVertex();
	}


	if(type == DS_CFG_ViewPos){
		if(iPrm >= m_renderCount || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		rc = (DS_Rect*)pPrm;
		m_renders[iPrm].displayrect = *rc;
	}

	if(type == DS_CFG_BlendTransMat){
		if(iPrm >= DS_CHAN_MAX*DS_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		memcpy(m_glmat44fBlend[iPrm].val, pPrm, sizeof(float)*16);
	}

	if(type == DS_CFG_BlendPrm){
		if(iPrm >= DS_CHAN_MAX*DS_CHAN_MAX || iPrm < 0)
			return -1;
		if(pPrm == NULL)
			return -2;
		memcpy(&m_glBlendPrm[iPrm], pPrm, sizeof(DS_BlendPrm));
	}

	return iRet;
}

GLuint CRender::async_display(int chId, int width, int height, int channels)
{
	assert(chId>=0 && chId<DS_CHAN_MAX);

	if(m_videoSize[chId].w  == width  && m_videoSize[chId].h == height && m_videoSize[chId].c == channels )
		return buffId_input[chId];

	//OSA_printf("%s: w = %d h = %d (%dx%d) cur %d\n", __FUNCTION__, width, height, m_videoSize[chId].w, m_videoSize[chId].h, buffId_input[chId]);

	if(m_videoSize[chId].w != 0){
		glDeleteBuffers(1, &buffId_input[chId]);
		glGenBuffers(1, &buffId_input[chId]);
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffId_input[chId]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width*height*channels, NULL, GL_DYNAMIC_COPY);//GL_STATIC_DRAW);//GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	m_videoSize[chId].w = width;
	m_videoSize[chId].h = height;
	m_videoSize[chId].c = channels;

	//OSA_printf("%s: w = %d h = %d (%dx%d) out %d\n", __FUNCTION__, width, height, m_videoSize[chId].w, m_videoSize[chId].h, buffId_input[chId]);
	return buffId_input[chId];
}

void CRender::run()
{
	m_bRun = true;
	m_timerRun = false;
	//glutSetOption();
	//glutMainLoopEvent();
	//glutMainLoop();
#if (!RENDMOD_TIME_ON)
	if(m_initPrm.timerfunc != NULL)
		glutTimerFunc(0, _timeFunc, m_initPrm.timerfunc_value);
#endif

	glutPostRedisplay();
}

void CRender::stop()
{
	m_bRun = false;
}

int CRender::setFullScreen(bool bFull)
{
	if(bFull)
		glutFullScreen();
	else{

	}
	m_bFullScreen = bFull;

	return 0;
}

/***********************************************************************/

static int getDisplayResolution(const char* display_name,uint32_t &width, uint32_t &height)
{
    int screen_num;
    Display * x_display = XOpenDisplay(display_name);
    if (NULL == x_display)
    {
        return  -1;
    }

    screen_num = DefaultScreen(x_display);
    width = DisplayWidth(x_display, screen_num);
    height = DisplayHeight(x_display, screen_num);

    XCloseDisplay(x_display);
    x_display = NULL;

    return 0;
}

int CRender::gl_create()
{
	char strParams[][32] = {"DS_RENDER", "-display", ":0"};
	char *argv[3];
	int argc = 3;
	for(int i=0; i<argc; i++)
		argv[i] = strParams[i];
	uint32_t screenWidth = 0, screenHeight = 0;
	if(getDisplayResolution(strParams[2], screenWidth, screenHeight) == 0)
	{
		m_mainWinWidth = screenWidth;
		m_mainWinHeight = screenHeight;
	}
	OSA_printf("screen resolution: %d x %d", screenWidth, screenHeight);
   // GLUT init
    glutInit(&argc, argv);
	//Double, Use glutSwapBuffers() to show
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
	//Single, Use glFlush() to show
	//glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );

	// Blue background
	glClearColor(0.0f, 0.0f, 0.01f, 0.0f );

	return 0;
}
void CRender::gl_destroy()
{
	gl_uninit();
	glutLeaveMainLoop();
	for(int chId=0; chId<DS_CHAN_MAX; chId++)
		cudaResource_UnregisterBuffer(chId);
}

#define TEXTURE_ROTATE (0)
#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4
void CRender::gl_init()
{
	int i;

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
		return;
	}

	gl_loadProgram();
	//glShaderManager.InitializeStockShaders();

	glGenBuffers(DS_CHAN_MAX, buffId_input);
	glGenTextures(DS_CHAN_MAX, textureId_input);
	for(i=0; i<DS_CHAN_MAX; i++)
	{
		glBindTexture(GL_TEXTURE_2D, textureId_input[i]);
		assert(glIsTexture(textureId_input[i]));
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);//GL_NEAREST);//GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);//GL_NEAREST);//GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//GL_CLAMP);//GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);//GL_CLAMP);//GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, 1920, 1080, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, NULL);
	}

	glGenBuffers(DS_DC_CNT, buffId_dc);
	glGenTextures(DS_DC_CNT, textureId_dc);
	for(i=0; i<DS_DC_CNT; i++)
	{
		glBindTexture(GL_TEXTURE_2D, textureId_dc[i]);
		assert(glIsTexture(textureId_dc[i]));
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffId_dc[i]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, m_imgDC[i].cols*m_imgDC[i].rows*m_imgDC[i].channels(), m_imgDC[i].data, GL_DYNAMIC_DRAW);//GL_DYNAMIC_COPY);//GL_STATIC_DRAW);//GL_DYNAMIC_DRAW);
		if(m_imgDC[i].channels() == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_imgDC[i].cols, m_imgDC[i].rows,0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
		else{
			glTexImage2D(GL_TEXTURE_2D, 0, m_imgDC[i].channels(), m_imgDC[i].cols, m_imgDC[i].rows,0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_imgOsd[i].cols, m_imgOsd[i].rows,0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		}
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}

	for(i=0; i<DS_CHAN_MAX; i++){
		m_glmat44fTrans[i] = cv::Matx44f::eye();
	}
	for(i=0; i<DS_CHAN_MAX*DS_CHAN_MAX; i++){
		m_glmat44fBlend[i] = cv::Matx44f::eye();
		m_glBlendPrm[i].fAlpha = 0.5f;
		m_glBlendPrm[i].thr0Min = 0;
		m_glBlendPrm[i].thr0Max = 0;
		m_glBlendPrm[i].thr1Min = 0;
		m_glBlendPrm[i].thr1Max = 0;
	}

	//glEnable(GL_LINE_SMOOTH);
	//glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	glClear(GL_COLOR_BUFFER_BIT);

	if(m_initPrm.initfunc != NULL)
		m_initPrm.initfunc();
}

void CRender::gl_uninit()
{
	gl_unloadProgram();

	glDeleteTextures(DS_CHAN_MAX, textureId_input);
	glDeleteBuffers(DS_CHAN_MAX, buffId_input);
	glDeleteTextures(DS_DC_CNT, textureId_dc);
	glDeleteBuffers(DS_DC_CNT, buffId_dc);

}

int CRender::gl_updateVertex(void)
{
	int iRet = 0;
	int winId, chId, i;
	DS_Rect rc;
	//GLfloat ftmp;

	for(winId=0; winId<m_renderCount; winId++)
	{
		m_glvVerts[winId][0] = -1.0f; m_glvVerts[winId][1] 	= 1.0f;
		m_glvVerts[winId][2] = 1.0f;  m_glvVerts[winId][3] 	= 1.0f;
		m_glvVerts[winId][4] = -1.0f; m_glvVerts[winId][5] 	= -1.0f;
		m_glvVerts[winId][6] = 1.0f;  m_glvVerts[winId][7] 	= -1.0f;

		/*for(i=0; i<4; i++){
			float x = m_glvVerts[winId][i*2+0];
			float y = m_glvVerts[winId][i*2+1];
			//m_glvVerts[winId][i*2+0] = m_renders[winId].transform[0][0] * x + m_renders[winId].transform[0][1] * y + m_renders[winId].transform[0][3];
			//m_glvVerts[winId][i*2+1] = m_renders[winId].transform[1][0] * x + m_renders[winId].transform[1][1] * y + m_renders[winId].transform[1][3];
			m_glvVerts[winId][i*2+0] = m_renders[winId].transform.val[0*4+0] * x + m_renders[winId].transform.val[0*4+1] * y + m_renders[winId].transform.val[0*4+3];
			m_glvVerts[winId][i*2+1] = m_renders[winId].transform.val[1*4+0] * x + m_renders[winId].transform.val[1*4+1] * y + m_renders[winId].transform.val[1*4+3];
		}*/

		m_glvTexCoords[winId][0] = 0.0; m_glvTexCoords[winId][1] = 0.0;
		m_glvTexCoords[winId][2] = 1.0; m_glvTexCoords[winId][3] = 0.0;
		m_glvTexCoords[winId][4] = 0.0; m_glvTexCoords[winId][5] = 1.0;
		m_glvTexCoords[winId][6] = 1.0; m_glvTexCoords[winId][7] = 1.0;
	}

	for(winId=0; winId<m_renderCount; winId++)
	{
		chId = m_renders[winId].video_chId;
		if(chId < 0 || chId >= DS_CHAN_MAX)
			continue;
		rc = m_renders[winId].croprect;
		if(m_videoSize[chId].w<=0 || m_videoSize[chId].h<=0){
			iRet ++;
			continue;
		}
		if(rc.width == 0 || rc.height == 0){
			continue;
		}
		m_glvTexCoords[winId][0] = (GLfloat)rc.x/m_videoSize[chId].w; 
		m_glvTexCoords[winId][1] = (GLfloat)rc.y/m_videoSize[chId].h;

		m_glvTexCoords[winId][2] = (GLfloat)(rc.x+rc.width)/m_videoSize[chId].w;
		m_glvTexCoords[winId][3] = (GLfloat)rc.y/m_videoSize[chId].h;

		m_glvTexCoords[winId][4] = (GLfloat)rc.x/m_videoSize[chId].w;
		m_glvTexCoords[winId][5] = (GLfloat)(rc.y+rc.height)/m_videoSize[chId].h;

		m_glvTexCoords[winId][6] = (GLfloat)(rc.x+rc.width)/m_videoSize[chId].w;
		m_glvTexCoords[winId][7] = (GLfloat)(rc.y+rc.height)/m_videoSize[chId].h;
	}

	return iRet;
}

void CRender::gl_updateTexDC()
{
	if(m_bDC)
	{
		for(int i=0; i<DS_DC_CNT; i++)
		{
			glBindTexture(GL_TEXTURE_2D, textureId_dc[i]);
#if 1
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffId_dc[i]);
			if(0){
				glBufferData(GL_PIXEL_UNPACK_BUFFER, m_imgDC[i].cols*m_imgDC[i].rows*m_imgDC[i].channels(), m_imgDC[i].data, GL_DYNAMIC_DRAW);//GL_DYNAMIC_COPY);//GL_DYNAMIC_COPY);//GL_STATIC_DRAW);//GL_DYNAMIC_DRAW);
			}else{
				unsigned int byteCount = m_imgDC[i].cols*m_imgDC[i].rows*m_imgDC[i].channels();
				unsigned char *dev_pbo = NULL;
				size_t tmpSize;
				cudaResource_RegisterBuffer(DS_CHAN_MAX+i, buffId_dc[i], byteCount);
				cudaResource_mapBuffer(DS_CHAN_MAX+i, (void **)&dev_pbo, &tmpSize);
				assert(tmpSize == byteCount);
				cudaMemcpy(dev_pbo, m_imgDC[i].data, byteCount, cudaMemcpyHostToDevice);
				//cudaDeviceSynchronize();
				cudaResource_unmapBuffer(DS_CHAN_MAX+i);
				cudaResource_UnregisterBuffer(DS_CHAN_MAX+i);
			}
			if(m_imgDC[i].channels() == 1)
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_imgDC[i].cols, m_imgDC[i].rows, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
			else
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_imgDC[i].cols, m_imgDC[i].rows, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#else
			if(m_imgDC[i].channels() == 1)
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_imgDC[i].cols, m_imgDC[i].rows, GL_RED, GL_UNSIGNED_BYTE, m_imgDC[i].data);
			else
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_imgDC[i].cols, m_imgDC[i].rows, GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_imgDC[i].data);
#endif
		}
	}
}
void CRender::gl_updateTexVideo()
{
	static int bCreate[DS_CHAN_MAX] = {0,0,0,0};
	static unsigned long nCnt[DS_CHAN_MAX] = {0,0,0,0};
	for(int chId = 0; chId < DS_CHAN_MAX; chId++)
	{
		bool bDevMem = false;
		OSA_BufInfo* info = NULL;
		Mat img;
		int count = OSA_bufGetFullCount(&m_bufQue[chId]);
		nCnt[chId] ++;
		if(nCnt[chId] > 300 && count>1){
			nCnt[chId] = 0;
			m_timerRun = false;
		}
		if(!m_timerRun && count>1){
			OSA_printf("[%d]%s: ch%d queue count = %d, sync!!!\n",
									OSA_getCurTimeInMsec(), __func__, chId, count);
			while(count>1){
				//image_queue_switchEmpty(&m_bufQue[chId]);
				info = image_queue_getFull(&m_bufQue[chId]);
				OSA_assert(info != NULL);
				image_queue_putEmpty(&m_bufQue[chId], info);
				count = OSA_bufGetFullCount(&m_bufQue[chId]);
			}
		}

		info = image_queue_getFull(&m_bufQue[chId]);
		if(info != NULL)
		{
			GLuint pbo = 0;
			bDevMem = (info->memtype == memtype_cudev || info->memtype == memtype_cumap);
			void *data = (bDevMem) ? info->physAddr : info->virtAddr;
			if(info->channels == 1){
				img = cv::Mat(info->height, info->width, CV_8UC1, data);
			}else{
				img = cv::Mat(info->height, info->width, CV_8UC3, data);
			}
			m_tmBak[chId] = info->timestampCap;

			OSA_assert(img.cols > 0 && img.rows > 0);
			if(bDevMem)
			{
				unsigned int byteCount = img.cols*img.rows*img.channels();
				unsigned char *dev_pbo = NULL;
				size_t tmpSize;
				pbo = async_display(chId, img.cols, img.rows, img.channels());
				OSA_assert(pbo == buffId_input[chId]);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
				cudaResource_RegisterBuffer(chId, pbo, byteCount);
				cudaResource_mapBuffer(chId, (void **)&dev_pbo, &tmpSize);
				assert(tmpSize == byteCount);
				cudaMemcpy(dev_pbo, img.data, byteCount, cudaMemcpyDeviceToDevice);
				//cudaDeviceSynchronize();
				cudaResource_unmapBuffer(chId);
				cudaResource_UnregisterBuffer(chId);
				img.data = NULL;
			}else if(info->memtype == memtype_glpbo){
				cuUnmap(info);
				pbo = info->pbo;
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
			}else{
				unsigned int byteCount = img.cols*img.rows*img.channels();
				unsigned char *dev_pbo = NULL;
				size_t tmpSize;
				pbo = async_display(chId, img.cols, img.rows, img.channels());
				OSA_assert(pbo == buffId_input[chId]);
				OSA_assert(img.data != NULL);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
				cudaResource_RegisterBuffer(chId, pbo, byteCount);
				cudaResource_mapBuffer(chId, (void **)&dev_pbo, &tmpSize);
				assert(tmpSize == byteCount);
				cudaMemcpy(dev_pbo, img.data, byteCount, cudaMemcpyHostToDevice);
				//cudaDeviceSynchronize();
				cudaResource_unmapBuffer(chId);
				cudaResource_UnregisterBuffer(chId);
			}
			glBindTexture(GL_TEXTURE_2D, textureId_input[chId]);
			if(!bCreate[chId])
			{
				if(img.channels() == 1)
					glTexImage2D(GL_TEXTURE_2D, 0, img.channels(), img.cols, img.rows, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
				else if(img.channels() == 3)
					glTexImage2D(GL_TEXTURE_2D, 0, img.channels(), img.cols, img.rows, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, NULL);
				else if(img.channels() == 4)
					glTexImage2D(GL_TEXTURE_2D, 0, img.channels(), img.cols, img.rows, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
				m_videoSize[chId].w = img.cols;
				m_videoSize[chId].h = img.rows;
				m_videoSize[chId].c = img.channels();
				bCreate[chId] = true;
			}
			else
			{
				if(img.channels() == 1)
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, GL_RED, GL_UNSIGNED_BYTE, NULL);
				else if(img.channels() == 3)
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, GL_BGR_EXT, GL_UNSIGNED_BYTE, NULL);
				else if(img.channels() == 4)
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
			}
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			if(info->memtype == memtype_glpbo){
				cuMap(info);
			}
			image_queue_putEmpty(&m_bufQue[chId], info);
		}
	}
}

void CRender::UpdateOSD(void)
{
	//cv::line(m_imgOsd[0], cv::Point(300, 300), cv::Point(800, 300), cv::Scalar(255), 2, CV_AA);
#if(0)
	{
		glViewport(0, 0, m_mainWinWidth, m_mainWinHeight);
		static GLOSD  glosd;
		GLOSDNumberedBox box(&glosd);
		box.numbox(128, Rect(1920/2-30, 1080/2-20, 60, 40), Scalar(255, 255, 0, 255), 1);
		glosd.Draw();
	}
#endif
#if(0)
	{
		using namespace cr_osd;
		static GLOSD  glosd;
		//static DCOSD  glosd(&m_imgOsd[0]);
		GLOSDLine line1(&glosd);
		GLOSDLine line2(&glosd);
		GLOSDRectangle rectangle1(&glosd);
		GLOSDPolygon ploygon0(&glosd, 3);
		GLOSDRect rect0(&glosd);
		static Point ct(0, 0);
		static int incx = 1;
		static int incy = 1;
		if(ct.x<-(1920/2-100) || ct.x>1920/2-100)
			incx *= -1;
		if(ct.y<-(1080/2-100) || ct.y>1080/2-100)
			incy *= -1;
		ct.x += incx;
		ct.y += incy;
		Point center(1920/2+ct.x, 1080/2+ct.y);
		line1.line(Point(center.x-100, center.y), Point(center.x+100, center.y), Scalar(0, 255, 0, 255), 2);
		line2.line(Point(center.x, center.y-100), Point(center.x, center.y+100), Scalar(255, 255, 0, 255), 2);
		rectangle1.rectangle(Rect(center.x-50, center.y-50, 100, 100), Scalar(255, 0, 0, 255), 1);
		cv::Point pts[] = {cv::Point(center.x, center.y-80),cv::Point(center.x-75, center.y+38),cv::Point(center.x+75, center.y+38)};
		ploygon0.polygon(pts, Scalar(0, 0, 255, 255), 3);
		rect0.rect(Rect(center.x-50, center.y-50, 100, 100), Scalar(28, 28, 28, 255), 6);
		//GLOSDLine line3(&glosd);
		//GLOSDLine line4(&glosd);
		//line3.line(Point(1920/2-50, 1080/2), Point(1920/2+50, 1080/2), Scalar(0, 255, 0, 255), 2);
		//line4.line(Point(1920/2, 1080/2-50), Point(1920/2, 1080/2+50), Scalar(255, 255, 0, 255), 2);
		GLOSDCross cross(&glosd);
		cross.cross(Point(1920/2, 1080/2), Size(50, 50), Scalar(255, 255, 0, 255), 1);
		GLOSDNumberedBox box(&glosd);
		box.numbox(128, Rect(1920/2-30, 1080/2-20, 60, 40), Scalar(255, 255, 0, 255), 1);
		GLOSDTxt txt1(&glosd);
		GLOSDTxt txt2(&glosd);
		static wchar_t strTxt1[128] = L"0";
		txt1.txt(Point(center.x-5, center.y-txt1.m_fontSize+10), strTxt1, Scalar(255, 0, 255, 128));
		static wchar_t strTxt2[128];
		swprintf(strTxt2, 128, L"%d, %d", center.x, center.y);
		txt2.txt(Point(center.x+10, center.y-txt1.m_fontSize-10), strTxt2, Scalar(255, 255, 255, 200));
		glosd.Draw();
	};
#endif
#if(0)
	{
		using namespace cr_osd;
		glViewport(0, 0, m_mainWinWidth, m_mainWinHeight);
		glShaderManager.UseStockShader(GLT_SHADER_TEXTURE_SHADED, 0);
		cv::Size viewSize(m_mainWinWidth, m_mainWinHeight);
		static GLTXT txt2;
		wchar_t strTxt[128] = L"常用命令 hello world ! 1234567890";
		//wchar_t strTxt[256] = L"(hello world ! 1234567890 `!@#$%^&_*__+=-~[]{}|:;',./<>?)";
		//wchar_t strTxt[256] = L"=_";
		swprintf(strTxt, 128, L"常用命令 %6.3f hello world !", OSA_getCurTimeInMsec()*0.001f);
		txt2.putText(viewSize, strTxt, cv::Point(20, 300), cv::Scalar(255, 255, 0, 255));
		txt2.putText(viewSize, strTxt, cv::Point(20, 350), cv::Scalar(0, 255, 255, 255));
		txt2.putText(viewSize, strTxt, cv::Point(20, 400), cv::Scalar(255, 0, 255, 255));
		txt2.putText(viewSize, strTxt, cv::Point(20, 450), cv::Scalar(255, 255, 255, 255));
		txt2.putText(viewSize, strTxt, cv::Point(20, 500), cv::Scalar(0, 0, 0, 255));
	}
#endif
#if(0)
	{
		using namespace cr_osd;
		static DCTXT txt2;
		//wchar_t strTxt[128] = L"常用命令 hello world ! 1234567890";
		wchar_t strTxt[256] = L"(常用命令 hello world ! 1234567890 `!@#$%^&_*__+=-~[]{}|:;',./<>?)";
		//wchar_t strTxt[256] = L"=_";
		//swprintf(strTxt, 128, L"常用命令 hello world !%6.3f", OSA_getCurTimeInMsec()*0.001f);
		txt2.putText(m_imgDC[0], strTxt, cv::Point(20, 300), cv::Scalar(255, 255, 0, 255));
		txt2.putText(m_imgDC[0], strTxt, cv::Point(20, 350), cv::Scalar(0, 255, 255, 255));
		txt2.putText(m_imgDC[0], strTxt, cv::Point(20, 400), cv::Scalar(255, 0, 255, 255));
		txt2.putText(m_imgDC[0], strTxt, cv::Point(20, 450), cv::Scalar(255, 255, 255, 255));
		txt2.putText(m_imgDC[0], strTxt, cv::Point(20, 500), cv::Scalar(0, 0, 0, 255));
	}
#endif

#if 0
	{
		using namespace cr_osd;
		static cv::Mat wave(60, 60, CV_32FC1);
		static cv::Rect rc(1500, 20, 400, 400);
		static IPattern* pattern = NULL;
		//static int cnt = 0;
		//cnt ^=1;
		//wave.setTo(Scalar::all((double)cnt));
		if(pattern == NULL){

			cv::RNG rng = cv::RNG(OSA_getCurTimeInMsec());
			for(int i=0; i<wave.rows; i++){
				for(int j=0; j<wave.cols; j++)
				{
//					wave.at<float>(i, j) = sin(i*2*CV_PI/180.0);
					wave.at<float>(i, j)= std::exp(-1.0*((i-wave.rows/2)*(i-wave.rows/2)+(j-wave.cols/2)*(j-wave.cols/2))/(2.0*10*10)) - 1.0;///(CV_PI*2.0*3.0*3.0);

				}
			}

			pattern = IPattern::Create(wave, rc);
		}
		pattern->draw();
	}
#endif
}

void CRender::gl_display(void)
{
	int winId, chId;
	GLint glProg = 0;
	int iret;

	for(chId = 0; chId<m_initPrm.nChannels; chId++){
		if(m_bufQue[chId].bMap){
			for(int i=0; i<m_bufQue[chId].numBuf; i++){
				cuMap(&m_bufQue[chId].bufInfo[i]);
				image_queue_putEmpty(&m_bufQue[chId], &m_bufQue[chId].bufInfo[i]);
			}
			m_bufQue[chId].bMap = false;
		}
	}

	int64 tStamp[10];
	tStamp[0] = getTickCount();
	static int64 tstart = 0ul, tend = 0ul;
	tstart = tStamp[0];
	if(tend == 0ul)
		tend = tstart;
#if (!RENDMOD_TIME_ON)
	//if(m_initPrm.renderfunc == NULL)
	{
		double wms = m_interval*0.000001 - m_telapse;
		if(m_waitSync && wms>2.0){
			struct timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = wms*1000.0;
			select( 0, NULL, NULL, NULL, &timeout );
		}else{
			wms = 0.0;
		}
	}
#endif
	tStamp[1] = getTickCount();

	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(RUN_ENTER, 0, 0);
	tStamp[2] = getTickCount();

	gl_updateTexVideo();
	tStamp[3] = getTickCount();

	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(RUN_DC, 0, (int)m_bDC);
	gl_updateTexDC();
	tStamp[4] = getTickCount();

    //int viewport[4];
    //glGetIntegerv(GL_VIEWPORT, viewport);
    //glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    //glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);

	glClear(GL_COLOR_BUFFER_BIT);
	if(1)
	{
		OSA_mutexLock(&m_mutex);
		for(winId=0; winId<m_renderCount; winId++)
		{
			chId = m_curMap[winId];
			if(m_curMap[winId]!= m_renders[winId].video_chId){
				m_curMap[winId] = m_renders[winId].video_chId;
			}
			if(chId < 0 || chId >= DS_CHAN_MAX)
				continue;

			glUseProgram(m_glProgram[1]);
			GLint Uniform_tex_in = glGetUniformLocation(m_glProgram[1], "tex_in");
			GLint Uniform_mvp = glGetUniformLocation(m_glProgram[1], "mvpMatrix");
			GLMatx44f mTrans = m_glmat44fTrans[chId]*m_renders[winId].transform;
			glUniformMatrix4fv(Uniform_mvp, 1, GL_FALSE, mTrans.val);
			glUniform1i(Uniform_tex_in, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureId_input[chId]);
			glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, m_glvVerts[winId]);
			glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, GL_FALSE, 0, m_glvTexCoords[winId]);
			glEnableVertexAttribArray(ATTRIB_VERTEX);
			glEnableVertexAttribArray(ATTRIB_TEXTURE);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glViewport(m_renders[winId].displayrect.x,
					m_renders[winId].displayrect.y,
					m_renders[winId].displayrect.width, m_renders[winId].displayrect.height);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glUseProgram(0);

			int blend_chId = m_blendMap[chId];
			if(blend_chId>=0 && blend_chId<DS_CHAN_MAX){
				mTrans = m_glmat44fBlend[chId*DS_CHAN_MAX+blend_chId]*m_glmat44fTrans[chId]*m_renders[winId].transform;
				int maskId = m_maskMap[blend_chId];
				if(maskId < 0 || maskId >= DS_CHAN_MAX){
					if(m_videoSize[blend_chId].c == 1){
						glProg = m_glProgram[2];
					}else{
						glProg = m_glProgram[3];
					}
					glUseProgram(glProg);
					GLint Uniform_tex_in = glGetUniformLocation(glProg, "tex_in");
					GLint uniform_fAlpha = glGetUniformLocation(glProg, "fAlpha");
					GLint uniform_thr0Min = glGetUniformLocation(glProg, "thr0Min");
					GLint uniform_thr0Max = glGetUniformLocation(glProg, "thr0Max");
					GLint uniform_thr1Min = glGetUniformLocation(glProg, "thr1Min");
					GLint uniform_thr1Max = glGetUniformLocation(glProg, "thr1Max");
					GLint Uniform_mvp = glGetUniformLocation(glProg, "mvpMatrix");
					glUniformMatrix4fv(Uniform_mvp, 1, GL_FALSE, mTrans.val);
					glUniform1f(uniform_fAlpha, m_glBlendPrm[chId*DS_CHAN_MAX+blend_chId].fAlpha);
					glUniform1f(uniform_thr0Min, m_glBlendPrm[chId*DS_CHAN_MAX+blend_chId].thr0Min);
					glUniform1f(uniform_thr0Max, m_glBlendPrm[chId*DS_CHAN_MAX+blend_chId].thr0Max);
					glUniform1f(uniform_thr1Min, m_glBlendPrm[chId*DS_CHAN_MAX+blend_chId].thr1Min);
					glUniform1f(uniform_thr1Max, m_glBlendPrm[chId*DS_CHAN_MAX+blend_chId].thr1Max);
					glUniform1i(Uniform_tex_in, 0);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, textureId_input[blend_chId]);
				}else{
					glUseProgram(m_glProgram[4]);
					GLint Uniform_tex_in = glGetUniformLocation(m_glProgram[4], "tex_in");
					GLint Uniform_tex_mask = glGetUniformLocation(m_glProgram[4], "tex_mask");
					GLint Uniform_mvp = glGetUniformLocation(m_glProgram[4], "mvpMatrix");
					glUniformMatrix4fv(Uniform_mvp, 1, GL_FALSE, mTrans.val);
					glUniform1i(Uniform_tex_in, 0);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, textureId_input[blend_chId]);
					glUniform1i(Uniform_tex_mask, 1);
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, textureId_input[maskId]);
				}
				glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, m_glvVerts[winId]);
				glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, GL_FALSE, 0, m_glvTexCoords[winId]);
				glEnableVertexAttribArray(ATTRIB_VERTEX);
				glEnableVertexAttribArray(ATTRIB_TEXTURE);
				glEnable(GL_MULTISAMPLE);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				//glViewport(m_renders[winId].displayrect.x,
				//		m_renders[winId].displayrect.y,
				//		m_renders[winId].displayrect.width, m_renders[winId].displayrect.height);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glDisable(GL_MULTISAMPLE);
				glDisable(GL_BLEND);
				glUseProgram(0);
			}

			if(m_initPrm.renderfunc != NULL)
				m_initPrm.renderfunc(RUN_WIN, winId, chId);
		}
		OSA_mutexUnlock(&m_mutex);
	}

	if(m_bDC)
	{
		glViewport(0, 0, m_mainWinWidth, m_mainWinHeight);
		for(int i=0; i<DS_DC_CNT; i++)
		{
			if(m_imgDC[i].channels() == 1){
				glProg = m_glProgram[5];
				glUseProgram(glProg);
				GLint Uniform_tex_in = glGetUniformLocation(glProg, "tex_in");
				GLint Uniform_vcolor = glGetUniformLocation(glProg, "vColor");
				glUniform1i(Uniform_tex_in, 0);
				glActiveTexture(GL_TEXTURE0);
				GLfloat vColor[4];
				vColor[0] = m_dcColor.val[0]/255.0;
				vColor[1] = m_dcColor.val[1]/255.0;
				vColor[2] = m_dcColor.val[2]/255.0;
				vColor[3] = m_dcColor.val[3]/255.0;
				glUniform4fv(Uniform_vcolor, 1, vColor);
			}else{
				glProg = m_glProgram[0];
				glUseProgram(glProg);
				GLint Uniform_tex_in = glGetUniformLocation(glProg, "tex_in");
				glUniform1i(Uniform_tex_in, 0);
				glActiveTexture(GL_TEXTURE0);
			}

			glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, m_glvVertsDefault);
			glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, GL_FALSE, 0, m_glvTexCoordsDefault);
			glEnableVertexAttribArray(ATTRIB_VERTEX);
			glEnableVertexAttribArray(ATTRIB_TEXTURE);

			//glEnable(GL_MULTISAMPLE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindTexture(GL_TEXTURE_2D, textureId_dc[i]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			//glDisable(GL_MULTISAMPLE);
			glDisable(GL_BLEND);
			glUseProgram(0);
		}
	}
	//glValidateProgram(m_glProgram);

	UpdateOSD();

	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(RUN_SWAP, 0, 0);

	tStamp[5] = getTickCount();

	m_waitSync = true;
	int64 tcur = tStamp[5];
	m_telapse = (tStamp[5] - tStamp[1])*0.000001f + 3.0;
	/*
	double telapse = ( (tcur - tstart)/getTickFrequency())*1000.0f;
	if(telapse > m_interval*0.00000098f)
	{
		//m_waitSync = false;
#if (!RENDMOD_TIME_ON)
		if(m_telapse<m_interval*0.000001f){
			m_telapse += 2.0f;
		}
#endif
	}*/

	glutSwapBuffers();
	tStamp[6] = getTickCount();
	if(m_initPrm.renderfunc != NULL)
		m_initPrm.renderfunc(RUN_LEAVE, 0, 0);
	tend = tStamp[6];
	float renderIntv = (tend - m_tmRender)/getTickFrequency();
	if(renderIntv > m_interval*0.0000000011f)
	{
		if(m_telapse<m_interval*0.000001f)
			m_telapse+=0.5;
	}else{
		if(m_telapse>0.0005001)
		m_telapse -= 0.0005;
	}

#if 1
	static unsigned long rCount = 0;
	if(rCount%(m_initPrm.disFPS*100) == 0){
		printf("\r\n[%d] %.4f (ws%.4f,cu%.4f,tv%.4f,to%.4f,rd%.4f,wp%.4f) %.4f",
			OSA_getCurTimeInMsec(),renderIntv,
			(tStamp[1]-tStamp[0])/getTickFrequency(),
			(tStamp[2]-tStamp[1])/getTickFrequency(),
			(tStamp[3]-tStamp[2])/getTickFrequency(),
			(tStamp[4]-tStamp[3])/getTickFrequency(),
			(tStamp[5]-tStamp[4])/getTickFrequency(),
			(tStamp[6]-tStamp[5])/getTickFrequency(),
			m_telapse
			);
	}
	rCount ++;
#endif
	//if(!(mask&1)){
	//	OSA_printf("%s %d: null", __func__, __LINE__);
	//}
	m_tmRender = tend;
#if RENDMOD_TIME_ON
	if(!m_timerRun){
		unsigned int intv = (m_interval-((telapse+0.009)*1000000000));
		intv /= 1000000;
		glutTimerFunc(intv, _timeFunc, m_initPrm.timerfunc_value);
	}
#else
	m_timerRun = true;
	glutPostRedisplay();
#endif
}

//////////////////////////////////////////////////////
//
static const char *szDefaultShaderVP = ""
		"attribute vec4 vVertex;"
		"attribute vec2 vTexCoords;"
		"varying vec2 vVaryingTexCoords;"
		"void main(void)"
		"{"
		"    gl_Position = vVertex;"
		"    vVaryingTexCoords = vTexCoords;"
		"}";
static const char *szFlatShaderVP = ""
		"attribute vec4 vVertex;"
		"attribute vec2 vTexCoords;"
		"varying vec2 vVaryingTexCoords;"
		"uniform mat4 mvpMatrix;"
		"void main(void)"
		"{"
		"    gl_Position = mvpMatrix*vVertex;"
		"    vVaryingTexCoords = vTexCoords;"
		"}";

static const char *szDefaultShaderFP = ""
		"varying vec2 vVaryingTexCoords;"
		"uniform sampler2D tex_in;"
		"void main(void)"
		"{"
		"	gl_FragColor = texture(tex_in, vVaryingTexCoords);"
		"}";
static const char *szAlphaTextureShaderFP = ""
		"varying vec2 vVaryingTexCoords;"
		"uniform sampler2D tex_in;"
		"uniform vec4 vColor;"
		"void main(void)"
		"{"
		"	vec4  vAlpha;"
		"	vAlpha = texture(tex_in, vVaryingTexCoords);"
		"	vColor.a = vAlpha.a;"
		"	gl_FragColor = vColor;"
		"}";
static const char *szBlendMaskTextureShaderFP = ""
		"varying vec2 vVaryingTexCoords;"
		"uniform sampler2D tex_in;"
		"uniform sampler2D tex_mask;"
		"void main(void)"
		"{"
		"	vec4  vColor;"
		"	vec4  vMask;"
		"	vColor = texture(tex_in, vVaryingTexCoords);"
		"	vMask = texture(tex_mask, vVaryingTexCoords);"
		"	vColor.a = vMask.r;"
		"	gl_FragColor = vColor;"
		"}";
static const char *szPolarityShaderFP = ""
		"varying vec2 vVaryingTexCoords;"
		"uniform sampler2D tex_in;"
		"uniform float fAlpha;"
		"uniform float thr0Min;"
		"uniform float thr0Max;"
		"uniform float thr1Min;"
		"uniform float thr1Max;"
		"void main(void)"
		"{"
		"	vec4  vColor;"
		"	vec4  vAlpha;"
		"	float ra0, ra1;"
		"	vColor = texture(tex_in, vVaryingTexCoords);"
		"	ra0 = (1- step(thr0Min,vColor.r)*step(vColor.r,thr0Max) );"
		"	ra1 = (1- step(thr1Min,vColor.r)*step(vColor.r,thr1Max) );"
		"	vColor.a = (1-ra0*ra1)*fAlpha;"
		"	gl_FragColor = vColor;"
		"}";

static const char *szPolarityShaderRGBFP = ""
		"varying vec2 vVaryingTexCoords;"
		"uniform sampler2D tex_in;"
		"uniform float fAlpha;"
		"uniform float thr0Min;"
		"uniform float thr0Max;"
		"uniform float thr1Min;"
		"uniform float thr1Max;"
		"void main(void)"
		"{"
		"	vec4  vColor;"
		"	vec4  vAlpha;"
		"	float ra0,ra1,ga0,ga1,ba0,ba1;"
		"	vColor = texture(tex_in, vVaryingTexCoords);"
#if 0
		"	//vColor.a = fAlpha;"
		"	//vColor.a = step(thrMin, vColor.r)*step(vColor.r, thrMax)*fAlpha;"
		"	//vColor.a = smoothstep(thrMin, thrMax, vColor.r)*step(vColor.r, thrMax)*fAlpha;"
		"	ra = (1-step(thr0Min, vColor.r)*step(vColor.r, thr0Max))*(1-step(thr1Min, vColor.r)*step(vColor.r, thr1Max));"
		"	ga = (1-step(thr0Min, vColor.g)*step(vColor.g, thr0Max))*(1-step(thr1Min, vColor.g)*step(vColor.g, thr1Max));"
		"	ba = (1-step(thr0Min, vColor.b)*step(vColor.b, thr0Max))*(1-step(thr1Min, vColor.b)*step(vColor.b, thr1Max));"
		"	ra = (1-(1-step(vColor.r,thr0Min))*(1-step(thr0Max, vColor.r)))*(1-(1-step(vColor.r,thr1Min))*step(vColor.r, thr1Max));"
		"	ga = (1-(1-step(vColor.g,thr0Min))*(1-step(thr0Max, vColor.g)))*(1-(1-step(vColor.g,thr1Min))*step(vColor.g, thr1Max));"
		"	ba = (1-(1-step(vColor.b,thr0Min))*(1-step(thr0Max, vColor.b)))*(1-(1-step(vColor.b,thr1Min))*step(vColor.b, thr1Max));"
		"	gray = vColor.r*0.299+vColor.g*0.587+vColor.b*0.114;"
#endif
		"	ra0 = (1- step(thr0Min,vColor.r)*step(vColor.r,thr0Max) );"
		"	ra1 = (1- step(thr1Min,vColor.r)*step(vColor.r,thr1Max) );"
		"	ga0 = (1- step(thr0Min,vColor.g)*step(vColor.g,thr0Max) );"
		"	ga1 = (1- step(thr1Min,vColor.g)*step(vColor.g,thr1Max) );"
		"	ba0 = (1- step(thr0Min,vColor.b)*step(vColor.b,thr0Max) );"
		"	ba1 = (1- step(thr1Min,vColor.b)*step(vColor.b,thr1Max) );"
		"	vColor.a = (1-ra0*ra1*ga0*ga1*ba0*ba1)*fAlpha;"
		"	gl_FragColor = vColor;"
		"}";
int CRender::gl_loadProgram()
{
	int iRet = OSA_SOK;
	m_glProgram[0] = gltLoadShaderPairWithAttributes(szDefaultShaderVP, szDefaultShaderFP, 2, ATTRIB_VERTEX, "vVertex", ATTRIB_TEXTURE, "vTexCoords");
	m_glProgram[1] = gltLoadShaderPairWithAttributes(szFlatShaderVP, szDefaultShaderFP, 2, ATTRIB_VERTEX, "vVertex", ATTRIB_TEXTURE, "vTexCoords");
	m_glProgram[2] = gltLoadShaderPairWithAttributes(szFlatShaderVP, szPolarityShaderFP, 2, ATTRIB_VERTEX, "vVertex", ATTRIB_TEXTURE, "vTexCoords");
	m_glProgram[3] = gltLoadShaderPairWithAttributes(szFlatShaderVP, szPolarityShaderRGBFP, 2, ATTRIB_VERTEX, "vVertex", ATTRIB_TEXTURE, "vTexCoords");
	m_glProgram[4] = gltLoadShaderPairWithAttributes(szFlatShaderVP, szBlendMaskTextureShaderFP, 2, ATTRIB_VERTEX, "vVertex", ATTRIB_TEXTURE, "vTexCoords");
	m_glProgram[5] = gltLoadShaderPairWithAttributes(szDefaultShaderVP, szAlphaTextureShaderFP, 2, ATTRIB_VERTEX, "vVertex", ATTRIB_TEXTURE, "vTexCoords");
	return iRet;
}

int CRender::gl_unloadProgram()
{
	int iRet = OSA_SOK;
	int i;
	for(i=0; i<8; i++){
		if(m_glProgram[i] != 0)
			glDeleteProgram(m_glProgram[i]);
		m_glProgram[i] = 0;
	}
	return iRet;
}

//////////////////////////////////////////////////////////////////////////
// Load the shader from the source text
bool CRender::gltLoadShaderSrc(const char *szShaderSrc, GLuint shader)
{
	if(szShaderSrc == NULL)
		return false;
	GLchar *fsStringPtr[1];

	fsStringPtr[0] = (GLchar *)szShaderSrc;
	glShaderSource(shader, 1, (const GLchar **)fsStringPtr, NULL);

	return true;
}

#define MAX_SHADER_LENGTH   8192
static GLubyte shaderText[MAX_SHADER_LENGTH];
////////////////////////////////////////////////////////////////
// Load the shader from the specified file. Returns false if the
// shader could not be loaded
bool CRender::gltLoadShaderFile(const char *szFile, GLuint shader)
{
	GLint shaderLength = 0;
	FILE *fp;

	// Open the shader file
	fp = fopen(szFile, "r");
	if(fp != NULL)
	{
		// See how long the file is
		while (fgetc(fp) != EOF)
			shaderLength++;

		// Allocate a block of memory to send in the shader
		assert(shaderLength < MAX_SHADER_LENGTH);   // make me bigger!
		if(shaderLength > MAX_SHADER_LENGTH)
		{
			fclose(fp);
			return false;
		}

		// Go back to beginning of file
		rewind(fp);

		// Read the whole file in
		if (shaderText != NULL){
			size_t ret = fread(shaderText, 1, (size_t)shaderLength, fp);
			OSA_assert(ret == shaderLength);
		}

		// Make sure it is null terminated and close the file
		shaderText[shaderLength] = '\0';
		fclose(fp);
	}
	else
		return false;    

	// Load the string
	gltLoadShaderSrc((const char *)shaderText, shader);

	return true;
}   

/////////////////////////////////////////////////////////////////
// Load a pair of shaders, compile, and link together. Specify the complete
// source text for each shader. After the shader names, specify the number
// of attributes, followed by the index and attribute name of each attribute
GLuint CRender::gltLoadShaderPairWithAttributes(const char *szVertexProg, const char *szFragmentProg, ...)
{
	// Temporary Shader objects
	GLuint hVertexShader;
	GLuint hFragmentShader; 
	GLuint hReturn = 0;   
	GLint testVal;

	// Create shader objects
	hVertexShader = glCreateShader(GL_VERTEX_SHADER);
	hFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load them. If fail clean up and return null
	// Vertex Program
	//if(gltLoadShaderFile(szVertexProg, hVertexShader) == false)
	if(gltLoadShaderSrc(szVertexProg, hVertexShader) == false)
	{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		fprintf(stderr, "The shader at %s could ot be found.\n", szVertexProg);
		return (GLuint)NULL;
	}

	// Fragment Program
	//if(gltLoadShaderFile(szFragmentProg, hFragmentShader) == false)
	if(gltLoadShaderSrc(szFragmentProg, hFragmentShader) == false)
	{
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		fprintf(stderr,"The shader at %s  could not be found.\n", szFragmentProg);
		return (GLuint)NULL;
	}

	// Compile them both
	glCompileShader(hVertexShader);
	glCompileShader(hFragmentShader);

	// Check for errors in vertex shader
	glGetShaderiv(hVertexShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
	{
		char infoLog[1024];
		glGetShaderInfoLog(hVertexShader, 1024, NULL, infoLog);
		fprintf(stderr, "The shader at %s failed to compile with the following error:\n%s\n", szVertexProg, infoLog);
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
	}

	// Check for errors in fragment shader
	glGetShaderiv(hFragmentShader, GL_COMPILE_STATUS, &testVal);
	if(testVal == GL_FALSE)
	{
		char infoLog[1024];
		glGetShaderInfoLog(hFragmentShader, 1024, NULL, infoLog);
		fprintf(stderr, "The shader at %s failed to compile with the following error:\n%s\n", szFragmentProg, infoLog);
		glDeleteShader(hVertexShader);
		glDeleteShader(hFragmentShader);
		return (GLuint)NULL;
	}

	// Create the final program object, and attach the shaders
	hReturn = glCreateProgram();
	glAttachShader(hReturn, hVertexShader);
	glAttachShader(hReturn, hFragmentShader);


	// Now, we need to bind the attribute names to their specific locations
	// List of attributes
	va_list attributeList;
	va_start(attributeList, szFragmentProg);

	// Iterate over this argument list
	char *szNextArg;
	int iArgCount = va_arg(attributeList, int);	// Number of attributes
	for(int i = 0; i < iArgCount; i++)
	{
		int index = va_arg(attributeList, int);
		szNextArg = va_arg(attributeList, char*);
		glBindAttribLocation(hReturn, index, szNextArg);
	}
	va_end(attributeList);

	// Attempt to link    
	glLinkProgram(hReturn);

	// These are no longer needed
	glDeleteShader(hVertexShader);
	glDeleteShader(hFragmentShader);  

	// Make sure link worked too
	glGetProgramiv(hReturn, GL_LINK_STATUS, &testVal);
	if(testVal == GL_FALSE)
	{
		char infoLog[1024];
		glGetProgramInfoLog(hReturn, 1024, NULL, infoLog);
		fprintf(stderr,"The programs %s and %s failed to link with the following errors:\n%s\n",
			szVertexProg, szFragmentProg, infoLog);
		glDeleteProgram(hReturn);
		return (GLuint)NULL;
	}

	// All done, return our ready to use shader program
	return hReturn;  
}   
