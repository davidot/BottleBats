add_executable(BBServer
        auth/BasicServer.cpp
        BotCreator.cpp
        auth/Authenticator.cpp
        crowtest.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        GamePlayer.cpp
        )

find_package(Crow)
target_compile_options(BBServer PRIVATE -Wno-error)
target_link_libraries(BBServer PUBLIC Crow::Crow pq pqxx SubProcess LibVijf)

add_executable(BBRunGames runsomegames.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        GamePlayer.cpp

        )
target_link_libraries(BBRunGames PUBLIC pq pqxx SubProcess LibVijf)
