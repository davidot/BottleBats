fetchcontent_declare(
        crow
        GIT_REPOSITORY https://github.com/CrowCpp/Crow
        GIT_TAG 31ca14571678763313d9f4eaa53b049d4791a1e3 # Commit right after my merge :) Update to 1.3 once released
)

FetchContent_GetProperties(crow)
if (NOT crow_POPULATED)
        FetchContent_Populate(crow)
        set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "Disable crow examples")
        set(CROW_BUILD_TESTS OFF CACHE BOOL "Disable crow tests")
        set(CROW_INSTALL OFF CACHE BOOL "Disable crow install")
        set(CROW_INSTALL OFF CACHE BOOL "Disable crow install")
        add_subdirectory(${crow_SOURCE_DIR} ${crow_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

add_library(BBServer INTERFACE)

target_sources(BBServer INTERFACE
        auth/BasicServer.cpp
        ContainerBuilder.cpp
        auth/Authenticator.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        )

target_compile_options(BBServer INTERFACE -Wno-error)
target_compile_definitions(BBServer INTERFACE ASIO_HAS_BOOST_DATE_TIME BOOST_REGEX_NO_LIB CROW_ENFORCE_WS_SPEC CROW_DISABLE_STATIC_DIR)
target_link_libraries(BBServer INTERFACE Crow::Crow pq pqxx SubProcess)

add_executable(vijf-server
        vijf-server.cpp
        vijf/GamePlayer.cpp
        vijf/BotCreator.cpp
        vijf/EndPoints.cpp
        )

target_link_libraries(vijf-server PUBLIC BBServer LibVijf)

add_executable(full-server
        full-server.cpp
        elevated/BotCreator.cpp
        elevated/Endpoints.cpp
        elevated/Runner.cpp
        vijf/GamePlayer.cpp
        vijf/BotCreator.cpp
        vijf/EndPoints.cpp
        )

target_link_libraries(full-server PUBLIC BBServer LibVijf LibElevated)


add_executable(BBRunGames vijf/runsomegames.cpp
        database/ConnectionPool.cpp
        database/User.cpp
        vijf/GamePlayer.cpp
        )
target_link_libraries(BBRunGames PUBLIC pq pqxx SubProcess LibVijf)

add_executable(minimal-ws minimal-ws.cpp)
target_link_libraries(minimal-ws PUBLIC BBServer)