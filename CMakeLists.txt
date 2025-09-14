cmake_minimum_required(VERSION 3.16)
project(SwitchPrimaryMonitor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Build as a GUI subsystem to avoid spawning a console window when launched
# from Explorer. When launched from a terminal, the program will attach to the
# parent console to print messages.
add_executable(SwitchPrimaryMonitor WIN32
    src/main.cpp
)

if(MSVC)
    target_compile_definitions(SwitchPrimaryMonitor PRIVATE UNICODE _UNICODE)
    target_compile_options(SwitchPrimaryMonitor PRIVATE /W4 /permissive-)
else()
    target_compile_options(SwitchPrimaryMonitor PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(SwitchPrimaryMonitor PRIVATE user32 gdi32)

# Installation layout (for packaging/CI)
install(TARGETS SwitchPrimaryMonitor
        RUNTIME DESTINATION bin)
install(FILES README.md
        DESTINATION .)
