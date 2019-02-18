
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

HHOOK hhook = 0;
HMODULE hmod = 0;
int isdown=0;
int downx;
int downy;
int ofsx;
int ofsy;
int dx;
int dy;
int down_hwnd=0;
HANDLE hthread = 0;
CRITICAL_SECTION cs;
HANDLE hevent = 0;
volatile int terminate = 0;

static DWORD __stdcall thread_proc(void *param);


int is_scrollbar_window(wchar_t *classname)
{
	if (wcsicmp(classname,L"ScrollBar") == 0)
	{
		return 1;
	}
	
	return 0;
}


int hscroll(HWND hwnd,int x)
{
	if (GetWindowLong(hwnd,GWL_STYLE) & WS_HSCROLL)
	{
		SCROLLINFO si;

		ZeroMemory(&si,sizeof(SCROLLINFO));
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS;
		
		if (GetScrollInfo(hwnd,SB_HORZ,&si))
		{
			if (x > 0)
			{
				SendMessage(hwnd,WM_HSCROLL,MAKEWPARAM(SB_LINERIGHT,0),(LPARAM)NULL);
				
				x--;
			}
			
			if (x < 0)
			{
				SendMessage(hwnd,WM_HSCROLL,MAKEWPARAM(SB_LINELEFT,0),(LPARAM)NULL);
				
				x++;
			}
			
			return 1;
		}
	}

	return 0;
}

int vscroll(HWND hwnd,int y)
{
	if (GetWindowLong(hwnd,GWL_STYLE) & WS_VSCROLL)
	{
		SCROLLINFO si;
		
		ZeroMemory(&si,sizeof(SCROLLINFO));
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS;
		
		if (GetScrollInfo(hwnd,SB_VERT,&si))
		{
			if (y > 0)
			{
				SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),(LPARAM)NULL);
				
				y--;
			}
			
			if (y < 0)
			{
				SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),(LPARAM)NULL);
				
				y++;
			}
			
			return 1;
		}
	}
	
	return 0;
}

int is_tooltip_window(HWND hwnd)
{
	wchar_t classname[MAX_PATH];
	
	GetClassName(hwnd,classname,MAX_PATH);

	if (wcscmp(classname,L"tooltips_class32") == 0)
	{
		return 1;
	}
	
	return 0;
}

typedef struct myEnumWindowsProcParam_s
{
	HWND hwnd;
	POINT pt;
	
}myEnumWindowsProcParam_t;

BOOL CALLBACK myEnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	RECT rect;
	
	GetWindowRect(hwnd,&rect);
	
	if (PtInRect(&rect,((myEnumWindowsProcParam_t *)lParam)->pt))
	{
		if (!is_tooltip_window(hwnd))	
		{
			((myEnumWindowsProcParam_t *)lParam)->hwnd = hwnd;
			
			return FALSE;
		}
	}

	return TRUE;
}

HWND myWindowFromPoint(POINT pt)
{
	myEnumWindowsProcParam_t param;
	param.hwnd = 0;
	param.pt = pt;
	
	EnumWindows(myEnumWindowsProc,(LPARAM)&param);
	
	if (param.hwnd)
	{
		EnumChildWindows(param.hwnd,myEnumWindowsProc,(LPARAM)&param);
	}
	
	return param.hwnd;
}

int is_vs_window(HWND hwnd)
{
	wchar_t classname[MAX_PATH];
	
	GetClassName(hwnd,classname,MAX_PATH);

	if (wcscmp(classname,L"VsTextEditPane") == 0)
	{
		if (GetWindowLong(hwnd,GWL_STYLE) & WS_VSCROLL)
		{
			return 0;
		}

		if (GetWindowLong(hwnd,GWL_STYLE) & WS_HSCROLL)
		{
			return 0;
		}
		
		return 1;
	}
	
	return 0;
}

int is_explorer_window(HWND hwnd)
{
	wchar_t classname[MAX_PATH];
	
	GetClassName(hwnd,classname,MAX_PATH);

	if (wcscmp(classname,L"DirectUIHWND") == 0)
	{
		return 1;
	}
	
	return 0;
}

int is_vved_window(HWND hwnd)
{
	wchar_t classname[MAX_PATH];
	
	GetClassName(hwnd,classname,MAX_PATH);

	if (wcscmp(classname,L"VOID_VECTOR_ED_EDIT") == 0)
	{
		return 1;
	}
	
	return 0;
}

int is_ws_scroll_window(HWND hwnd)
{
	if ((GetWindowStyle(hwnd) & WS_VSCROLL))
	{
		SCROLLINFO si;
		
		ZeroMemory(&si,sizeof(SCROLLINFO));
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS;
		
		if (GetScrollInfo(hwnd,SB_VERT,&si))
		{
			return 1;
		}
	}
		
	if ((GetWindowStyle(hwnd) & WS_HSCROLL))
	{
		SCROLLINFO si;

		ZeroMemory(&si,sizeof(SCROLLINFO));
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS;
		
		if (GetScrollInfo(hwnd,SB_HORZ,&si))
		{
			return 1;
		}
	}
	
	return 0;
}

int is_firefox_window(HWND hwnd)
{
	wchar_t classname[MAX_PATH];
	
	GetClassName(hwnd,classname,MAX_PATH);

	if (wcscmp(classname,L"MozillaWindowClass") == 0)
	{
		return 1;
	}
	
	return 0;
}

int is_treeview_window(HWND hwnd)
{
	wchar_t classname[MAX_PATH];
	
	GetClassName(hwnd,classname,MAX_PATH);

	if (wcscmp(classname,L"SysTreeView32") == 0)
	{
		return 1;
	}
	
	return 0;
}

int is_hscroll_hwnd(HWND hwnd)
{
	wchar_t classname[MAX_PATH];
	
	GetClassName(hwnd,classname,MAX_PATH);
	
	if (wcscmp(classname,L"ScrollBar") == 0)
	{
		if (!(GetWindowLong(hwnd,GWL_STYLE) & SBS_VERT))
		{
			return 1;
		}
	}
	
	return 0;
}

int is_vscroll_hwnd(HWND hwnd)
{
	wchar_t classname[MAX_PATH];
	
	GetClassName(hwnd,classname,MAX_PATH);
	
	if (wcscmp(classname,L"ScrollBar") == 0)
	{
		if (GetWindowLong(hwnd,GWL_STYLE) & SBS_VERT)
		{
			return 1;
		}
	}
	
	return 0;
}

BOOL CALLBACK enum_child_hscroll_hwnd_proc(HWND hwnd,LPARAM lParam)
{
	if (is_hscroll_hwnd(hwnd))
	{
		*((HWND *)lParam) = hwnd;
		
		return FALSE;
	}
	
	return TRUE;
}

BOOL CALLBACK enum_child_vscroll_hwnd_proc(HWND hwnd,LPARAM lParam)
{
	if (is_vscroll_hwnd(hwnd))
	{
		*((HWND *)lParam) = hwnd;
		
		return FALSE;
	}
	
	return TRUE;
}

HWND get_child_hscroll_hwnd(HWND parent)
{
	HWND hwnd;
	
	hwnd = 0;
	
	EnumChildWindows(parent,enum_child_hscroll_hwnd_proc,&hwnd);
	
	return hwnd;
}

HWND get_child_vscroll_hwnd(HWND parent)
{
	HWND hwnd;
	
	hwnd = 0;
	
	EnumChildWindows(parent,enum_child_vscroll_hwnd_proc,&hwnd);
	
	return hwnd;
}

int scroll_vs(HWND hwnd,int dx,int dy)
{
	if (is_vs_window(hwnd))
	{
		int pos;
		
		// horz
		pos = SendMessage(GetDlgItem(GetParent(hwnd),1),SBM_GETPOS,0,0);
		
		SendMessage(GetDlgItem(GetParent(hwnd),1),SBM_SETPOS,pos + dx,TRUE);
		SendMessage(GetParent(hwnd),WM_HSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos + dx),GetDlgItem(GetParent(hwnd),1));
		
		// vert
		pos = SendMessage(GetDlgItem(GetParent(hwnd),2),SBM_GETPOS,0,0);
		
		SendMessage(GetDlgItem(GetParent(hwnd),2),SBM_SETPOS,pos + dy,TRUE);
		SendMessage(GetParent(hwnd),WM_VSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos + dy),GetDlgItem(GetParent(hwnd),2));
		
		return 1;
	}
	
	return 0;
}

int scroll_vved(HWND hwnd,int dx,int dy)
{
	if (is_vved_window(hwnd))
	{
		return 1;
	}
	
	return 0;
}

int scroll_explorer(HWND hwnd,int dx,int dy)
{
	if (is_explorer_window(hwnd))
	{
		HWND scroll_hwnd;
			
		scroll_hwnd = get_child_hscroll_hwnd(hwnd);
		if (scroll_hwnd)
		{
			int pos;
			
			pos = SendMessage(scroll_hwnd,SBM_GETPOS,0,0);
			
			SendMessage(scroll_hwnd,SBM_SETPOS,pos + dx,TRUE);
			SendMessage(scroll_hwnd,WM_HSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos + dx),scroll_hwnd);
		}
		
		scroll_hwnd = get_child_vscroll_hwnd(hwnd);
		if (scroll_hwnd)
		{
			int pos;
			
			pos = SendMessage(scroll_hwnd,SBM_GETPOS,0,0);

			SendMessage(scroll_hwnd,SBM_SETPOS,pos + (dy * 100),TRUE);
			SendMessage(scroll_hwnd,WM_VSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos + (dy * 100)),scroll_hwnd);
		}
		
		return 1;
	}
	
	return 0;
}

int scroll_ws_scroll(HWND hwnd,int dx,int dy)
{
	if (is_ws_scroll_window(hwnd))
	{
		int pos;
		
		if ((GetWindowStyle(hwnd) & WS_VSCROLL))
		{
			SCROLLINFO si;
			
			ZeroMemory(&si,sizeof(SCROLLINFO));
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS;
			
			if (GetScrollInfo(hwnd,SB_VERT,&si))
			{
				pos = si.nPos + dy;
				
				ZeroMemory(&si,sizeof(SCROLLINFO));
				si.cbSize = sizeof(SCROLLINFO);
				si.fMask = SIF_TRACKPOS;
				si.nTrackPos = pos;
				SetScrollInfo(hwnd,SB_VERT,&si,FALSE);
				
				SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos),0);
			}
		}
					
		if ((GetWindowStyle(hwnd) & WS_HSCROLL))
		{
			SCROLLINFO si;
			
			ZeroMemory(&si,sizeof(SCROLLINFO));
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS;
			
			if (GetScrollInfo(hwnd,SB_HORZ,&si))
			{
				pos = si.nPos + dx;
				
				ZeroMemory(&si,sizeof(SCROLLINFO));
				si.cbSize = sizeof(SCROLLINFO);
				si.fMask = SIF_TRACKPOS;
				si.nTrackPos = pos;
				SetScrollInfo(hwnd,SB_HORZ,&si,FALSE);
				
				SendMessage(hwnd,WM_HSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos),0);
			}
		}
					
		return 1;
	}
	
	return 0;
}


int scroll_firefox(HWND hwnd,int dx,int dy)
{
	if (is_firefox_window(hwnd))
	{
		int pos;
		SCROLLINFO si;

		ZeroMemory(&si,sizeof(SCROLLINFO));
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS;
		
		GetScrollInfo(hwnd,SB_VERT,&si);
		pos = si.nPos;
		
//		pos = GetScrollPos(hwnd,SB_VERT);
printf("%d\n",pos)	;
		SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_THUMBTRACK, dx),0);
		
		return 1;
	}
	
	return 0;
}

int scroll_treeview(HWND hwnd,int dx,int dy)
{
	if (is_treeview_window(hwnd))
	{
		int pos;
		SCROLLINFO si;

			
			/*
		pos = GetScrollPos(hwnd,SB_HORZ);
		SetScrollPos(hwnd,SB_HORZ,pos + dx,TRUE);
		SendMessage(hwnd,WM_HSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos + dx),0);
*/
		

		pos = GetScrollPos(hwnd,SB_VERT);
		SendMessage(hwnd,WM_VSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos + dy),hwnd);
		
		return 1;
	}
	
	return 0;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode,WPARAM wParam,LPARAM lParam)
{
	MSLLHOOKSTRUCT *mhs;

	mhs = (MSLLHOOKSTRUCT *)lParam;

	if (nCode < 0)  
	{
		// do not process the message 

		return CallNextHookEx(hhook, nCode, wParam, lParam); 
	}
	
//	printf("code %08x: %08x %08x\n",nCode,wParam,lParam);
	
	if (wParam == WM_MBUTTONDOWN)
	{
//		HWND hwnd;
		
//		hwnd = WindowFromPoint(mhs->pt);
//		if (!is_vved_window(hwnd))
		{
			EnterCriticalSection(&cs);
			isdown = 1;
			downx = mhs->pt.x;
			downy = mhs->pt.y;
			dx = 0;
			dy = 0;
			down_hwnd = WindowFromPoint(mhs->pt);
			
			if (down_hwnd)
			{
				if (is_tooltip_window(down_hwnd))
				{
					down_hwnd = myWindowFromPoint(mhs->pt);
				}
			}
			
			LeaveCriticalSection(&cs);

	printf("down %d %d\n",downx,downy);

			return -1; 
		}
	}
	else
	if (wParam == WM_MOUSEMOVE)
	{
		EnterCriticalSection(&cs);
		if (isdown)
		{
			dx += (mhs->pt.x - downx);
			dy += (mhs->pt.y - downy);
			
			LeaveCriticalSection(&cs);
			
			SetEvent(hevent);
		
		    return -1; 
		}

		LeaveCriticalSection(&cs);
	}
	else
	if (wParam == WM_MBUTTONUP)
	{
		int eatit;
		
		eatit = 0;
		
		EnterCriticalSection(&cs);
		if (isdown)
		{
			eatit = 1;
			isdown = 0;
		}
		LeaveCriticalSection(&cs);

		if (eatit)
		{
		    return -1; 
		}
	}
	

    return CallNextHookEx(hhook, nCode, wParam, lParam); 
}

__declspec( dllexport ) void __stdcall UninitializeHook(void)
{
	if (hhook)
	{
		UnhookWindowsHookEx(hhook);
		
		hhook = 0;
	}

	terminate = 1;
	WaitForSingleObject(hthread,INFINITE);
	CloseHandle(hthread);
	DeleteCriticalSection(&cs);
	CloseHandle(hevent);
}

__declspec( dllexport ) void __stdcall initializeHook(void)
{
	DWORD threadid;
	
	hevent = CreateEvent(0,FALSE,FALSE,0);
	InitializeCriticalSection(&cs);
	
	hthread = CreateThread(0,0,thread_proc,0,0,&threadid);
	
	hhook = SetWindowsHookEx(WH_MOUSE_LL,LowLevelMouseProc,hmod,0);
}


BOOL APIENTRY DllMain(HMODULE hModule,DWORD ul_reason_for_call,LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		hmod = hModule;
	}
	
    return TRUE;
}

static DWORD __stdcall thread_proc(void *param)
{
//	AllocConsole();
//	freopen("CONOUT$", "wt", stdout);
	
	for(;;)
	{
		WaitForSingleObject(hevent,INFINITE);
		
		if (terminate) 
		{
			return 0;
		}
		
		{
			POINT p;
			int temp_dx;
			int temp_dy;
			int temp_isdown;
			int temp_downx;
			int temp_downy;
			HWND hwnd;
			
			EnterCriticalSection(&cs);
		
		/*	
			if (dx < 0)
			{
				dx = -(dx * dx);
			}
			else
			{
				dx = dx * dx;
			}
			
			dx /= 9;
			
			if (dy < 0)
			{
				dy = -(dy * dy);
			}
			else
			{
				dy = dy * dy;
			}

			dy /= 9;
*/
			temp_dx = dx + ofsx;
			temp_dy = dy + ofsy;
			ofsx = 0;
			ofsy = 0;
			temp_isdown = isdown;
			temp_downx = downx;
			temp_downy = downy;
			dx = 0;
			dy = 0;
			hwnd = down_hwnd;

			LeaveCriticalSection(&cs);
			
			// move teh window!
			p.x = temp_downx;
			p.y = temp_downy;
			
			{
				int vs_dy;
				
				vs_dy = temp_dy / 3;
				
				if (scroll_vs(hwnd,temp_dx,vs_dy))
				{
					ofsy += temp_dy - (vs_dy * 3);
					temp_dx = 0;
					temp_dy = 0;
				}
			}
/*
			if (scroll_vved(hwnd,temp_dx,temp_dy))
			{
				temp_dx = 0;
				temp_dy = 0;
			}
*/			
			if (scroll_explorer(hwnd,temp_dx,temp_dy))
			{
				temp_dx = 0;
				temp_dy = 0;
			}
			
/*
			if (scroll_ws_scroll(hwnd,temp_dx,temp_dy))
			{
				temp_dx = 0;
				temp_dy = 0;
			}
*/
			if (is_firefox_window(hwnd))
			{
				if (temp_dx)
				{
					SendMessage(hwnd,0x020E,MAKEWPARAM(0,temp_dx*3),MAKELPARAM(p.x,p.y));
				}

				if (temp_dy)
				{
					SendMessage(hwnd,WM_MOUSEWHEEL,MAKEWPARAM(0,-temp_dy*3),MAKELPARAM(p.x,p.y));
				}
				
				temp_dx = 0;
				temp_dy = 0;
			}

/*
			if (scroll_treeview(hwnd,temp_dx,temp_dy))
			{
				temp_dx = 0;
				temp_dy = 0;
			}
*/
			if (temp_dx)
			{
				if (hscroll(hwnd,temp_dx))
				{
					temp_dx = 0;
				}
			}
			
			if (temp_dy)	
			{
				if (vscroll(hwnd,temp_dy))
				{
					temp_dy = 0;
				}
			}
			
			if (temp_dx)
			{
				int wx;
				
				wx = temp_dx / WHEEL_DELTA;

				if (wx) SendMessage(hwnd,0x020E,MAKEWPARAM(0,wx * WHEEL_DELTA),MAKELPARAM(p.x,p.y));

				ofsx += temp_dx - (wx * WHEEL_DELTA);
			}

			if (temp_dy)
			{
				int wy;
				
				wy = temp_dy / WHEEL_DELTA;
				
				if (wy) SendMessage(hwnd,WM_MOUSEWHEEL,MAKEWPARAM(0,-wy * WHEEL_DELTA),MAKELPARAM(p.x,p.y));
				
				ofsy += temp_dy - (wy * WHEEL_DELTA);
			}
		
		}

			
	}
	
	return 0;
}

