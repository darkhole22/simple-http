#include <iostream>
#include <SimpleHTTP/http.h>
#include <SimpleHTTP/executor/DefaultExecutor.h>

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

static bool processRequest(const HttpRequest& request, HttpResponse& response) {
    response.setVersion(HttpVersion::V1_0);

    HttpMethod method = request.getMethod();
    const URI& uri = request.getURI();

    std::cout << std::format("Http Request: {} {} {}", request.getVersion(), method, uri) << std::endl;

    // for (auto& fieldLine : request.getAllHeaderFields()) {
    //     std::cout << std::format("\t{}: {}", fieldLine.first, fieldLine.second) << std::endl;
    // }

    response.addHeaderField("Allow", "GET, HEAD");

    if (method != HttpMethod::GET && method != HttpMethod::HEAD) {
        response.setStatusCode(StatusCode::NOT_IMPLEMENTED);
        return true;
    }

    if (uri.toString() == "/") {
        response.setStatusCode(StatusCode::OK);

        response.addHeaderField("Content-Length", std::format("{}", indexHTML.size()));
        response.addHeaderField("Content-Type", "text/html; charset=utf-8");
        response.addHeaderField("Cache-Control", "no-cache");
        response.addHeaderField("X-Content-Type-Options", "nosniff");

        response.send([](ClientSocket* socket) {
            socket->send(indexHTML.data(), indexHTML.size());
        });
        return true;
    }

    response.setStatusCode(StatusCode::NOT_FOUND);

    return true;
}

int main(int argc, char const* argv[]) {

    try {
        auto addresses = getLocalAddresses();

        for (auto& address : addresses) {
            std::cout << "Name: " << address.name <<
                "\tType: " << (address.type == AddressType::IPV4 ? "IPV4" : "IPV6") <<
                "\tIP: " << address.value << std::endl;
        }

        HttpServerSettings s{};
        HttpServer server{ s };

        std::cout << "\nLocal Address: http://" << getDefaultAddress().value << ":" << server.getPort() << std::endl;

        DefaultExecutor executor{};

        std::jthread closingThread([&executor, &server](std::stop_token st) {
            std::cin.get();
            executor.stop();
            server.stop();
        });

        executor.setProcessRequest(processRequest);

        executor.run(server);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}
