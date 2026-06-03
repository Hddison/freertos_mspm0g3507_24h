set(SDK_DIR "C:/ti/mspm0_sdk_2_10_00_04")
set(TOOLCHAIN_ROOT "D:/ti/ccs2050/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS")

# ── 应用源文件 ──
add_executable(posix_demo
    main.c
    syscfg/ti_msp_dl_config.c
    freertos/ticlang/startup_mspm0g350x_ticlang.c
)

# ── 包含路径 ──
target_include_directories(posix_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/syscfg
    ${CMAKE_SOURCE_DIR}
    ${SDK_DIR}/source/third_party/CMSIS/Core/Include
    ${SDK_DIR}/kernel/freertos/Source/include
    ${SDK_DIR}/source
    ${SDK_DIR}/kernel/freertos/Source/portable/TI_ARM_CLANG/ARM_CM0
    ${CMAKE_SOURCE_DIR}/freertos_builds_LP_MSPM0G3507_release_ticlang
    ${TOOLCHAIN_ROOT}/include
)

# ── 编译选项 ──
target_compile_definitions(posix_demo PRIVATE __MSPM0G3507__ __ti_version__)
target_compile_options(posix_demo PRIVATE -gdwarf-3 -O2 -Wall -Wno-macro-redefined)

# ── 库搜索路径 ──
target_link_directories(posix_demo PRIVATE
    ${CMAKE_BINARY_DIR}/freertos_builds_LP_MSPM0G3507_release_ticlang
    ${SDK_DIR}/source
    ${TOOLCHAIN_ROOT}/lib
)

# ── 链接选项 ──
target_link_options(posix_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mspm0g3507.cmd
    -Wl,--warn_sections
    -Wl,--xml_link_info=posix_demo_linkInfo.xml
)

# ── 链接库 ──
target_link_libraries(posix_demo
    hw
    bsp
    lib_gui
    freertos_builds
    ${SDK_DIR}/source/ti/driverlib/lib/ticlang/m0p/mspm0g1x0x_g3x0x/driverlib.a
)
