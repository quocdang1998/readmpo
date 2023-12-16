cmake_minimum_required(VERSION 3.26)
project(readmpo LANGUAGES CXX)

find_package(HDF5 COMPONENTS CXX)
set(HDF5_USE_STATIC_LIBRARIES FORCE ON)
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
set_property(TARGET libreadmpo PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
target_include_directories(libreadmpo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${HDF5_INCLUDE_DIRS})
target_compile_features(libreadmpo PUBLIC cxx_std_20)
target_link_libraries(libreadmpo PUBLIC ${HDF5_CXX_LIBRARIES} ${HDF5_C_LIBRARIES})

add_executable(readmpo ${CMAKE_CURRENT_SOURCE_DIR}/src/readmpo/main.cpp)
target_link_libraries(readmpo PUBLIC libreadmpo)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION "include" FILES_MATCHING PATTERN "*.[ht]pp")
install(TARGETS libreadmpo RUNTIME DESTINATION "lib")

set(READMPO_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/readmpo_config.py)
file(WRITE ${READMPO_CONFIG} "")
file(APPEND ${READMPO_CONFIG} "H5_INCLUDE = \"${HDF5_INCLUDE_DIRS}\"\n")
file(APPEND ${READMPO_CONFIG} "H5_CXX_LIB = \"${HDF5_CXX_LIBRARIES}\"\n")
file(APPEND ${READMPO_CONFIG} "H5_C_LIB = \"${HDF5_C_LIBRARIES}\"\n")
