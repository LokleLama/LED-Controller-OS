
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
                                     hardware_rtc
                                     hardware_flash)

target_include_directories(${OUTPUT_NAME} PRIVATE ${INCLUDE_DIRS})

pico_add_extra_outputs(${OUTPUT_NAME})
