cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

add_executable(${PROJECT_NAME}
    dialog.c
    err-throw.c)
target_link_libraries(${PROJECT_NAME}
    PkgConfig::GLIB2
    PkgConfig::GTK2
    PkgConfig::X11)
