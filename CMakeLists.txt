cmake_minimum_required(VERSION 3.7)

project(bin2coff2 CXX)

#main project
add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/bin2coff2.cpp)

set(TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)
set(TEST_SOURCE_FILE main.cpp)
set(TEST_BINARY_FILE $<TARGET_FILE_DIR:${PROJECT_NAME}>/${TEST_SOURCE_FILE}.o)

#test project
add_executable(${PROJECT_NAME}_test ${TEST_SOURCE_DIR}/${TEST_SOURCE_FILE})
target_link_libraries(${PROJECT_NAME}_test PUBLIC ${PROJECT_NAME}_test_dependencies PRIVATE ${CMAKE_DL_LIBS})

#helper library
add_library(${PROJECT_NAME}_test_dependencies INTERFACE)
target_link_libraries(${PROJECT_NAME}_test_dependencies INTERFACE ${TEST_BINARY_FILE})

#for mingw, gcc and clang use native ld tool
if(MSVC)
    set(CMD $<TARGET_FILE:${PROJECT_NAME}>)
else()
    set(CMD ld -r -b binary -o)
endif()

#translate source file of test project to object file
add_custom_target(
    quine
    COMMAND ${CMD} ${TEST_BINARY_FILE} ${TEST_SOURCE_FILE}
    WORKING_DIRECTORY ${TEST_SOURCE_DIR}
)

add_dependencies(${PROJECT_NAME}_test ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME}_test quine)
