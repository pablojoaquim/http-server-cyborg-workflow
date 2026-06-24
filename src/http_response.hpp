#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>

inline std::string buildHttpResponse(
    const std::string& status,
    const std::string& content_type,
    const std::string& body
) {
    std::string response;
    response += "HTTP/1.1 " + status + "\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    return response;
}

inline std::string buildFormHtml() {
    return "<!doctype html>"
           "<html lang=\"en\">"
           "<head><meta charset=\"utf-8\"><title>HTTP Server Form</title></head>"
           "<body>"
           "<h1>Submit Name</h1>"
           "<form method=\"POST\" action=\"/\">"
           "<label for=\"fname\">First name:</label>"
           "<input id=\"fname\" name=\"fname\" type=\"text\" required>"
           "<br>"
           "<label for=\"lname\">Last name:</label>"
           "<input id=\"lname\" name=\"lname\" type=\"text\" required>"
           "<br>"
           "<button type=\"submit\">Send</button>"
           "</form>"
           "</body>"
           "</html>";
}

#endif
