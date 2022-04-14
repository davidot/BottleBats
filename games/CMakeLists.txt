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
