#ifndef HTTP_HANDLER_HPP
#define HTTP_HANDLER_HPP

#include <map>
#include <string>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

bool readHttpRequest(int client_fd, HttpRequest& request, std::string& error);
void handleClient(int client_fd);

#endif
