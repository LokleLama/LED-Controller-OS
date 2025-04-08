
set(OUTPUT_NAME LEDController)

set (INCLUDE_DIRS "")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "include/*.h*" "src/Commands/*.h*")
file(GLOB_RECURSE SRC "src/*.c*" "src/Commands/*.c*")


foreach (_headerFile ${HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND INCLUDE_DIRS ${_dir})
endforeach()
list (REMOVE_DUPLICATES INCLUDE_DIRS)

add_executable(
    ${OUTPUT_NAME}
    ${SRC}
)

target_link_libraries(${OUTPUT_NAME} pico_stdlib tinyusb_device tinyusb_board)

target_include_directories(${OUTPUT_NAME} PRIVATE ${INCLUDE_DIRS})

pico_add_extra_outputs(${OUTPUT_NAME})
