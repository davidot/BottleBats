
#include "elevated/Endpoints.h"
#include "vijf/EndPoints.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/system/error_code.hpp>
#include <thread>

boost::asio::io_service io_service;
boost::posix_time::seconds interval(5);
boost::asio::deadline_timer timer(io_service, interval);


void tick(const boost::system::error_code&) {
    timer.expires_at(timer.expires_at() + interval);
    timer.async_wait(tick);
}

int main()
{
    srand(time(nullptr));

    try {
        BBServer::ConnectionPool::initialize_pool(4, "postgresql://postgres:passwrd@localhost:6543/postgres");
    } catch (std::exception const &e)
    {
        std::cerr << "Could not establish connection to database!\n";
        std::cerr << e.what() << '\n';
        return 1;
    }

    BBServer::ServerType app;

    BBServer::add_authentication(app);

    BBServer::add_vijf_endpoints(app, io_service);
    BBServer::add_elevated_endpoints(app, io_service);

    auto running = app.port(18081)
                       .bindaddr("127.0.0.1")
                       .concurrency(4)
                       .run_async();

    timer.async_wait(tick);

    std::thread t {[&]{
        io_service.run();
    }};

    running.wait();
    app.stop();
    io_service.stop();

    t.join();

}
