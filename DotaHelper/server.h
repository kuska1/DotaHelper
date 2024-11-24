#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <functional>

using ServerCallback = std::function<void(const std::string&)>;
void startServer(ServerCallback callback);

#endif // SERVER_H
