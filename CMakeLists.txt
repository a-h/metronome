cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)

# Export build commands for the CCLS language server.
# See https://github.com/MaskRay/ccls/wiki/Project-Setup
# See https://stackoverflow.com/questions/20059670/how-to-use-cmake-export-compile-commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(my_project)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(main
    main.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(main pico_stdlib hardware_i2c)

# Enable USB serial.
pico_enable_stdio_usb(main 1)
# Disable uart output
pico_enable_stdio_uart(main 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(main)
