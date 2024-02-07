#include <SimpleHTTP/http.h>
#include <SimpleHTTP/executor/DefaultExecutor.h>
#include <SimpleHTTP/handler/DefaultRequestHandler.h>

#include <iostream>
#include <string>
#include <fstream>

using namespace simpleHTTP;

static std::filesystem::path serverRootPath = "data";

static std::filesystem::path getPathFromURI(const URI& uri) {
    auto path = serverRootPath / uri;
    std::error_code ec;

    if (std::filesystem::equivalent(path, serverRootPath, ec)) {
        return path / "index.html";
    }

    if (std::filesystem::is_regular_file(path, ec)) {
        return path;
    }

    auto pathWithExtension = path;
    pathWithExtension.replace_extension("html");

    if (std::filesystem::is_regular_file(pathWithExtension, ec)) {
        return pathWithExtension;
    }

    return (path / path.filename()).replace_extension("html");
}

static std::unique_ptr<Resource> getProcess(const HttpRequest& request) {
    return FileResource::make(getPathFromURI(request.getURI()));
}

static std::unique_ptr<Resource> headProcess(const HttpRequest& request) {
    return FileResource::make(getPathFromURI(request.getURI()));
}

static bool readConfig(int argc, char const* argv[]) {
    std::filesystem::path configPath(argv[0]);
    configPath = configPath.parent_path() / "config.txt";


    std::ifstream configFile(configPath);

    if (!configFile) {
        return false;
    }
    std::string line;
    if (!std::getline(configFile, line)) {
        return false;
    }

    serverRootPath = line;

    return std::filesystem::exists(serverRootPath);
}

static void printInfo() {
    auto addresses = getLocalAddresses();

    for (auto& address : addresses) {
        std::cout << "Name: " << address.name <<
            "\tType: " << (address.type == AddressType::IPV4 ? "IPV4" : "IPV6") <<
            "\tIP: " << address.value << std::endl;
    }

    std::cout << "Server Root: " << serverRootPath << std::endl;
}

int main(int argc, char const* argv[]) {
    if (!readConfig(argc, argv)) {
        std::cout << "Unable to read config file.\n"
            "Please insert a valid root directory for the server in the first line of the config.txt file."
            << std::endl;
        return 1;
    }

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
    }
    catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}
