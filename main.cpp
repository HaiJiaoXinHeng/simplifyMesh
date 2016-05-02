

//#define WIN32_LEAN_AND_MEAN // we can't use this because we use the OPENFILENAME struct.
#include <windows.h>		// Header File For Windows
#include <sys/stat.h>
#include <cstdio>

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma warning(disable:4710) // function not inlined
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4514) // unreferenced inline function has been removed
#pragma warning(disable:4786) // disable "identifier was truncated to '255' characters in the browser information" warning in Visual C++ 6*
#endif

#include "resource.h"
#include "mesh.h"
#include "pmesh.h"
#include "glmodelwin.h"

// Menu positions
const int SIMPLICATION_MENU = 1;
const int UPDATE_MENU		= 2;
const int FILL_MENU         = 3;
const int SHADING_MENU      = 4;


// App. Instance
HINSTANCE	g_hInstance = NULL;

// Triangle model
Mesh* g_pMesh = NULL;

// Progressive Mesh
PMesh* g_pProgMesh = NULL;

// Edge Collapse Options
PMesh::EdgeCost g_edgemethod = PMesh::QUADRICTRI;

// OpenGL Window
glModelWindow* g_pWindow = NULL;

// file name
char g_filename[256] = {'\0'};


LRESULT	CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI aboutDlgProc( HWND, UINT, WPARAM, LPARAM);


// allow user to chose which mesh to load.
void loadMesh()
{
	static char szFilter[]= "Ply files (*.ply)\0*.ply\0";
	OPENFILENAME ofn;
	char pszFileLocn[256] = {'\0'};

	// Set up OPENFILENAME struct to use commond dialog box for open

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(ofn);	// size of struct
	ofn.hwndOwner = NULL;			// window that owns Dlg
	ofn.lpstrFilter = szFilter;		// Filter text
	ofn.lpstrFile = pszFileLocn;		// File name string
	ofn.nMaxFile = sizeof(pszFileLocn); // size of file name
	ofn.Flags = OFN_HIDEREADONLY;	// don't display "Read Only"
	ofn.lpstrDefExt = "ply";		// extension name
	ofn.lpstrTitle = "Open Mesh File"; // title of dlg box

	// call common dlg control for file open
	if (!GetOpenFileName(&ofn)) {
		return ;	
	}

	// see if file exists
    struct stat fileStat;
    if (stat(ofn.lpstrFile, &fileStat))
	{
		char errormsg[1024];
		sprintf(errormsg, "%s not found.", ofn.lpstrFile);
		MessageBox(NULL,errormsg,"File Not Found Error",MB_OK | MB_ICONINFORMATION);
		return ;	
	}

	if (!g_pProgMesh) // 1st time through
	{
		// load "plus sign" cursor
		SetClassLong(g_pWindow->getHWnd(), GCL_HCURSOR, (LONG) LoadCursor(NULL, IDC_CROSS));
	}

	delete g_pMesh;
	g_pMesh = NULL; // not necessary, but a nice CYA habit
	delete g_pProgMesh;
	g_pProgMesh = NULL;

	SetWindowText(g_pWindow->getHWnd(), "Jeff Somers Mesh Simplification Viewer - (loading....)");
	g_pMesh = new Mesh(ofn.lpstrFile);
	strcpy(g_filename, ofn.lpstrFile);

	if (g_pMesh) g_pMesh->Normalize();// center mesh around the origin & shrink to fit

	g_pProgMesh = new PMesh(g_pMesh, g_edgemethod );
	
	// reset the position of the mesh
	g_pWindow->resetOrientation();

	g_pWindow->displayWindowTitle();
}


// User has selected a new mesh simplification algorithm
void changeSimplificationAlgorithm(const char* name, const PMesh::EdgeCost &ec)
{
	if (ec != g_edgemethod)
	{
		char temp[1024];
		strcpy(temp, "Jeff Somers Mesh Simplification Viewer - ");
		strcat(temp, name);
		SetWindowText(g_pWindow->getHWnd(), temp);
		g_edgemethod = ec;
		if (0 != strlen(g_filename))
		{
			strcat(temp, " (loading....)");
			SetWindowText(g_pWindow->getHWnd(), temp);
			if (g_pMesh == NULL)
			{
				g_pMesh = new Mesh(g_filename);
				if (g_pMesh) g_pMesh->Normalize();
			}
			delete g_pProgMesh;
			g_pProgMesh = new PMesh(g_pMesh, g_edgemethod);
			g_pWindow->displayWindowTitle();
			InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
		}
	}
}

// User selected a menu item
int handleMenuCommands(WPARAM wParam, LPARAM lParam)
{
	const int REDUCE_TRI_PERCENT = 5;	// when page up/page down, inc/dec # tris by this percent 
										// (percent applies to # of tris in *original* mesh.)
	const int NUM_PAGEUPDN_INTERVALS = 100/REDUCE_TRI_PERCENT;

	switch (LOWORD(wParam)) 
	{
		case IDM_FILE_EXIT: 
		{
			SendMessage (g_pWindow->getHWnd(), WM_CLOSE, wParam, lParam) ;
			return 0 ;
		}

		case IDM_FILE_OPEN:
		{
			loadMesh();
			InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			return 0;
		}
		case IDM_UPDATE_ADDONETRI:  // split vertex
		{
			if (g_pProgMesh)
			{
				bool ret = g_pProgMesh->splitVertex();
				if (!ret) MessageBeep(0);
				g_pWindow->displayWindowTitle();
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}
		case IDM_UPDATE_REMOVEONETRI: // collapse vertex
		{
			if (g_pProgMesh)
			{
				bool ret = g_pProgMesh->collapseEdge();
				if (!ret) MessageBeep(0);
				g_pWindow->displayWindowTitle();
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}
		case IDM_UPDATE_INCREASETRI5PERC: // split vertices
		{
			if (g_pProgMesh)
			{
				int size = (g_pProgMesh->numEdgeCollapses()) / NUM_PAGEUPDN_INTERVALS;
				if (size == 0) size = 1;
				bool ret = true;
				for (int i = 0; ret && i < size; ++i) {
					 ret = g_pProgMesh->splitVertex();
				}
				if (!ret) MessageBeep(0);
				g_pWindow->displayWindowTitle();
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}
		case IDM_UPDATE_REDUCETRI5PERC: // collapse vertices
		{
			if (g_pProgMesh)
			{
				int size = (g_pProgMesh->numEdgeCollapses()) / NUM_PAGEUPDN_INTERVALS;
				if (size == 0) size = 1;
				bool ret = true;
				for (int i = 0; ret && i < size; ++i) {
					 ret = g_pProgMesh->collapseEdge();
				}
				if (!ret) MessageBeep(0);
				g_pWindow->displayWindowTitle();
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}

		case IDM_METHOD_QUADRIC:
		{
			changeSimplificationAlgorithm("Quadric", PMesh::QUADRIC);
			return 0;
		}

		case IDM_METHOD_QUADRICTRI:
		{
			changeSimplificationAlgorithm("Quadric Weighted by Triangle Area", PMesh::QUADRICTRI );
			return 0;
		}

		case IDM_METHOD_MELAX:
		{
			changeSimplificationAlgorithm("Melax", PMesh::MELAX);
			return 0;
		}

		case IDM_METHOD_SHORTEST:
		{
			changeSimplificationAlgorithm("Shortest Edge", PMesh::SHORTEST);
			return 0;
		}

		case IDM_FULLSCREEN_FULLSCREEN_640X480:
		{
			// Set settings for new display mode
			g_pWindow->flipFullScreen(640, 480);
			InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			return 0;
		}
		case IDM_FULLSCREEN_FULLSCREEN_800X600:
		{
			// Set settings for new display mode
			g_pWindow->flipFullScreen(800, 600);
			InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			return 0;
		}
		case IDM_FULLSCREEN_FULLSCREEN_1024X768:
		{
			// Set settings for new display mode
			g_pWindow->flipFullScreen(1024, 768);
			InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			return 0;
		}
		case IDM_FULLSCREEN_FULLSCREEN_1280X1024:
		{
			// Set settings for new display mode
			g_pWindow->flipFullScreen(1280, 1024);
			InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			return 0;
		}
		case IDM_FULLSCREEN_FULLSCREEN_1600X1200:
		{
			// Set settings for new display mode
			g_pWindow->flipFullScreen(1600, 1200);
			InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			return 0;
		}
		case IDM_FILL_FILLED:
		{
			if (!g_pWindow->isFillTriMode())
			{
				g_pWindow->setFillTriMode(true);
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}
		case IDM_FILL_WIREFRAME:
		{
			if (g_pWindow->isFillTriMode())
			{
				g_pWindow->setFillTriMode(false);
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}
		case IDM_SHADING_FLATSHADING:
		{
			if (g_pWindow->isSmoothShadingMode())
			{
				g_pWindow->setSmoothShadingMode(false);
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}
		case IDM_SHADING_SMOOTHSHADING:
		{
			if (!g_pWindow->isSmoothShadingMode())
			{
				g_pWindow->setSmoothShadingMode(true);
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}
		case IDM_HELP_ABOUT:
		{
			DialogBox( g_hInstance, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), g_pWindow->getHWnd(), (DLGPROC)
				aboutDlgProc );
			return 0;
		}
		default:
		{
			break;
		}
	}
	return 0;
}

// The popup menu is coming down.  Find out if menu items
// are currently set or reset, and gray out the inappropriate items.
void handleInitMenuPopup(WPARAM wParam, int menu)
{
	if (menu == SIMPLICATION_MENU) 
	{
		switch(g_edgemethod)
		{
		case PMesh::QUADRICTRI:
		{
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_QUADRICTRI, MF_CHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_QUADRIC, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_MELAX, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_SHORTEST, MF_UNCHECKED) ;
			break;
		}
		case PMesh::QUADRIC:
		{
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_QUADRICTRI, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_QUADRIC, MF_CHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_MELAX, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_SHORTEST, MF_UNCHECKED) ;
			break;
		}
		case PMesh::MELAX:
		{
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_QUADRICTRI, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_QUADRIC, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_MELAX, MF_CHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_SHORTEST, MF_UNCHECKED) ;
			break;
		}
		case PMesh::SHORTEST:
		{
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_QUADRICTRI, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_QUADRIC, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_MELAX, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_METHOD_SHORTEST, MF_CHECKED) ;
			break;
		}
		}
	}
	else if (menu == UPDATE_MENU)
	{
		if (g_pMesh)
		{
			EnableMenuItem ((HMENU)wParam,IDM_UPDATE_REDUCETRI5PERC, MF_ENABLED) ;
			EnableMenuItem ((HMENU)wParam,IDM_UPDATE_INCREASETRI5PERC, MF_ENABLED) ;
			EnableMenuItem ((HMENU)wParam,IDM_UPDATE_REMOVEONETRI, MF_ENABLED) ;
			EnableMenuItem ((HMENU)wParam,IDM_UPDATE_ADDONETRI, MF_ENABLED) ;
		}
		else
		{
			EnableMenuItem ((HMENU)wParam,IDM_UPDATE_REDUCETRI5PERC, MF_GRAYED) ;
			EnableMenuItem ((HMENU)wParam,IDM_UPDATE_INCREASETRI5PERC, MF_GRAYED) ;
			EnableMenuItem ((HMENU)wParam,IDM_UPDATE_REMOVEONETRI, MF_GRAYED) ;
			EnableMenuItem ((HMENU)wParam,IDM_UPDATE_ADDONETRI, MF_GRAYED) ;
		}
	}
	else if (menu == FILL_MENU)
	{
		if (g_pWindow->isFillTriMode())
		{
			CheckMenuItem ((HMENU)wParam,IDM_FILL_WIREFRAME, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_FILL_FILLED, MF_CHECKED) ;
		}
		else
		{
			CheckMenuItem ((HMENU)wParam,IDM_FILL_WIREFRAME, MF_CHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_FILL_FILLED, MF_UNCHECKED) ;
		}
	}
	else if (menu == SHADING_MENU)
	{
		if (g_pWindow->isSmoothShadingMode())
		{
			CheckMenuItem ((HMENU)wParam,IDM_SHADING_FLATSHADING, MF_UNCHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_SHADING_SMOOTHSHADING, MF_CHECKED) ;
		}
		else
		{
			CheckMenuItem ((HMENU)wParam,IDM_SHADING_FLATSHADING, MF_CHECKED) ;
			CheckMenuItem ((HMENU)wParam,IDM_SHADING_SMOOTHSHADING, MF_UNCHECKED) ;
		}
	}
}


// Handles Windows Messages
LRESULT CALLBACK wndProc(HWND hWnd,	UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_MONITORPOWER: // Prevent screen saver, monitor from	going into Power Save mode
				case SC_SCREENSAVE:
				return 0;
			}
			break;
		}

		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			SetCapture(g_pWindow->getHWnd()); // capture mouse outside client area
			g_pWindow->setNewXY(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}

		case WM_LBUTTONUP:
		{
			ReleaseCapture(); // stop capturing mouse outside client area
			return 0;
		}

		case WM_RBUTTONDOWN:
		{
			SetCapture(g_pWindow->getHWnd()); // capture mouse outside client area
			g_pWindow->setNewXY(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}

		case WM_RBUTTONUP:
		{
			ReleaseCapture(); // stop capturing mouse outside client area
			return 0;
		}

		case WM_MOUSEMOVE:
		{
			if ((wParam & MK_LBUTTON) || (wParam & MK_RBUTTON)) // if left or right button pressed
			{
				bool leftButton = (wParam & MK_LBUTTON) ? true : false;
				bool rightButton = (wParam & MK_RBUTTON) ? true : false;;
				g_pWindow->mouseMotion(LOWORD(lParam), HIWORD(lParam), leftButton, rightButton);
				InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
			}
			return 0;
		}

		case WM_PAINT:
		{
			static PAINTSTRUCT ps;

			BeginPaint(g_pWindow->getHWnd(), &ps);
			g_pWindow->displayMesh();
			EndPaint(g_pWindow->getHWnd(), &ps);
			return 0;
		}

		case WM_SIZE:
		{
			g_pWindow->reSizeScene(LOWORD(lParam),HIWORD(lParam));
			return 0;
		}
		case WM_COMMAND:
	  	{
			handleMenuCommands(wParam, lParam);
			return 0;
		}
		case WM_CHAR:
				// handle keyboard input 
				switch ((int)wParam) {
				case VK_ESCAPE: // fall through
				case VK_SPACE:  // fall through
				case VK_RETURN: // fall through
					if (g_pWindow->isFullScreen())
					{
						// exit Full screen mode
						g_pWindow->flipFullScreen(0, 0);
						InvalidateRect(g_pWindow->getHWnd(), NULL, TRUE);
					}
					return 0;
				default:
					break;
				}

		case WM_INITMENUPOPUP:
		{
			// Deal with menu initialization
			handleInitMenuPopup(wParam, LOWORD(lParam));
			return 0;
		}
		default:
		{
			break;
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

// Handle the "About" Dialog
LRESULT CALLBACK aboutDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM  )
{
   switch( uMsg ) {
      case WM_INITDIALOG:
         return true;
      case WM_COMMAND:
         switch( wParam ) {
            case IDOK:
               EndDialog( hDlg, TRUE );
               return true;
         }
      break;
   }
   return false;
}

// Main function for executable
int PASCAL WinMain(HINSTANCE  hInst,HINSTANCE  ,LPSTR  ,int  )
{
	MSG msg;
	HACCEL hAccel; /* Keyboard accelerators */

	g_hInstance	= hInst;

	/* Load keyboard shortcuts */
	hAccel = LoadAccelerators(g_hInstance, "MAINACCEL");

	bool bFullScreen = false;

	int width = 640; // initial width
	int height = 480; // initial height
	unsigned char depth = 16; // 16 bit color

	// Create Window
	g_pWindow = new glModelWindow();
	if (!g_pWindow || !g_pWindow->createMyWindow(width,height,depth,bFullScreen))
	{
		return 0;
	}

	// We don't use PeekMessage here since this is not an interactive
	// game.  Framerate is not crucial.
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(g_pWindow->getHWnd(), hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Shutdown
	g_pWindow->killMyWindow();

	delete g_pProgMesh; // this is here to keep Boundschecker happy
	delete g_pMesh;

	return (msg.wParam);
}
