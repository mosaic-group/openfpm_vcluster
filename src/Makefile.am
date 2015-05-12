
LINKLIBS = $(PTHREAD_LIBS) $(OPT_LIBS) $(BOOST_LDFLAGS)

bin_PROGRAMS = vcluster
vcluster_SOURCES = main.cpp VCluster.cpp
vcluster_CXXFLAGS = $(INCLUDES_PATH)
vcluster_CFLAGS = $(CUDA_CFLAGS)
vcluster_LDADD = $(LINKLIBS)

.cu.o :
	$(NVCC) $(NVCCFLAGS) -o $@ -c $<
