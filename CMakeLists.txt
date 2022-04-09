cmake_minimum_required(VERSION 3.21)
project(BottleBats)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE ccache)
if (CCACHE)
    message(STATUS "found and using ccache")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
else ()
    message(STATUS "ccache not found cannot use")
endif ()

if (MSVC)
    add_compile_options(/W2 /WX)
    add_compile_options(/Zc:preprocessor)
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-Werror)
endif()

add_library(SubProcess INTERFACE)
target_sources(SubProcess PUBLIC util/Process_Base.cpp)

if (UNIX)
    target_compile_definitions(SubProcess INTERFACE POSIX_PROCESS=1)
    target_sources(SubProcess INTERFACE util/Process_Unix.cpp)
elseif(WIN32)
    target_compile_definitions(SubProcess INTERFACE WINDOWS_PROCESS=1 WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN=1)
    target_sources(SubProcess INTERFACE util/Process_Windows.cpp)
elseif (MINGW)
    message(ERROR "Do not support MINGW at the moment")
    # Explicitly target Windows 10. This allows us to use features that are only available on newer versions of Windows.
    #    add_definitions(-D_WIN32_WINNT=0x0A00 -DWIN32_LEAN_AND_MEAN)
    #    add_compile_definitions(WPWRAPPER_WIN=1)
else()
    message(ERROR "Don't know this operating system.")
endif()

add_subdirectory(games)
