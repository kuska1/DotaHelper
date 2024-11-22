#include <windows.h>
#include <iostream>
#include <string>
#include "win_hook.h"
using namespace std;

static std::function<void(const std::string&)> g_callback;

// Hook event
void CALLBACK WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime) {
    if (event == EVENT_SYSTEM_FOREGROUND && hwnd) {
        char windowTitle[256];
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

        // Hook callback
        if (g_callback) {
            g_callback(windowTitle);
        }
    }
}

// Hook start
bool StartWinEventHook(std::function<void(const std::string&)> callback) {
    g_callback = callback;

    HWINEVENTHOOK hWinEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        NULL,
        WinEventProc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    if (!hWinEventHook) {
        std::cerr << "Failed to set hook!" << std::endl;
        return false;
    }
    return true;
}

// Hook stop
void StopWinEventHook() {
    g_callback = nullptr;
}