add_library(BBServer INTERFACE)

target_sources(BBServer INTERFACE
        auth/BasicServer.cpp
        ContainerBuilder.cpp
        auth/Authenticator.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        )

find_package(Crow)
target_compile_options(BBServer INTERFACE -Wno-error)
target_link_libraries(BBServer INTERFACE Crow::Crow pq pqxx SubProcess)

add_executable(vijf-server
        vijf-server.cpp
        vijf/GamePlayer.cpp
        vijf/BotCreator.cpp
        vijf/EndPoints.cpp
        )

target_link_libraries(vijf-server PUBLIC BBServer LibVijf)

add_executable(BBRunGames runsomegames.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        vijf/GamePlayer.cpp
        )
target_link_libraries(BBRunGames PUBLIC pq pqxx SubProcess LibVijf)
