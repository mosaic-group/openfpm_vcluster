cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

########################### Executables

if (CUDA_FOUND OR CUDA_ON_CPU)
	set(CUDA_SOURCES VCluster/cuda/VCluster_semantic_unit_cuda_tests.cu VCluster/cuda/VCluster_unit_tests.cu )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	add_definitions("-DBOOST_MPL_CFG_HAS_TYPEOF")
endif()

if (CUDA_ON_CPU)
	add_definitions(-DCUDA_ON_CPU -D__NVCC__ -DCUDART_VERSION=11000)
        set_source_files_properties(${CUDA_SOURCES} PROPERTIES LANGUAGE CXX)
	# set_source_files_properties(${CUDA_SOURCES} PROPERTIES COMPILE_FLAGS "-D__NVCC__ -DCUDA_GPU -DCUDART_VERSION=11000")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
                add_definitions("-x c++")
        endif()
endif()

add_executable(vcluster_test main.cpp VCluster/VCluster.cpp VCluster/VCluster_unit_tests.cpp VCluster/VCluster_semantic_unit_tests.cpp ${CUDA_SOURCES})

if ( CMAKE_COMPILER_IS_GNUCC )
    target_compile_options(vcluster_test PRIVATE "-Wno-deprecated-declarations")
    if (TEST_COVERAGE)
        target_compile_options(vcluster_test PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -fprofile-arcs -ftest-coverage>)
    endif()
endif()

if (CMAKE_CUDA_COMPILER_ID STREQUAL "Clang")
        add_definitions(-D__STRICT_ANSI__)
endif()

###########################

add_library(vcluster STATIC VCluster/VCluster.cpp)
add_library(vcluster_dl SHARED VCluster/VCluster.cpp)

if(CUDA_FOUND)
	if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
		target_compile_options(vcluster_test PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: ${WARNING_SUPPRESSION_AND_OPTION_NVCC} >)
        	target_compile_options(vcluster PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: ${WARNING_SUPPRESSION_AND_OPTION_NVCC} >)
        	target_compile_options(vcluster_dl PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: ${WARNING_SUPPRESSION_AND_OPTION_NVCC} >)
	else()
        	target_compile_options(vcluster_test PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: -D__NVCC__ -D__CUDACC_VER_MAJOR__=${CUDA_VERSION_MAJOR} -D__CUDACC_VER_MINOR__=${CUDA_VERSION_MINOR} -D__CUDACC_VER_BUILD__=0 >)
        	target_compile_options(vcluster PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: -D__NVCC__ -D__CUDACC_VER_MAJOR__=${CUDA_VERSION_MAJOR} -D__CUDACC_VER_MINOR__=${CUDA_VERSION_MINOR} -D__CUDACC_VER_BUILD__=0 >)
		target_compile_options(vcluster_dl PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: -D__NVCC__ -D__CUDACC_VER_MAJOR__=${CUDA_VERSION_MAJOR} -D__CUDACC_VER_MINOR__=${CUDA_VERSION_MINOR} -D__CUDACC_VER_BUILD__=0 >)
	endif()
	target_include_directories (vcluster_test PUBLIC ${MPI_C_INCLUDE_DIRS})
        if (TEST_COVERAGE)
                target_compile_options(vcluster_test PRIVATE $<$<COMPILE_LANGUAGE:CUDA>: -Xcompiler "-fprofile-arcs -ftest-coverage" >)
        endif()

        if (NOT CUDA_ON_CPU)
                set_source_files_properties(VCluster/VCluster.cpp PROPERTIES LANGUAGE CUDA)
        endif()
endif()


add_dependencies(vcluster_test ofpmmemory)

target_include_directories (vcluster_test PUBLIC ${CUDA_INCLUDE_DIRS})
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_devices/src/)
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_vcluster/src/)
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_data/src/)
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories (vcluster_test PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories (vcluster_test PUBLIC ${PETSC_INCLUDES})
target_include_directories (vcluster_test PUBLIC ${ALPAKA_ROOT}/include)

target_include_directories (vcluster PUBLIC ${CUDA_INCLUDE_DIRS})
target_include_directories (vcluster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (vcluster PUBLIC ${PETSC_INCLUDES})
target_include_directories (vcluster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories (vcluster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_data/src/)
target_include_directories (vcluster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_devices/src/)
target_include_directories (vcluster PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories (vcluster PUBLIC ${ALPAKA_ROOT}/include)

target_include_directories (vcluster_dl PUBLIC ${CUDA_INCLUDE_DIRS})
target_include_directories (vcluster_dl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (vcluster_dl PUBLIC ${PETSC_INCLUDES})
target_include_directories (vcluster_dl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories (vcluster_dl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_data/src/)
target_include_directories (vcluster_dl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_devices/src/)
target_include_directories (vcluster_dl PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories (vcluster_dl PUBLIC ${ALPAKA_ROOT}/include)

target_link_libraries(vcluster_test ${Boost_LIBRARIES})
target_link_libraries(vcluster_test ${PETSC_LIBRARIES})
target_link_libraries(vcluster_test ofpmmemory)

target_link_libraries(vcluster_dl ${PETSC_LIBRARIES})
target_link_libraries(vcluster ${PETSC_LIBRARIES})

if (TEST_COVERAGE)
    target_link_libraries(vcluster_test -lgcov)
endif()

# Request that particles be built with -std=c++11
# As this is a public compile feature anything that links to particles
# will also build with -std=c++11
target_compile_features(vcluster_test PUBLIC cxx_std_11)
target_link_libraries(vcluster_test ${MPI_C_LIBRARIES})

install(TARGETS vcluster vcluster_dl DESTINATION openfpm_vcluster/lib)
install(FILES MPI_wrapper/MPI_IallreduceW.hpp 
	      MPI_wrapper/MPI_IrecvW.hpp 
	      MPI_wrapper/MPI_IBcastW.hpp  
	      MPI_wrapper/MPI_IsendW.hpp 
	      MPI_wrapper/MPI_util.hpp 
	      MPI_wrapper/MPI_IAllGather.hpp
	      DESTINATION openfpm_vcluster/include/MPI_wrapper)

install(FILES VCluster/VCluster_base.hpp
	      VCluster/VCluster.hpp 
	      VCluster/VCluster_meta_function.hpp
	      DESTINATION openfpm_vcluster/include/VCluster )

install (FILES util/Vcluster_log.hpp
	 DESTINATION openfpm_vcluster/include/util)

#if(BUILD_TESTING)

#  add_executable(particle_test test.cu)

#  set_target_properties(particle_test PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
#  target_link_libraries(particle_test PRIVATE particles)

#  add_test(NAME particles_10k COMMAND particle_test 10000 )
#  add_test(NAME particles_256k COMMAND particle_test 256000 )

#  if(APPLE)
    # We need to add the default path to the driver (libcuda.dylib) as an rpath,
    # so that the static cuda runtime can find it at runtime.
    #    set_property(TARGET particle_test PROPERTY BUILD_RPATH ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
    #  endif()
    #endif()

