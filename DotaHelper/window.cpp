#include <iostream>
#include <string>
#include "app_manager.h"
#include <windows.h>
#include "window.h"
#include "resource.h"
#include "win_hook.h"
using namespace std;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Drawing a simple rectangle on the overlay
        HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0)); // Red color
        RECT rect = { 50, 50, 200, 200 };
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GetDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

int RunWindow(HINSTANCE hInstance, int nCmdShow) {

    if (!StartWinEventHook()) {
        cout << "Failed to start event hook..." << endl;
        return 1;
    }


    APP app;
    auto app_info = app_manager();

    const string window_title = app.app_name + " Overlay";
    // Convert app_name to wide string
    std::wstring wAppName = std::wstring(window_title.begin(), window_title.end());
    //delete window_title;
    const wchar_t* CLASS_NAME = wAppName.c_str();

    int* horizontal = new int[4];
    int* vertical = new int[4];
    *horizontal = 0;
    *vertical = 0;
    GetDesktopResolution(*horizontal, *vertical);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // Default application icon
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // Transparent background
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, // Transparent, click-through, and not in ALT+TAB
        CLASS_NAME,
        wAppName.c_str(), // Title
        WS_POPUP,
        0, 0,
        *horizontal, *vertical, // 1920x1080
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Free ram
    delete horizontal, vertical;

    // Set the window to be always on top
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // Set transparency (alpha = 0 for fully transparent)
    COLORREF transparentColor = RGB(0, 0, 0); // Black as the transparent color
    SetLayeredWindowAttributes(hwnd, transparentColor, 0, LWA_COLORKEY);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    StopWinEventHook();
    return 0;
}
