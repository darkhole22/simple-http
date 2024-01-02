#include <linuxSocket.h>

#include <stdexcept>

#include <format>
#include <errno.h>

static inline int closeSocket(int fd) {
    return close(fd);
}

static inline int acceptSocket(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return accept(sockfd, addr, addrlen);
}

static inline ssize_t sendSocket(int sockfd, const void* buf, size_t len, int flags) {
    return send(sockfd, buf, len, flags);
}

namespace simpleHTTP {

Ref<ServerSocketImpl> ServerSocket::make(u16 port) {
    return makeRef<LinuxServerSocket>(port);
}

LinuxClientSocket::LinuxClientSocket(i32 fd)
    : m_Socket(fd) {}

u64 LinuxClientSocket::receive(void* buf, u64 size) {
    i64 result = recv(m_Socket, buf, size, 0);

    if (result < 0) {
        const char* err;

        switch (errno) {
        case EAGAIN:
            /* code */
            break;
        case EBADF:
            err = "Bad file descriptor";
            break;
        case ECONNREFUSED:
            err = "Remote host refused to connect";
            break;
        default:
            break;
        }

        throw std::runtime_error(std::format("Error while receiving data ({})!", err));
    }

    return result;
}

u64 LinuxClientSocket::send(const void* buf, u64 size) {
    i64 result = sendSocket(m_Socket, buf, size, 0);

    if (result < 0) {
        throw std::runtime_error("Error sending receiving data!");
    }

    return result;
}

void LinuxClientSocket::close() {
    if (m_Socket == -1) {
        return;
    }

    closeSocket(m_Socket);
    m_Socket = -1;
}

LinuxClientSocket::~LinuxClientSocket() {
    close();
}

LinuxServerSocket::LinuxServerSocket(u16 port)
    : m_Port(port) {
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);

    if (m_Socket == -1) {
        throw std::runtime_error("Failed to open socket!");
    }

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(m_Port);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

    if (bind(m_Socket, reinterpret_cast<sockaddr*>(&hint), sizeof(hint)) == -1) {
        throw std::runtime_error("Failed to bind socket!");
    }

    if (listen(m_Socket, SOMAXCONN) == -1) {
        throw std::runtime_error("Failed to listen on socket!");
    }
}

Ref<ClientSocketImpl> LinuxServerSocket::accept() {
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    i32 clientSocket = acceptSocket(m_Socket, reinterpret_cast<sockaddr*>(&client), &clientSize);

    if (clientSize == -1) {
        throw std::runtime_error("Failed to connect to client!");
    }

    return makeRef<LinuxClientSocket>(clientSocket);
}

u16 LinuxServerSocket::getPort() const {
    return m_Port;
}

void LinuxServerSocket::close() {
    if (m_Socket < 1) {
        return;
    }

    closeSocket(m_Socket);
    m_Socket = -1;
}

LinuxServerSocket::~LinuxServerSocket() {
    close();
}

} // namespace simpleHTTP
