cmake_minimum_required(VERSION 3.21)
project(freertos_mspm0 LANGUAGES C)

set(SDK_DIR "C:/ti/mspm0_sdk_2_10_00_04")

add_subdirectory(bsp)
add_subdirectory(hal)
add_subdirectory(hardware)
add_subdirectory(lib)
add_subdirectory(freertos_builds_LP_MSPM0G3507_release_ticlang)
add_subdirectory(posix_demo)
