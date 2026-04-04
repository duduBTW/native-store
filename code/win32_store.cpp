#include <Windows.h>
#include <WebView2.h>

#include "store.cpp"
#include "win32_d2d.cpp"

global_variable bool Running = false;
global_variable ICoreWebView2 *g_WebView = nullptr;
global_variable ICoreWebView2Controller *g_WebViewController = nullptr;
global_variable ICoreWebView2Environment *g_WebViewEnvironment = nullptr;
global_variable RECT g_WebViewBounds = {};
global_variable const wchar_t *g_PendingURL = nullptr;
global_variable HWND g_Window = nullptr;

struct WebViewCtrlHandler;
struct WebViewEnvHandler;

struct WebViewEnvHandler : ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
  HWND Window;
  LONG refCount = 1;

  WebViewEnvHandler(HWND wnd) : Window(wnd) {}

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment *env) override;

  ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refCount); }
  ULONG STDMETHODCALLTYPE Release() override
  {
    ULONG count = InterlockedDecrement(&refCount);
    if (count == 0)
      delete this;
    return count;
  }
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
  {
    if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)
    {
      *ppv = this;
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
};

struct WebViewCtrlHandler : ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
  HWND Window;
  LONG refCount = 1;

  WebViewCtrlHandler(HWND wnd) : Window(wnd) {}

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller *controller) override
  {
    if (controller)
    {
      g_WebViewController = controller;
      g_WebViewController->AddRef();
      g_WebViewController->get_CoreWebView2(&g_WebView);

      g_WebViewController->put_Bounds(g_WebViewBounds);
      g_WebViewController->put_IsVisible(false);

      if (g_PendingURL)
      {
        g_WebViewController->put_IsVisible(true);
        g_WebView->Navigate(g_PendingURL);
        g_PendingURL = nullptr;
      }
    }
    return S_OK;
  }

  ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refCount); }
  ULONG STDMETHODCALLTYPE Release() override
  {
    ULONG count = InterlockedDecrement(&refCount);
    if (count == 0)
      delete this;
    return count;
  }
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
  {
    if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)
    {
      *ppv = this;
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
};

HRESULT STDMETHODCALLTYPE WebViewEnvHandler::Invoke(HRESULT result, ICoreWebView2Environment *env)
{
  g_WebViewEnvironment = env; // save it
  g_WebViewEnvironment->AddRef();

  WebViewCtrlHandler *ctrlHandler = new WebViewCtrlHandler(Window);
  env->CreateCoreWebView2Controller(Window, ctrlHandler);
  ctrlHandler->Release();
  return S_OK;
}

void DestroyWebView()
{
  if (g_WebView)
  {
    g_WebView->Release();
    g_WebView = nullptr;
  }
  if (g_WebViewController)
  {
    g_WebViewController->Close(); // detaches the child HWND
    g_WebViewController->Release();
    g_WebViewController = nullptr;
  }
}

internal void StartWebViewInternal(HWND Window, const wchar_t *url)
{
  if (g_WebView)
  {
    // already running
    return;
  }

  // Reuse the existing environment
  if (!g_WebViewEnvironment)
  {
    // Not ready yet, bail
    return;
  }

  // Store the URL so the controller callback can navigate to it
  g_PendingURL = url;

  WebViewCtrlHandler *ctrlHandler = new WebViewCtrlHandler(Window);
  g_WebViewEnvironment->CreateCoreWebView2Controller(Window, ctrlHandler);
  ctrlHandler->Release();
}

void StartWebView(const wchar_t *url, long bottom, long left, long top, long right)
{
  RECT bounds;
  GetClientRect(g_Window, &bounds);

  if (bottom)
  {
    bounds.bottom = bottom;
  }
  if (left)
  {
    bounds.left = left;
  }
  if (right)
  {
    bounds.right = right;
  }
  if (top)
  {
    bounds.top = top;
  }

  g_WebViewBounds = bounds;
  StartWebViewInternal(g_Window, url);
  // g_WebViewController->put_Bounds(bounds);
}

internal LRESULT MainWindowCallback(HWND Window,
                                    UINT Message,
                                    WPARAM WParam,
                                    LPARAM LParam)
{
  LRESULT result = 0;

  switch (Message)
  {
  case WM_SIZE:
  {
    Win32D2DResize(LOWORD(LParam), HIWORD(LParam));

    if (g_WebViewController)
    {
      RECT bounds;
      GetClientRect(Window, &bounds);
      g_WebViewController->put_Bounds(bounds);
    }
    break;
  }

    // case WM_PAINT:
    // {
    //   PAINTSTRUCT ps;
    //   BeginPaint(Window, &ps);

    //   // Obtain the size of the drawing area.
    //   RECT rc;
    //   GetClientRect(
    //       Window,
    //       &rc);

    //   // Save the original object
    //   HGDIOBJ original = NULL;
    //   original = SelectObject(
    //       ps.hdc,
    //       GetStockObject(DC_PEN));

    //   // Create a pen.
    //   HPEN blackPen = CreatePen(PS_SOLID, 3, 255);

    //   // Select the pen.
    //   SelectObject(ps.hdc, blackPen);

    //   // Draw a rectangle.
    //   Rectangle(
    //       ps.hdc,
    //       rc.left + 100,
    //       rc.top + 100,
    //       rc.right - 100,
    //       rc.bottom - 100);

    //   DeleteObject(blackPen);

    //   // Restore the original object
    //   SelectObject(ps.hdc, original);

    //   EndPaint(Window, &ps);
    //   break;
    // };

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

  g_Window = CreateWindowA(
      windowClass.lpszClassName,
      "Store",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      0, 0,
      instance,
      0);

  if (!g_Window)
  {
    // Todo(Carlos): Log error.
  }

  if (!Win32D2DInit(g_Window))
  {
    // Todo(Carlos): Log error.
  }

  WebViewEnvHandler *handler = new WebViewEnvHandler(g_Window);
  HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, handler);
  handler->Release();

  if (FAILED(hr))
  {
    MessageBoxA(g_Window, "WebView2 runtime not found.", "Error", MB_OK);
  }

  app_memory Memory = {};
  Memory.PermanentStorageSize = Megabytes(4);
  Memory.TransientStorageSize = Gigabytes(1);
  uint64 TotalMemorySize = Memory.PermanentStorageSize + Memory.TransientStorageSize;

  Memory.PermanentStorage = VirtualAlloc(0, TotalMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  Memory.TransientStorage = (uint8 *)Memory.PermanentStorage + Memory.PermanentStorageSize;

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

    AppUpdateHandler(&Memory);
  }

  return 0;
}

float32 WindowWidth()
{
  RECT bounds;
  GetClientRect(g_Window, &bounds);
  return bounds.right;
}