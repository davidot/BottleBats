add_library(LibVijf
        vijf/Cards.cpp
        vijf/Game.cpp
        vijf/Vijf.cpp
        vijf/players/BasicPlayers.cpp
        vijf/players/ProcessPlayer.cpp
        vijf/players/VijfPlayer.cpp
        )

target_link_libraries(LibVijf PUBLIC SubProcess)
target_include_directories(LibVijf INTERFACE .)

add_executable(VijfTemp vijf/main.cpp)
target_link_libraries(VijfTemp PUBLIC LibVijf)

add_executable(test-vijf vijf/vijf-test.cpp)
target_link_libraries(test-vijf PUBLIC LibVijf)

include(FetchContent)

fetchcontent_declare(
        catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.13.9
)

FetchContent_GetProperties(catch)
if (NOT catch_POPULATED)
    FetchContent_Populate(catch)
    add_subdirectory(${catch_SOURCE_DIR} ${catch_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

add_library(LibElevated
        elevated/Elevator.cpp
        elevated/Building.cpp
        elevated/generation/Generation.cpp
        elevated/stats/Listener.cpp
        )

target_include_directories(LibElevated INTERFACE .)

add_executable(elevated-test
        elevated/test/test-main.cpp
        elevated/test/elevator-test.cpp
        elevated/test/event-test.cpp
        )
target_link_libraries(elevated-test PUBLIC Catch2::Catch2 LibElevated)
