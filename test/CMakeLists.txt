cmake_minimum_required(VERSION 3.14)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ---------------------------------------------------------------------------
#  App sources to compile for testing (no hardware, no Main.cpp)
# ---------------------------------------------------------------------------
set(APP_SRC_DIR ${CMAKE_SOURCE_DIR}/app/src)
set(CORE_SOURCES
    ${APP_SRC_DIR}/Flash/flash.cpp
    ${APP_SRC_DIR}/Flash/SPFS.cpp
    ${APP_SRC_DIR}/Flash/SPFS.File.cpp
    ${APP_SRC_DIR}/Flash/SPFS.Directory.cpp
    ${APP_SRC_DIR}/Flash/SPFS.ReadOnlyFile.cpp
    ${APP_SRC_DIR}/VariableStore/VariableStore.cpp
    ${APP_SRC_DIR}/Utils/base64.cpp
    ${APP_SRC_DIR}/Utils/hexadecimal.cpp
    ${APP_SRC_DIR}/Utils/ValueConverter.cpp
    ${APP_SRC_DIR}/Mainloop.cpp
    ${APP_SRC_DIR}/devices/PIODevice.cpp
    ${APP_SRC_DIR}/devices/WS2812.cpp
    ${APP_SRC_DIR}/devices/dotMatrix8x8.cpp
)

# ---------------------------------------------------------------------------
#  Mock sources (replace Pico SDK and FlashHAL)
# ---------------------------------------------------------------------------
set(MOCK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/src/mock_flash_hal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/src/mock_pio.cpp
)

# ---------------------------------------------------------------------------
#  Testable library: app logic + mocks, no hardware
# ---------------------------------------------------------------------------
add_library(ledcontroller_testable STATIC ${CORE_SOURCES} ${MOCK_SOURCES})

# Discover all app header directories (same pattern as app/CMakeLists.txt)
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/app/include/*.h"
)
set(APP_INCLUDE_DIRS "")
foreach(_headerFile ${APP_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND APP_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES APP_INCLUDE_DIRS)

# Include order matters: mock headers first to intercept real SDK includes,
# then app header directories, then third-party
target_include_directories(ledcontroller_testable PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/pico_sdk   # Stub SDK + mock FlashHAL (FIRST)
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/src         # mock_flash.h helper
    ${APP_INCLUDE_DIRS}                           # App headers (all subdirs)
    ${CMAKE_SOURCE_DIR}/ThirdParty/json/include   # ArduinoJson
)

target_compile_definitions(ledcontroller_testable PUBLIC
    PICO_ON_DEVICE=0
    PICO_RP2040=1
    SPFS_FLASH_OFFSET=1835008
    SPFS_FLASH_SIZE=262144
    PICO_FLASH_SIZE_BYTES=2097152
    TOTAL_RAM_BYTES=270336
)

target_compile_features(ledcontroller_testable PUBLIC cxx_std_17)

# ---------------------------------------------------------------------------
#  Test executables
# ---------------------------------------------------------------------------
add_executable(test_utils unit/test_utils.cpp)
target_link_libraries(test_utils ledcontroller_testable GTest::gtest_main)

add_executable(test_variable_store unit/test_variable_store.cpp)
target_link_libraries(test_variable_store ledcontroller_testable GTest::gtest_main)

add_executable(test_spfs unit/test_spfs.cpp)
target_link_libraries(test_spfs ledcontroller_testable GTest::gtest_main)

add_executable(test_dot_matrix unit/test_dot_matrix.cpp)
target_link_libraries(test_dot_matrix ledcontroller_testable GTest::gtest_main)

add_executable(test_all
    unit/test_utils.cpp
    unit/test_variable_store.cpp
    unit/test_spfs.cpp
    unit/test_dot_matrix.cpp
)
target_link_libraries(test_all ledcontroller_testable GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_utils)
gtest_discover_tests(test_variable_store)
gtest_discover_tests(test_spfs)
gtest_discover_tests(test_dot_matrix)
gtest_discover_tests(test_all)
