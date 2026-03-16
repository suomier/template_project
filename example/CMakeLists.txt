message("[application build type  :${CMAKE_BUILD_TYPE}]")
message("[application build flags :${CMAKE_C_FLAGS_RELEASE}]")

message("COMMON_LIBS :${COMMON_LIBS}")


aux_source_directory(${PROJECT_SOURCE_DIR}/example exe_file_list)
foreach (src_file ${exe_file_list}) # 对每个文件生成对应的测试程序
    # 从文件路径中提取文件名, 文件路径: ${src_file} 提取出的文件名: ${exe_name}
    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" exe_name ${src_file})
    message([src_file : ${src_file} , exe_name : ${exe_name}])

    add_executable(${exe_name} ${src_file})
    target_link_libraries(${exe_name} PUBLIC template_common)

endforeach (src_file ${exe_file_list})

