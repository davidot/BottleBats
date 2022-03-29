cmake_minimum_required(VERSION 3.21)
project(BottleBats)

set(CMAKE_CXX_STANDARD 20)


add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Werror)
add_compile_options(-Wno-sign-compare)


add_executable(BottleBats
        main.cpp
        Process_Base.cpp)

if (UNIX)
    target_compile_definitions(BottleBats PUBLIC POSIX_PROCESS=1)
    target_sources(BottleBats PRIVATE Process_Unix.cpp)
elseif(WIN32)
    target_compile_definitions(BottleBats PUBLIC WINDOWS_PROCESS=1 WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN=1)
    target_sources(BottleBats PRIVATE Process_Windows.cpp)
elseif (MINGW)
    message(ERROR "Do not support MINGW at the moment")
    # Explicitly target Windows 10. This allows us to use features that are only available on newer versions of Windows.
    #    add_definitions(-D_WIN32_WINNT=0x0A00 -DWIN32_LEAN_AND_MEAN)
    #    add_compile_definitions(WPWRAPPER_WIN=1)
else()
    message(ERROR "Don't know this operating system.")
endif()
