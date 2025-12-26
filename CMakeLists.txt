cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(LEDController C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Set build type if not specified
# Debug;Release;MinSizeRel;RelWithDebInfo
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Or force specific flags:
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

pico_sdk_init()

add_subdirectory(app)
pico_enable_stdio_usb(LEDController 1)
pico_enable_stdio_uart(LEDController 0)
