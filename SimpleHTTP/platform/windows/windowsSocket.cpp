#include <windowsSocket.h>

#include <exception>
#include <stdexcept>
#include <ranges>
#include <numeric>
#include <charconv>

#pragma comment(lib, "Ws2_32.lib")

static SOCKET platformAccept(SOCKET server) {
    return accept(server, NULL, NULL);
}

static simpleHTTP::i32 platformSend(SOCKET socket, const void* data, simpleHTTP::u64 size) {
    return send(socket, reinterpret_cast<const char*>(data), size, 0);
}

namespace simpleHTTP {

class WindowsSocketInitializer
{
public:
    WindowsSocketInitializer() {
        WSADATA wsaData;

        // Initialize Winsock
        i32 iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            // printf("WSAStartup failed: %d\n", iResult);
            return;
        }

        m_Initialized = true;
    }

    ~WindowsSocketInitializer() {
        if (!m_Initialized)
            return;

        WSACleanup();
    }

private:
    bool m_Initialized = false;
};

static WindowsSocketInitializer _initializer = {};

static constexpr u64 MAX_HOST_NAME_LEN = 256;
static constexpr u64 MAX_IPV6_NAME_LEN = 46;

std::vector<Address> getLocalAddresses() {
    std::vector<Address> result{};

    char ac[MAX_HOST_NAME_LEN];
    if (gethostname(ac, MAX_HOST_NAME_LEN) == SOCKET_ERROR) {
        // cerr << "Error " << WSAGetLastError() << " when getting local host name." << endl;
        // return 1;
        throw std::runtime_error("gethostname failed");
    }
    // cout << "Host name is " << ac << "." << endl;

    struct addrinfo* addr = nullptr;

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    i32 r = getaddrinfo(ac, nullptr, &hints, &addr);

    if (r != 0) {
        throw std::runtime_error("getaddrinfo failed");
    }

    for (; addr != nullptr; addr = addr->ai_next) {
        switch (addr->ai_family) {
        case AF_INET:
        {
            sockaddr_in* sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(addr->ai_addr);
            result.emplace_back(addr->ai_canonname == nullptr ? "" : addr->ai_canonname,
                                inet_ntoa(sockaddr_ipv4->sin_addr), AddressType::IPV4);
        }
        break;
        case AF_INET6:
        {
            LPSOCKADDR sockaddr_ip = (LPSOCKADDR)addr->ai_addr;

            char ipStr[MAX_IPV6_NAME_LEN];
            DWORD ipStrLength = MAX_IPV6_NAME_LEN;

            r = WSAAddressToStringA(sockaddr_ip, addr->ai_addrlen, nullptr, ipStr, &ipStrLength);

            if (r)
                break;

            result.emplace_back(addr->ai_canonname == nullptr ? "" : addr->ai_canonname,
                                ipStr, AddressType::IPV6);
        }
        break;
        default:
        break;
        }
    }

    return result;
}

static u32 addressIPV4ToInt(std::string_view addr) {
    using std::operator""sv;

    u32 result = 0;
    for (const auto& val : std::views::split(addr, "."sv)) {
        u32 i{};
        if (std::from_chars(val.data(), val.data() + val.size(), i).ec != std::errc{}) {
            continue;
        }
        result <<= 8;
        result |= 0xff & i;
    }

    return result;
}

Address getDefaultAddress() {
    const auto addresses = getLocalAddresses();
    if (addresses.size() == 0) {
        throw std::runtime_error("Couldn't find the default address!");
    }

    constexpr auto localIpFilter = [](const Address& a)
    {
        return a.type == AddressType::IPV4 && a.value.rfind("192.", 0) == 0;
    };

    constexpr auto toPair = [](const Address& a) -> std::pair<Address, u32>
    {
        return { a, addressIPV4ToInt(a.value) };
    };

    auto candidatesView = addresses | std::views::filter(localIpFilter) | std::views::transform(toPair);

    std::vector<std::pair<Address, u32>> candidates;

    std::ranges::copy(candidatesView, std::back_inserter(candidates));

    if (candidates.size() == 0) {
        throw std::runtime_error("Couldn't find the default address!");
    }

    return std::ranges::min_element(candidates, {}, &std::pair<Address, u32>::second)->first;
}

Ref<ServerSocketImpl> ServerSocket::make(u16 port) {
    return makeRef<WindowsServerSocket>(port);
}

WindowsClientSocket::WindowsClientSocket(SOCKET socket)
    : m_Socket(socket) {}

u64 WindowsClientSocket::receive(void* buf, u64 size) {
    i32 len = recv(m_Socket, reinterpret_cast<char*>(buf), size, 0);

    if (len == SOCKET_ERROR) {
        throw std::runtime_error("recv failed");
    }

    return len;
}

u64 WindowsClientSocket::send(const void* buf, u64 size) {
    i32 len = platformSend(m_Socket, buf, size);

    if (len == SOCKET_ERROR) {
        throw std::runtime_error("send failed");
    }

    return len;
}

void WindowsClientSocket::close() {
    if (m_Socket == INVALID_SOCKET)
        return;

    shutdown(m_Socket, SD_BOTH);

    closesocket(m_Socket);
    m_Socket = INVALID_SOCKET;
}

WindowsClientSocket::~WindowsClientSocket() {
    close();
}

WindowsServerSocket::WindowsServerSocket(u16 port)
    : m_Port(port) {
    std::string portString = std::to_string(port);

    addrinfo* result = NULL, * ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    i32 iResult = getaddrinfo(NULL, portString.c_str(), &hints, &result);
    if (iResult != 0) {
        //  WSAGetLastError()
        throw std::runtime_error("getaddrinfo failed.");
    }

    m_Socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (m_Socket == INVALID_SOCKET) {
        // printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        throw std::runtime_error("socket failed.");
    }

    // Setup the TCP listening socket
    iResult = bind(m_Socket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        // printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(m_Socket);
        throw std::runtime_error("bind failed.");
    }

    freeaddrinfo(result);

    if (listen(m_Socket, SOMAXCONN) == SOCKET_ERROR) {
        // printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(m_Socket);
        throw std::runtime_error("listen failed.");
    }
}

Ref<ClientSocketImpl> WindowsServerSocket::accept() {
    SOCKET client = platformAccept(m_Socket);

    if (client == INVALID_SOCKET) {
        // printf("accept failed: %d\n", WSAGetLastError());
        throw std::runtime_error("accept failed.");
    }

    return makeRef<WindowsClientSocket>(client);
}

u16 WindowsServerSocket::getPort() const {
    return m_Port;
}

void WindowsServerSocket::close() {
    if (m_Socket == INVALID_SOCKET)
        return;

    closesocket(m_Socket);
    m_Socket = INVALID_SOCKET;
}

WindowsServerSocket::~WindowsServerSocket() {
    close();
}

} // namespace simpleHTTP
