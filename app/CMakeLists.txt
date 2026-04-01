
set(OUTPUT_NAME LEDController)

set (INCLUDE_DIRS "")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "include/*.h" 
                                            "include/Commands/*.h"
                                            "include/HLKStack/*.h" 
                                            "include/LED/*.h" 
                                            "include/PIO/*.h" 
                                            "include/RTC/*.h" 
                                            "include/Utils/*.h"
                                            "include/VariableStore/*.h" )
file(GLOB_RECURSE SRC "src/*.c*" 
                      "src/Commands/*.c*" 
                      "src/HLKStack/*.c*" 
                      "src/LED/*.c*" 
                      "src/RTC/*.c*" 
                      "src/Utils/*.c*"
                      "src/VariableStore/*.c*" )

foreach (_headerFile ${HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND INCLUDE_DIRS ${_dir})
endforeach()
list (REMOVE_DUPLICATES INCLUDE_DIRS)

add_executable(
    ${OUTPUT_NAME}
    ${SRC}
)

# Suppress GCC psABI compatibility notes (e.g., iterator parameter passing changed in GCC 7.1).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${OUTPUT_NAME} PRIVATE -Wno-psabi)
endif()

# Detect platform and set defaults
if(PICO_PLATFORM STREQUAL "rp2040")
    set(TARGET_CHIP "rp2040")
    if(NOT DEFINED PICO_FLASH_SIZE_BYTES)
        set(PICO_FLASH_SIZE_BYTES 2097152)  # 2MB default for RP2040
    endif()
    set(TOTAL_RAM_BYTES 270336)  # 264KB
else()
    set(TARGET_CHIP "rp2350")
    if(NOT DEFINED PICO_FLASH_SIZE_BYTES)
        set(PICO_FLASH_SIZE_BYTES 4194304)  # 4MB default for RP2350
    endif()
    set(TOTAL_RAM_BYTES 532480)  # 520KB
endif()

message(STATUS "Building for ${TARGET_CHIP} (board: ${PICO_BOARD}, flash: ${PICO_FLASH_SIZE_BYTES})")

set(SPFS_RESERVED_SIZE 262144)  # 256KB for filesystem
math(EXPR PROGRAM_FLASH_SIZE "${PICO_FLASH_SIZE_BYTES} - ${SPFS_RESERVED_SIZE}")
math(EXPR SPFS_FLASH_OFFSET "${PROGRAM_FLASH_SIZE}")
math(EXPR SPFS_FLASH_ORIGIN "0x10000000 + ${SPFS_FLASH_OFFSET}")

# Add compile definitions so your code knows where SPFS lives
target_compile_definitions(${OUTPUT_NAME} PRIVATE
    PICO_FLASH_SIZE_BYTES=${PICO_FLASH_SIZE_BYTES}
    SPFS_FLASH_OFFSET=${SPFS_FLASH_OFFSET}
    SPFS_FLASH_SIZE=${SPFS_RESERVED_SIZE}
    TOTAL_RAM_BYTES=${TOTAL_RAM_BYTES}
)

# Create custom flash region file to override SDK default
set(CUSTOM_FLASH_REGION ${CMAKE_CURRENT_BINARY_DIR}/pico_flash_region.ld)
file(WRITE ${CUSTOM_FLASH_REGION} "/* Custom flash region with SPFS reservation */\n")
file(APPEND ${CUSTOM_FLASH_REGION} "FLASH(rx) : ORIGIN = 0x10000000, LENGTH = ${PROGRAM_FLASH_SIZE}\n")
file(APPEND ${CUSTOM_FLASH_REGION} "SPFS(rx) : ORIGIN = ${SPFS_FLASH_ORIGIN}, LENGTH = ${SPFS_RESERVED_SIZE}\n")

# Tell CMake to use our custom flash region
target_link_options(${OUTPUT_NAME} PRIVATE "LINKER:--defsym=__SPFS_start__=${SPFS_FLASH_ORIGIN}")
target_link_options(${OUTPUT_NAME} PRIVATE "LINKER:--defsym=__SPFS_size__=${SPFS_RESERVED_SIZE}")
target_link_options(${OUTPUT_NAME} PRIVATE "LINKER:-L${CMAKE_CURRENT_BINARY_DIR}")

# Generate detailed memory map file
target_link_options(${OUTPUT_NAME} PRIVATE 
    "LINKER:-Map=${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}.elf.map"
    "LINKER:--print-memory-usage"
)

# Post-build: print comprehensive memory layout
add_custom_command(TARGET ${OUTPUT_NAME} POST_BUILD
    COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/analyze_memory.sh $<TARGET_FILE:${OUTPUT_NAME}> ${PROGRAM_FLASH_SIZE} ${TARGET_CHIP}
    COMMENT "Displaying comprehensive memory layout after build"
)

pico_generate_pio_header(LEDController ${CMAKE_CURRENT_LIST_DIR}/include/PIO/led.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/include/PIO)


target_link_libraries(${OUTPUT_NAME} pico_stdlib
                                     tinyusb_device 
                                     tinyusb_board 
                                     hardware_pio 
                                     hardware_dma 
                                     hardware_adc 
                                     hardware_pwm 
                                     hardware_clocks 
                                     hardware_irq 
                                     hardware_timer 
                                     hardware_watchdog 
                                     hardware_uart 
                                     hardware_i2c 
                                     hardware_spi
                                     hardware_flash)

# Platform-specific libraries
if(TARGET_CHIP STREQUAL "rp2040")
    target_link_libraries(${OUTPUT_NAME} hardware_rtc)
else()
    target_compile_definitions(${OUTPUT_NAME} PRIVATE PICO_RP2350=1)
endif()

target_include_directories(${OUTPUT_NAME} PRIVATE ${INCLUDE_DIRS}
                                                  ${CMAKE_SOURCE_DIR}/ThirdParty/json/include)

pico_add_extra_outputs(${OUTPUT_NAME})
