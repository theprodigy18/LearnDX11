#pragma once

typedef struct _WndHandle
{
    HWND hwnd;
    u32  width;
    u32  height;
} _WndHandle;

typedef _WndHandle* WndHandle;

typedef struct _WndCallback
{
    bool (*OnClose)();
    bool (*OnResize)(RECT* pRect);
} WndCallback;

typedef struct _WndInitProps
{
    wchar_t*    title;
    u32         width;
    u32         height;
    WndCallback callback;
} WndInitProps;

bool DROP_CreateWindow(const WndInitProps* pProps, WndHandle* pHandle);
void DROP_DestroyWindow(WndHandle* pHandle);
void DROP_PollEvents();