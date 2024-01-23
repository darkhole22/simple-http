#include <SimpleHTTP/socket.h>

#include <algorithm>

namespace simpleHTTP {

simpleHTTP::ClientSocket::ClientSocket(Ref<ClientSocketImpl>&& impl)
    : m_Implementation(impl) {
    m_Buffer.reserve(SOCKET_BUFFER_SIZE);
}

u64 ClientSocket::receiveUntil(void* _buf, u64 size, const void* _delimiter, u64 delimiterSize) {
    u8* buf = static_cast<u8*>(_buf);
    u8* bufIt = buf;
    const u8* delimiter = static_cast<const u8*>(_delimiter);
    bool match = false;

    u64 outLen = 0;

    while (!match && outLen < size) {
        if (m_Buffer.size() <= 0) {
            m_Buffer.resize(SOCKET_BUFFER_SIZE);
            u64 byteRead = receive(m_Buffer.data(), SOCKET_BUFFER_SIZE);
            m_Buffer.resize(byteRead);
        }

        auto find = std::search(m_Buffer.begin(), m_Buffer.end(), delimiter, delimiter + delimiterSize);

        u64 toCopy = std::distance(m_Buffer.begin(), find);
        toCopy = std::min(toCopy, size - outLen);

        bufIt = std::copy(m_Buffer.begin(), m_Buffer.begin() + toCopy, bufIt);
        outLen += toCopy;

        match = find != m_Buffer.end();

        auto newEnd = std::shift_left(m_Buffer.begin(), m_Buffer.end(), toCopy + delimiterSize);
        m_Buffer.resize(std::distance(m_Buffer.begin(), newEnd));
    }

    return outLen;
}

ServerSocket::ServerSocket(u16 port)
    : m_Implementation(make(port)) {}

} // namespace simpleHTTP
