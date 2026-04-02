
cmake_minimum_required(VERSION 3.16)
project(NetworkingProjectClientCS3 C)
set(CMAKE_C_STANDARD 99)

include(FetchContent)

# --- Fetch Nuklear (single header + backends) ---
FetchContent_Declare(
        nuklear
        GIT_REPOSITORY https://github.com/Immediate-Mode-UI/Nuklear.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(nuklear)

# --- Find SDL2 (native Wayland support) ---
find_package(SDL2 REQUIRED)

# --- Your executable ---
add_executable(NetworkingProjectClientCS3 main.c)

# Nuklear header + SDL2 renderer backend
target_include_directories(NetworkingProjectClientCS3 PRIVATE
        ${nuklear_SOURCE_DIR}
        ${nuklear_SOURCE_DIR}/demo/sdl_renderer
)

target_link_libraries(NetworkingProjectClientCS3 PRIVATE
        SDL2::SDL2
        pthread
        m
)