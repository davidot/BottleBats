add_executable(BBServer
        auth/BasicServer.cpp
        ContainerBuilder.cpp
        auth/Authenticator.cpp
        crowtest.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        vijf/GamePlayer.cpp
        vijf/BotCreator.cpp
        elevated/BotCreator.cpp
        )

find_package(Crow)
target_compile_options(BBServer PRIVATE -Wno-error)
target_link_libraries(BBServer PUBLIC Crow::Crow pq pqxx SubProcess LibVijf)

add_executable(BBRunGames runsomegames.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        vijf/GamePlayer.cpp
        )
target_link_libraries(BBRunGames PUBLIC pq pqxx SubProcess LibVijf)
