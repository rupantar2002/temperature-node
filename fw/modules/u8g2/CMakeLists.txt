zephyr_library()

file(GLOB U8G2_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/csrc/*.c
)

zephyr_library_sources(${U8G2_SOURCES})

zephyr_include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/csrc
)

# Settings
zephyr_compile_definitions(U8X8_WITH_USER_PTR)