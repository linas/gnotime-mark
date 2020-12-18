cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

add_executable(${PROJECT_NAME}
    err-throw.c
    fake_qof.c)
target_link_libraries(${PROJECT_NAME}
    PkgConfig::GLIB2)
