fetchcontent_declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML
        GIT_TAG 12c091e0ce57675526b55297c856968e8800a3e2
        GIT_PROGRESS TRUE
)

set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_USE_STATIC_STD_LIBS TRUE CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS FALSE CACHE BOOL "" FORCE)

if (WIN32)
    set(WARNINGS_AS_ERRORS FALSE)
endif()

FetchContent_GetProperties(sfml)
if (NOT sfml_POPULATED)
    FetchContent_Populate(sfml)
    add_subdirectory(${sfml_SOURCE_DIR} ${sfml_BINARY_DIR} EXCLUDE_FROM_ALL)
    set_property(TARGET sfml-system PROPERTY INTERPROCEDURAL_OPTIMIZATION False)
    set_property(TARGET sfml-window PROPERTY INTERPROCEDURAL_OPTIMIZATION False)
    set_property(TARGET sfml-graphics PROPERTY INTERPROCEDURAL_OPTIMIZATION False)
    if (TARGET sfml-main)
        set_property(TARGET sfml-main PROPERTY INTERPROCEDURAL_OPTIMIZATION False)
    endif ()
endif ()

if (WIN32)
    unset(WARNINGS_AS_ERRORS)
endif()

fetchcontent_declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG e135cdbc247490df2e070ad515bec0cd18ecce6f
        GIT_PROGRESS TRUE
)


FetchContent_GetProperties(imgui)
if (NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    #    add_subdirectory(${imgui_SOURCE_DIR} ${imgui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

fetchcontent_declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/eliasdaler/imgui-sfml
        GIT_TAG a80d9547832c57a149cda452ed9456def5bb6328
        GIT_PROGRESS TRUE
)

FetchContent_GetProperties(imgui-sfml)
if (NOT imgui-sfml_POPULATED)
    FetchContent_Populate(imgui-sfml)

    # Set relevant vars
    set(IMGUI_DIR ${imgui_SOURCE_DIR})
    # This is a hacky way to force imconfig settings, this may infact be written multiple times
    # which is why the #undef is also written.
    file(APPEND ${imgui_SOURCE_DIR}/imconfig.h "\n#undef ImDrawIdx\n#define ImDrawIdx unsigned int")
    set(IMGUI_SFML_FIND_SFML CACHE BOOL FALSE)
    set(IMGUI_SFML_IMGUI_DEMO ON)

    message(STATUS ${imgui-sfml_SOURCE_DIR} , ${imgui-sfml_BINARY_DIR})

    add_subdirectory(${imgui-sfml_SOURCE_DIR} ${imgui-sfml_BINARY_DIR} EXCLUDE_FROM_ALL)

    set_property(TARGET ImGui-SFML PROPERTY INTERPROCEDURAL_OPTIMIZATION False)
endif ()


fetchcontent_declare(
        implot
        GIT_REPOSITORY https://github.com/epezent/implot
        GIT_TAG dbb461db24b44410786e6fad6cffa3f98656b357
        GIT_PROGRESS TRUE
)

FetchContent_GetProperties(implot)
if (NOT implot_POPULATED)
    FetchContent_Populate(implot)

    add_library(Implot INTERFACE)
    target_sources(Implot INTERFACE ${implot_SOURCE_DIR}/implot.cpp ${implot_SOURCE_DIR}/implot_items.cpp ${implot_SOURCE_DIR}/implot_demo.cpp)
    target_include_directories(Implot INTERFACE ${implot_SOURCE_DIR})
endif ()

add_executable(elevated
        main.cpp
        VisualGeneratorSettings.cpp
        ImGui/ImGuiExtensions.cpp
        Visualizer.cpp
        Config.cpp
        DirWatcher.cpp
        )

target_link_libraries(elevated PUBLIC sfml-graphics LibElevated ImGui-SFML::ImGui-SFML Implot)
