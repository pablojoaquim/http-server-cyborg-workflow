#include "http_handler.hpp"

#include "http_response.hpp"
#include "server.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <sstream>

namespace {
constexpr std::size_t kMaxRequestSize = 64 * 1024;

std::string trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool parseHeaders(const std::string& header_blob, HttpRequest& request, std::string& error) {
    std::istringstream stream(header_blob);
    std::string request_line;
    if (!std::getline(stream, request_line)) {
        error = "Missing request line";
        return false;
    }

    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::istringstream request_line_stream(request_line);
    if (!(request_line_stream >> request.method >> request.path >> request.version)) {
        error = "Malformed request line";
        return false;
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const std::size_t separator = line.find(':');
        if (separator == std::string::npos) {
            error = "Malformed header line";
            return false;
        }

        const std::string key = toLower(trim(line.substr(0, separator)));
        const std::string value = trim(line.substr(separator + 1));
        request.headers[key] = value;
    }

    return true;
}

bool parseContentLength(const HttpRequest& request, std::size_t& content_length, std::string& error) {
    content_length = 0;

    const auto it = request.headers.find("content-length");
    if (it == request.headers.end()) {
        return true;
    }

    try {
        const std::size_t parsed = static_cast<std::size_t>(std::stoull(it->second));
        if (parsed > kMaxRequestSize) {
            error = "Content-Length exceeds 64 KiB limit";
            return false;
        }
        content_length = parsed;
    } catch (const std::exception&) {
        error = "Invalid Content-Length";
        return false;
    }

    return true;
}

bool sendAll(int client_fd, const std::string& payload) {
    std::size_t sent_total = 0;
    while (sent_total < payload.size()) {
        const ssize_t sent = send(
            client_fd,
            payload.data() + sent_total,
            payload.size() - sent_total,
            0
        );

        if (sent <= 0) {
            return false;
        }
        sent_total += static_cast<std::size_t>(sent);
    }

    return true;
}

void sendSimpleResponse(int client_fd, const std::string& status, const std::string& type, const std::string& body) {
    const std::string response = buildHttpResponse(status, type, body);
    if (!sendAll(client_fd, response)) {
        logError(std::string("send failed: ") + std::strerror(errno));
    }
}
}  // namespace

bool readHttpRequest(int client_fd, HttpRequest& request, std::string& error) {
    request = HttpRequest{};

    std::string raw;
    raw.reserve(4096);

    std::size_t header_end = std::string::npos;
    std::array<char, 4096> buffer{};

    while (header_end == std::string::npos) {
        const ssize_t received = recv(client_fd, buffer.data(), buffer.size(), 0);
        if (received <= 0) {
            error = "Connection closed while reading headers";
            return false;
        }

        raw.append(buffer.data(), static_cast<std::size_t>(received));
        if (raw.size() > kMaxRequestSize) {
            error = "Request exceeds 64 KiB limit";
            return false;
        }

        header_end = raw.find("\r\n\r\n");
    }

    const std::size_t body_start = header_end + 4;
    const std::string header_blob = raw.substr(0, header_end);

    if (!parseHeaders(header_blob, request, error)) {
        return false;
    }

    std::size_t content_length = 0;
    if (!parseContentLength(request, content_length, error)) {
        return false;
    }

    if (body_start + content_length > kMaxRequestSize) {
        error = "Headers + body exceed 64 KiB limit";
        return false;
    }

    while (raw.size() < body_start + content_length) {
        const ssize_t received = recv(client_fd, buffer.data(), buffer.size(), 0);
        if (received <= 0) {
            error = "Connection closed while reading body";
            return false;
        }

        raw.append(buffer.data(), static_cast<std::size_t>(received));
        if (raw.size() > kMaxRequestSize) {
            error = "Request exceeds 64 KiB limit";
            return false;
        }
    }

    request.body = raw.substr(body_start, content_length);
    return true;
}

void handleClient(int client_fd) {
    HttpRequest request;
    std::string error;

    if (!readHttpRequest(client_fd, request, error)) {
        logError("Bad request: " + error);
        sendSimpleResponse(client_fd, "400 Bad Request", "text/plain; charset=utf-8", "Bad Request\n");
        return;
    }

    if (request.method == "GET" && request.path == "/") {
        sendSimpleResponse(
            client_fd,
            "200 OK",
            "text/html; charset=utf-8",
            buildFormHtml()
        );
        return;
    }

    if (request.method == "POST" && request.path == "/") {
        logInfo("POST body: " + request.body);
        sendSimpleResponse(
            client_fd,
            "200 OK",
            "text/plain; charset=utf-8",
            "Form submission received\n"
        );
        return;
    }

    if (request.method == "GET" || request.method == "POST") {
        sendSimpleResponse(client_fd, "404 Not Found", "text/plain; charset=utf-8", "Not Found\n");
        return;
    }

    sendSimpleResponse(
        client_fd,
        "405 Method Not Allowed",
        "text/plain; charset=utf-8",
        "Method Not Allowed\n"
    );
}
