cmake_minimum_required(VERSION 3.26)
project(readmpo LANGUAGES CXX)

find_package(HDF5 COMPONENTS CXX)
list(APPEND READMPO_SRC_CPP
     h5_utils.cpp
     nd_array.cpp
     master_mpo.cpp
     single_mpo.cpp
)
list(TRANSFORM READMPO_SRC_CPP PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/readmpo/)

add_library(libreadmpo STATIC ${READMPO_SRC_CPP})
set_property(TARGET libreadmpo PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
target_include_directories(libreadmpo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${HDF5_INCLUDE_DIRS})
target_compile_features(libreadmpo PUBLIC cxx_std_20)
message(STATUS "H5 include : ${HDF5_INCLUDE_DIRS}")
target_link_libraries(libreadmpo PUBLIC ${HDF5_CXX_LIBRARIES} ${HDF5_C_LIBRARIES})

add_executable(readmpo ${CMAKE_CURRENT_SOURCE_DIR}/src/readmpo/main.cpp)
target_link_libraries(readmpo PUBLIC libreadmpo)


# list(APPEND AP3_MPO_SRC_CPP
#      ap3_xs.cpp
#      hdf5_utils.cpp
#      help_message.cpp
#      glob.cpp
#      main.cpp
#      properties.cpp
#      query_mpo.cpp
# )
# list(TRANSFORM AP3_MPO_SRC_CPP PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/ap3_mpo/)
# add_executable(ap3_mpo ${AP3_MPO_SRC_CPP})
# set_property(TARGET ap3_mpo PROPERTY OUTPUT_NAME ap3_mpo)
# set_property(TARGET ap3_mpo PROPERTY INTERPROCEDURAL_OPTIMIZATION OFF)
# target_include_directories(ap3_mpo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${HDF5_INCLUDE_DIRS} /home/quocdang/Projects/Merlin/src)
# target_compile_features(ap3_mpo PUBLIC cxx_std_20)
# target_link_libraries(ap3_mpo PUBLIC merlin merlincuda ${HDF5_CXX_LIBRARIES} ${HDF5_C_LIBRARIES})
# target_link_directories(ap3_mpo PUBLIC /home/quocdang/Projects/Merlin/build)
# set_property(TARGET ap3_mpo PROPERTY CUDA_SEPARABLE_COMPILATION OFF)



install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION "include" FILES_MATCHING PATTERN "*.[ht]pp")
install(TARGETS libreadmpo RUNTIME DESTINATION "lib")
