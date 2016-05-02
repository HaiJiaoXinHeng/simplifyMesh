

#include "glmodelwin.h"
#include "pmesh.h"


extern PMesh* g_pProgMesh; 
extern Mesh* g_pMesh;
extern HINSTANCE g_hInstance;
extern char g_filename[];

LRESULT	CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);


// Display title text for window
void glModelWindow::displayWindowTitle()
{
	assert(g_pProgMesh);
	char temp[1024] = {'\0'};
	sprintf(temp, "Jeff Somers Mesh Simplification Viewer - %s, # Tris: %d, %s",
		g_pProgMesh->getEdgeCostDesc(), g_pProgMesh->numVisTris(), g_filename);
	SetWindowText(getHWnd(), temp);
}


// Resize the OpenGL window
GLvoid glModelWindow::reSizeScene(GLsizei width, GLsizei height)
{
	if (height==0) height = 1; // height == 0 not allowed

	glViewport(0,0,width,height);

	width_ = width;
	height_ = height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Calculate The Perspective of the winodw
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,MIN_DISTANCE,MAX_DISTANCE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// Initialize OpenGL
int glModelWindow::initOpenGL(GLvoid)
{
	glShadeModel(GL_SMOOTH); // Gouraud shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Enable lights and lighting
	glEnable(GL_LIGHT0); // default value is (1.0, 1.0, 1.0, 1.0)
	glEnable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE); // backface culling
	
	return true;
}

// Display the mesh
bool glModelWindow::displayMesh()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Set lookat point
    glLoadIdentity();
	gluLookAt(0,0,dist_, 0,0,0, 0,1,0);

    glRotatef(elevation_,1,0,0);
    glRotatef(azimuth_,0,1,0);
	
	if (bSmooth_)
	{
		glShadeModel(GL_SMOOTH); // already defined in initOpenGL
	}
	else 
	{
		glShadeModel(GL_FLAT);
	}

	if (bFill_) // fill in triangles or just display outlines?
	{
		glPolygonMode( GL_FRONT, GL_FILL );
	}
	else
	{
		glPolygonMode( GL_FRONT, GL_LINE );
	}

	// NOTE: we could use display lists here.  That would speed things
	// up which the user is rotating the mesh.  However, since speed isn't
	// a bit issue, I didn't use them.
	if (g_pProgMesh)
	{
		// Make everything grey
		glColor3ub(128, 128, 128);
		glBegin(GL_TRIANGLES);
		for (int i=0; i<g_pProgMesh->numTris(); i++)
		{
			triangle t;
			float a[3];
			if (g_pProgMesh->getTri(i, t) && t.isActive())
			{
				if (bSmooth_)
				{
					const vertex& v1 = t.getVert1vertex();
					const float* pFltV1Norm = v1.getArrayVertNorms();
					glNormal3fv(pFltV1Norm);
					const float* pFltArray1 = v1.getArrayVerts();
					glVertex3fv(pFltArray1);
					const vertex& v2 = t.getVert2vertex();
					const float* pFltV2Norm = v2.getArrayVertNorms();
					glNormal3fv(pFltV2Norm);
					const float* pFltArray2 = v2.getArrayVerts();
					glVertex3fv(pFltArray2);
					const vertex& v3 = t.getVert3vertex();
					const float* pFltV3Norm = v3.getArrayVertNorms();
					glNormal3fv(pFltV3Norm);
					const float* pFltArray3 = v3.getArrayVerts();
					glVertex3fv(pFltArray3);

					// flip it
					a[0] = -pFltV3Norm[0];
					a[1] = -pFltV3Norm[1];
					a[2] = -pFltV3Norm[2];
					glNormal3fv(a);
					glVertex3fv(pFltArray3);
					a[0] = -pFltV2Norm[0];
					a[1] = -pFltV2Norm[1];
					a[2] = -pFltV2Norm[2];
					glNormal3fv(a);
					glVertex3fv(pFltArray2);
					a[0] = -pFltV1Norm[0];
					a[1] = -pFltV1Norm[1];
					a[2] = -pFltV1Norm[2];
					glNormal3fv(a);
					glVertex3fv(pFltArray1);
				}
				else
				{
					const float* pFltArrayN = t.getNormal();
					glNormal3fv(pFltArrayN);
					const float* pFltArray1 = t.getVert1();
					glVertex3fv(pFltArray1);
					const float* pFltArray2 = t.getVert2();
					glVertex3fv(pFltArray2);
					const float* pFltArray3 = t.getVert3();
					glVertex3fv(pFltArray3);
		
					a[0] = -pFltArrayN[0];
					a[1] = -pFltArrayN[1];
					a[2] = -pFltArrayN[2];
					glNormal3fv(a);
					glVertex3fv(pFltArray3);
					glVertex3fv(pFltArray2);
					glVertex3fv(pFltArray1);
				}
			}
		}
		glEnd();
	}
	
	SwapBuffers(hDC_);
	return true;
}


// shut down OpenGL window
GLvoid glModelWindow::killMyWindow(GLvoid)
{
	if (bFullScreen_)
	{
		ChangeDisplaySettings(NULL,0); // undo Full Screen mode
	}

	if (hGLRC_) // OpenGL rendering context
	{
		if (!wglMakeCurrent(NULL,NULL))
		{
			assert(false);
		}

		if (!wglDeleteContext(hGLRC_)) // get rid of rendering context
		{
			assert(false);
		}
		hGLRC_ = NULL;
	}

	if (hDC_ && !ReleaseDC(hWnd_,hDC_)) // get rid of display context
	{
		assert(false);
		hDC_ = NULL;
	}

	if (hWnd_ && !DestroyWindow(hWnd_)) // get rid of window
	{
		assert(false);
		hWnd_ = NULL;
	}

	if (!UnregisterClass(szClassName_,g_hInstance)) // get rid of window class
	{
		assert(false);
		g_hInstance = NULL;
	}
}

// Create OpenGL window
bool glModelWindow::createMyWindow(int width, int height, unsigned char bits, bool fullscreenflag)
{
	width_ = width;
	height_ = height;

	GLuint		PixelFormat;
	WNDCLASS	wc;
	DWORD		dwExStyle;
	DWORD		dwStyle;
	RECT		WindowRect;
	WindowRect.left = 0;
	WindowRect.right = width;
	WindowRect.top = 0;
	WindowRect.bottom = height;

	bFullScreen_ = fullscreenflag;

	wc.style			= CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= wndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= g_hInstance;
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= NULL;

	if (bFullScreen_)
	{
		wc.lpszMenuName		= NULL; // no menu
	} 
	else
	{
		wc.lpszMenuName		= "MainMenu";
	}
	wc.lpszClassName	= szClassName_;

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	
	if (bFullScreen_) // full screen mode
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= width;
		dmScreenSettings.dmPelsHeight	= height;
		dmScreenSettings.dmBitsPerPel	= bits;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try to change to full screen mode.
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			// mode failed
			if (MessageBox(NULL,
				"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				bFullScreen_ = false; // we won't run in full screen mode
			}
			else
			{
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return false;
			}
		}
	}

	if (bFullScreen_)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle=WS_OVERLAPPEDWINDOW;
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, false, dwExStyle);		// Adjust Window To True Requested Size


	hWnd_=CreateWindowEx(dwExStyle,
							szClassName_,
							szAppName_,
							dwStyle |	
							WS_CLIPSIBLINGS |
							WS_CLIPCHILDREN,
							0, 0,
							WindowRect.right - WindowRect.left,
							WindowRect.bottom - WindowRect.top,
							NULL,
							NULL,
							g_hInstance,
							NULL);
	if (!hWnd_)
	{
		killMyWindow();	// window creation failed
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	PIXELFORMATDESCRIPTOR pfd=	
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		bits,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,	
		16,	
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0	
	};

	hDC_=GetDC(hWnd_);	// device context
	if (!hDC_)
	{
		killMyWindow();	
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	PixelFormat = ChoosePixelFormat(hDC_,&pfd);
	if (!PixelFormat) // get pixel format
	{
		killMyWindow();
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(!SetPixelFormat(hDC_,PixelFormat,&pfd))	// set pixel format
	{
		killMyWindow();
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	hGLRC_ = wglCreateContext(hDC_);
	if (!hGLRC_)	// get rendering context
	{
		killMyWindow();
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(!wglMakeCurrent(hDC_,hGLRC_)) // make it the current rendering context
	{
		killMyWindow();
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	ShowWindow(hWnd_,SW_SHOW);

	SetForegroundWindow(hWnd_);
	SetFocus(hWnd_);
	reSizeScene(width, height);

	if (!initOpenGL())
	{
		killMyWindow();
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	InvalidateRect(hWnd_, NULL, TRUE); // repaint window

	return true;
}

// Handle mouse motion
void glModelWindow::mouseMotion(int x, int y, bool leftButton, bool rightButton)
{
	oldX_ = newX_; 
	oldY_ = newY_;
	newX_ = x;  
	newY_ = y; 

	if (newX_ & 0x1000) newX_ = -(0xFFFF - newX_); // when move mouse up, turns from 0 to 65535
	if (newY_ & 0x1000) newY_ = -(0xFFFF - newY_); // turn an unsigned value to a signed value so math works

	float RelX = (newX_ - oldX_) / (float)width_;
	float RelY = (newY_ - oldY_) / (float)height_;

	// Change distance
	if (rightButton) 
	{
		dist_ += 10 * RelY;

		if (dist_ > MAX_DISTANCE) dist_ = MAX_DISTANCE;
		if (dist_ < MIN_DISTANCE) dist_ = MIN_DISTANCE;
	// Rotation
	} 
	else if (leftButton)
	{ // left button down
		azimuth_ += (RelX*180);
		elevation_ += (RelY*180);
	}
}

// Change window to full screen
void glModelWindow::flipFullScreen(int width, int height)
{
	killMyWindow();		
	bFullScreen_=!bFullScreen_;

	if (bFullScreen_)
	{
		oldWidth_ = width_;
		oldHeight_ = height_;
		MessageBox(NULL,TEXT("To leave full screen mode, press the SPACE bar."),"Jeff Somers Mesh Simplification Viewer",MB_OK);
	}
	else
	{
		width = oldWidth_;
		height = oldHeight_;
	}

	// Create fullscreen window
	const unsigned char depth = 16;
	if (!createMyWindow(width,height,depth,bFullScreen_))
	{
		return;
	}

	if (g_pMesh) displayWindowTitle();
}
