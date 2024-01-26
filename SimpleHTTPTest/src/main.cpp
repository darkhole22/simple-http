#include <iostream>
#include <SimpleHTTP/http.h>
#include <SimpleHTTP/executor/DefaultExecutor.h>
#include <SimpleHTTP/handler/DefaultRequestHandler.h>

#include <string>

using namespace simpleHTTP;
constexpr unsigned long long BUFFER_SIZE = 1000ULL;

static const std::string indexHTML =
"<!DOCTYPE html>"
"<html lang=\'en\'>"
"<head>"
"<meta charset=\'utf-8\'>"
"<title>Page Title</title>"
"<meta name=\'viewport\' content=\'width=device-width, initial-scale=1\'>"
"</head>"
"<body>"
"</body>"
"</html>";

static std::unique_ptr<Resource> getProcess(const HttpRequest&) {
    return std::make_unique<Resource>();
}

static std::unique_ptr<Resource> headProcess(const HttpRequest&) {
    return std::make_unique<Resource>();
}

static void printInfo() {
    auto addresses = getLocalAddresses();

    for (auto& address : addresses) {
        std::cout << "Name: " << address.name <<
            "\tType: " << (address.type == AddressType::IPV4 ? "IPV4" : "IPV6") <<
            "\tIP: " << address.value << std::endl;
    }
}

int main(int argc, char const* argv[]) {
    try {
        printInfo();

        HttpServerSettings s{};
        HttpServer server{ s };

        std::cout << "\nLocal Address: http://" << getDefaultAddress().value << ":" << server.getPort() << std::endl;

        DefaultExecutor executor{};

        std::jthread closingThread([&executor, &server](std::stop_token st) {
            std::cin.get();
            executor.stop();
            server.stop();
        });

        DefaultRequestHandlerSettings defaultRequestHandlerSettings{};

        defaultRequestHandlerSettings.registerRequestProcessor("/", {
                                                                   { HttpMethod::GET, getProcess},
                                                                   { HttpMethod::HEAD, headProcess}
                                                               });

        DefaultRequestHandler requestHandler(defaultRequestHandlerSettings);

        executor.setProcessRequest([&requestHandler](const HttpRequest& request, HttpResponse& response) {
            return requestHandler.processRequest(request, response);
        });

        executor.run(server);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}
