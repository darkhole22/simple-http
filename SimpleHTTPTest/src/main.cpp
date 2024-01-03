#include <iostream>
#include <SimpleHTTP/socket.h>

#include <string>

constexpr unsigned long long BUFFER_SIZE = 1000ULL;

int main(int argc, char const* argv[]) {
    using namespace simpleHTTP;

    try {
        auto addresses = getLocalAddresses();

        for (auto& address : addresses) {
            std::cout << "Name: " << address.name << "\tIP: " << address.value << "\tType: " << (address.type == AddressType::IPV4 ? "IPV4" : "IPV6") << std::endl;
        }
        std::cout << "Local Address: " << getDefaultAddress().value << std::endl;

        ServerSocket server(54001);

        ClientSocket client = server.accept();

        char buf[BUFFER_SIZE];
        while (true) {
            i64 bytesReceived = client.receive(buf, BUFFER_SIZE);

            if (bytesReceived == 0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            }

            std::cout << "Received: " << std::string(buf, 0, bytesReceived) << std::endl;

            client.send(buf, bytesReceived);
        }
    }
    catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}
