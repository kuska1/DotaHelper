#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <string>
#include <sstream>
#include <utility>

class APP {
public:
    const std::string app_name = "Dota Helper";
    const std::wstring wide_app_name = std::wstring(app_name.begin(), app_name.end());
    const float app_version = 1.2;
    const std::string app_full = createAppFull();
    const std::wstring wide_app_full = std::wstring(app_full.begin(), app_full.end());
private:
    std::string createAppFull() const {
        std::ostringstream stream;
        stream.precision(1); // Set precision to 1 digit after the decimal point
        stream << std::fixed << app_version; // Fixed-point notation
        return app_name + " v" + stream.str();
    }
};

class BUILD {
public:
    #ifdef _DEBUG
        const std::string app_mode = "Debug";
    #else
        const std::string app_mode = "Release";
    #endif
    #if defined(_M_X64) || defined(__x86_64__)
        const int app_platform = 64;
    #elif defined(_M_IX86) || defined(__i386__)
        const int app_platform = 86;
    #else
        const int app_platform = 0;
    #endif
        const std::string build_full = createAppFull();
        const std::wstring wide_build_full = std::wstring(build_full.begin(), build_full.end());
private:
    std::string createAppFull() const {
        return app_mode + " x" + std::to_string(app_platform);
    }
};

std::pair<APP, BUILD> app_manager();

#endif
