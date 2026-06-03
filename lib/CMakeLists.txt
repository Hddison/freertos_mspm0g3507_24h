set(LIB_SOURCES gui_paint.c fonts/font8.c fonts/font12.c fonts/font16.c fonts/font20.c fonts/font24.c)
add_library(lib_gui STATIC ${LIB_SOURCES})

target_include_directories(lib_gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/posix_demo/syscfg
    ${CMAKE_SOURCE_DIR}/posix_demo
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/hardware
    ${SDK_DIR}/source
    ${SDK_DIR}/source/third_party/CMSIS/Core/Include
)
