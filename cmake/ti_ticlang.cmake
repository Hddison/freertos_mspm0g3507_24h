set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_ROOT "D:/ti/ccs2050/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS")

set(CMAKE_C_COMPILER    "${TOOLCHAIN_ROOT}/bin/tiarmclang.exe")
set(CMAKE_CXX_COMPILER  "${TOOLCHAIN_ROOT}/bin/tiarmclang.exe")
set(CMAKE_AR             "${TOOLCHAIN_ROOT}/bin/tiarmar.exe")
set(CMAKE_ASM_COMPILER   "${TOOLCHAIN_ROOT}/bin/tiarmclang.exe")

set(CMAKE_C_FLAGS_INIT          "-march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb")
set(CMAKE_CXX_FLAGS_INIT        "-march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--diag_wrap=off -Wl,--display_error_number -Wl,--rom_model")
set(CMAKE_STATIC_LIBRARY_FLAGS_INIT "")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
