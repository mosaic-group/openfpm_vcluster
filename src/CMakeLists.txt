cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

########################### Executables

if (NOT CUDA_ON_BACKEND STREQUAL "None")
	set(CUDA_SOURCES VCluster/cuda/VCluster_semantic_unit_cuda_tests.cu VCluster/cuda/VCluster_unit_tests.cu )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	add_definitions("-DBOOST_MPL_CFG_HAS_TYPEOF")
endif()

if (CUDA_ON_BACKEND STREQUAL "OpenMP" OR CUDA_ON_BACKEND STREQUAL "SEQUENTIAL")
	add_definitions( -D__NVCC__ -DCUDART_VERSION=11000)
	set_source_files_properties(${CUDA_SOURCES} PROPERTIES LANGUAGE CXX)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
        	add_definitions("-x c++")
	endif()
endif()

if ( CUDA_ON_BACKEND STREQUAL "HIP" AND HIP_FOUND )

	list(APPEND HIP_HIPCC_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})

        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
                list(APPEND HIP_HIPCC_FLAGS -O0)
        endif()

	list(APPEND HIP_HIPCC_FLAGS -D__NVCC__ -D__HIP__  -DCUDART_VERSION=11000 -D__CUDACC__ -D__CUDACC_VER_MAJOR__=11 -D__CUDACC_VER_MINOR__=0 -D__CUDACC_VER_BUILD__=0)
        set_source_files_properties(${CUDA_SOURCES} PROPERTIES LANGUAGE CXX)
        

	set_source_files_properties(VCluster/VCluster.cpp PROPERTIES HIP_SOURCE_PROPERTY_FORMAT 1)

	hip_add_executable(vcluster_test main.cpp VCluster/VCluster.cpp VCluster/VCluster_unit_tests.cpp VCluster/VCluster_semantic_unit_tests.cpp ${CUDA_SOURCES})

        hip_add_library(vcluster STATIC VCluster/VCluster.cpp)
        hip_add_library(vcluster_dl SHARED VCluster/VCluster.cpp)

        set_property(TARGET vcluster PROPERTY POSITION_INDEPENDENT_CODE ON)
        set_property(TARGET vcluster_dl PROPERTY POSITION_INDEPENDENT_CODE ON)
        set_property(TARGET vcluster PROPERTY CMAKE_CXX_FLAGS "-Xcompiler -fPIC")
        set_property(TARGET vcluster_dl PROPERTY CMAKE_CXX_FLAGS "-Xcompiler -fPIC")

else()

	add_executable(vcluster_test main.cpp VCluster/VCluster.cpp VCluster/VCluster_unit_tests.cpp VCluster/VCluster_semantic_unit_tests.cpp ${CUDA_SOURCES})

	add_library(vcluster STATIC VCluster/VCluster.cpp)
	add_library(vcluster_dl SHARED VCluster/VCluster.cpp)

	set_property(TARGET vcluster_test PROPERTY CUDA_ARCHITECTURES OFF)
	set_property(TARGET vcluster PROPERTY CUDA_ARCHITECTURES OFF)
	set_property(TARGET vcluster_dl PROPERTY CUDA_ARCHITECTURES OFF)

endif()

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



if(CUDA_FOUND)
	target_include_directories(vcluster_test PUBLIC ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
	if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
		target_compile_options(vcluster_test PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: ${WARNING_SUPPRESSION_AND_OPTION_NVCC} >)
        target_compile_options(vcluster PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: ${WARNING_SUPPRESSION_AND_OPTION_NVCC} >)
        target_compile_options(vcluster_dl PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: ${WARNING_SUPPRESSION_AND_OPTION_NVCC} >)
	else()
        target_compile_options(vcluster_test PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: -D__NVCC__ -D__CUDACC_VER_MAJOR__=${CUDA_VERSION_MAJOR} -D__CUDACC_VER_MINOR__=${CUDA_VERSION_MINOR} -D__CUDACC_VER_BUILD__=0 >)
        target_compile_options(vcluster PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: -D__NVCC__ -D__CUDACC_VER_MAJOR__=${CUDA_VERSION_MAJOR} -D__CUDACC_VER_MINOR__=${CUDA_VERSION_MINOR} -D__CUDACC_VER_BUILD__=0 >)
		target_compile_options(vcluster_dl PUBLIC $<$<COMPILE_LANGUAGE:CUDA>: -D__NVCC__ -D__CUDACC_VER_MAJOR__=${CUDA_VERSION_MAJOR} -D__CUDACC_VER_MINOR__=${CUDA_VERSION_MINOR} -D__CUDACC_VER_BUILD__=0 >)
	endif()
    if (TEST_COVERAGE)
        target_compile_options(vcluster_test PRIVATE $<$<COMPILE_LANGUAGE:CUDA>: -Xcompiler "-fprofile-arcs -ftest-coverage" >)
    endif()

    if (CUDA_ON_BACKEND STREQUAL "CUDA")
        set_source_files_properties(VCluster/VCluster.cpp PROPERTIES LANGUAGE CUDA)
    endif()
endif()

add_dependencies(vcluster ofpmmemory)
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_devices/src/)
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_vcluster/src/)
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_data/src/)
target_include_directories (vcluster_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories (vcluster_test PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories (vcluster_test PUBLIC ${ALPAKA_ROOT}/include)
target_include_directories (vcluster_test PUBLIC ${MPI_C_INCLUDE_DIRS})

target_include_directories (vcluster PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (vcluster PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories (vcluster PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_data/src/)
target_include_directories (vcluster PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_devices/src/)
target_include_directories (vcluster PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories (vcluster PRIVATE ${ALPAKA_ROOT}/include)
target_include_directories (vcluster PRIVATE ${MPI_C_INCLUDE_DIRS})

target_include_directories (vcluster_dl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (vcluster_dl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories (vcluster_dl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_data/src/)
target_include_directories (vcluster_dl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../openfpm_devices/src/)
target_include_directories (vcluster_dl PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories (vcluster_dl PRIVATE ${ALPAKA_ROOT}/include)
target_include_directories (vcluster_dl PRIVATE ${MPI_C_INCLUDE_DIRS})

if (ENABLE_NUMERICS)
	target_include_directories (vcluster_test PUBLIC ${PETSC_INCLUDES})
	target_include_directories (vcluster_dl PRIVATE ${PETSC_INCLUDES})
	target_include_directories (vcluster PRIVATE ${PETSC_INCLUDES})
	target_link_libraries(vcluster_test ${PETSC_LIBRARIES})
	target_link_libraries(vcluster_dl ${PETSC_LIBRARIES})
	target_link_libraries(vcluster ${PETSC_LIBRARIES})
endif()

target_link_libraries(vcluster_test ${Boost_LIBRARIES})

if (HIP_FOUND)
	add_dependencies(vcluster_test ofpmmemory_dl)
	add_dependencies(vcluster_dl ofpmmemory_dl)
	target_link_libraries(vcluster_test ofpmmemory_dl)
	target_link_libraries(vcluster_dl ofpmmemory_dl)
else()
	add_dependencies(vcluster_test ofpmmemory)
	add_dependencies(vcluster_dl ofpmmemory)
	target_link_libraries(vcluster_test ofpmmemory)
	target_link_libraries(vcluster_dl ofpmmemory)
endif()

if (OPENMP_FOUND)
	target_link_libraries(vcluster_test OpenMP::OpenMP_CXX)
endif()

target_link_libraries(vcluster ofpmmemory)

if (HIP_FOUND)
	target_link_libraries(vcluster_test hip::host)
	target_link_libraries(vcluster hip::host)
        target_link_libraries(vcluster_dl hip::host)
endif()

if (TEST_COVERAGE)
    target_link_libraries(vcluster_test -lgcov)
endif()

# Request that particles be built with -std=c++11
# As this is a public compile feature anything that links to particles
# will also build with -std=c++11
target_compile_features(vcluster_test PUBLIC cxx_std_11)
target_link_libraries(vcluster_test ${MPI_C_LIBRARIES})
target_link_libraries(vcluster_test ${MPI_CXX_LIBRARIES})


install(TARGETS vcluster vcluster_dl  ${ADDITIONAL_OPENFPM_LIBS}  DESTINATION openfpm_vcluster/lib COMPONENT OpenFPM)
install(FILES MPI_wrapper/MPI_IallreduceW.hpp
	      MPI_wrapper/MPI_IrecvW.hpp
	      MPI_wrapper/MPI_IBcastW.hpp
	      MPI_wrapper/MPI_IsendW.hpp
	      MPI_wrapper/MPI_util.hpp
	      MPI_wrapper/MPI_IAllGather.hpp
	      DESTINATION openfpm_vcluster/include/MPI_wrapper
	      COMPONENT OpenFPM)

install(FILES VCluster/VCluster_base.hpp
	      VCluster/VCluster.hpp
	      VCluster/VCluster_meta_function.hpp
	      DESTINATION openfpm_vcluster/include/VCluster
	      COMPONENT OpenFPM)

install (FILES util/Vcluster_log.hpp
	 DESTINATION openfpm_vcluster/include/util
	 COMPONENT OpenFPM)

