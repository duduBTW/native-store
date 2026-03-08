#include "store.h"

#include <Windows.h>

global_variable bool Running = false;

LRESULT MainWindowCallback(HWND Window,
                           UINT Message,
                           WPARAM WParam,
                           LPARAM LParam)
{
  LRESULT result = 0;

  switch (Message)
  {
  case WM_DESTROY:
  case WM_CLOSE:
  {
    Running = false;
    break;
  }

  default:
  {
    result = DefWindowProc(Window, Message, WParam, LParam);
    break;
  }
  }

  return result;
}

int CALLBACK
WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR cmdLine,
    int nShowCmd)
{
  WNDCLASSA windowClass = {};
  windowClass.style = CS_OWNDC | CS_VREDRAW;
  windowClass.lpfnWndProc = MainWindowCallback;
  windowClass.hInstance = instance;
  windowClass.lpszClassName = "HandmadeHeroWindowClass";

  if (!RegisterClassA(&windowClass))
  {
    // Todo(Carlos): Log error.
  }

  HWND Window = CreateWindowA(
      windowClass.lpszClassName,
      "Store",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      0, 0,
      instance,
      0);

  if (!Window)
  {
    // Todo(Carlos): Log error.
  }

  Running = true;
  while (Running)
  {
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
      if (Message.message == WM_QUIT)
      {
        Running = false;
      }

      TranslateMessage(&Message);
      DispatchMessage(&Message);
    }
  }

  return 0;
}