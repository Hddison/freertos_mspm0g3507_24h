#
# BSP (Board Support Package) — TI DriverLib 薄封装
#

set(BSP_SOURCES
    bsp_system.c
    bsp_uart.c
    bsp_i2c.c
    bsp_spi.c
    bsp_button.c
)

add_library(bsp STATIC ${BSP_SOURCES})

target_include_directories(bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/posix_demo/syscfg
    ${CMAKE_SOURCE_DIR}/posix_demo
    ${SDK_DIR}/source
    ${SDK_DIR}/source/third_party/CMSIS/Core/Include
    ${SDK_DIR}/kernel/freertos/Source/include
    ${SDK_DIR}/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0
    ${CMAKE_SOURCE_DIR}/freertos_builds_LP_MSPM0G3507_release_ticlang
)

target_compile_definitions(bsp PRIVATE __MSPM0G3507__)
