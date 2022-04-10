#pragma once

#include <crow/middleware.h>
#include <crow/middlewares/cookie_parser.h>
#include <iostream>
#include <pqxx/connection>
#include "../database/ConnectionPool.h"
#include "../database/User.h"

namespace BBServer {

constexpr char const* const COOKIE_AUTH_NAME = "BB_Login";

struct BaseMiddleware {
    struct context {
        std::unique_ptr<pqxx::connection> database_connection;
        LoggedInUser user{};
    };

    template<typename AllContext>
    void before_handle(crow::request&, crow::response& resp, context& ctx, AllContext& all_context)
    {
        // Set default so we get slightly less errors....
        resp.set_header("Content-Type", "text/plain");

        ctx.database_connection = ConnectionPool::get_connection();
        crow::CookieParser::context& cookies = all_context.template get<crow::CookieParser>();

        auto log_in = cookies.get_cookie(COOKIE_AUTH_NAME);
        if (log_in.empty()) {
            return;
        }

        std::string decrypted_log_in = crow::utility::base64decode(log_in);

        if (std::count(decrypted_log_in.begin(), decrypted_log_in.end(), ':') == 1) {

            auto user_or_none = LoggedInUser::get_user_from_auth_header(*ctx.database_connection, decrypted_log_in);

            if (user_or_none.has_value()) {
                ctx.user = std::move(*user_or_none);
                return;
            }
        }

        // Log out because else we might get weird stuff
        cookies.set_cookie(COOKIE_AUTH_NAME, "this-cookie-should-be-deleted!; expires=Thu, 01 Jan 1970 00:00:00 GMT");
    }

    void after_handle(crow::request&, crow::response&, context& ctx);

};

struct AuthGuard : crow::ILocalMiddleware {
    struct context {
    };

    template <typename AllContext>
    void before_handle(crow::request& req, crow::response& res, context&, AllContext& all_ctx)
    {
        BaseMiddleware::context& auth_context = all_ctx.template get<BaseMiddleware>();

        if (auth_context.user.logged_in)
            return;

        res.code = 403;
        res.set_header("Content-Type", "text/plain");
        res.end("Login first!");
    }

    void after_handle(crow::request&, crow::response&, context&);
};

struct AdminGuard : crow::ILocalMiddleware {
    struct context {
    };

    template <typename AllContext>
    void before_handle(crow::request& req, crow::response& res, context& ctx, AllContext& all_ctx)
    {
        BaseMiddleware::context& auth_context = all_ctx.template get<BaseMiddleware>();

        if (!auth_context.user.logged_in || !auth_context.user.is_admin()) {
            res.code = 403;
            res.end();
            return;
        }
    }

    void after_handle(crow::request&, crow::response&, context&);
};

}
