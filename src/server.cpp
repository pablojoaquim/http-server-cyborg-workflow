#include "server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <mutex>

namespace {
std::mutex g_log_mutex;

void closeIfValid(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}
}

ClientSocket::ClientSocket(int fd) noexcept : fd_(fd) {}

ClientSocket::~ClientSocket() {
    closeIfValid(fd_);
}

ClientSocket::ClientSocket(ClientSocket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

ClientSocket& ClientSocket::operator=(ClientSocket&& other) noexcept {
    if (this != &other) {
        closeIfValid(fd_);
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

int ClientSocket::get() const noexcept {
    return fd_;
}

int ClientSocket::release() noexcept {
    const int raw_fd = fd_;
    fd_ = -1;
    return raw_fd;
}

void logInfo(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cout << "[INFO] " << message << std::endl;
}

void logError(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cerr << "[ERROR] " << message << std::endl;
}

int createListeningSocket(int port) {
    const int listening_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_fd < 0) {
        logError(std::string("socket failed: ") + std::strerror(errno));
        return -1;
    }

    const int opt = 1;
    if (setsockopt(listening_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logError(std::string("setsockopt(SO_REUSEADDR) failed: ") + std::strerror(errno));
        closeIfValid(listening_fd);
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(listening_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        logError(std::string("bind failed (is port busy?): ") + std::strerror(errno));
        closeIfValid(listening_fd);
        return -1;
    }

    if (listen(listening_fd, SOMAXCONN) < 0) {
        logError(std::string("listen failed: ") + std::strerror(errno));
        closeIfValid(listening_fd);
        return -1;
    }

    logInfo("Listening on http://127.0.0.1:" + std::to_string(port));
    return listening_fd;
}

int acceptClient(int listening_fd) {
    return accept(listening_fd, nullptr, nullptr);
}
