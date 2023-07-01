#ifdef _WIN32

#include <string>

#include <wintunimp.h>
#include <Windows.h>
#include <steam_api.h>

#include "resource.h"
#include "appmain.h"

HINSTANCE HINST;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	int argc;
	LPWSTR *argvw = CommandLineToArgvW(GetCommandLineW(), &argc);
	char **argv = new char*[argc];
	for (int i = 0; i < argc; i++) {
		int len = WideCharToMultiByte(CP_UTF8, 0, argvw[i], -1, NULL, 0, NULL, NULL);
		argv[i] = new char[len];
		WideCharToMultiByte(CP_UTF8, 0, argvw[i], -1, argv[i], len, NULL, NULL);
	}

	HINST = hInst;
	return appMain(argc, argv);
}

#endif
