cmake_minimum_required(VERSION 3.16)
project(NetworkingProjectClientCS3 C)

set(CMAKE_C_STANDARD 99)

include(FetchContent)

# --- Fetch raylib ---
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5  # or latest stable tag
)
FetchContent_MakeAvailable(raylib)

# --- Fetch raygui (header-only, just needs the .h) ---
FetchContent_Declare(
        raygui
        GIT_REPOSITORY https://github.com/raysan5/raygui.git
        GIT_TAG        4.0
)
FetchContent_MakeAvailable(raygui)

# --- Your executable ---
add_executable(NetworkingProjectClientCS3 main.c)

target_link_libraries(NetworkingProjectClientCS3 PRIVATE raylib)

# Give your code access to raygui's header
target_include_directories(NetworkingProjectClientCS3 PRIVATE ${raygui_SOURCE_DIR}/src)
