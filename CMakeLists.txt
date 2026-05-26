cmake_minimum_required(VERSION 3.20)
project(Wrens_Nest)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")

# Python agent script embedding
file(GLOB SCRIPTS "scripts/*.py")

foreach(SCRIPT ${SCRIPTS})
    get_filename_component(SCRIPT_NAME ${SCRIPT} NAME_WE)
    add_custom_command(
        OUTPUT "src/embedded/${SCRIPT_NAME}_py.h"
        COMMAND xxd -i ${SCRIPT} > "${CMAKE_SOURCE_DIR}/src/embedded/${SCRIPT_NAME}_py.h"
        DEPENDS ${SCRIPT}
        COMMENT "Embedding ${SCRIPT_NAME}.py"
    )
    list(APPEND EMBEDDED_HEADERS "src/embedded/${SCRIPT_NAME}_py.h")
endforeach()

add_executable(${PROJECT_NAME} ${SOURCES} ${EMBEDDED_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE include)