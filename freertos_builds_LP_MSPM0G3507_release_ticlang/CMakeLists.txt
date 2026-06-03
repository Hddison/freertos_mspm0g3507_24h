set(SDK_DIR "C:/ti/mspm0_sdk_2_10_00_04")
set(TOOLCHAIN_ROOT "D:/ti/ccs2050/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS")

# ── FreeRTOS 内核源文件 ──
set(FREERTOS_KERNEL_SRCS
    ${SDK_DIR}/kernel/freertos/Source/croutine.c
    ${SDK_DIR}/kernel/freertos/Source/event_groups.c
    ${SDK_DIR}/kernel/freertos/Source/list.c
    ${SDK_DIR}/kernel/freertos/Source/queue.c
    ${SDK_DIR}/kernel/freertos/Source/stream_buffer.c
    ${SDK_DIR}/kernel/freertos/Source/tasks.c
    ${SDK_DIR}/kernel/freertos/Source/timers.c
    ${SDK_DIR}/kernel/freertos/Source/portable/MemMang/heap_4.c
)

# ── Portable 层（TiClang / Cortex-M0+） ──
set(FREERTOS_PORT_SRCS
    ${SDK_DIR}/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0/port.c
    ${SDK_DIR}/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0/portasm.c
)

# ── DPL 驱动抽象层 ──
set(FREERTOS_DPL_SRCS
    ${SDK_DIR}/kernel/freertos/dpl/AppHooks_freertos.c
    ${SDK_DIR}/kernel/freertos/dpl/ClockP_freertos.c
    ${SDK_DIR}/kernel/freertos/dpl/DebugP_freertos.c
    ${SDK_DIR}/kernel/freertos/dpl/HwiPMSPM0_freertos.c
    ${SDK_DIR}/kernel/freertos/dpl/MutexP_freertos.c
    ${SDK_DIR}/kernel/freertos/dpl/SemaphoreP_freertos.c
    ${SDK_DIR}/kernel/freertos/dpl/StaticAllocs_freertos.c
    ${SDK_DIR}/kernel/freertos/dpl/SystemP_freertos.c
    ${SDK_DIR}/kernel/freertos/dpl/TaskP_freertos.c
)

# ── 启动文件 (已移至 posix_demo/freertos/ticlang/, 由可执行文件编译) ──

# ── 构建静态库 ──
add_library(freertos_builds STATIC
    ${FREERTOS_KERNEL_SRCS}
    ${FREERTOS_PORT_SRCS}
    ${FREERTOS_DPL_SRCS}
)

target_include_directories(freertos_builds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDK_DIR}/kernel/freertos/Source/include
    ${SDK_DIR}/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0
    ${SDK_DIR}/source/third_party/CMSIS/Core/Include
    ${SDK_DIR}/source
    ${TOOLCHAIN_ROOT}/include
)

target_compile_definitions(freertos_builds PRIVATE __MSPM0G3507__ __ti_version__)
target_compile_options(freertos_builds PRIVATE -gdwarf-3 -O0 -Wall -Wno-macro-redefined)
