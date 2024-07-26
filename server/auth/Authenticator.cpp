#include "Authenticator.h"

namespace BBServer {

void AuthGuard::after_handle(crow::request&, crow::response&, AuthGuard::context&)
{
}
void AdminGuard::after_handle(crow::request&, crow::response&, AdminGuard::context&)
{
}
void BaseMiddleware::after_handle(crow::request&, crow::response&, BaseMiddleware::context& ctx)
{
    if (ctx.database_connection)
        ConnectionPool::return_connection(std::move(ctx.database_connection));
}
}
