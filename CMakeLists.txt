cmake_minimum_required(VERSION 4.3)

project(intergrid)

add_library(intergrid STATIC
    src/intergrid.c
    src/config.c
    src/gossip.c
    src/measure.c
    src/protect.c
)

target_include_directories(intergrid PUBLIC include)

find_library(NGSPICE_LIBRARY
    NAMES ngspice
    PATHS /usr/lib64 /lib64
    REQUIRED
)

add_executable(ngspice_platform
    platforms/ngspice/platform.c
)

target_link_libraries(ngspice_platform PRIVATE
    ${NGSPICE_LIBRARY}
)

target_link_libraries(ngspice_platform PRIVATE
    intergrid
    m
)
