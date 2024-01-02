#include <SimpleHTTP/socket.h>

namespace simpleHTTP {

simpleHTTP::ClientSocket::ClientSocket(Ref<ClientSocketImpl>&& impl)
    : m_Implementation(impl) {}

ServerSocket::ServerSocket(u16 port)
    : m_Implementation(make(port)) {}

} // namespace simpleHTTP
