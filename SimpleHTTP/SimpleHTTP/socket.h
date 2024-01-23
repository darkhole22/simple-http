#pragma once
#include <SimpleHTTP/types.h>

#include <string>
#include <vector>

namespace simpleHTTP {

constexpr u64 SOCKET_BUFFER_SIZE = 0x1000;

enum class AddressType
{
    IPV4, IPV6
};

struct Address
{
    std::string name;
    std::string value;
    AddressType type;
};

std::vector<Address> getLocalAddresses();
Address getDefaultAddress();

class ClientSocketImpl
{
public:
    virtual u64 receive(void* buf, u64 size) = 0;
    virtual u64 send(const void* buf, u64 size) = 0;

    virtual void close() = 0;

    virtual inline ~ClientSocketImpl() {}
private:
};

class ServerSocketImpl
{
public:
    virtual Ref<ClientSocketImpl> accept() = 0;

    virtual u16 getPort() const = 0;

    virtual void close() = 0;

    virtual inline ~ServerSocketImpl() {}
};

class ClientSocket
{
public:
    ClientSocket(Ref<ClientSocketImpl>&& impl);

    inline u64 receive(void* buf, u64 size) {
        return m_Implementation->receive(buf, size);
    }

    inline u64 send(const void* buf, u64 size) {
        return m_Implementation->send(buf, size);
    }

    u64 receiveUntil(void* buf, u64 size, const void* delimiter, u64 delimiterSize);

    inline void close() {
        m_Implementation->close();
    }
private:
    Ref<ClientSocketImpl> m_Implementation;
    std::vector<u8> m_Cache;
    std::vector<u8>::iterator m_CacheBegin;
    std::vector<u8>::iterator m_CacheEnd;
};

class ServerSocket
{
public:
    ServerSocket(u16 port);

    inline ClientSocket accept() {
        return ClientSocket{ m_Implementation->accept() };
    }

    inline u16 getPort() const {
        return m_Implementation->getPort();
    }

    inline void close() {
        m_Implementation->close();
    }
private:
    Ref<ServerSocketImpl> m_Implementation;

    Ref<ServerSocketImpl> make(u16 port);
};

} // namespace simpleHTTP

