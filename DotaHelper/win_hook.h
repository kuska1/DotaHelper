#ifndef HOOK_H
#define HOOK_H

#include <windows.h>
#include <string>
#include <functional>

bool StartWinEventHook(std::function<void(const std::string&)> callback);
void StopWinEventHook();

#endif // HOOK_H
