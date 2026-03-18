project(template_common)


# 跨平台头文件配置: 编码/头文件/
if (WIN32)
    # 使用utf-8编码
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

    # 防止 Windows.h 包含 Winsock.h
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

    #
    list(APPEND COMMON_DEFINED WINDOWS)
    list(APPEND COMMON_LIBS ws2_32 netapi32)
endif ()

include(FetchContent)


# yaml-cpp
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.7.0
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/yaml-cpp  # 指定源码下载目录
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/yaml-cpp  # 指定构建目录
)
FetchContent_MakeAvailable(yaml-cpp)
list(APPEND COMMON_LIBS yaml-cpp)

# spdlog
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.16.0
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/spdlog  # 指定源码下载目录
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/spdlog  # 指定构建目录
)
FetchContent_MakeAvailable(spdlog)
list(APPEND COMMON_LIBS spdlog::spdlog_header_only)


message("COMMON_LIBS:${COMMON_LIBS}")
message("CMAKE_SOURCE_DIR:${CMAKE_SOURCE_DIR}")
message("CMAKE_CURRENT_SOURCE_DIR:${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/base/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/settings/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/logger/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
        )

file(GLOB HEADER_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/base/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/settings/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/logger/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h"
        )

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        )

target_link_libraries(${PROJECT_NAME} PUBLIC ${COMMON_LIBS})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${COMMON_DEFINED})
target_compile_options(${PROJECT_NAME} PUBLIC
        $<$<C_COMPILER_ID:GNU>:-Wno-deprecated-declarations>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations>)

# 构建后将 ${CMAKE_SOURCE_DIR}/conf 目录拷贝到 ${CMAKE_BINARY_DIR}/bin
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config
        ${CMAKE_BINARY_DIR}/bin/config
        COMMENT "Copying conf directory to ${CMAKE_BINARY_DIR}/bin/conf")

