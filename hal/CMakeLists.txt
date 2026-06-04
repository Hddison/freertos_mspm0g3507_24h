set(HAL_SOURCES hal_spi.c)
add_library(hal STATIC ${HAL_SOURCES})

target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/posix_demo/syscfg
    ${CMAKE_SOURCE_DIR}/posix_demo
    ${SDK_DIR}/source
    ${SDK_DIR}/source/third_party/CMSIS/Core/Include
    ${SDK_DIR}/kernel/freertos/Source/include
    ${SDK_DIR}/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0
    ${CMAKE_SOURCE_DIR}/freertos_builds_LP_MSPM0G3507_release_ticlang
)

target_compile_definitions(hal PRIVATE __MSPM0G3507__)
