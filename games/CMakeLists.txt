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
if (NOT MSVC)
    target_compile_options(LibVijf PRIVATE -Werror)
endif()

add_executable(VijfTemp vijf/main.cpp)
target_link_libraries(VijfTemp PUBLIC LibVijf)

add_executable(test-vijf vijf/vijf-test.cpp)
target_link_libraries(test-vijf PUBLIC LibVijf)

add_library(LibElevated
        elevated/Elevator.cpp
        elevated/Building.cpp
        elevated/Simulation.cpp
        elevated/generation/FullGenerators.cpp
        elevated/generation/BasicGenerator.cpp
        elevated/generation/Generation.cpp
        elevated/generation/MetaGenerators.cpp
        elevated/generation/factory/NamedScenarios.cpp
        elevated/generation/factory/GeneratorSettings.cpp
        elevated/generation/factory/StringSettings.cpp
        elevated/generation/factory/OutputSettings.cpp
        elevated/generation/factory/FactoryFactory.cpp
        elevated/generation/building/SplitBuildingGenerators.cpp
        elevated/stats/Listener.cpp
        elevated/stats/MetaListener.cpp
        elevated/stats/PassengerStats.cpp
        elevated/stats/PowerStatsListener.cpp
        elevated/stats/SpecialEventsListener.cpp
        elevated/stats/ReplayListener.cpp
        elevated/algorithm/Algorithm.cpp
        elevated/algorithm/CyclingAlgorithm.cpp
        elevated/algorithm/ProcessAlgorithm.cpp
        )

target_include_directories(LibElevated INTERFACE .)
target_link_libraries(LibElevated PUBLIC SubProcess)
if (MSVC)
    target_compile_options(LibElevated PRIVATE /WX)
else()
    target_compile_options(LibElevated PRIVATE -Werror)
endif()



add_executable(elevated-test
        elevated/test/test-main.cpp
        elevated/test/elevator-test.cpp
        elevated/test/event-test.cpp
        elevated/test/building-test.cpp
        elevated/test/simulator-test.cpp
        elevated/test/protocol-test.cpp
        elevated/test/gen-test.cpp
        elevated/test/info-test.cpp
        )

target_link_libraries(elevated-test PUBLIC Catch2::Catch2 LibElevated)

add_test(NAME ElevatedUnitTests COMMAND elevated-test)

add_executable(elevated-tester
        elevated/elevated-algorithm-tester.cpp)
target_link_libraries(elevated-tester PUBLIC LibElevated)
set_target_properties(elevated-tester PROPERTIES
        LINK_SEARCH_START_STATIC ON
        LINK_SEARCH_END_STATIC ON
        )


add_test(NAME elevated-protocol-test COMMAND elevated-tester python3 examples/python/cycle.py WORKING_DIRECTORY ..)
add_test(NAME elevated-protocol-test2 COMMAND elevated-tester python3 examples/python/random_travel.py WORKING_DIRECTORY ..)

add_test(NAME elevated-cwd-test1 COMMAND elevated-tester --cwd examples/ python3 python/random_travel.py WORKING_DIRECTORY ..)
add_test(NAME elevated-cwd-test2 COMMAND elevated-tester --cwd examples/python python3 random_travel.py WORKING_DIRECTORY ..)
