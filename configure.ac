#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.


AC_PREREQ(2.59)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/main.cpp])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([src/config/config.h])
m4_ifdef([ACX_PTHREAD],,[m4_include([m4/acx_pthread.m4])])
m4_ifdef([AX_BOOST],,[m4_include([m4/ax_boost.m4])])
m4_ifdef([ACX_MPI],,[m4_include([m4/acx_mpi.m4])])
m4_ifdef([AX_OPENMP],,[m4_include([m4/ax_openmp.m4])])
m4_ifdef([AX_CUDA],,[m4_include([m4/ax_cuda.m4])])

CXXFLAGS+=" --std=c++11 -march=native -mtune=native -Wno-unused-local-typedefs -Wextra -Wno-unused-parameter "
NVCCFLAGS=" "
INCLUDES_PATH=" "

# Add openfpm_vcluster to prefix forlder (if prefix end if openfpm_pdata remove that path)
if [ echo "$prefix" | grep -qE "/openfpm_pdata$" ]; then
  prefix=$(echo "$prefix" | sed 's/\(.*\)\/openfpm_pdata$/\1/g')
fi

prefix="$prefix/openfpm_vcluster"
echo "Installation dir is: $prefix"

AC_PROG_RANLIB
AM_PROG_AR

# Checks for programs.
AC_PROG_CXX

# Checks g++ flags

AC_CANONICAL_HOST

###### Check for verbose

AC_MSG_CHECKING(whether to build with verbose print)
verbose=no
AC_ARG_ENABLE(verbose,
	AC_HELP_STRING(
	    [--enable-verbose],
	    [enable verbose print (def=no)]
	),
  	verbose="$enableval"
)



AC_MSG_RESULT($verbose)
if test x"$verbose" = x"yes"; then
	AC_DEFINE([VERBOSE_TEST],[],[Verbose test])
fi

###### Check for debug compilation

AC_MSG_CHECKING(whether to build with debug information)
debuger=no
AC_ARG_ENABLE(debug,
	AC_HELP_STRING(
	    [--enable-debug],
	    [enable debug data generation (def=no)]
	),
  	debuger="$enableval"
)



AC_MSG_RESULT($debuger)
if test x"$debuger" = x"yes"; then
	AC_DEFINE([DEBUG_MODE],[],[Debug])
	AC_DEFINE([DEBUG],[],[Debug])
	CXXFLAGS="$CXXFLAGS -g3  -Wall -O0 "
	NVCCFLAGS+="$NVCCFLAGS -g -O0 "
else
	CXXFLAGS="$CXXFLAGS -Wall -O3 -g3 -funroll-loops "
	NVCCFLAGS+="$NVCCFLAGS -O3 "
fi

####### include openfpm_devices include path

INCLUDES_PATH+=" -I/usr/local/include -I. -Iconfig -I../../openfpm_devices/src -I../../openfpm_data/src"

###### Check for memleak check compilation

AC_MSG_CHECKING(whether to build with memory check)
mem_check=no
AC_ARG_ENABLE(memcheck,
	AC_HELP_STRING(
	    [--enable-memcheck],
	    [enable memory checking (def=no)]
	),
  	mem_check="$enableval"
)


AC_MSG_RESULT($mem_check)
if test x"$mem_check" = x"yes"; then
	AC_DEFINE([MEMLEAK_CHECK],[],[Memory check])
fi

####################################

DEFAULT_LIB=""
# At least for glibc, clock_gettime is in librt.  But don't pull that
# in if it still doesn't give us the function we want.
AC_CHECK_LIB(rt, clock_gettime,[DEFAULT_LIB+=" -lrt "],[],[])

##### CHECK FOR BOOST ##############

AX_BOOST([1.52],[],[])

####### Checking for GPU support

AX_CUDA

if test x"$NVCC_EXIST" = x"yes"; then
  AC_MSG_CHECKING(whether to build with GPU support)
  gpu_support=yes
  AC_ARG_ENABLE(gpu,
        AC_HELP_STRING(
            [--enable-gpu],
            [enable gpu support]
        ),
        gpu_support="$enableval"
  )

  AC_MSG_RESULT($gpu_support)
  if test x"$gpu_support" = x"yes"; then
        AC_DEFINE([CUDA_GPU],[],[CUDA GPU support])
  fi
else
  gpu_support=no
fi


# Set this conditional if cuda is wanted

#

AM_CONDITIONAL(BUILDCUDA, test x$gpu_support = x"yes")

##########################

AC_SUBST(NVCCFLAGS)
AC_SUBST(INCLUDES_PATH)
AC_SUBST(DEFAULT_LIB)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
echo ""
echo   "***********************************"
echo   "*                                 *"
if [ test x"$profiler" = x"yes" ]; then
  echo "*    profiler: yes                *"
else
  echo "*    profiler: no                 *"
fi
if [ test x"$memcheck" = x"yes" ]; then
  echo "*    memcheck: yes                *"
else
  echo "*    memcheck: no                 *"
fi
if [ test x"$debuger" = x"yes" ]; then
  echo "*    debug: yes                   *"
else
  echo "*    debug: no                    *"
fi
if [ test x"$gpu_support" = x"no" ]; then
  echo "*    gpu: no                      *"
else
  echo "*    gpu: yes                     *"
fi
echo   "*                                 *"
echo   "***********************************"

