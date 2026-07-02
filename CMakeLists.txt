cmake_minimum_required(VERSION 3.20)

project(RedRenderer LANGUAGES CXX)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../imgui-master/imgui-master")

if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui was not found at ${IMGUI_DIR}")
endif()

add_executable(RedRenderer WIN32
    MyRenderer!!!/geometry.cpp
    MyRenderer!!!/graphic.cpp
    MyRenderer!!!/imgui_main.cpp
    MyRenderer!!!/imgui_sw.cpp
    MyRenderer!!!/mesh.cpp
    MyRenderer!!!/tgaimage.cpp
    MyRenderer!!!/tool.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

target_compile_features(RedRenderer PRIVATE cxx_std_17)

target_include_directories(RedRenderer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/MyRenderer!!!"
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

target_link_libraries(RedRenderer PRIVATE
    user32
    gdi32
    imm32
)

target_compile_options(RedRenderer PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

set_target_properties(RedRenderer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
