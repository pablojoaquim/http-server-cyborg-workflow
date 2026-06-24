#include "http_handler.hpp"
#include "server.hpp"

#include <cerrno>
#include <cstring>
#include <thread>
#include <utility>

int main() {
    // WSL2 POSIX socket spike result: use <sys/socket.h>, <netinet/in.h>, <unistd.h> and int file descriptors.
    const int listening_fd = createListeningSocket(kDefaultPort);
    if (listening_fd < 0) {
        return 1;
    }

    while (true) {
        const int accepted_fd = acceptClient(listening_fd);
        if (accepted_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            logError(std::string("accept failed: ") + std::strerror(errno));
            continue;
        }

        ClientSocket client_socket(accepted_fd);

        std::thread worker([client = std::move(client_socket)]() mutable {
            handleClient(client.get());
        });
        worker.detach();
    }

    return 0;
}
