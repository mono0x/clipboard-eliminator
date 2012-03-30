#include "stdafx.hpp"
#include "util.hpp"
#include "resource.h"

namespace {

  LPCTSTR CLASS_NAME = _T("ClipboardEliminatorWindow");
  LPCTSTR MUTEX_OBJECT_NAME = _T("ClipboardEliminatorMutex");

  const UINT_PTR EVENT_ID = 1;
  const UINT TIMEOUT = 3 * 60 * 1000;

  HWND hwndNext = 0;

  LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch(Msg) {
      case WM_DRAWCLIPBOARD:
        SendMessage(hwndNext, Msg, wParam, lParam);
        if(CountClipboardFormats() > 0) {
          SetTimer(hWnd, EVENT_ID, TIMEOUT, 0);
        }
        return 0;

      case WM_CHANGECBCHAIN:
        if(reinterpret_cast<HWND>(wParam) == hwndNext) {
          hwndNext = reinterpret_cast<HWND>(lParam);
        }
        else {
          SendMessage(hwndNext, Msg, wParam, lParam);
        }
        return 0;

      case WM_TIMER:
        if(static_cast<UINT_PTR>(wParam) == EVENT_ID) {
          OpenClipboard(hWnd) && EmptyClipboard() && CloseClipboard();
          return 0;
        }
        break;

      case WM_DESTROY:
        ChangeClipboardChain(hWnd, hwndNext);
        PostQuitMessage(0);
        return 0;

      default:
        break;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
  }

  int Main() {
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(0));

    TCHAR szPath[MAX_PATH];
    GetModuleFileName(0, szPath, MAX_PATH);
    PathRemoveFileSpec(szPath);
    SetCurrentDirectory(szPath);

    CreateMutex(0, TRUE, MUTEX_OBJECT_NAME);
    if(GetLastError() == ERROR_ALREADY_EXISTS) {
      SendMessage(FindWindow(CLASS_NAME, 0), WM_CLOSE, 0, 0);
      return 0;
    }

    ImmDisableIME(static_cast<DWORD>(-1));

    HWND hWnd = CreateHiddenWindow(hInstance, CLASS_NAME, WindowProc);
    if(!hWnd) {
      ShowError(hInstance, IDS_ERROR_CREATE_WINDOW);
      return 1;
    }

    ShowWindow(hWnd, SW_MINIMIZE);
    ShowWindow(hWnd, SW_HIDE);

    hwndNext = SetClipboardViewer(hWnd);

    MSG msg;
    while(GetMessage(&msg, 0, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
  }

}

#ifdef _DEBUG
int APIENTRY _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int) {
  return Main();
}
#else
void __cdecl WinMainCRTStartup() {
  ExitProcess(Main());
}
#endif
