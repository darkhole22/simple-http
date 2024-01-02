#include <SimpleHTTP/socket.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

namespace simpleHTTP {

class LinuxClientSocket : public ClientSocketImpl
{
public:
    LinuxClientSocket(i32 fd);

    virtual u64 receive(void* buf, u64 size) override;
    virtual u64 send(const void* buf, u64 size) override;

    virtual void close() override;

    virtual ~LinuxClientSocket() override;
private:
    i32 m_Socket = -1;
};

class LinuxServerSocket : public ServerSocketImpl
{
public:
    LinuxServerSocket(u16 port);

    virtual Ref<ClientSocketImpl> accept() override;

    virtual u16 getPort() const override;

    virtual void close() override;

    virtual ~LinuxServerSocket() override;
private:
    i32 m_Socket = -1;
    u16 m_Port = 0;
};

} // namespace simpleHTTP