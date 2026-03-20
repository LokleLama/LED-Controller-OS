cmake_minimum_required(VERSION 3.14)

option(BUILD_TESTS "Build host-side unit tests instead of firmware" OFF)

if(BUILD_TESTS)
    project(LEDControllerTests C CXX)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
    endif()

    add_subdirectory(test)
else()
    # Select target board: "pico" (RP2040) or "pico2" (RP2350)
    # Override with: cmake -DPICO_BOARD=pico2 ..
    if(NOT DEFINED PICO_BOARD)
        set(PICO_BOARD pico CACHE STRING "Target board (pico or pico2)")
    endif()

    include(pico_sdk_import.cmake)

    project(LEDController C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Set build type if not specified
    # Debug;Release;MinSizeRel;RelWithDebInfo
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()

    pico_sdk_init()

    add_subdirectory(app)
    pico_enable_stdio_usb(LEDController 1)
    pico_enable_stdio_uart(LEDController 0)
endif()
