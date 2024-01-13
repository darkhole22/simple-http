#include <linuxSocket.h>

#include <stdexcept>

#include <format>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <poll.h>

static inline int closeSocket(int fd) {
    return close(fd);
}

static inline int acceptSocket(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    pollfd fd{};
    fd.fd = sockfd;
    fd.events = POLLIN | POLLPRI;

    do {
        poll(&fd, 1, 1000);
    } while (fd.revents == 0);

    return accept4(sockfd, addr, addrlen, SOCK_CLOEXEC);
}

static inline ssize_t sendSocket(int sockfd, const void* buf, size_t len, int flags) {
    return send(sockfd, buf, len, flags);
}

namespace simpleHTTP {

std::vector<Address> getLocalAddresses() {
    std::vector<Address> result{};

    ifaddrs* ifaddr, * ifa;

    if (getifaddrs(&ifaddr) == -1) {
        throw std::runtime_error("getifaddrs() failed.");
    }

    char host[NI_MAXHOST];

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        if (ifa->ifa_addr->sa_family != AF_INET && ifa->ifa_addr->sa_family != AF_INET6) {
            continue;
        }

        socklen_t familyLen = (ifa->ifa_addr->sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

        int r = getnameinfo(ifa->ifa_addr, familyLen,
            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if (r != 0) {
            if (r == EAI_AGAIN) {
                // TODO: retry?
            }

            throw std::runtime_error(std::format("getnameinfo() failed: {}", gai_strerror(r)));
        }

        //std::cout << "Name: " << ifa->ifa_name << "\tIP: " << host << std::endl;
        result.emplace_back(ifa->ifa_name, host, (ifa->ifa_addr->sa_family == AF_INET) ? AddressType::IPV4 : AddressType::IPV6);
    }

    freeifaddrs(ifaddr);

    return result;
}

Address getDefaultAddress() {
    char* p;
    char* c;

    std::ifstream f("/proc/net/route");

    if (!f) {
        throw std::runtime_error("Unable to find dafault address.");
    }

    std::string line;

    while (std::getline(f, line)) {
        p = strtok(line.data(), " \t");
        c = strtok(NULL, " \t");

        if (p != NULL && c != NULL) {
            if (strcmp(c, "00000000") == 0) {
                // printf("Default interface is : %s \n", p);
                break;
            }
        }
    }

    auto addresses = getLocalAddresses();
    for (auto& address : addresses) {
        if (address.name == p) {
            return address;
        }
    }

    throw std::runtime_error("Unable to find default address.");
}

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
            // TODO Other Error code
        default:
            err = "Unrecognized Error code";
            break;
        }

        throw std::runtime_error(std::format("Error while receiving data ({}, {})!", err, errno));
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

        switch (errno) {
        case EACCES:
            throw std::runtime_error("Failed to bind socket for lack of privilages!");
        case EADDRINUSE:
            throw std::runtime_error("Failed to bind socket because the addres is already in use!");
        }

        throw std::runtime_error("Failed to bind socket!");
    }

    if (listen(m_Socket, SOMAXCONN) == -1) {
        throw std::runtime_error("Failed to listen on socket!");
    }
}

Ref<ClientSocketImpl> LinuxServerSocket::accept() {
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    // TODO: Use non blocking operation or see `select`
    i32 clientSocket = acceptSocket(m_Socket, reinterpret_cast<sockaddr*>(&client), &clientSize);

    if (clientSocket == -1) {
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
