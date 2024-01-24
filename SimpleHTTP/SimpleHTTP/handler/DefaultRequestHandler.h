#pragma once
#include <SimpleHTTP/http.h>

namespace simpleHTTP {

class DefaultRequestHandler
{
public:
    DefaultRequestHandler();
    DefaultRequestHandler(const DefaultRequestHandler&) = delete;

    bool processRequest(const HttpRequest& request, HttpResponse& response);

    DefaultRequestHandler& operator=(const DefaultRequestHandler&) = delete;

    ~DefaultRequestHandler();
private:
    
};

} // namespace simpleHTTP
