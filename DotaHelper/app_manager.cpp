#include <iostream>
#include <string>
#include "app_manager.h"
using namespace std;
std::pair<APP, BUILD> app_manager() {
    APP app;
    BUILD build;
    return { app, build };
}