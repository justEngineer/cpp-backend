#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {
    
    void ReportError(beast::error_code ec, std::string_view operation) {
        std::cout << "Error while operation " << operation << " , error code: " << ec << std::endl;
    }
}  // namespace http_server
