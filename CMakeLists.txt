cmake_minimum_required(VERSION 3.16)
project(myna VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(myna_core
    src/core/catalogue.c
    src/core/search.c
)
target_include_directories(myna_core PUBLIC include)

set(MYNA_CLI_SOURCES
    src/main.c
    src/cli/cli.c
    src/cli/terminal.c
    src/cli/logo.c
)
if(WIN32)
    # Embeds the icon in myna.exe.
    enable_language(RC)
    list(APPEND MYNA_CLI_SOURCES assets/myna.rc)
endif()

add_executable(myna ${MYNA_CLI_SOURCES})
target_link_libraries(myna PRIVATE myna_core)

enable_testing()
add_executable(test_search tests/test_search.c)
target_link_libraries(test_search PRIVATE myna_core)
add_test(NAME test_search
    COMMAND test_search ${CMAKE_CURRENT_SOURCE_DIR}/data/concepts.csv)
