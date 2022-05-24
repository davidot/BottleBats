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
    option(DISABLE_ZI_DEBUG "Disable Z[iI7] flags for CI" OFF)
    if (DISABLE_ZI_DEBUG)
        message(STATUS "Attempting to disable ZI")
        message(STATUS "FLags: ${CMAKE_CXX_FLAGS}, Debug: ${CMAKE_CXX_FLAGS_DEBUG}, Release: ${CMAKE_CXX_FLAGS_RELEASE}")
        string(REGEX REPLACE "/Z[iI7]" ""
                CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        message(STATUS "FLags: ${CMAKE_CXX_FLAGS}, Debug: ${CMAKE_CXX_FLAGS_DEBUG}, Release: ${CMAKE_CXX_FLAGS_RELEASE}")
    endif()
    add_compile_options(/W2)
    add_compile_options(/Zc:preprocessor)
    add_compile_options(/EHsc)
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-Werror)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if(supported AND (CMAKE_BUILD_TYPE MATCHES Release))
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported or disabled: <${error}>")
endif()

add_library(SubProcess INTERFACE)
target_sources(SubProcess PUBLIC util/Process_Base.cpp)

if (UNIX)
    target_compile_definitions(SubProcess INTERFACE POSIX_PROCESS=1)
    target_sources(SubProcess INTERFACE util/Process_Unix.cpp)
    target_link_libraries(SubProcess INTERFACE pthread)
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

enable_testing()

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

add_subdirectory(games)

set(BUILD_SERVER OFF CACHE BOOL "Build server")
if (BUILD_SERVER)
    add_subdirectory(server)
else()
    message(STATUS "Not building server parts")
endif()

set(BUILD_GUI OFF CACHE BOOL "Build server")
if (BUILD_GUI)
    add_subdirectory(gui)
else()
    message(STATUS "Not building GUI")
endif()
