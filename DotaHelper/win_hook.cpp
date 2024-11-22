#include <windows.h>
#include <iostream>
#include <string>
#include "win_hook.h"
using namespace std;

// Global vaar
HWINEVENTHOOK g_hWinEventHook = NULL;

// Hook callback
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

        char className[256];
        GetClassNameA(hwnd, className, sizeof(className));

        string title(windowTitle);
        string class_name(className);

        // Is "Dota 2" active
        if (title == "Dota 2") {
            #ifdef _DEBUG
                cout << endl << "[D] Dota 2 focused!";
            #endif
        }
        else {
            #ifdef _DEBUG
                cout << endl << "[D] Focused window: " << title << " (" << class_name << ")";
            #endif
        }
    }
}

// Hook start
bool StartWinEventHook() {
    g_hWinEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        NULL,
        WinEventProc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    if (!g_hWinEventHook) {
        cerr << "Failed to set hook!" << endl;
        return false;
    }
    return true;
}

// Hook stop
void StopWinEventHook() {
    if (g_hWinEventHook) {
        UnhookWinEvent(g_hWinEventHook);
        g_hWinEventHook = NULL;
    }
}
