#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <string>
#include <sstream>
#include <utility>

class APP {
public:
    const std::string app_name = "Dota Helper";
    const float app_version = 1.0;
    const std::string app_full = createAppFull();
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
        std::string app_mode = "Debug";
    #else
        std::string app_mode = "Release";
    #endif
    #if defined(_M_X64) || defined(__x86_64__)
        int app_platform = 64;
    #elif defined(_M_IX86) || defined(__i386__)
        int app_platform = 86;
    #else
        int app_platform = 0;
    #endif
};

std::pair<APP, BUILD> app_manager();

#endif
