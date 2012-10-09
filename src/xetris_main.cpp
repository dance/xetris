/* this is the mix of lcc (previous version) and MS VC 6 and Code::Blocks so don't be afraid :).
Всего по чуть-чуть*/
/*
xetris needs libraries to include: user32.lib gdi32.lib ddraw.lib dinput.lib
dinput8.lib
*/
/*<---------------------------------------------------------------------->*/
#define INITGUID
#define WIN32_LEAN_AND_MEAN

#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof ddstruct); \
											ddstruct.dwSize = sizeof ddstruct; }
#define DIKEYDOWN(data,n) (data[(n)] & 0x80)
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
/*<---------------------------------------------------------------------->*/
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

#include <objbase.h>
#include <ddraw.h>
#include <dinput.h>
#include <commdlg.h>
#include "ftime.h"
#include "targa.h"
#include "../resource.h"
/***********************************************/
#define DM_WIDTH		640
#define DM_HEIGHT		480
#define	DM_BPP			16
#define DM_REFRESHRATE	0

#define ERROR_STRING	"ERROR"
#define GAME_NAMESTR	"Xetris 1.0"

// FPS
#define MAXFPS_GAME			85
#define MAXFPS_MENU			12

// game states
#define GS_MENU				0
#define GS_PLAYING			1
#define GS_EXIT				2
#define GS_INITLEVEL		3
#define GS_GAMEOVER			4

// menu active items
#define MS_CONTINUE			0
#define MS_NEWGAME			1
#define MS_EXIT				2

// save\load dialog defines
#define DLG_NO				0
#define DLG_LOAD			1
#define DLG_SAVE			2

// game map dimensions
#define MAP_X					12
#define MAP_Y					15

// game pauses
#define FFDELAY_START			0.9			// start delay
#define FFDELAY_DELTA			0.025		// FF - falling figures, default 0.025
#define SUB_DELAY_AFTER			3			// subtract delay with FFDELAY_DELTA every SUB_DELAY_AFTER burned lines
#define FFDELAY_MIN				0.05		// minimum delay

// keyboard
#define KBD_DELAY					0.1

// burned line cost
#define SCORE_INC					100
// burned lines coef.
#define SCORE_INC_COEF				0.25

#define ALPHA_COEF				0.00005

// figures, initialized arrays
// figures 2x2:
// cube 2x2 - figure 1
int	f1 [2][2] = { {1,1},
                  {1,1} };
// figures 3x3
// figure 2
int f2_hor [3][3] = { {0,1,1},
                      {1,1,0},
                      {0,0,0} };
int f2_ver [3][3] = { {1,0,0},
                      {1,1,0},
                      {0,1,0} };
// figure 3
int f3_1 [3][3] = { {1,0,0},
                    {1,0,0},
                    {1,1,0} };
int f3_2 [3][3] = { {0,0,0},
                    {0,0,1},
                    {1,1,1} };
int f3_3 [3][3] = { {0,1,1},
                    {0,0,1},
                    {0,0,1} };
int f3_4 [3][3] = { {1,1,1},
                    {1,0,0},
                    {0,0,0} };
// figure 4
int f4_1 [3][3] = { {1,0,0},
                    {1,1,0},
                    {1,0,0} };
int f4_2 [3][3] = { {0,0,0},
                    {0,1,0},
                    {1,1,1} };
int f4_3 [3][3] = { {0,0,1},
                    {0,1,1},
                    {0,0,1} };
int f4_4 [3][3] = { {1,1,1},
                    {0,1,0},
                    {0,0,0} };

// figure 6, invert 3
int f6_1 [3][3] = { {0,0,1},
					{0,0,1},
					{0,1,1} };
int f6_2 [3][3] = { {1,1,1},
					{0,0,1},
					{0,0,0} };
int f6_3 [3][3] = { {1,1,0},
					{1,0,0},
					{1,0,0} };
int f6_4 [3][3] = { {0,0,0},
					{1,0,0},
					{1,1,1} };

// figure 7, invert 2
int f7_hor [3][3] = { {1,1,0},
                      {0,1,1},
                      {0,0,0} };
int f7_ver [3][3] = { {0,0,1},
                      {0,1,1},
                      {0,1,0} };
                    
// figures 4x4
// figure 5
int f5_ver [4][4] = { {0,1,0,0},
                      {0,1,0,0},
                      {0,1,0,0},
                      {0,1,0,0} };
int f5_hor [4][4] = { {0,0,0,0},
                      {0,0,0,0},
                      {1,1,1,1},
                      {0,0,0,0} };

/*<---------------------------------------------------------------------->*/
HINSTANCE hInst;		// Instance handle
HWND hwndMain;		//Main window handle
HWND g_hWndDlg = NULL;	// save\load dialog window handle

/**************** MY GLOBALS ****************************************/
LPDIRECTDRAW7			lpdd		 = NULL;		// main direct draw object
LPDIRECTDRAWSURFACE7	lpddsprimary = NULL,	// primary surface
						lpddsback	 = NULL;		// back buffer surface
LPDIRECTINPUT8			lpdi		 = NULL;	// main dinput object
LPDIRECTINPUTDEVICE8	lpdikbd		 = NULL;	// pointer to keyboard

LPDIRECTDRAWSURFACE7	lpddspic	= NULL;	// surface to draw alpha-blend picture to

bool g_bActive = true;	// is our app active?

// used for keyboard input
UCHAR	keystate[256];
int		up_pressed, down_pressed, right_pressed, left_pressed;	// are they pressed
double	up_start_time, down_start_time, left_start_time, right_start_time;	// start times
double	up_time, down_time, left_time, right_time;	 // how long they are pressed

int		can_rotate, move_left, move_right, fall_down;	// needed actions

DDSURFACEDESC2		ddsd;		// usually used for creating surfaces, temporary

// time vars
double 	old_time,
cur_time,
g_fps;

// game state
int game_state = GS_MENU;
int game_over = 0;
// active menu item
int menu_state = MS_NEWGAME;
// is save\load dialog active?
int game_saveload = DLG_NO;

int continue_enabled = 0;	// can we continue the game?

char buffer[256];	// used for printing messages

// background
TGA_Image		back_menu,		// menu background 
				back_game,		// game background 
				i_continue,	// continue item
				i_newgame,	// new game item
				i_exit,		// exit item
				i_arrow_l,	// two pointing arrows for menu items
				i_arrow_r;
TGA_Image		cube_big,	// big cube, part of figures
				cube_small;	// small cube, used in preview of next figure
TGA_Image		back_gamea;

// coordinates of left corner (images in main menu)
int		cont_left = 0,
		newgame_left = 0,
		exit_left = 0;

// maximum fps, for the game and for the menu
unsigned int maxfps = MAXFPS_MENU;

// GAME variables
int		score = 0;	// game score
int		map [ MAP_Y ] [ MAP_X ] = {0};	// map of the game, there will be stored:
// map[i][j] = 0 if this cell (i,j) is empty,
// 1 if this cell is occupied by piece of STATIC figure
// current figure is not marked

// i think that 4x4 will be sufficient
int		cur_fig	[4][4] = {0};		// map of the current figure
int		current_figure = 0;		// current figure, main number
int		current_figure_sub = 0;	// current figure, sub number
int		next_fig [4][4] = {0};	// map of the next figure
int		next_figure = 0;	// next figure, main number
int		cur_fig_dim = 0;	// current figure size: can be 2 (cube), 3 or 4
int		next_fig_dim = 0;	// next figure dimension
int		cur_fig_posx = 0;	// current figure position in RELATIVE coords in map
int		cur_fig_posy = 0;	// --=--

double		delay = 0;	// current figure delay
int			burned_lines = 0;	// number of burned lines, 0..SUB_DELAY_AFTER
double		figure_time = 0;	// used for calculate delay
double		start_figure_time = 0;	// --=--

double		kbd_time = 0;	// used to be sure that the keyboard rate is not too big
double		kbd_start_time = 0;	// --=--

// i think that all needed will be stored in this structure
struct GAMEDATA {
	int 		score;
	int			map [MAP_Y][MAP_X];
	int			current_figure;
	int			current_figure_sub;
	int			next_figure;
	int			next_fig_dim;
	int			cur_fig_posx;
	int			cur_fig_posy;
	int			burned_lines;
};

/***********************************/
LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK SaveLoadDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

/*************** MY FUNCTIONS **********/
int Game_Init();
int Game_Shutdown();
int Game_Main();

// draw targa on ddraw surface
int Draw_Targa(TGA_Image *tga,int x,int y,LPDIRECTDRAWSURFACE7 lpds,int transparent=0);
// draw targa on ddraw surface, use alpha transparency
// by default simply draws the picture (a = 0)
int Draw_TargaA(TGA_Image *tga,int x,int y,LPDIRECTDRAWSURFACE7 lpds,double a=1.0);
// draw text message
int Draw_Text(char *text,int x,int y,COLORREF color,LPDIRECTDRAWSURFACE7 lpdds);

// get keyboard state
void GetKbdInput();

// show and browse main menu
int MainMenu();

// game procedure
void Process_Game();

// set current or next figure
// next: 0 - set current figure,
//			 1 - set next figure.
// what_figure - [1,5] - what figure to set
void SetFigure(int next,int what_figure);

// update map - store current figure in map
int UpdateMap();

// check map
// returns 0 as default: nothing burned, still playing
// returns 1 if line burned
// returns 2 if game is over
int CheckMap();

// used to update map, set current and next figures
// let it be 1, maybe there will be some more Init's
// this function also checks map - if there are lines to burn it increases score and processes map
void Init1();

// move down, if succesful return 1 otherwise 0
int MoveDown();

// rotate current figure
// returns 1 if rotated successfully, otherwise 0
int Rotate();
// used in Rotate()
int CheckRotate(int *);
void CopyFigure(int *);

// read game data from save file
int ReadGameData(char *filename, GAMEDATA *);
// write game data to save file
int WriteGameData(char *filename, GAMEDATA *);

// save current game status in GAMEDATA
void StoreGameData(GAMEDATA *);
//load game status from GAMEDATA
void LoadGameData(GAMEDATA *);

// function used to get a filename from editBox text
void GetFilename( char ** );
// load list of files in save directory into listBox of our save/load dialog
void LoadFileList(HWND hDlg);

/*<---------------------------------------------------------------------->*/
static BOOL InitApplication(void)
{
    WNDCLASS wc;

    memset(&wc,0,sizeof(WNDCLASS));
    wc.style = CS_HREDRAW|CS_VREDRAW |CS_DBLCLKS ;
    wc.lpfnWndProc = (WNDPROC)MainWndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "lcc_dxWndClass";
    wc.lpszMenuName = NULL;
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);
    wc.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_APPICON));
    if (!RegisterClass(&wc))
        return 0;

    // ---TODO--- Call module specific initialization routines here

    return 1;
}

/*<---------------------------------------------------------------------->*/
HWND Createlcc_dxWndClassWnd(void)
{
    return CreateWindow("lcc_dxWndClass",GAME_NAMESTR,
                        WS_POPUP | WS_VISIBLE,
                        CW_USEDEFAULT,0,CW_USEDEFAULT,0,
                        NULL,
                        NULL,
                        hInst,
                        NULL);
}

/*<---------------------------------------------------------------------->*/
LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    
    switch (msg)
    {
		case WM_PAINT:
			hdc = BeginPaint(hwndMain,&ps);
			// flip surfaces (use blt 4 this because flipping does not show dialog boxes correctly)
			// why, i do not know
			if (lpddsprimary->Blt(NULL, lpddsback, NULL, DDBLT_WAIT, NULL) == DDERR_SURFACELOST)
				lpdd->RestoreAllSurfaces();
			EndPaint(hwndMain,&ps);
		break;

		case WM_SIZE:
            // Check to see if we are losing our window...
			if(SIZE_MINIMIZED == wParam)
                g_bActive = false;
		break;

		case WM_ACTIVATE:
			// window activated (maximized or any else)
			if ( wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
				g_bActive = true;
		break;
		
		case WM_DESTROY:
		case WM_CLOSE:
			PostQuitMessage(0);
			g_bActive = false;
		break;

		default:
			return DefWindowProc(hwnd,msg,wParam,lParam);
    }

    return 0;
}

/*<---------------------------------------------------------------------->*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    MSG msg;

    hInst = hInstance;
    if (!InitApplication())
        return 0;
    if ((hwndMain = Createlcc_dxWndClassWnd()) == (HWND)0)
        return 0;
    ShowWindow(hwndMain,SW_SHOW);

    if (!Game_Init())
    {
        Game_Shutdown();
    }

	ShowCursor(FALSE);	// hide cursor

    while (TRUE)
    {
        if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

			// Translate and dispatch the message. If the dialog is showing, 
            // translate messages for it since it's a modeless dialog.
            if( g_hWndDlg == NULL || !IsDialogMessage( g_hWndDlg, &msg ) )
            {
				TranslateMessage( &msg ); 
                DispatchMessage( &msg );
            }
        }
		else // we have no messages
		{
			if ( g_bActive )	// game is active, process it
				Game_Main();
			else
				// Make sure we go to sleep if we have nothing else to do
                WaitMessage();
		}
        
    }

    Game_Shutdown();

	ShowCursor(TRUE);	// show cursor

    return msg.wParam;
}

/***********************************************************************/
// load list of files in save directory into listBox of our save/load dialog
void LoadFileList(HWND hDlg)
{
	// clear list box
	SendDlgItemMessage(hDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);

	// load file list ( in '/save' directory)
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	// Find the first file in the directory.
	hFind = FindFirstFile("./save/*.xsv", &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
		return;
	else 
	{
		int len = strlen(FindFileData.cFileName);
		char *str = (char *)malloc(len+1);
		strcpy_s(str, len+1, FindFileData.cFileName);
		str[len-4] = 0;
		// add first save name
		SendDlgItemMessage(hDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM) str);
		free(str);

		// List all the other files in the directory.
		while (FindNextFile(hFind, &FindFileData) != 0) 
		{ 
			len = strlen(FindFileData.cFileName);
			str = (char *)malloc(len+1);
			strcpy_s(str, len+1, FindFileData.cFileName);
			str[len-4] = 0;
			// add next save name
			SendDlgItemMessage(hDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM) str);
			free(str);
		}
	}
	FindClose(hFind);

	return;
}

// function used in next 2 functions to get a filename from editBox text
void GetFilename( char **filename )
{
	// get length of string
	int len = (WORD) SendDlgItemMessage( g_hWndDlg, IDC_EDIT, EM_LINELENGTH, (WPARAM) 0, (LPARAM) 0);
	if ( len == 0 )	{ // empty string
		*filename = NULL;
		return;
	}
	char *str = (char *)malloc(len+1);	// alloc mem
	*((LPWORD)str) = len;	// store buff length to use next func properly
	SendDlgItemMessage( g_hWndDlg, IDC_EDIT, EM_GETLINE, 0, (LPARAM)str);	// get text
	str[len] = 0;	// Null-terminate the string. 
	*filename = (char *)malloc(45);
	sprintf_s(*filename, 45, "./save/%s.xsv\0", str);
	free(str);

	return;
}

// SAVE or LOAD game dialog
INT_PTR CALLBACK SaveLoadDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch (msg)
    {
        case WM_INITDIALOG:
			// set caption
			if (game_saveload == DLG_LOAD)
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM) "Загрузить игру");
			else	// DLG_SAVE
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM) "Сохранить игру");

			// load file list (saves)
			LoadFileList(hDlg);
			
			// set focus
			SetFocus(GetDlgItem(hDlg, IDC_LIST));

            return TRUE;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
				case IDC_LIST:
					if ( HIWORD(wParam) == LBN_SELCHANGE ) {	// selection changed

						HWND hwndList = GetDlgItem(hDlg, IDC_LIST); 

                        // Get selected index.
                        int nItem = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);

						int len = SendMessage(hwndList, LB_GETTEXTLEN, nItem, (LPARAM) 0);
						LPSTR str = (char *) malloc (len+1);
						SendMessage(hwndList, LB_GETTEXT, nItem, (LPARAM) str);	// get selected string

						//copy name of selected item in edit box
						SendDlgItemMessage(hDlg, IDC_EDIT, WM_SETTEXT, 0, (LPARAM) str); 

						// get save score
						GAMEDATA gamed;

						free(str);	// we will realloc mem in GetFilename
						GetFilename(&str);	// get filename
						if (ReadGameData(str,&gamed))	//read game data
						{
							char *sc = (char *)malloc(7);
							sprintf_s(sc, 7, "%d", gamed.score);
							SendDlgItemMessage(hDlg, IDC_SCORE, WM_SETTEXT, 0, (LPARAM) sc);
							free(sc);
						}

						free(str);
					}
				break;

				case IDBTNDEL:
					// get length of string
					int len;
					char *str;
					len = (WORD) SendDlgItemMessage( hDlg, IDC_EDIT, EM_LINELENGTH, (WPARAM) 0, (LPARAM) 0);
					str = (char *)malloc(len+1);	// alloc mem
					*((LPWORD)str) = len;	// store buff length to use next func properly
					SendDlgItemMessage( hDlg, IDC_EDIT, EM_GETLINE, 0, (LPARAM)str);	// get text
					str[len] = 0;	// Null-terminate the string. 
					// make question
					char *str1;
					str1 = (char *)malloc(len+1+60);
					sprintf_s(str1, len+1+59 , "Вы собираетесь удалить сохранение '%s', вы уверены?", str);
					// confirm
					if ( IDOK == MessageBox(hDlg, str1, "Удаление", MB_OKCANCEL | MB_ICONQUESTION) )	{
						free(str);
						GetFilename(&str);
						// delete file
						if ( !DeleteFile(str) )	{
							// error occured
							MessageBox(hDlg, "Невозможно удалить файл", ERROR_STRING, MB_ICONERROR);
						}
						// reload list
						LoadFileList(hDlg);
					}

					free(str);
					free(str1);
				break;

                case IDCANCEL:	// game NOT loaded or saved
					DestroyWindow(hDlg);	// close dlg
					game_saveload = DLG_NO;	// dialog is closed
					g_bActive = 1;	// let game run
					return TRUE;

                case IDOK:
					// we have to load or save game, it's a lot of work ;)

					// make filename from editbox text
					char *filename = NULL;
					GetFilename(&filename);

					if (filename == NULL)	{	// EMPTY string
						// cannot load or save, name is not specified
						MessageBox(hDlg, "Введите имя сохранения", ERROR_STRING, MB_ICONINFORMATION);
						free(filename);
						return FALSE;
					}
					
					GAMEDATA gamed;
					if (game_saveload == DLG_LOAD)	{	// read & load game data
						if (ReadGameData(filename, &gamed))
							LoadGameData(&gamed);
					}
					else	{	// store in GAMEDATA and write it to file
						StoreGameData(&gamed);
						WriteGameData(filename, &gamed);
					}
					// release mem
					free(filename);
					// close dialog
                    DestroyWindow(hDlg);
					game_saveload = DLG_NO;	// dialog is closed
					g_bActive = 1;	// let game run
                    return TRUE;					
            }
            break;

        case WM_MOVE:
            // The window is moving around, so re-draw the backbuffer
			MainMenu();
			// flip surfaces (use blt 4 this because flipping does not show dialog boxes correctly)
			// why, i do not know
			if (lpddsprimary->Blt(NULL, lpddsback, NULL, DDBLT_WAIT, NULL) == DDERR_SURFACELOST) {
				lpdd->RestoreAllSurfaces();
			}
		break;

		case WM_CLOSE:
        case WM_DESTROY:
            g_hWndDlg = NULL;
			// hide cursor
			ShowCursor(FALSE);
			return TRUE;
    }

	return FALSE;
}

/*******	GAME_INIT	*********************/
int	Game_Init()
{
    // section 1
    // создаем Direct Draw object
    if (FAILED(DirectDrawCreateEx(NULL,(void **)&lpdd,IID_IDirectDraw7,NULL)))
    {
        MessageBox(hwndMain,"Error creating Direct Draw!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }

    // set fullscreen exclusive mode
    if (FAILED(lpdd->SetCooperativeLevel(hwndMain, DDSCL_EXCLUSIVE |
                                         DDSCL_FULLSCREEN |
                                         DDSCL_ALLOWREBOOT |
                                         DDSCL_ALLOWMODEX)))
    {
        MessageBox(hwndMain,"Error Setting Cooperative Level!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }

    // set display mode
    if (FAILED(lpdd->SetDisplayMode(DM_WIDTH, DM_HEIGHT, DM_BPP, DM_REFRESHRATE, 0)))
    {
        MessageBox(hwndMain,"Error Setting Display Mode!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }

    // section 2
    // create surfaces
    memset(&ddsd,0,sizeof ddsd);	// zero memory
    ddsd.dwSize = sizeof ddsd;	// size of structure
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;	// correct fields
    ddsd.dwBackBufferCount = 1;	// 1 back buffer
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |	// primary surface
                          DDSCAPS_COMPLEX |		// complex surface with a back buffer
                          //DDSCAPS_MODEX |			// support mode x
                          DDSCAPS_FLIP;				// with page flipping
    // create primary surface
    if (FAILED(lpdd->CreateSurface(&ddsd,&lpddsprimary,NULL)))
    {
        MessageBox(hwndMain,"Error Creating Primary Surface!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }

	ZeroMemory( &ddsd.ddsCaps, sizeof( ddsd.ddsCaps ) );
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;	// back buffer
    
    // create back buffer
    if (FAILED(lpddsprimary->GetAttachedSurface(&ddsd.ddsCaps,&lpddsback)))
    {
        MessageBox(hwndMain,"Error Creating Back Buffer!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }

	// create surface for alpha-blended picture
	DDRAW_INIT_STRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwHeight = 479;
	ddsd.dwWidth = 384;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	if (FAILED(lpdd->CreateSurface(&ddsd, &lpddspic, NULL)))	{
		MessageBox(hwndMain,"Error Creating pic surface!",ERROR_STRING,MB_ICONERROR);
        return 0;
	}
	// clear it
	DDBLTFX ddbltfx;
	DDRAW_INIT_STRUCT(ddbltfx);
	ddbltfx.dwFillColor = 0;
	lpddspic->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
	
	// Check if the device supports DDCAPS2_CANRENDERWINDOWED.  
    // If it does, then it supports GDI writing directly to the primary surface
	// otherwise quit with error
	DDCAPS ddcaps;
    ZeroMemory( &ddcaps, sizeof(ddcaps) );
    ddcaps.dwSize = sizeof(ddcaps);
    lpdd->GetCaps( &ddcaps, NULL );
    if( (ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED) == 0 )
    {
        MessageBox( hwndMain, "This display card can not render GDI.",
                    ERROR_STRING, MB_ICONERROR | MB_OK );
        return 0;
    }

	// Create a clipper when using GDI to draw on the primary surface
	LPDIRECTDRAWCLIPPER pClipper;

    if( FAILED( lpdd->CreateClipper( 0, &pClipper, NULL ) ) )
        return 0;

    pClipper->SetHWnd( 0, hwndMain );

    if( FAILED( lpddsprimary->SetClipper( pClipper ) ) )
        return 0;

    // We can release the clipper now since lpddsprimary 
    // now maintains a ref count on the clipper
    SAFE_RELEASE( pClipper );

    // section 3
    // initialize time system
    if (!initftime())
    {
        MessageBox(hwndMain,"Error Initializing time system!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }

    // section 4
    // init direct input
    if (FAILED(DirectInput8Create(hInst,DIRECTINPUT_VERSION,IID_IDirectInput8,(void **)&lpdi,NULL)))
    {
        MessageBox(hwndMain,"Error initializing direct input!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }
    if (FAILED(lpdi->CreateDevice(GUID_SysKeyboard,&lpdikbd,NULL)))
    {
        MessageBox(hwndMain,"Error initializing direct input device!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }
    if (FAILED(lpdikbd->SetCooperativeLevel(hwndMain,DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)))
    {
        MessageBox(hwndMain,"Error set cooperative level in lpdikbd!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }
    if (FAILED(lpdikbd->SetDataFormat(&c_dfDIKeyboard)))
    {
        MessageBox(hwndMain,"Error set data format in lpdikbd!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }
    if (FAILED(lpdikbd->Acquire()))
    {
        MessageBox(hwndMain,"Error lpdikbd Acquire!",ERROR_STRING,MB_ICONERROR);
        return 0;
    }

    // other
    // load game object images
    if(TGA_Load_Image(&back_game,"images\\game_back.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading game background!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }

    // load images used in menu
    back_menu.buffer = NULL;
    if(TGA_Load_Image(&back_menu,"images\\menu_back.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading menu background!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }
    i_continue.buffer = NULL;
    if(TGA_Load_Image(&i_continue,"images\\continue.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading continue item!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }
    i_newgame.buffer = NULL;
    if(TGA_Load_Image(&i_newgame,"images\\newgame.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading newgame item!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }
    i_exit.buffer = NULL;
    if(TGA_Load_Image(&i_exit,"images\\exit.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading exit item!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }
    i_arrow_l.buffer = NULL;
    if(TGA_Load_Image(&i_arrow_l,"images\\arrow_l.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading arrow_l item!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }
    i_arrow_r.buffer = NULL;
    if(TGA_Load_Image(&i_arrow_r,"images\\arrow_r.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading arrow_r item!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }
    cube_big.buffer = NULL;
    if(TGA_Load_Image(&cube_big,"images\\cube_big.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading cube_big item!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }
    cube_small.buffer = NULL;
    if(TGA_Load_Image(&cube_small,"images\\cube_small.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading cube_small item!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }
	// load my precious :)
	if(TGA_Load_Image(&back_gamea,"images\\myprecious.tga")!=TLI_OK)
    {
        MessageBox(hwndMain,"Error loading myprecious item!",ERROR_STRING,MB_ICONERROR);
		return 0;
    }

    // calculate coordinates
    cont_left = (DM_WIDTH / 2) - (i_continue.Width / 2);
    newgame_left = (DM_WIDTH / 2) - (i_newgame.Width / 2);
    exit_left = (DM_WIDTH / 2) - (i_exit.Width / 2);

    old_time = ftime();

    return 1;
}

/*******	GAME_SHUTDOWN	*****************/
int Game_Shutdown()
{
	// release pic surface
	SAFE_RELEASE(lpddspic);

    // release main ddraw object
	SAFE_RELEASE(lpdd);

    // release keyboard
    if (lpdikbd)
    {
        lpdikbd->Unacquire();
        lpdikbd->Release();
    }
    // release dinput
    SAFE_RELEASE(lpdi);

    TGA_Unload_Image(&back_menu);
    TGA_Unload_Image(&i_continue);
	TGA_Unload_Image(&i_newgame);
	TGA_Unload_Image(&i_exit);
	TGA_Unload_Image(&i_arrow_l);
	TGA_Unload_Image(&i_arrow_r);
	TGA_Unload_Image(&back_game);
	TGA_Unload_Image(&cube_big);
	TGA_Unload_Image(&cube_small);
	TGA_Unload_Image(&back_gamea);

    return 1;
}

/*******	GAME_MAIN	******************/
int Game_Main()
{
    // get input
    GetKbdInput();

	int i=0, j=0; 	// for loops
    // get current time
    cur_time = ftime();
    // calculate fps
    g_fps = 1.0 / (cur_time - old_time);

    // test ESCAPE, or if gamer try to save/load
    if (DIKEYDOWN(keystate,DIK_ESCAPE) || DIKEYDOWN(keystate,DIK_F3) || DIKEYDOWN(keystate,DIK_F4))
    {
		game_state = GS_MENU;
        maxfps = MAXFPS_MENU;
    }

    srand((UINT)ftime());

    // check game_state
    switch (game_state)
    {

    case  GS_MENU:
        // call main menu func
        MainMenu();
        break;

    case GS_PLAYING:
        // call game procedure
        Process_Game();
        break;

    case GS_INITLEVEL:
        game_state = GS_PLAYING;
        // init game variables:
        score = 0;
        continue_enabled = 1;
        memset(map,0,MAP_X*MAP_Y*sizeof(int));
        current_figure = rand()%7+1;
        next_figure = rand()%7+1;
        SetFigure(0,current_figure);	// set current figure
        SetFigure(1,next_figure);	// set next figure
        delay = FFDELAY_START;
        burned_lines = 0;
        start_figure_time = ftime();
        kbd_start_time = ftime();
        game_over = 0;
		// clear our surface
		DDBLTFX ddbltfx;
		DDRAW_INIT_STRUCT(ddbltfx);
		ddbltfx.dwFillColor = 0;
		lpddspic->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
        break;

    case GS_EXIT:
        /* just do nothing */
        break;

    case GS_GAMEOVER:
		// draw background
		Draw_Targa(&back_game,0,0,lpddsback);

		// draw score
		sprintf_s(buffer,"%d",score);
		Draw_Text(buffer,560,36,RGB(100,0,0),lpddsback);

		// draw next figure
		for (i=0; i<next_fig_dim; i++)
			for (j=0; j<next_fig_dim; j++)
				if (next_fig[i][j]!=0)
					Draw_Targa(&cube_small,j*16 + 560,i*16 + 220,lpddsback);
		// draw map
		for (i=0; i<MAP_Y; i++)
			for (j=0; j<MAP_X; j++)
				if (map[i][j]!=0)
					Draw_Targa(&cube_big,j*32 + 128,i*32,lpddsback);
		
        Draw_Text("GAME OVER",540,75,RGB(255,0,0),lpddsback);
        break;

    default:
        // unexpected value, error, exit
        Game_Shutdown();
        exit(EXIT_FAILURE);
    }

    // flip surfaces (use blt 4 this because flipping does not show dialog boxes correctly)
	// why, i do not know
	if (lpddsprimary->Blt(NULL, lpddsback, NULL, DDBLT_WAIT, NULL) == DDERR_SURFACELOST) {
		lpdd->RestoreAllSurfaces();
	}

    // sync fps
    while (g_fps > maxfps)
    {
        // get current time
        cur_time = ftime();
        // calculate fps
        g_fps = 1.0 / (cur_time - old_time);
    }

    // save clock value
    old_time = ftime();

    return 1;
}

/* ******************************************************************** */

// draw targa on ddraw surface
int Draw_Targa(TGA_Image *tga,int x,int y,LPDIRECTDRAWSURFACE7 lpdds,int transparent)
{
    DDRAW_INIT_STRUCT(ddsd);
    // lock surface for drawing
    lpdds->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL);
    // get pointer
    USHORT	*buf;
    buf = (USHORT *)ddsd.lpSurface;
    // valid pointer?
    if (!buf)
        return 0;

    USHORT *dest_addr,   // starting address of bitmap in destination
    *source_addr; // starting adddress of bitmap data in source
    USHORT pixel;
    int lpitch = ddsd.lPitch>>1;
    // compute starting destination address
    dest_addr = (buf + y*lpitch + x);

    // compute the starting source address
    source_addr = tga->buffer;

    // is this bitmap transparent
    if (transparent)
    {
        // copy each line of bitmap into destination with transparency
        for (int i=0; i<tga->Height; i++)
        {
            // copy the memory
            for (int k=0; k<tga->Width; k++)
            {
                if ((pixel = source_addr[k])!=0)
                    dest_addr[k] = pixel;

            } // end if

            // advance all the pointers
            dest_addr   += lpitch;
            source_addr += tga->Width;

        } // end for index
    } // end if
    else
    {
        // non-transparent version
        // copy each line of bitmap into destination

        for (int i=0; i < tga->Height; i++)
        {
            // copy the memory
            memcpy(dest_addr, source_addr, tga->Width*2);

            // advance all the pointers
            dest_addr   += lpitch;
            source_addr += tga->Width;

        } // end for index

    } // end else

    // unlock surface
    lpdds->Unlock(NULL);

    return 1;
}

/*******************************************************/
// draw targa on ddraw surface, use alpha transparency
// by default simply draws the picture (a = 0)
int Draw_TargaA(TGA_Image *tga,int x,int y,LPDIRECTDRAWSURFACE7 lpdds, double a)
{
	if (a >= 1.0)		// not transparent, just call old func
		return Draw_Targa(tga, x, y, lpdds);
	else if ( a == 0.0)
		return 1;

	// else process alpha-transp drawing
	DDRAW_INIT_STRUCT(ddsd);
    // lock surface for drawing
    lpdds->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL);
    // get pointer
    USHORT	*buf;
    buf = (USHORT *)ddsd.lpSurface;
    // valid pointer?
    if (!buf)
        return 0;

    USHORT *dest_addr,   // starting address of bitmap in destination
    *source_addr; // starting adddress of bitmap data in source
    int lpitch = ddsd.lPitch >> 1;
    // compute starting destination address
    dest_addr = (buf + y*lpitch + x);

    // compute the starting source address
    source_addr = tga->buffer;

	// process each pixel, we must compute color blending

	int al = (int)(a * 255);
	USHORT pix;
	int r,g,b;	// result RGB components
	int src_r, src_g, src_b;	// source

	for (int i = 0; i < tga->Height; i++)	{
		for (int j = 0; j < tga->Width; j++)	{

			pix = source_addr[j];
			GetRGB16Bit565(pix, src_r, src_g, src_b);

			r = (src_r * al) >> 8;
			g = (src_g * al) >> 8;
			b = (src_b * al) >> 8;

            dest_addr[j] = RGB16Bit565(r, g, b);

		}
		// advance all the pointers
        dest_addr   += lpitch;
        source_addr += tga->Width;
	}

	// unlock surface
    lpdds->Unlock(NULL);

	return 1;
}

/*******************************************/
// draw text
int Draw_Text(char *text,int x,int y,COLORREF color,LPDIRECTDRAWSURFACE7 lpdds)
{
    // function from Andre Lamoth. Thank you, Andre :)
    HDC xdc;
    if (FAILED(lpdds->GetDC(&xdc)))
        return 0;

    SetTextColor(xdc,color);
    SetBkMode(xdc,TRANSPARENT);
    TextOut(xdc,x,y,text,strlen(text));
    lpdds->ReleaseDC(xdc);

    return 1;
}

// ************************************************************
// get keyboard input
void GetKbdInput()
{
    HRESULT result;

    result = lpdikbd->GetDeviceState(sizeof keystate,(LPVOID)keystate);
    if (FAILED(result))	// serious error
    {
        MessageBox(hwndMain,"Error lpdikbd GetDeviceState!!!",ERROR_STRING,MB_ICONERROR);
        Game_Shutdown();
        exit(EXIT_FAILURE);
    }
}

// ******************************************************************
int MainMenu()
{
	// process key pressing if we have no active dialog
	if (game_saveload == DLG_NO)	{

    // check if ENTER is pressed
    if (DIKEYDOWN(keystate,DIK_RETURN))
    {
        if (menu_state == MS_CONTINUE)
        {
            game_state = GS_PLAYING;
            maxfps = MAXFPS_GAME;
        }
        else if (menu_state == MS_NEWGAME)
        {
            /*	some initial code there */
            game_state = GS_INITLEVEL;
            maxfps = MAXFPS_GAME;
        }
        else if (menu_state == MS_EXIT)
        {
            PostQuitMessage(0);
            game_state = GS_EXIT;
        }
        return 1;
    }

    // check arrows
    if (DIKEYDOWN(keystate,DIK_UP))
    {
        if (continue_enabled && !game_over)
            if (menu_state != MS_CONTINUE)
                menu_state -= 1;
            else
                ;
        else
            if (menu_state != MS_NEWGAME)
                menu_state -= 1;
    }
    else if (DIKEYDOWN(keystate,DIK_DOWN))
    {
        if (menu_state != MS_EXIT)
            menu_state += 1;
    }

	}	// if game_saveload = DLG_NO
    
    // draw images
    Draw_Targa(&back_menu,0,0,lpddsback);

    if (continue_enabled && !game_over)
        Draw_Targa(&i_continue,cont_left,160,lpddsback,1);

    Draw_Targa(&i_newgame,newgame_left,230,lpddsback,1);
    Draw_Targa(&i_exit,exit_left,300,lpddsback,1);

    Draw_Targa(&i_arrow_l,30,160 + 70*menu_state,lpddsback,1);
    Draw_Targa(&i_arrow_r,510,160 + 70*menu_state,lpddsback,1);
    
    // save if F4 is pressed, there is no active dialog and user played the game
	if (DIKEYDOWN(keystate,DIK_F4) && !game_saveload && continue_enabled)
    {
		game_saveload = DLG_SAVE;	// dialog activated
		// create dialog
		g_hWndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SAVELOADDLG), hwndMain, (DLGPROC)SaveLoadDlgProc);
		// show it
        ShowWindow( g_hWndDlg, SW_SHOWNORMAL );
		// show cursor
		ShowCursor(TRUE);
		// freeze game running
		g_bActive = 0;
    }
    
    // load
    if (DIKEYDOWN(keystate,DIK_F3) && !game_saveload)
    {
		game_saveload = DLG_LOAD;	// dialog activated
		// create dialog
		g_hWndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SAVELOADDLG), hwndMain, (DLGPROC)SaveLoadDlgProc);
		// show it
        ShowWindow( g_hWndDlg, SW_SHOWNORMAL );
		// show cursor
		ShowCursor(TRUE);
		// freeze game running
		g_bActive = 0;
    }

    return 1;
}

/* *****************************************************	*/
void Process_Game()
{
    int i,j;

    // i want improve input system a little
    GetKbdInput();
    // up arrow
    if (DIKEYDOWN(keystate,DIK_UP))
    {
        if (!up_pressed)
        { // just pressed
            up_pressed = 1;
            up_start_time = ftime();
            can_rotate = 1;
        }
        else
        { // still down
            up_time = cur_time - up_start_time;
            if (up_time > KBD_DELAY)
            {
                up_time = 0;
                up_start_time = ftime();
                can_rotate = 1;
            }
            else
                can_rotate = 0;
        }
    }
    else
    { // not down
        up_pressed = 0;
        can_rotate = 0;
    }
    // down arrow
    if (DIKEYDOWN(keystate,DIK_DOWN))
    {
		down_pressed = 1;
		fall_down = 1;
	}
    else
    { // not down
        down_pressed = 0;
        fall_down = 0;
    }
    // right arrow
    if (DIKEYDOWN(keystate,DIK_RIGHT))
    {
        if (!right_pressed)
        { // just pressed
            right_pressed = 1;
            right_start_time = ftime();
            move_right = 1;
            right_time = 0;
        }
        else
        { // still down
            right_time = cur_time - right_start_time;
            if (right_time > KBD_DELAY)
            {
                right_time = 0;
                right_start_time = ftime();
                move_right = 1;
            }
            else
                move_right = 0;
        }
    }
    else
    { // not down
        right_pressed = 0;
        move_right = 0;
    }
	// left arrow
    if (DIKEYDOWN(keystate,DIK_LEFT))
    {
        if (!left_pressed)
        { // just pressed
            left_pressed = 1;
            left_start_time = ftime();
            move_left = 1;
            left_time = 0;
        }
        else
        { // still down
            left_time = cur_time - left_start_time;
            if (left_time > KBD_DELAY)
            {
                left_time = 0;
                left_start_time = ftime();
                move_left = 1;
            }
            else
                move_left = 0;
        }
    }
    else
    { // not down
        left_pressed = 0;
        move_left = 0;
    }

    if (move_left)
    {	// move left
        if (cur_fig_posx > 0)
        {	// case 1
            int ok=1;
            for (i=0; i<cur_fig_dim; i++)
                for (j=0; j<cur_fig_dim; j++)
                    if (cur_fig[i][j] && map[cur_fig_posy+i][cur_fig_posx+j-1])
                    {
                        ok = 0;
                        break;
                    }
            if (ok)
                cur_fig_posx -= 1;
        }	// end if > 0, case 1
        else if (!cur_fig_posx)
        {	// case 2
            int ok=1;
            // check 1
            for (i=0; i<cur_fig_dim; i++)
                for (j=1; j<cur_fig_dim; j++)
                    if (cur_fig[i][j] && map[cur_fig_posy+i][cur_fig_posx+j-1])
                    {
                        ok = 0;
                        break;
                    }
            // check 2
            for (i=0; i<cur_fig_dim; i++)
                if (cur_fig[i][0])
                    ok = 0;
            // all ok, move left
            if (ok)
                cur_fig_posx -= 1;
        }
    } // end if move left

    if (move_right)
    {	// move right
        if (cur_fig_posx + cur_fig_dim < MAP_X)
        {
            int ok=1;
            for (i=0; i<cur_fig_dim; i++)
                for (j=0; j<cur_fig_dim; j++)
                    if (cur_fig[i][j] && map[cur_fig_posy+i][cur_fig_posx+j+1])
                    {
                        ok = 0;
                        break;
                    }
            if (ok)
                cur_fig_posx += 1;
        }
        else if (cur_fig_posx + cur_fig_dim == MAP_X)
        {
            int ok=1;
            // check 1
            for (i=0; i<cur_fig_dim; i++)
                for (j=0; j<cur_fig_dim-1; j++)
                    if (cur_fig[i][j] && map[cur_fig_posy+i][cur_fig_posx+j+1])
                    {
                        ok = 0;
                        break;
                    }
            // check 2
            for (i=0; i<cur_fig_dim; i++)
                if (cur_fig[i][cur_fig_dim-1])
                    ok = 0;
            if (ok)
                cur_fig_posx += 1;
        }
        else if (cur_fig_posx + cur_fig_dim - 1 == MAP_X)
        {	// only for 5 figure it's possible
            if (current_figure==5)
            {
                int ok=1;
                // check 1
                for (i=0; i<cur_fig_dim; i++)
                    for (j=0; j<cur_fig_dim-2; j++)
                        if (cur_fig[i][j] && map[cur_fig_posy+i][cur_fig_posx+j+1])
                        {
                            ok = 0;
                            break;
                        }
                // check 2 is not needed, skip it
                if (ok)
                    cur_fig_posx += 1;
            }
        }
    }	// end move right

    if (can_rotate)
    {
        // rotate current figure
        Rotate();
    }

    // current figure
    figure_time = cur_time - start_figure_time;
    if ((figure_time > delay) || (fall_down && (figure_time > FFDELAY_DELTA)))
    {
        figure_time = 0;
        start_figure_time = ftime();

        // try move down
        if (!MoveDown())
        { // impossible, init new figures
            if (UpdateMap())	// only if still playing exec Init1
                Init1();
        }
    }	// main if

    // draw background
    Draw_Targa(&back_game,0,0,lpddsback);

	// draw alpha-blended picture, it's on the surface
	RECT target;
	target.left = 128;
	target.top = 0;
	target.right = 512;
	target.bottom = DM_HEIGHT;
	if (FAILED(lpddsback->Blt(&target, lpddspic, NULL, DDBLT_WAIT, NULL)))
		lpdd->RestoreAllSurfaces();

    // draw score
    sprintf_s(buffer,"%d",score);
    Draw_Text(buffer,560,36,RGB(100,0,0),lpddsback);

    // draw current figure
    for (i=0; i<cur_fig_dim; i++)
        for (j=0; j<cur_fig_dim; j++)
            if (cur_fig[i][j]!=0)
                Draw_Targa(&cube_big,j*32 + cur_fig_posx*32 + 128,i*32 + cur_fig_posy*32,lpddsback);
    // draw next figure
    for (i=0; i<next_fig_dim; i++)
        for (j=0; j<next_fig_dim; j++)
            if (next_fig[i][j]!=0)
                Draw_Targa(&cube_small,j*16 + 560,i*16 + 220,lpddsback);
    // draw map
    for (i=0; i<MAP_Y; i++)
        for (j=0; j<MAP_X; j++)
            if (map[i][j]!=0)
                Draw_Targa(&cube_big,j*32 + 128,i*32,lpddsback);

}

/**************************************************	*/
// set figure
void SetFigure(int next,int what_figure)
{
    int i,j;	// loop vars
    int *array = NULL;	// points to needed array
    int *dim = NULL;	// points to needed dimension
    // what to set?
    if (!next)
    {
        array = &cur_fig[0][0];
        dim = &cur_fig_dim;
        cur_fig_posx = 5;
        cur_fig_posy = 0;
        current_figure_sub = 1;
    }
    else
    {
        array = &next_fig[0][0];
        dim = &next_fig_dim;
    }
    memset(array,0,4*4*sizeof(int));	// fill array with zeros
    switch (what_figure)
    {	// switch begin
    case 1:
        // cube
        *array = 1;
        *(array+1) = 1;
        *(array+4) = 1;
        *(array+5) = 1;
        *dim = 2;
        break;
    case 2:
        // fill with figure 2
        for (i=0; i<3; i++)
            for (j=0; j<3; j++)
                *(array + i*4 + j) = f2_hor[i][j];
        *dim = 3;
        break;
    case 3:
        // fill with figure 3
        for (i=0; i<3; i++)
            for (j=0; j<3; j++)
                *(array + i*4 + j) = f3_1[i][j];
        *dim = 3;
        break;
    case 4:
        // fill with figure 4
        for (i=0; i<3; i++)
            for (j=0; j<3; j++)
                *(array + i*4 + j) = f4_1[i][j];
        *dim = 3;
        break;
    case 5:
        // fill with figure 5
        for (i=0; i<4; i++)
            for (j=0; j<4; j++)
                *(array + i*4 + j) = f5_ver[i][j];
        *dim = 4;
        break;
	case 6:
		// fill with figure 6
        for (i=0; i<3; i++)
            for (j=0; j<3; j++)
                *(array + i*4 + j) = f6_1[i][j];
        *dim = 3;
		break;
	case 7:
		// fill with figure 7
        for (i=0; i<3; i++)
            for (j=0; j<3; j++)
                *(array + i*4 + j) = f7_hor[i][j];
        *dim = 3;
        break;
    default:	// some error maybe
        exit(EXIT_FAILURE);
    }	// switch end
}
/************************/
// update map - store current figure in map, check for game over etc
int UpdateMap()
{
    int cm;	// for check map
    // just save current figure mask in map
    for (int i=0; i<cur_fig_dim; i++)
        for (int j=0; j<cur_fig_dim; j++)
            if (cur_fig[i][j] != 0)
                map[cur_fig_posy+i][cur_fig_posx+j] = cur_fig[i][j];

    // check map
    cm = CheckMap();
    if (cm == -1)
    { // GAME OVER
        game_state = GS_GAMEOVER;
        game_over = 1;
        if (menu_state == MS_CONTINUE)
          menu_state = MS_NEWGAME;
        return 0;
    }
    else if (cm)	// cm != 0
    {	// something burned
        // inc score
		score += (int)((double)SCORE_INC * ((double)cm * (1 + SCORE_INC_COEF) - SCORE_INC_COEF));
		burned_lines += cm;
		// check how much lines are burned, update delay
		if (burned_lines >= SUB_DELAY_AFTER)
		{
			delay -= FFDELAY_DELTA;
			if (delay < FFDELAY_MIN)
				delay = FFDELAY_MIN;
			burned_lines -= SUB_DELAY_AFTER;	
		}
		// draw pic with alpha blending
		Draw_TargaA(&back_gamea,0,0,lpddspic, score * ALPHA_COEF);
    }

    return 1;
}

/*********************	*/
void Init1()
{
    // set figures
    current_figure = next_figure;
    SetFigure(0,current_figure);
    next_figure = rand()%7+1;
    SetFigure(1,next_figure);
    // update time
    start_figure_time = ftime();
    kbd_start_time = ftime();
	return;
}

/**************************************/
// move current figure down, if succesful return 1 otherwise 0
int MoveDown()
{
    // case 1
    if (cur_fig_posy + cur_fig_dim < MAP_Y)
    {
        int ok=1;
        // check if we can move it down
        for (int i=0; i<cur_fig_dim; i++)
            for (int j=0; j<cur_fig_dim; j++)
                if (cur_fig[i][j] && map[cur_fig_posy+i+1][cur_fig_posx+j])
                {
                    ok = 0;
                    break;
                }
        if (ok)
        {
            cur_fig_posy += 1;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else if (cur_fig_posy + cur_fig_dim == MAP_Y)
    {	// case 2
        int ok=1;
		int i, j;
        // check 1
        for (i=0; i<cur_fig_dim-1; i++)
            for (j=0; j<cur_fig_dim; j++)
                if (cur_fig[i][j] && map[cur_fig_posy+i+1][cur_fig_posx+j])
                {
                    ok = 0;
                    break;
                }
        // check 2
        for (i=0; i<cur_fig_dim; i++)
            if (cur_fig[cur_fig_dim-1][i] != 0)
                ok = 0;
        if (ok)
        {
            cur_fig_posy += 1;
            return 1;
        }
        else
            return 0;
    }
    else
        return 0;
}

/*********************************/
/* check map
  returns 0 as default: nothing burned, still playing
  returns -1 if game is over
  otherwise returns number of burned lines
*/
int CheckMap()
{
    int i,j,k;	// loop variables
    int ok, burned=0;
    // check for lines to burn
    for (i=0; i<MAP_Y; i++)
    {
        ok = 1;
        for (j=0; j<MAP_X; j++)
            if (!map[i][j])
            {
                ok = 0;
                break;
            }
        if (ok)
        {	// we have a line
            // shift map
            for (j=i; j>0; j--)
                for (k=0; k<MAP_X; k++)
                    map[j][k] = map[j-1][k];
            burned++;	// inc number of burned lines
            i--;
        }
    }
    // check for game over
    for (i=0; i<4; i++)
    {
        ok = 1;
        for (j=0; j<4; j++)
            if (next_fig[i][j] && map[i][5+j])
            {
                ok = 0;
                break;
            }
        if (ok == 0)
            return -1;
    }

    return burned;
}

/********************************/
// rotate current figure
// returns 1 if rotated successfully, otherwise 0
int Rotate()
{
    // please sorry for many unneeded "break"s, i hurried
    //	switch main figure number
    switch (current_figure)
    {
    case 1:	// nothing to rotate - it's cube
        break;
    case 2:	// figure 2
        // switch sub number
        switch (current_figure_sub)
        {
        case 1:
            if (CheckRotate(&f2_ver[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f2_ver[0][0]);
                current_figure_sub = 2;
            }
            break;
        case 2:
            if (CheckRotate(&f2_hor[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f2_hor[0][0]);
                current_figure_sub = 1;
            }
            break;
        }
        break;
    case 3:	// figure 3
        switch (current_figure_sub)
        {
        case 1:
            if (CheckRotate(&f3_2[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f3_2[0][0]);
                current_figure_sub = 2;
            }
            break;
        case 2:
            if (CheckRotate(&f3_3[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f3_3[0][0]);
                current_figure_sub = 3;
            }
            break;
        case 3:
            if (CheckRotate(&f3_4[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f3_4[0][0]);
                current_figure_sub = 4;
            }
            break;
        case 4:
            if (CheckRotate(&f3_1[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f3_1[0][0]);
                current_figure_sub = 1;
            }
            break;
        }
        break;
    case 4:	// figure 4
        switch (current_figure_sub)
        {
        case 1:
            if (CheckRotate(&f4_2[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f4_2[0][0]);
                current_figure_sub = 2;
            }
            break;
        case 2:
            if (CheckRotate(&f4_3[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f4_3[0][0]);
                current_figure_sub = 3;
            }
            break;
        case 3:
            if (CheckRotate(&f4_4[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f4_4[0][0]);
                current_figure_sub = 4;
            }
            break;
        case 4:
            if (CheckRotate(&f4_1[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f4_1[0][0]);
                current_figure_sub = 1;
            }
        }
        break;
    case 5:	// figure 5
        switch (current_figure_sub)
        {
        case 1:
            if (CheckRotate(&f5_hor[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f5_hor[0][0]);
                current_figure_sub = 2;
            }
            break;
        case 2:
            if (CheckRotate(&f5_ver[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f5_ver[0][0]);
                current_figure_sub = 1;
            }
        }
        break;
	case 6:	// figure 6
		switch (current_figure_sub)
        {
        case 1:
            if (CheckRotate(&f6_2[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f6_2[0][0]);
                current_figure_sub = 2;
            }
            break;
        case 2:
            if (CheckRotate(&f6_3[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f6_3[0][0]);
                current_figure_sub = 3;
            }
            break;
        case 3:
            if (CheckRotate(&f6_4[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f6_4[0][0]);
                current_figure_sub = 4;
            }
            break;
        case 4:
            if (CheckRotate(&f6_1[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f6_1[0][0]);
                current_figure_sub = 1;
            }
            break;
        }		
		break;
	case 7:
		// switch sub number
        switch (current_figure_sub)
        {
        case 1:
            if (CheckRotate(&f7_ver[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f7_ver[0][0]);
                current_figure_sub = 2;
            }
            break;
        case 2:
            if (CheckRotate(&f7_hor[0][0]))
            { // can rotate, assign next position
                CopyFigure(&f7_hor[0][0]);
                current_figure_sub = 1;
            }
            break;
        }
        break;
    default:	// error, this cannot be
        Game_Shutdown();
        return 0;
    }

    return 1;
}

/*************************************/
// checks if current figure could be rotated
int CheckRotate(int *a)
{
    int i,j,ok;
    if (cur_fig_posx < 0)
    {
        return 0;
    }
    else if (cur_fig_posx + cur_fig_dim > MAP_X)
    {
        return 0;
    }
    else
    {
        for (i=0; i<cur_fig_dim; i++)
        {
            ok=1;
            for (j=0; j<cur_fig_dim; j++)
                if (map[cur_fig_posy+i][cur_fig_posx+j] && *(a+j+i*cur_fig_dim))
                {
                    ok = 0;
                    return 0;
                }
        }
        return 1;
    }
}

/************************************/
// copies figure, pointed by a in cur_fig
void CopyFigure(int *a)
{
    int i,j;
    for (i=0; i<cur_fig_dim; i++)
        for (j=0; j<cur_fig_dim; j++)
            cur_fig[i][j] = *(a+j+i*cur_fig_dim);
    return;
}

/*******************************************/
// save current game status in GAMEDATA
void StoreGameData(GAMEDATA *gd)
{
	gd->score = score;
	memcpy(gd->map,map,MAP_Y*MAP_X*sizeof(int));
	gd->current_figure = current_figure;
	gd->current_figure_sub = current_figure_sub;
	gd->next_figure = next_figure;
	gd->next_fig_dim = next_fig_dim;
	gd->cur_fig_posx = cur_fig_posx;
	gd->cur_fig_posy = cur_fig_posy;
	gd->burned_lines = burned_lines;
}
/***************************************/
// load function
void LoadGameData(GAMEDATA *gd)
{
	score = gd->score;
	memcpy(map,gd->map,MAP_Y*MAP_X*sizeof(int));
	current_figure = gd->current_figure;
	current_figure_sub = gd->current_figure_sub;
	next_figure = gd->next_figure;
	next_fig_dim = gd->next_fig_dim;
	cur_fig_posx = gd->cur_fig_posx;
	cur_fig_posy = gd->cur_fig_posy;
	burned_lines = gd->burned_lines;
	
	SetFigure(0,current_figure);
	SetFigure(1,next_figure);

	game_state = GS_PLAYING;
	maxfps = MAXFPS_GAME;
	menu_state = MS_CONTINUE;
	continue_enabled = 1;
	start_figure_time = ftime();
	figure_time = 0;
	kbd_start_time = ftime();
	kbd_time = 0;
	game_over = 0;

	// calc current delay
	delay = FFDELAY_START - ((score / SCORE_INC) / SUB_DELAY_AFTER ) * FFDELAY_DELTA;

	// draw pic with alpha blending
	Draw_TargaA(&back_gamea,0,0,lpddspic, score * ALPHA_COEF);

	return;
}

/************************************************************************/
// read game data from save file
int ReadGameData(char *filename, GAMEDATA *gd)
{
	// open file, read data, close file
	FILE *f;
	if (fopen_s(&f, filename,"rb") != 0)	// open for binary read
	{	// error
		MessageBox(hwndMain,"Game was not loaded, cannot open file (wtf?)","Load game",0);
		return FALSE;
	} 
	else	// all ok
	{
		fread(gd,sizeof GAMEDATA,1,f);	//read data
		fclose(f);
	}

	return TRUE;
}

/***************************************************/
// write game data to save file
int WriteGameData(char *filename, GAMEDATA *gd)
{
	FILE *f;
	if (fopen_s(&f, filename, "wb") != 0)	// open for binary write
	{	// error
		MessageBox(hwndMain,"Game was not saved, cannot create or rewrite file","Save game",0);
	} 
	else	// all ok
	{
		fwrite(gd,sizeof GAMEDATA,1,f);
		fclose(f);
	}

	return TRUE;
}