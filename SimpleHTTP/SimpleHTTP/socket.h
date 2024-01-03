#pragma once
#include <SimpleHTTP/types.h>

#include <vector>

namespace simpleHTTP {

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
private:
    Ref<ClientSocketImpl> m_Implementation;
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
private:
    Ref<ServerSocketImpl> m_Implementation;

    Ref<ServerSocketImpl> make(u16 port);
};

} // namespace simpleHTTP

