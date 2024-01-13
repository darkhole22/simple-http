#include <iostream>
#include <SimpleHTTP/http.h>
#include <SimpleHTTP/executor/DefaultExecutor.h>

#include <string>

constexpr unsigned long long BUFFER_SIZE = 1000ULL;

int main(int argc, char const* argv[]) {
    using namespace simpleHTTP;

    try {
        auto addresses = getLocalAddresses();

        for (auto& address : addresses) {
            std::cout << "Name: " << address.name <<
                "\tType: " << (address.type == AddressType::IPV4 ? "IPV4" : "IPV6") <<
                "\tIP: " << address.value << std::endl;
        }

        std::cout << "Local Address: " << getDefaultAddress().value << std::endl;

        HttpServerSettings s{};
        HttpServer server{ s };

        DefaultExecutor executor{};

        std::jthread closingThread([&executor, &server](std::stop_token st) {
            std::cin.get();
            executor.stop();
            server.stop();
        });

        executor.run(server);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}
