# Get all project files
file(GLOB_RECURSE CLANG_FORMAT_FILES
    src/*.cpp
    src/*.h
    )

# if user provides CMAKE_CXX_CLANG_FORMAT
set(CLANG_FORMAT_NAME ${CMAKE_CXX_CLANG_FORMAT})
set(CMAKE_CXX_CLANG_FORMAT "" CACHE STRING "" FORCE)
# Adding clang-format target if executable is found
find_program(CLANG_FORMAT NAMES ${CLANG_FORMAT_NAME} "clang-format-11" "clang-format")
if(CLANG_FORMAT)
    add_custom_target(
        clang-format
        COMMAND ${CLANG_FORMAT} -style=file
        -i
        ${CLANG_FORMAT_FILES}
        )

    add_custom_target(
        check-format
        )

    foreach(SOURCE_FILE ${CLANG_FORMAT_FILES})
        add_custom_command(TARGET check-format PRE_BUILD
            COMMAND
            ${CLANG_FORMAT} -style=file ${SOURCE_FILE} | diff -U5 ${SOURCE_FILE} -
            )
    endforeach()
    message(STATUS "clang-format found: ${CLANG_FORMAT}")
else()
    message(STATUS "clang-format not found disable target clang-format")
endif()

# Additional targets to perform clang-format/clang-tidy
# Get all project files
file(GLOB_RECURSE ALL_TIDY_FILES
    src/*.cpp
    )

# Adding clang-tidy target if executable is found
set(INCLUDE_DIRECTORIES_TIDY
    -I/usr/local/include
    -I/usr/include
    -I${CMAKE_SOURCE_DIR}/deps/
    )

# Disables native clang tidy integration
# even if user provides CMAKE_CXX_CLANG_TIDY
set(CLANG_TIDY_NAME ${CMAKE_CXX_CLANG_TIDY})
set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE)

find_program(CLANG_TIDY NAMES ${CLANG_TIDY_NAME} "clang-tidy-11" "clang-tidy")
if(CLANG_TIDY)
    add_custom_target(
        clang-tidy
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/clang-tidy.sh
        ${CLANG_TIDY}
        8
        ${ALL_TIDY_FILES}
        )
    message(STATUS "clang-tidy found: ${CLANG_TIDY}")
else()
    message(STATUS "clang-tidy not found disable target clang-tidy")
endif()

find_program(IWYU NAMES "iwyu_tool" "iwyu_tool.py")
if(IWYU)
    add_custom_target(
        check-includes
        COMMAND ${IWYU}
        -p
        .
        )
    message(STATUS "include-what-you-use found: ${IWYU} use: make check-includes")
else()
    message(STATUS "include-what-you-use not found, consider to install it to use check-includes\n apt-get -y install iwyu\n brew install iwyu")
endif()
