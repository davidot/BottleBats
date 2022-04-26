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

add_library(LibElevated
        elevated/Elevator.cpp
        elevated/Building.cpp
        elevated/Simulation.cpp
        elevated/generation/FullGenerators.cpp
        elevated/generation/Generation.cpp
        elevated/stats/Listener.cpp
        elevated/algorithm/Algorithm.cpp
        )

target_include_directories(LibElevated INTERFACE .)

add_executable(elevated-test
        elevated/test/test-main.cpp
        elevated/test/elevator-test.cpp
        elevated/test/event-test.cpp
        elevated/test/building-test.cpp
        elevated/test/simulator-test.cpp
        elevated/test/gen-test.cpp
        )

target_link_libraries(elevated-test PUBLIC Catch2::Catch2 LibElevated)

add_test(NAME ElevatedUnitTests COMMAND elevated-test)
