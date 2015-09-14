
LINKLIBS = $(DEFAULT_LIB) $(PTHREAD_LIBS) $(OPT_LIBS) $(BOOST_LDFLAGS)

noinst_PROGRAMS = vcluster
vcluster_SOURCES = main.cpp VCluster.cpp ../../openfpm_devices/src/memory/HeapMemory.cpp ../../openfpm_devices/src/memory/PtrMemory.cpp
vcluster_CXXFLAGS = $(INCLUDES_PATH) $(BOOST_CPPFLAGS)
vcluster_CFLAGS = $(CUDA_CFLAGS)
vcluster_LDADD = $(LINKLIBS)

lib_LIBRARIES = libvcluster.a
libvcluster_a_SOURCES = VCluster.cpp
libvcluster_a_CXXFLAGS = $(INCLUDES_PATH) $(BOOST_CPPFLAGS)
libvcluster_a_CFLAGS =

nobase_include_HEADERS = MPI_wrapper/MPI_IallreduceW.hpp MPI_wrapper/MPI_IrecvW.hpp MPI_wrapper/MPI_IsendW.hpp MPI_wrapper/MPI_util.hpp \
Pack_selector.hpp Pack_stat.hpp Packer.hpp Unpacker.hpp VCluster.hpp VCluster_object.hpp VCluster_object_array.hpp VObject.hpp


.cu.o :
	$(NVCC) $(NVCCFLAGS) -o $@ -c $<
