cmake_minimum_required(VERSION 3.25)
project(template_base)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
# 强制使用 C++
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指定可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# 指定库文件的输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 指定静态库文件的输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 指定临时文件的输出目录
set(CMAKE_TEMPORARY_OBJECTS_DIRECTORY ${CMAKE_BINARY_DIR}/temp)

add_subdirectory(common)
add_subdirectory(example)
add_subdirectory(test)
