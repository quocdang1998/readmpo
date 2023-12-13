find_package(HDF5 COMPONENTS CXX)
list(APPEND READMPO_SRC_CPP
     h5_utils.cpp
)
list(TRANSFORM READMPO_SRC_CPP PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/readmpo/)

add_library(libreadmpo STATIC ${READMPO_SRC_CPP})
set_property(TARGET libreadmpo PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
target_include_directories(libreadmpo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${HDF5_INCLUDE_DIRS})
target_link_libraries(libreadmpo PUBLIC libmerlin ${HDF5_CXX_LIBRARIES} ${HDF5_C_LIBRARIES})

# add_executable(ap3_mpo ${READMPO_SRC_CPP})
# set_property(TARGET ap3_mpo PROPERTY OUTPUT_NAME ap3_mpo)
# set_property(TARGET ap3_mpo PROPERTY INTERPROCEDURAL_OPTIMIZATION OFF)
# target_include_directories(ap3_mpo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${HDF5_INCLUDE_DIRS})
# target_link_libraries(ap3_mpo PUBLIC libmerlin ${HDF5_CXX_LIBRARIES} ${HDF5_C_LIBRARIES})
# set_property(TARGET ap3_mpo PROPERTY CUDA_SEPARABLE_COMPILATION OFF)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION "include" FILES_MATCHING PATTERN "*.[ht]pp")
install(TARGETS libreadmpo RUNTIME DESTINATION "lib")
