#include <SimpleHTTP/socket.h>

#include <algorithm>

namespace simpleHTTP {

simpleHTTP::ClientSocket::ClientSocket(Ref<ClientSocketImpl>&& impl)
    : m_Implementation(impl) {
    m_Cache.resize(SOCKET_BUFFER_SIZE);
    m_CacheRange = { m_Cache.begin(), m_Cache.begin() };
}

inline u64 ClientSocket::receive(void* buf, u64 size) {
    const u64 rangeSize = m_CacheRange.size();
    if (rangeSize > 0) {
        if (rangeSize >= size) {
            std::memcpy(buf, m_CacheRange.data(), size);
            m_CacheRange = { m_CacheRange.begin() + size, m_CacheRange.end() };
            return size;
        }

        std::memcpy(buf, m_CacheRange.data(), rangeSize);
        buf = static_cast<i8*>(buf) + rangeSize;
        size -= rangeSize;
        m_CacheRange = { m_CacheRange.end(), m_CacheRange.end() };
    }

    return rangeSize + m_Implementation->receive(buf, size);
}

u64 ClientSocket::receiveUntil(void* _buf, u64 size, const void* _delimiter, u64 delimiterSize) {
    u8* buf = static_cast<u8*>(_buf);
    u8* bufIt = buf;
    const u8* delimiter = static_cast<const u8*>(_delimiter);
    bool match = false;

    u64 outLen = 0;

    u32 nullRead = 0;
    constexpr u32 MAX_NULL_READ = 16;

    while (!match && outLen < size && nullRead < MAX_NULL_READ) {
        if (m_CacheRange.size() <= 0) {
            u64 byteRead = m_Implementation->receive(m_Cache.data(), SOCKET_BUFFER_SIZE);
            if (byteRead == 0)
                ++nullRead;
            m_CacheRange = { m_Cache.begin(), byteRead };
        }

        auto find = std::search(m_CacheRange.begin(), m_CacheRange.end(), delimiter, delimiter + delimiterSize);

        u64 toCopy = std::distance(m_CacheRange.begin(), find);
        toCopy = std::min(toCopy, size - outLen);

        bufIt = std::copy(m_CacheRange.begin(), m_CacheRange.begin() + toCopy, bufIt);
        outLen += toCopy;

        match = find != m_CacheRange.end();

        if (m_CacheRange.size() < toCopy + delimiterSize) {
            m_CacheRange = { m_CacheRange.begin(), m_CacheRange.begin() };
            continue;
        }

        m_CacheRange = { m_CacheRange.begin() + toCopy + delimiterSize, m_CacheRange.end() };
    }

    return outLen;
}

ServerSocket::ServerSocket(u16 port)
    : m_Implementation(make(port)) {}

} // namespace simpleHTTP
