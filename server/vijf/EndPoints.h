#pragma once

#include "../auth/BasicServer.h"

namespace BBServer {

void add_vijf_endpoints(ServerType& app, boost::asio::io_service& io_service);

}
