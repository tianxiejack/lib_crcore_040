/*
 * gluWindow.cpp
 *
 *  Created on: Dec 17, 2018
 *      Author: wzk
 */

#include "gluWindow.hpp"
#include "osa_image_queue.h"
#include <cuda.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime_api.h>
#include "cuda_mem.hpp"
#include <linux/videodev2.h>
#include "glvideo.hpp"
#include "glvideoRender.hpp"

//using namespace cv;

int CGluWindow::count = 0;
static std::vector<CGluWindow *> vwindows;
CGluWindow::CGluWindow(const cv::Rect& rc, int parent)
:m_winId(-1),m_rc(rc),m_rcReal(rc),m_parent(parent)
{
	m_index = count++;
}
CGluWindow::~CGluWindow()
{
	Destroy();
}

void CGluWindow::_reshape(int width, int height)
{
	int winId = glutGetWindow();
	//OSA_printf("[%d] %s %d: GLU%d %d x %d", OSA_getCurTimeInMsec(), __func__, __LINE__, winId, width, height);
	CGluWindow *win = vwindows[winId];
	win->Reshape(width, height);
}

void CGluWindow::_display(void)
{
	int winId = glutGetWindow();
	//OSA_printf("[%d] %s %d: GLU%d enter", OSA_getCurTimeInMsec(), __func__, __LINE__, winId);
	CGluWindow *win = vwindows[winId];
	win->Display();
	glutPostRedisplay();
}

int CGluWindow::Create(bool FullScreen)
{
	if(m_parent>0){
		m_winId = glutCreateSubWindow(m_parent,m_rc.x,m_rc.y,m_rc.width,m_rc.height);
	}else{
		glutInitWindowPosition(m_rc.x, m_rc.y);
		glutInitWindowSize(m_rc.width, m_rc.height);
		glutSetOption(GLUT_RENDERING_CONTEXT,GLUT_USE_CURRENT_CONTEXT);
		char title[32];
		sprintf(title, "window%d", m_index);
		m_winId = glutCreateWindow(title);
	}
	OSA_assert(m_winId > 0);
	glutSetWindow(m_winId);
	if(vwindows.size()<m_winId+1)
		vwindows.resize(m_winId+1);
	vwindows[m_winId] = this;
	glutReshapeFunc(_reshape);
	glutDisplayFunc(_display);

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "\n[GLU%d] %s %d: Error in glewInit. %s\n", m_winId, __func__, __LINE__, glewGetErrorString(err));
		return -1;
	}

	glClearColor(0.0f, 1.0f, 0.01f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_MULTISAMPLE);
	glDisable(GL_COLOR_MATERIAL);

	glutSetWindow(m_winId);
	if(FullScreen){
		glutFullScreen();
	}

	return err;
}

void CGluWindow::Destroy()
{

}

void CGluWindow::Display()
{
	//OSA_printf("[%d] %s %d: GLU%d enter", OSA_getCurTimeInMsec(), __func__, __LINE__, m_winId);
	glutSwapBuffers();
}

void CGluWindow::Reshape(int width, int height)
{
	m_rcReal.x = glutGet(GLUT_WINDOW_X);
	m_rcReal.y = glutGet(GLUT_WINDOW_Y);
	m_rcReal.width = glutGet(GLUT_WINDOW_WIDTH);
	m_rcReal.height = glutGet(GLUT_WINDOW_HEIGHT);
	OSA_printf("[%d] %s %d: GLU%d rc(%d,%d,%d,%d)(%d,%d,%d,%d)", OSA_getCurTimeInMsec(), __func__, __LINE__, m_winId,
			m_rcReal.x, m_rcReal.y, m_rcReal.width, m_rcReal.height,
			m_rc.x, m_rc.y, m_rc.width, m_rc.height);
}

