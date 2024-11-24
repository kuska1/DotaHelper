#include <iostream>
#include <string>
#include <cctype>
#include "app_manager.h"
#include <windows.h>
#include <windowsx.h>
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
bool first_paint = false;

// Global var
wstring text_provider = L"...";
int map_game_time, map_clock_time;
string map_name, map_game_state = "lobby";
int gold, gold_reliable, gold_unreliable, gold_from_hero_kills, gold_from_creep_kills, gold_from_income, gold_from_shared, gpm, xpm;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    APP app;
    BUILD build;
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Clear
        if (map_game_state != "lobby") {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &clientRect, hBrush);
            DeleteObject(hBrush);
        } else {
            if (first_paint == false) {
                first_paint = true;
            } else {
                break;
            }
        }

        // Set up text properties
        SetTextColor(hdc, RGB(255, 255, 255)); // White text color
        SetBkMode(hdc, TRANSPARENT); // Transparent background for text

        // Build info
        LPCWSTR text_build = build.wide_build_full.c_str();
        RECT textRect_build = { 5, vertical - 20 }; // Bottom left
        DrawText(hdc, text_build, -1, &textRect_build, DT_SINGLELINE | DT_NOCLIP);
        #ifdef _DEBUG
            RECT textRect_provider = { 5, vertical - 35 }; // Bottom left
            DrawText(hdc, text_provider.c_str(), -1, &textRect_provider, DT_SINGLELINE | DT_NOCLIP);
        #endif

        if (map_game_state != "lobby") {
            int gxx = 150;
            int gxy = 13;
            if (map_name != "hero_demo_main") {
                if (GetAsyncKeyState(VK_MENU) & 0x8000) { // VK_MENU corresponds to the Alt key
                    RECT textRect_gold = { gxx, gxy }; // Top left
                    DrawText(hdc, (L"Gold: " + to_wstring(gold)).c_str(), -1, &textRect_gold, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_reliable = { gxx, gxy * 2 }; // Top left
                    DrawText(hdc, (L"Reliable: " + to_wstring(gold_reliable)).c_str(), -1, &textRect_reliable, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_unreliable = { gxx, gxy * 3 }; // Top left
                    DrawText(hdc, (L"Unreliable: " + to_wstring(gold_unreliable)).c_str(), -1, &textRect_unreliable, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_fhk = { gxx, gxy * 4 }; // Top left
                    DrawText(hdc, (L"From hero kills: " + to_wstring(gold_from_hero_kills)).c_str(), -1, &textRect_fhk, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_fck = { gxx, gxy * 5 }; // Top left
                    DrawText(hdc, (L"From creep kills: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_fck, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_income = { gxx, gxy * 6 }; // Top left
                    DrawText(hdc, (L"From income: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_income, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_shared = { gxx, gxy * 7 }; // Top left
                    DrawText(hdc, (L"From shared: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_shared, DT_SINGLELINE | DT_NOCLIP);
                }
                else {
                    RECT textRect_gpm = { gxx, gxy }; // Top left
                    DrawText(hdc, (L"GPM: " + to_wstring(gpm)).c_str(), -1, &textRect_gpm, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_xpm = { gxx, gxy * 2 }; // Top left
                    DrawText(hdc, (L"XPM: " + to_wstring(xpm)).c_str(), -1, &textRect_xpm, DT_SINGLELINE | DT_NOCLIP);
                }
            }
            else {
                if (GetAsyncKeyState(VK_MENU) & 0x8000) { // VK_MENU corresponds to the Alt key
                    RECT textRect_gold = { gxx + 50, gxy }; // Top left
                    DrawText(hdc, (L"Gold: " + to_wstring(gold)).c_str(), -1, &textRect_gold, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_reliable = { gxx + 50, gxy * 2 }; // Top left
                    DrawText(hdc, (L"Reliable: " + to_wstring(gold_reliable)).c_str(), -1, &textRect_reliable, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_unreliable = { gxx + 50, gxy * 3 }; // Top left
                    DrawText(hdc, (L"Unreliable: " + to_wstring(gold_unreliable)).c_str(), -1, &textRect_unreliable, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_fhk = { gxx + 50, gxy * 4 }; // Top left
                    DrawText(hdc, (L"From hero kills: " + to_wstring(gold_from_hero_kills)).c_str(), -1, &textRect_fhk, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_fck = { gxx + 50, gxy * 5 }; // Top left
                    DrawText(hdc, (L"From creep kills: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_fck, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_income = { gxx + 50, gxy * 6 }; // Top left
                    DrawText(hdc, (L"From income: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_income, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_shared = { gxx + 50, gxy * 7 }; // Top left
                    DrawText(hdc, (L"From shared: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_shared, DT_SINGLELINE | DT_NOCLIP);
                }
                else {
                    RECT textRect_gpm = { gxx + 50, gxy }; // Top left
                    DrawText(hdc, (L"GPM: " + to_wstring(gpm)).c_str(), -1, &textRect_gpm, DT_SINGLELINE | DT_NOCLIP);
                    RECT textRect_xpm = { gxx + 50, gxy * 2 }; // Top left
                    DrawText(hdc, (L"XPM: " + to_wstring(xpm)).c_str(), -1, &textRect_xpm, DT_SINGLELINE | DT_NOCLIP);
                }
            }
        }
        else {
            RECT textRect_settings = { horizontal - 680, 19}; // Top right
            HFONT hFont = CreateFont(
                22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            SetTextColor(hdc, RGB(95, 101, 109));
            DrawText(hdc, (app.wide_app_full).c_str(), -1, &textRect_settings, DT_SINGLELINE | DT_NOCLIP);
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_CREATE:
        SetTimer(hwnd, 1, 1000 / 24, NULL); // 24 FPS refresh rate (adjust as needed)
        break;
    case WM_TIMER:
        // Trigger data update or periodic UI refresh
        InvalidateRect(hwnd, NULL, TRUE); // Request window redraw
        break;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
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
    if (json_data.contains("provider") && !json_data["provider"].empty()) {
        string str_some_data = to_string(json_data["provider"]["name"]) + " (v" + to_string(json_data["provider"]["version"]) + "): " + to_string(json_data["provider"]["timestamp"]);
        text_provider = wstring(str_some_data.begin(), str_some_data.end()).c_str();
    }
    if (json_data.contains("map") && !json_data["map"].empty()) {
        map_name = json_data["map"]["name"];
        map_game_time = json_data["map"]["game_time"];
        map_clock_time = json_data["map"]["clock_time"];
        map_game_state = json_data["map"]["game_state"];
    }
    else {
        map_game_state = "lobby";
    }
    if (json_data.contains("player") && !json_data["player"].empty()) {
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
    //InvalidateRect(hwnd, NULL, TRUE);
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
