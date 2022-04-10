#include <pqxx/transaction>
#include "BasicServer.h"

namespace BBServer {

void add_authentication(ServerType& app)
{
    // FIXME: Do not hardcode the max-age
    //
    static std::string cookie_extra_data = "; Max-Age: 360000; HttpOnly";
    CROW_ROUTE(app, "/api/auth/login")([&](crow::request& req, crow::response& resp){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        if (base_context.user.logged_in)
            return fail_response_with_message(resp, 400, "Already logged in");

        auto& authorization_header = req.get_header_value("Authorization");

        if (authorization_header.empty())
            return fail_response_with_message(resp, 400, "No credentials given");

        if (std::count(authorization_header.begin(), authorization_header.end(), ':') > 1)
            return fail_response_with_message(resp, 400, "Cannot have : in name or token");

        auto user_or_none = BBServer::LoggedInUser::get_user_from_auth_header(*base_context.database_connection, authorization_header);

        if (!user_or_none.has_value())
            return fail_response_with_message(resp, 403, "Combination of name and token unknown");

        auto& cookies = app.get_context<crow::CookieParser>(req);
        std::string cookie_value = crow::utility::base64encode(authorization_header, authorization_header.size()) + cookie_extra_data;
        cookies.set_cookie(BBServer::COOKIE_AUTH_NAME, cookie_value);

        resp.end("Logged in");
    });

    CROW_ROUTE(app, "/api/auth/register").methods(crow::HTTPMethod::POST)
        ([&](crow::request& req, crow::response& resp){
            auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
            if (base_context.user.logged_in)
                return fail_response_with_message(resp, 400, "Already logged in");


            auto& authorization_header = req.get_header_value("Authorization");

            if (authorization_header.empty())
                return fail_response_with_message(resp, 400, "No credentials given");


            size_t found = authorization_header.find(':');
            if (found == std::string::npos)
                return fail_response_with_message(resp, 400, "missing value");

            std::string username = authorization_header.substr(0, found);
            std::string token = authorization_header.substr(found+1);
            if (token.find(':') != std::string::npos)
                return fail_response_with_message(resp, 400, "Cannot have : in name or token");

            if (BBServer::user_exists(*base_context.database_connection, username))
                return fail_response_with_message(resp, 400, "Username already in user");

            pqxx::work transaction{*base_context.database_connection};

            base_context.database_connection->prepare("INSERT INTO users (name, token, display_name) VALUES ($1, $2, $1)");
            auto result = transaction.exec_prepared("", username, token);
            if (result.affected_rows() == 0)
                return fail_response_with_message(resp, 500, "Failed to register, try again maybe?");

            transaction.commit();

            auto& cookies = app.get_context<crow::CookieParser>(req);
            std::string cookie_value = crow::utility::base64encode(username + ":" + token, authorization_header.size()) + cookie_extra_data;
            cookies.set_cookie(BBServer::COOKIE_AUTH_NAME, cookie_value);

            resp.end("Registered user");
        });

    CROW_ROUTE(app, "/api/auth/info")
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&](crow::request const& req) {
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        return crow::json::wvalue {
            {"displayName", base_context.user.display_name},
            {"admin", base_context.user.is_admin()}
        };
    });

    CROW_ROUTE(app, "/api/auth/logout")
    .methods(crow::HTTPMethod::POST)
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&](crow::request const& req) {
        auto& cookies = app.get_context<crow::CookieParser>(req);
        cookies.set_cookie(COOKIE_AUTH_NAME, "this-cookie-should-be-deleted; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
        return "Logged out";
    });

}

void fail_response_with_message(crow::response& resp, int code, std::string const& message)
{
    resp.code = code;
    resp.set_header("Content-Type", "text/plain");
    resp.end(message);
}

}
