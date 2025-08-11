#include "pch.h"
#include "Platform/Window.h"

#pragma region INTERNAL
static LRESULT CALLBACK InternalWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WndCallback* pCallback = (WndCallback*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    ASSERT(pCallback);

    bool result = false;

    switch (msg)
    {
    case WM_CLOSE:
        result = pCallback->OnClose ? pCallback->OnClose() : false;
        break;
    default:
        break;
    }

    return result ? 0 : DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK TempWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        // Change the wnd proc to the InternalWndProc since this proc are only for setup the callback.
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR) InternalWndProc);

        // Get the callback from lParam that we pass earlier when create window.
        // And then set it on window long ptr as user data so we can access it latter when dealing with event.
        CREATESTRUCTW* pCS       = (CREATESTRUCTW*) lParam;
        WndCallback*   pCallback = (WndCallback*) pCS->lpCreateParams;
        if (!pCallback)
        {
            ASSERT_MSG(false, "Failed to attach window callback.");
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) pCallback);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static bool           s_isInitialized = false;
static HINSTANCE      s_hInstance     = NULL;
static const wchar_t* WND_CLASS_NAME  = L"DROP_WINDOW_CLASS";
static u32            s_wndCount      = 0;
#pragma endregion

bool DROP_CreateWindow(const WndInitProps* pProps, WndHandle* pHandle)
{
    ASSERT_MSG(pProps, "Window properties are null.");
    ASSERT_MSG(pHandle, "Window handle pointer are null.");

    *pHandle = NULL;

    // Init HINSTANCE and register Window Class.
    if (!s_isInitialized)
    {
        SetProcessDPIAware();

        s_hInstance = GetModuleHandleW(NULL);
        if (!s_hInstance)
        {
            ASSERT_MSG(false, "Failed to get module handle.");
            return false;
        }

        WNDCLASSEXW wcex = {
            .cbSize        = sizeof(WNDCLASSEXW),
            .style         = 0,
            .hInstance     = s_hInstance,
            .cbClsExtra    = 0,
            .cbWndExtra    = 0,
            .hCursor       = LoadCursorW(s_hInstance, IDC_ARROW),
            .hIcon         = NULL,
            .hIconSm       = NULL,
            .lpszClassName = WND_CLASS_NAME,
            .lpszMenuName  = NULL,
            .lpfnWndProc   = TempWndProc};

        if (!RegisterClassExW(&wcex))
        {
            ASSERT_MSG(false, "Failed to register window class.");
            return false;
        }

        s_isInitialized = true;
    }

    // Calculate all rect. We want width and height from properties only for client rect.
    // So we need to calculate entire window rect from that to make sure it.
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    RECT  rc      = {0, 0, pProps->width, pProps->height};
    AdjustWindowRect(&rc, dwStyle, FALSE);
    u32 width  = rc.right - rc.left;
    u32 height = rc.bottom - rc.top;

    // Allocation pointer to callback struct and copy the value from props.
    WndCallback* pCallback = (WndCallback*) DROP_Allocate(PERSISTENT, sizeof(WndCallback));
    if (!pCallback)
    {
        ASSERT_MSG(false, "Failed to allocate memory on window callback.");
        return false;
    }
    *pCallback = pProps->callback;

    // Create the window instance.
    HWND hwnd = CreateWindowExW(
        0, WND_CLASS_NAME, pProps->title, dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, s_hInstance, (LPVOID) pCallback);
    if (!hwnd)
    {
        ASSERT_MSG(false, "Failed to create window.");
        return false;
    }

    // Creating new handle.
    WndHandle handle = (WndHandle) DROP_Allocate(PERSISTENT, sizeof(_WndHandle));
    if (!handle)
    {
        ASSERT_MSG(false, "Failed to allocate memory on window handle.");
        DestroyWindow(hwnd);
        return false;
    }
    handle->hwnd   = hwnd;
    handle->width  = pProps->width;
    handle->height = pProps->height;
    *pHandle       = handle;

    ++s_wndCount;
    return true;
}

void DROP_DestroyWindow(WndHandle* pHandle)
{
    ASSERT_MSG(pHandle && *pHandle, "Window handle is null.");
    WndHandle handle = *pHandle;

    // Destroy the window.
    if (handle)
    {
        DestroyWindow(handle->hwnd);

        handle->hwnd = NULL;
        --s_wndCount;
    }

    *pHandle = NULL;

    // Unregister class at  the end of the program.
    if (s_wndCount == 0 && s_isInitialized)
    {
        UnregisterClassW(WND_CLASS_NAME, s_hInstance);

        s_hInstance     = NULL;
        s_isInitialized = false;
    }
}

void DROP_PollEvents()
{
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            break;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}