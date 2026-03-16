message("[application build type  :${CMAKE_BUILD_TYPE}]")
message("[application build flags :${CMAKE_C_FLAGS_RELEASE}]")

# gtest
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/googletest
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/googletest
)
FetchContent_MakeAvailable(googletest)

list(APPEND TEST_LIBS GTest::gtest_main)

include(GoogleTest)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} exe_file_list)
foreach (src_file ${exe_file_list}) # 对每个文件生成对应的测试程序
    # 从文件路径中提取文件名, 文件路径: ${src_file} 提取出的文件名: ${exe_name}
    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" exe_name ${src_file})
    message([src_file : ${src_file} , exe_name : ${exe_name}])

    add_executable(${exe_name} ${src_file})
    target_link_libraries(${exe_name}
            PUBLIC template_common
            PUBLIC ${TEST_LIBS}
            )

    # 为每个测试目标启用测试发现
    gtest_discover_tests(${exe_name})

endforeach (src_file ${exe_file_list})

