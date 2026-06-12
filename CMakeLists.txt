cmake_minimum_required(VERSION 3.20)
project(wn)

include(FetchContent)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.14.0
)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG v6.1.9
)

set(CPR_CURL_USE_LIBPSL OFF CACHE INTERNAL "")

find_package(OpenSSL REQUIRED)

FetchContent_MakeAvailable(cpr ftxui)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")

# GO agent script embedding
file(GLOB SCRIPTS "agent/agent" "agent/agent_64")

foreach(SCRIPT ${SCRIPTS})
    get_filename_component(SCRIPT_NAME ${SCRIPT} NAME_WE)
    add_custom_command(
        OUTPUT "src/embedded/${SCRIPT_NAME}_go.h"
        COMMAND xxd -i ${SCRIPT_NAME} > "${CMAKE_SOURCE_DIR}/src/embedded/${SCRIPT_NAME}_go.h"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/agent"
        DEPENDS ${SCRIPT}
        COMMENT "Embedding ${SCRIPT_NAME}"
    )
    list(APPEND EMBEDDED_HEADERS "src/embedded/${SCRIPT_NAME}_go.h")
endforeach()

add_executable(${PROJECT_NAME} ${SOURCES} ${EMBEDDED_HEADERS})

target_link_libraries(wn PRIVATE OpenSSL::Crypto cpr::cpr ftxui::screen ftxui::dom ftxui::component)
target_include_directories(${PROJECT_NAME} PRIVATE include)