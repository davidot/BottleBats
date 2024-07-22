#pragma once
#include "../auth/BasicServer.h"

namespace BBServer {

void add_elevated_endpoints(ServerType& app, asio::io_service& io_service);

}
