


#ifndef  __glwin_h_
#define  __glwin_h_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>		// Header File For Windows
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library

// Model orientation
const float ORIG_ELEVATION = 0.0f;
const float ORIG_AZIMUTH = 0.0f;
const float ORIG_DIST = 3.0f;

const float MIN_DISTANCE = 0.1f;
const float MAX_DISTANCE = 100.0f;

class glModelWindow
{
public:

	// constructor
	glModelWindow() : hWnd_(NULL), hDC_(NULL), hGLRC_(NULL),
						szClassName_("MeshSimp"), 
						szAppName_("Mesh Simplication Demo by Jeff Somers"),
						width_(0), height_(0), oldWidth_(0), oldHeight_(0),
						oldX_(0), oldY_(0), newX_(0), newY_(0),
						bFullScreen_(false), bFill_(true), bSmooth_(false)
	{
		resetOrientation();
	};

	~glModelWindow() {};

	// resize the OpenGL window
	GLvoid reSizeScene(GLsizei width, GLsizei height);

	// Initialize OpenGL
	int initOpenGL(GLvoid);

	// display the mesh within the window
	bool displayMesh();

	// shutdown the OpenGL window
	GLvoid killMyWindow(GLvoid);

	// create the OpenGL window
	bool createMyWindow(int width, int height, unsigned char bits, bool fullscreenflag);

	// return the Win32 windows handle
	HWND getHWnd() {return hWnd_;};

	// If in windowed mode, set to full screen mode.
	// If in full screen mode, reset to windowed mode (width, height parameters 
	// are ignored, the previous size of the window is used instead)
	void flipFullScreen(int width, int height);

	// deal with mesh rotation via mouse movement
	void mouseMotion(int x, int y, bool leftButton, bool rightButton);

	// set new mouse coordiantes
	void setNewXY(int x, int y) {newX_ = x; newY_ = y;};

	// reset the mesh orientation
	void resetOrientation() {	glMatrixMode(GL_MODELVIEW);
								glLoadIdentity();
								elevation_ = ORIG_ELEVATION;
								azimuth_ = ORIG_AZIMUTH;
								dist_ = ORIG_DIST;
							}

	// In order to change the full screen indicator, call flipFullScreen.
	bool isFullScreen() {return bFullScreen_;};

	// The mesh may be displayed in wireframe or filled-triangle mode
	bool isFillTriMode() {return bFill_;};
	void setFillTriMode(bool newFillbool) {bFill_ = newFillbool;};

	// The mesh may be flat shaded (Lambert shaded) or smooth (Gouraud) shaded
	bool isSmoothShadingMode() {return bSmooth_;};
	void setSmoothShadingMode(bool newSmooth) {bSmooth_ = newSmooth;};

	// Display title text for window
	void displayWindowTitle();

private:
	// Window Handle
	HWND	hWnd_;

	// Display context
	HDC		hDC_;

	// Rendering context
	HGLRC	hGLRC_;

	// Used to create a Win32 window
	const char *szClassName_;
	const char *szAppName_;

	// width, height of window
	int width_;
	int height_;

	// previous window width, height
	int oldWidth_;
	int oldHeight_;

	// used for mouse motion
	int oldX_, oldY_, newX_, newY_;

	// Mesh orientation variables
	float elevation_;
	float azimuth_;
	float dist_;

	// Full Screen display indicator
	bool bFullScreen_;

	// Fill in triangles (or use wireframe mode?)
	bool bFill_;

	// Use Gouraud shading?
	bool bSmooth_;

	// no assignment, copy ctor allowed (implementation not provided).
	glModelWindow(const glModelWindow&);
	glModelWindow& operator=(const glModelWindow&);
};

#endif
