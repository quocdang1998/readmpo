cmake_minimum_required(VERSION 3.26)
project(readmpo LANGUAGES CXX)

find_package(HDF5 COMPONENTS CXX)
find_package(OpenMP QUIET REQUIRED)

list(APPEND READMPO_SRC_CPP
     glob.cpp
     h5_utils.cpp
     nd_array.cpp
     master_mpo.cpp
     query_mpo.cpp
     single_mpo.cpp
)
list(TRANSFORM READMPO_SRC_CPP PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/readmpo/)

add_library(libreadmpo STATIC ${READMPO_SRC_CPP})
set_property(TARGET libreadmpo PROPERTY OUTPUT_NAME readmpo)
set_property(TARGET libreadmpo PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libreadmpo PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libreadmpo PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
target_include_directories(libreadmpo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${HDF5_INCLUDE_DIRS})
target_compile_features(libreadmpo PUBLIC cxx_std_20)
target_link_libraries(libreadmpo PUBLIC ${HDF5_CXX_LIBRARIES} ${HDF5_C_LIBRARIES})
target_link_libraries(libreadmpo PRIVATE OpenMP::OpenMP_CXX)

add_executable(readmpo ${CMAKE_CURRENT_SOURCE_DIR}/src/readmpo/main.cpp)
target_link_libraries(readmpo PUBLIC libreadmpo)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION "include" FILES_MATCHING PATTERN "*.[ht]pp")
install(TARGETS libreadmpo RUNTIME DESTINATION "lib")

set(READMPO_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/readmpo_config.py)
file(WRITE ${READMPO_CONFIG} "")
file(APPEND ${READMPO_CONFIG} "H5_INCLUDE = \"${HDF5_INCLUDE_DIRS}\"\n")
