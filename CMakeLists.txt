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

find_path(NGSPICE_INCLUDE_DIR
    NAMES ngspice/sharedspice.h
    REQUIRED
)

add_executable(ngspice_platform
    platforms/ngspice/platform.c
)

target_include_directories(ngspice_platform PRIVATE
    ${NGSPICE_INCLUDE_DIR}
)

target_link_libraries(ngspice_platform PRIVATE
    ${NGSPICE_LIBRARY}
)

target_link_libraries(ngspice_platform PRIVATE
    intergrid
    m
)

# Arduino: `make arduino_hello` builds the sketch, `make arduino_upload` flashes it,
# `make arduino_emulate` runs it in simavr (quit with Ctrl-C).
set(ARDUINO_FQBN "arduino:avr:uno" CACHE STRING "Arduino board (FQBN)")
set(ARDUINO_PORT "" CACHE STRING "Arduino serial port, e.g. /dev/cu.usbmodem1101")
set(ARDUINO_SKETCH ${CMAKE_SOURCE_DIR}/platforms/arduino/hello)
set(ARDUINO_OUT ${CMAKE_BINARY_DIR}/arduino)

add_custom_target(arduino_hello
    COMMAND arduino-cli compile
        --fqbn ${ARDUINO_FQBN}
        --build-property "build.extra_flags=-I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}"
        --output-dir ${ARDUINO_OUT}
        ${ARDUINO_SKETCH}
    USES_TERMINAL
)

add_custom_target(arduino_upload
    COMMAND arduino-cli upload --fqbn ${ARDUINO_FQBN} --port ${ARDUINO_PORT} --input-dir ${ARDUINO_OUT} ${ARDUINO_SKETCH}
    DEPENDS arduino_hello
    USES_TERMINAL
)

add_custom_target(arduino_emulate
    COMMAND simavr -m atmega328p -f 16000000 ${ARDUINO_OUT}/hello.ino.elf
    DEPENDS arduino_hello
    USES_TERMINAL
)

# Generate platforms/arduino/.clangd with this machine's Arduino paths (editor only).
execute_process(
    COMMAND arduino-cli config get directories.data
    OUTPUT_VARIABLE ARDUINO_DATA_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
file(GLOB ARDUINO_AVR_CORE "${ARDUINO_DATA_DIR}/packages/arduino/hardware/avr/*")
file(GLOB ARDUINO_AVR_LIBC "${ARDUINO_DATA_DIR}/packages/arduino/tools/avr-gcc/*/avr/include")

if(ARDUINO_AVR_CORE AND ARDUINO_AVR_LIBC)
    list(GET ARDUINO_AVR_CORE -1 ARDUINO_AVR_CORE)
    list(GET ARDUINO_AVR_LIBC -1 ARDUINO_AVR_LIBC)
    configure_file(
        ${CMAKE_SOURCE_DIR}/platforms/arduino/clangd.in
        ${CMAKE_SOURCE_DIR}/platforms/arduino/.clangd
        @ONLY
    )
endif()
