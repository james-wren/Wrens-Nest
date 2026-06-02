cmake_minimum_required(VERSION 3.20)
project(wn)

include(FetchContent)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.14.0
)

set(CPR_CURL_USE_LIBPSL OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(cpr)

find_package(OpenSSL REQUIRED)

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
        COMMAND xxd -i ${SCRIPT_NAME}.py > "${CMAKE_SOURCE_DIR}/src/embedded/${SCRIPT_NAME}_py.h"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/scripts"
        DEPENDS ${SCRIPT}
        COMMENT "Embedding ${SCRIPT_NAME}.py"
    )
    list(APPEND EMBEDDED_HEADERS "src/embedded/${SCRIPT_NAME}_py.h")
endforeach()

add_executable(${PROJECT_NAME} ${SOURCES} ${EMBEDDED_HEADERS})

target_link_libraries(wn PRIVATE OpenSSL::Crypto cpr::cpr)
target_include_directories(${PROJECT_NAME} PRIVATE include)