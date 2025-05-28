/*
TODO:
Customize which mouse button to use for scrolling.
Prevent voidMouse from running multiple instances when started multiple times.
Customize scroll speed
*/

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

HMODULE hdll = 0;

int install_hook(void)
{
	int ret;
	
	ret = 0;
	
 	hdll = LoadLibraryW(L"voidmouse.dll");
	if (hdll)
	{
		void (__stdcall *initializeHook)(void);
		
		initializeHook = (void (__stdcall *)(void))GetProcAddress(hdll,"initializeHook");
		if (initializeHook)
		{
		printf("call hook\n");
			initializeHook();
		}

		ret = 1;
	}
	else
	{
		printf("no dll %d\n",GetLastError());
	}
	
	return ret;
}

void uninstall_hook(void)
{
	if (hdll)	
	{
		void (__stdcall *UninitializeHook)(void);
		
		UninitializeHook = (void (__stdcall *)(void))GetProcAddress(hdll,"UninitializeHook");
		if (UninitializeHook)
		{
			UninitializeHook();
		}
		
		FreeLibrary(hdll);
		hdll = 0;
	}
}

int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	MSG msg;
	int ret;

	// AllocConsole();
	printf("voidmouse\n");
	
	{
		HMODULE user32_hmod;
		
		user32_hmod = GetModuleHandleA("user32.dll");
		if (user32_hmod)
		{
			printf("user32\n");
				
			{
				BOOL (WINAPI *SetProcessDpiAwarenessContext_proc)(int value);
			
				SetProcessDpiAwarenessContext_proc = (void *)GetProcAddress(user32_hmod,"SetProcessDpiAwarenessContext");
				
				if (SetProcessDpiAwarenessContext_proc)
				{
					printf("SetProcessDpiAwarenessContext_proc\n");
					
					// DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
					if (SetProcessDpiAwarenessContext_proc(-4))
					{
						printf("SetProcessDpiAwarenessContext_proc OK\n");
					}
					else
					{
						printf("SetProcessDpiAwarenessContext_proc FAILED %u\n",GetLastError());
					}
				}
			}
			/*
			{
				HANDLE (WINAPI *GetThreadDpiAwarenessContext_proc)(void);
				
				GetThreadDpiAwarenessContext_proc = (void *)GetProcAddress(user32_hmod,"GetThreadDpiAwarenessContext");
				
				if (GetThreadDpiAwarenessContext_proc)
				{
					HANDLE ctx;

					printf("GetThreadDpiAwarenessContext_proc\n");
					
					ctx = GetThreadDpiAwarenessContext_proc();
					if (ctx)
					{
						int (WINAPI *GetAwarenessFromDpiAwarenessContext_proc)(HANDLE ctx);

						printf("ctx %p\n",ctx);
						
						GetAwarenessFromDpiAwarenessContext_proc = (void *)GetProcAddress(user32_hmod,"GetAwarenessFromDpiAwarenessContext");
						if (GetAwarenessFromDpiAwarenessContext_proc)
						{
							int level;

							level = GetAwarenessFromDpiAwarenessContext_proc(ctx);
							
							printf("DPI awareness level %d\n",level);
						}
					}
				}
			}*/
		}
	}
	
	if (!install_hook())
	{
		printf("install hook failed\n");
		
		return 0;
	}
	
	
loop:

	// update windows
	if (PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) 
	{
		ret = (int)GetMessage(&msg,0,0,0);
		if (ret <= 0) goto exit_loop;

		// process normally...
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	else
	{
		WaitMessage();
	}

	goto loop;

exit_loop:

	return 0;
}

int __cdecl main(int argc,char **argv)
{
	return WinMain(0,0,0,0);
}