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
