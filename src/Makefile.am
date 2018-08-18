
LINKLIBS = $(DEFAULT_LIB) $(PTHREAD_LIBS) $(OPT_LIBS) $(HDF5_LDFLAGS) $(HDF5_LIBS) $(BOOST_LDFLAGS) $(CUDA_LIBS)

if BUILDCUDA
 CUDA_SOURCES=../../openfpm_devices/src/memory/CudaMemory.cu VCluster/cuda/VCluster_semantic_unit_cuda_tests.cu
else
 CUDA_SOURCES=
endif

FLAGS_NVCC = -Xcudafe "--display_error_number --diag_suppress=2888 --diag_suppress=111 --diag_suppress=186 " $(MPI_INC_PATH)

noinst_PROGRAMS = vcluster_test
vcluster_test_SOURCES = main.cpp VCluster/VCluster.cpp  ../../openfpm_devices/src/memory/HeapMemory.cpp ../../openfpm_devices/src/memory/PtrMemory.cpp ../../openfpm_devices/src/Memleak_check.cpp $(CUDA_SOURCES)
vcluster_test_CXXFLAGS = -Wunknown-pragmas $(AM_CXXFLAGS) $(INCLUDES_PATH) $(MPI_INC_PATH) $(BOOST_CPPFLAGS) $(CUDA_CFLAGS)
vcluster_test_CFLAGS = $(CUDA_CFLAGS)
vcluster_test_LDADD = $(LINKLIBS)

lib_LIBRARIES = libvcluster.a
libvcluster_a_SOURCES = VCluster/VCluster.cpp
libvcluster_a_CXXFLAGS = -Wunknown-pragmas $(AM_CXXFLAGS) $(INCLUDES_PATH) $(BOOST_CPPFLAGS) $(CUDA_CFLAGS)
libvcluster_a_CFLAGS =

nobase_include_HEADERS = MPI_wrapper/MPI_IallreduceW.hpp MPI_wrapper/MPI_IrecvW.hpp MPI_wrapper/MPI_IBcastW.hpp  MPI_wrapper/MPI_IsendW.hpp MPI_wrapper/MPI_util.hpp MPI_wrapper/MPI_IAllGather.hpp  \
VCluster/VCluster_base.hpp VCluster/VCluster.hpp VCluster/VCluster_meta_function.hpp\
util/Vcluster_log.hpp

.cu.o :
	$(NVCC) $(NVCCFLAGS) $(FLAGS_NVCC) $(INCLUDES_PATH) -o $@ -c $<

test: vcluster_test
	source $(HOME)/openfpm_vars &&  cd .. && mpirun -np 3 ./src/vcluster_test && mpirun -np 4 ./src/vcluster_test

