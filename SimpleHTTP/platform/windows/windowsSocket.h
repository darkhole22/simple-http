#pragma once
#include <SimpleHTTP/socket.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

namespace simpleHTTP {

class WindowsClientSocket : public ClientSocketImpl
{
public:
    WindowsClientSocket(SOCKET socket);

    virtual u64 receive(void* buf, u64 size) override;
    virtual u64 send(const void* buf, u64 size) override;

    virtual void close() override;

    virtual ~WindowsClientSocket() override;
private:
    SOCKET m_Socket = INVALID_SOCKET;
};

class WindowsServerSocket : public ServerSocketImpl
{
public:
    WindowsServerSocket(u16 port);

    virtual Ref<ClientSocketImpl> accept() override;

    virtual u16 getPort() const override;

    virtual void close() override;

    virtual ~WindowsServerSocket() override;
private:
    SOCKET m_Socket = INVALID_SOCKET;
    u16 m_Port = 0;
};

} // namespace simpleHTTP
