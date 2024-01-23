#include <SimpleHTTP/socket.h>

#include <algorithm>

namespace simpleHTTP {

simpleHTTP::ClientSocket::ClientSocket(Ref<ClientSocketImpl>&& impl)
    : m_Implementation(impl) {
    m_Cache.resize(SOCKET_BUFFER_SIZE);
    m_CacheBegin = m_Cache.begin();
    m_CacheEnd = m_Cache.begin();
}

u64 ClientSocket::receiveUntil(void* _buf, u64 size, const void* _delimiter, u64 delimiterSize) {
    u8* buf = static_cast<u8*>(_buf);
    u8* bufIt = buf;
    const u8* delimiter = static_cast<const u8*>(_delimiter);
    bool match = false;

    u64 outLen = 0;

    while (!match && outLen < size) {
        u64 cacheSize = std::distance(m_CacheBegin, m_CacheEnd);
        if (cacheSize <= 0) {
            u64 byteRead = receive(m_Cache.data(), SOCKET_BUFFER_SIZE);
            m_CacheBegin = m_Cache.begin();
            m_CacheEnd = m_CacheBegin + byteRead;
        }

        auto find = std::search(m_CacheBegin, m_CacheEnd, delimiter, delimiter + delimiterSize);

        u64 toCopy = std::distance(m_CacheBegin, find);
        toCopy = std::min(toCopy, size - outLen);

        bufIt = std::copy(m_CacheBegin, m_CacheBegin + toCopy, bufIt);
        outLen += toCopy;

        match = find != m_CacheEnd;

        m_CacheBegin += toCopy + delimiterSize;
    }

    return outLen;
}

ServerSocket::ServerSocket(u16 port)
    : m_Implementation(make(port)) {}

} // namespace simpleHTTP
