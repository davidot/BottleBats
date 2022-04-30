#pragma once

#include "Authenticator.h"
#include <crow/app.h>
#include <crow/middlewares/cookie_parser.h>

namespace BBServer {

using ServerType = crow::App<crow::CookieParser, BBServer::BaseMiddleware, BBServer::AuthGuard>;

void add_authentication(ServerType& app);

void fail_response_with_message(crow::response& resp, int code, std::string const& message);


}
