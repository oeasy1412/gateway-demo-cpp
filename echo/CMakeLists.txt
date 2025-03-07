add_executable(echo echo.cpp)

target_link_libraries(echo PRIVATE
    d3d11
    dwmapi
    gdi32
    d3dcompiler
    ws2_32
    winmm
)
target_compile_options(echo PRIVATE
    -O2
    -DNDEBUG
)

set(OUTPUT ${CMAKE_SOURCE_DIR}/output)
set(EXECUTABLE_OUTPUT_PATH ${OUTPUT})