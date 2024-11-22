#include <iostream>
#include <string>
#include "app_manager.h"
#include <windows.h>
#include "window.h"
using namespace std;

int main() {
    APP app;
    BUILD build;
    auto app_info = app_manager();

    // Convert app_name to wide string
    SetConsoleTitle(std::wstring(app.app_full.begin(), app.app_full.end()).c_str());

    cout << endl;
    cout << R"(         ____  _____ _____ _____ _____ _____ __    _____ _____ _____ )" << endl;
    cout << R"(        |    \|     |_   _|  _  |  |  |   __|  |  |  _  |   __| __  |)" << endl;
    cout << R"(        |  |  |  |  | | | |     |     |   __|  |__|   __|   __|    -|)" << endl;
    cout << R"(        |____/|_____| |_| |__|__|__|__|_____|_____|__|  |_____|__|__|)" << endl;
    cout << endl;

    #ifdef _DEBUG
        cout << endl << "[D] Mode: " << build.app_mode;
        cout << endl << "[D] Platform: x" << build.app_platform;
    #endif

    HINSTANCE hInstance = GetModuleHandle(NULL);
    int nCmdShow = SW_SHOW;
    return RunWindow(hInstance, nCmdShow);
}