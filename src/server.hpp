#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

constexpr int kDefaultPort = 8080;

class ClientSocket {
public:
    explicit ClientSocket(int fd = -1) noexcept;
    ~ClientSocket();

    ClientSocket(const ClientSocket&) = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;

    ClientSocket(ClientSocket&& other) noexcept;
    ClientSocket& operator=(ClientSocket&& other) noexcept;

    int get() const noexcept;
    int release() noexcept;

private:
    int fd_;
};

int createListeningSocket(int port);
int acceptClient(int listening_fd);

void logInfo(const std::string& message);
void logError(const std::string& message);

#endif
