cmake_minimum_required(VERSION 3.16)
project(myna VERSION 0.2.1 LANGUAGES C)

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
    src/core/prompt.c
)
target_include_directories(myna_core PUBLIC include)

set(MYNA_CLI_SOURCES
    src/main.c
    src/cli/cli.c
    src/cli/terminal.c
    src/cli/logo.c
    src/cli/json.c
    src/cli/mcp.c
    src/cli/report.c
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
    COMMAND test_search ${CMAKE_CURRENT_SOURCE_DIR}/data/concepts)

add_executable(test_prompt tests/test_prompt.c)
target_link_libraries(test_prompt PRIVATE myna_core)
add_test(NAME test_prompt COMMAND test_prompt)

add_executable(test_mcp tests/test_mcp.c src/cli/json.c src/cli/mcp.c src/cli/report.c)
target_include_directories(test_mcp PRIVATE src/cli)
target_link_libraries(test_mcp PRIVATE myna_core)
add_test(NAME test_mcp
    COMMAND test_mcp ${CMAKE_CURRENT_SOURCE_DIR}/data/concepts)
