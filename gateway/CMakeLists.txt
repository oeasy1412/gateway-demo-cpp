add_executable(gateway gateway.cpp)

target_link_libraries(gateway PRIVATE
    d3d11
    dwmapi
    gdi32
    d3dcompiler
    ws2_32
    winmm
)
target_compile_options(gateway PRIVATE
    -O2
    -DNDEBUG
)

set(OUTPUT ${CMAKE_SOURCE_DIR}/output)
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT})