
LINKLIBS = $(DEFAULT_LIB) $(PTHREAD_LIBS) $(OPT_LIBS) $(BOOST_LDFLAGS)

bin_PROGRAMS = vcluster
vcluster_SOURCES = main.cpp VCluster.cpp ../../OpenFPM_devices/src/memory/HeapMemory.cpp ../../OpenFPM_devices/src/memory/PtrMemory.cpp
vcluster_CXXFLAGS = $(INCLUDES_PATH) $(BOOST_CPPFLAGS)
vcluster_CFLAGS = $(CUDA_CFLAGS)
vcluster_LDADD = $(LINKLIBS)

.cu.o :
	$(NVCC) $(NVCCFLAGS) -o $@ -c $<
