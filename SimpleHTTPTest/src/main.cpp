#include <iostream>
#include <SimpleHTTP/socket.h>

#include <string>

constexpr unsigned long long BUFFER_SIZE = 1000ULL;

int main(int argc, char const* argv[]) {
    using namespace simpleHTTP;

    try {
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
