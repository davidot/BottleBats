add_executable(BBServer
        crowtest.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        auth/Authenticator.cpp
        BasicServer.cpp
        )

find_package(Crow)
target_compile_options(BBServer PRIVATE -Wno-error)
target_link_libraries(BBServer PUBLIC Crow::Crow pq pqxx)
