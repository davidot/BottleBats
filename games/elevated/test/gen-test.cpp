#include <catch2/catch.hpp>
#include <elevated/Types.h>
#include <elevated/generation/Generation.h>

using namespace Elevated;

TEST_CASE("Next requests at semantics", "[types]") {
    SECTION("Same requests are equal") {
        SECTION("At") {
            Time t = GENERATE(0, 1, 5);
            auto request_at_0 = NextRequests::at(t);
            auto request_at_1 = NextRequests::at(t);
            REQUIRE(request_at_0 == request_at_1);
            REQUIRE_FALSE(request_at_0 != request_at_1);
            REQUIRE(request_at_0.next_request_time == t);
            REQUIRE(request_at_1.next_request_time == t);
        }

        auto unknown_0 = NextRequests::unknown();
        auto unknown_1 = NextRequests::unknown();
        REQUIRE(unknown_0 == unknown_1);
        REQUIRE_FALSE(unknown_0 != unknown_1);

        auto done_0 = NextRequests::done();
        auto done_1 = NextRequests::done();
        REQUIRE(done_0 == done_1);
        REQUIRE_FALSE(done_0 != done_1);
    }

    SECTION("Earlier times at come first") {
        Time t = GENERATE(0, 1, 5, 10, 1000, 10000);
        Time t_with_offset = t + GENERATE(1, 2, 3, 4, 5, 1000, 1500);
        auto request_at_t = NextRequests::at(t);
        auto request_at_t_with_offset = NextRequests::at(t_with_offset);
        REQUIRE(request_at_t < request_at_t_with_offset);
    }

    SECTION("Any time comes before unknown and done") {
        Time t = GENERATE(0, 1, 5, 10, 1000, 10000);
        auto request_at = NextRequests::at(t);
        REQUIRE(request_at < NextRequests::unknown());
        REQUIRE(request_at < NextRequests::done());
    }

    SECTION("Unknown comes before done") {
        REQUIRE(NextRequests::unknown() < NextRequests::done());
    }
}
