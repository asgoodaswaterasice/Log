include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(test0 test0.cc)
target_link_libraries(test0 log_base pthread)

add_executable(test1 test1.cc)
target_link_libraries(test1 log_base pthread)
