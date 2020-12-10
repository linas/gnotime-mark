cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

set(GTTGLADEDIR "/usr/local/share/gnotime")
configure_file(../cmake_config.h.in config.h @ONLY)

add_executable(${PROJECT_NAME}
    dialog.c
    err-throw.c
    util.c)
target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME}
    PkgConfig::GLIB2
    PkgConfig::GTK2
    PkgConfig::X11)
