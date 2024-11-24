#include <iostream>
#include <string>
#include "app_manager.h"
#include <windows.h>
#include "window.h"
#include "resource.h"
#include "win_hook.h"
#include <thread>
#include "server.h"
#include <nlohmann/json.hpp>
#include <locale>
#include <codecvt>
using namespace std;

using json = nlohmann::json;

// Global data
HWND hwnd = NULL;
int horizontal, vertical = 0;
json json_data;

// Global var
wstring text_provider = L"...";
int gold, gold_reliable, gold_unreliable, gold_from_hero_kills, gold_from_creep_kills, gold_from_income, gold_from_shared, gpm, xpm;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    APP app;
    BUILD build;
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Clear
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &clientRect, hBrush);
        DeleteObject(hBrush);

        // Set up text properties
        SetTextColor(hdc, RGB(255, 255, 255)); // White text color
        SetBkMode(hdc, TRANSPARENT); // Transparent background for text

        // Build info
        LPCWSTR text_build = build.wide_build_full.c_str();
        RECT textRect_build = { 5, vertical-20 }; // Bottom left
        DrawText(hdc, text_build, -1, &textRect_build, DT_SINGLELINE | DT_NOCLIP);
        #ifdef _DEBUG
            // Focus status text
            RECT textRect_provider = { 5, vertical - 35 }; // Bottom left
            DrawText(hdc, text_provider.c_str(), -1, &textRect_provider, DT_SINGLELINE | DT_NOCLIP);
        #endif

        RECT textRect_gpm = { 150, 12 }; // Top left
        DrawText(hdc, (L"GPM: "+to_wstring(gpm)).c_str(), -1, &textRect_gpm, DT_SINGLELINE | DT_NOCLIP);

        RECT textRect_xpm = { 150, 27 }; // Top left
        DrawText(hdc, (L"XPM: " + to_wstring(xpm)).c_str(), -1, &textRect_xpm, DT_SINGLELINE | DT_NOCLIP);

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

void OnWindowChange(const std::string& windowTitle) {
    #ifdef _DEBUG
        cout << "[D] Active window changed to: " << windowTitle << endl;
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    #endif
    if (windowTitle == "Dota 2") {
        #ifndef _DEBUG
            ShowWindow(hwnd, SW_SHOW);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); // IDK, without it, will be lower layer then dota...
        #endif
    }
    else {
        #ifndef _DEBUG
            ShowWindow(hwnd, SW_HIDE);
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        #endif
    }
}

void onServerDataReceived(const std::string& data) {
    #ifdef _DEBUG
        cout << "[D] Received: " << data << endl;
    #endif
    json json_data = json::parse(data);
    string str_some_data;
    if (json_data.contains("provider")) {
        string str_some_data = to_string(json_data["provider"]["name"]) + " (v" + to_string(json_data["provider"]["version"]) + "): " + to_string(json_data["provider"]["timestamp"]);
        text_provider = wstring(str_some_data.begin(), str_some_data.end()).c_str();
    }
    if (json_data.contains("player")) {
        gold = json_data["player"]["gold"];
        gold_reliable = json_data["player"]["gold_reliable"];
        gold_unreliable = json_data["player"]["gold_unreliable"];
        gold_from_hero_kills = json_data["player"]["gold_from_hero_kills"];
        gold_from_creep_kills = json_data["player"]["gold_from_creep_kills"];
        gold_from_income = json_data["player"]["gold_from_income"];
        gold_from_shared = json_data["player"]["gold_from_shared"];
        gpm = json_data["player"]["gpm"];
        xpm = json_data["player"]["xpm"];
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

void startServerAsync() {
    std::thread serverThread([]() {
        startServer(onServerDataReceived);
        });

    serverThread.detach();
}


int RunWindow(HINSTANCE hInstance, int nCmdShow) {

    startServerAsync();

    if (!StartWinEventHook(OnWindowChange)) {
        cout << "[!] Failed to start hook..." << endl;
        return 1;
    }

    APP app;
    auto app_info = app_manager();

    const string window_title = app.app_name + " Overlay";
    // Convert app_name to wide string
    wstring wAppName = wstring(window_title.begin(), window_title.end());
    const wchar_t* CLASS_NAME = wAppName.c_str();

    GetDesktopResolution(horizontal, vertical);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // Application icon
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // Transparent background
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, // Transparent, click-through, and not in ALT+TAB
        CLASS_NAME,
        wAppName.c_str(), // Title
        WS_POPUP,
        0, 0,
        horizontal, vertical, // 1920x1080 most possible
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Set the window to be always on top
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);


    // Set transparency
    COLORREF transparentColor = RGB(0, 0, 0); // Black as the transparent color
    SetLayeredWindowAttributes(hwnd, transparentColor, 0, LWA_COLORKEY);

    // Hide or show overlay
    #ifdef _DEBUG
        ShowWindow(hwnd, SW_SHOW);
    #else
        ShowWindow(hwnd, SW_HIDE);
    #endif

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(hwnd);
    StopWinEventHook();
    return 0;
}
