#define PRINT_STACKTRACE

#include "config.h"
#include "VCluster.hpp"

#include "util/print_stack.hpp"
#include "util/math_util_complex.hpp"

//init_options global_option;
Vcluster<> * global_v_cluster_private_heap = NULL;
Vcluster<CudaMemory> * global_v_cluster_private_cuda = NULL;

//
std::vector<int> sieve_spf;

// number of vcluster instances
size_t n_vcluster = 0;
bool ofp_initialized = false;

size_t tot_sent = 0;
size_t tot_recv = 0;

std::string program_name;

#ifdef CUDA_GPU

#include "memory/CudaMemory.cuh"

CudaMemory mem_tmp;

CudaMemory rem_tmp;
CudaMemory rem_tmp2[MAX_NUMER_OF_PROPERTIES];

CudaMemory exp_tmp;
CudaMemory exp_tmp2[MAX_NUMER_OF_PROPERTIES];

#endif

// Segmentation fault signal handler
void bt_sighandler(int sig, siginfo_t * info, void * ctx_p)
{
	if (sig == SIGSEGV)
		std::cout << "Got signal " << sig << " faulty address is %p, " << info->si_addr << " from " << info->si_pid << std::endl;
	else
		std:: cout << "Got signal " << sig << std::endl;

	print_stack();

	exit(1);
}

double time_spent = 0.0;


/*! \brief Initialize the library
 *
 * This function MUST be called before any other function
 *
 */
void openfpm_init_vcl(int *argc, char ***argv, MPI_Comm ext_comm)
{

#if defined (ENABLE_NUMERICS) && defined (HAVE_PETSC)
	#ifndef PETSC_SAFE_CALL
	#define PETSC_SAFE_CALL(call) {\
		PetscErrorCode err = call;\
		if (err != 0) {\
			std::string msg("Petsc error: ");\
			msg += std::string(__FILE__) + std::string(" ") + std::to_string(__LINE__);\
			PetscInt ln = __LINE__;\
			PetscError(PETSC_COMM_WORLD,ln,__FUNCTION__,__FILE__,err,PETSC_ERROR_INITIAL,"Error petsc");\
		}\
	}
	#endif

	if (ext_comm!=MPI_COMM_WORLD)
	{
		PETSC_COMM_WORLD = ext_comm;
	}
	PetscBool initialized;
	PETSC_SAFE_CALL(PetscInitialized(&initialized));
	if(initialized==PETSC_FALSE)
	{
		PETSC_SAFE_CALL(PetscInitialize(argc,argv,NULL,NULL));
	}
#endif

	init_global_v_cluster_private(argc,argv,ext_comm);

#ifdef SE_CLASS1
	std::cout << "OpenFPM is compiled with debug mode LEVEL:1. Remember to remove SE_CLASS1 when you go in production" << std::endl;
#endif

#ifdef SE_CLASS2
	std::cout << "OpenFPM is compiled with debug mode LEVEL:2. Remember to remove SE_CLASS2 when you go in production" << std::endl;
#endif

#ifdef SE_CLASS3
	std::cout << "OpenFPM is compiled with debug mode LEVEL:3. Remember to remove SE_CLASS3 when you go in production" << std::endl;
#endif

	init_wrappers();

	// install segmentation fault signal handler

	struct sigaction sa;

	sa.sa_sigaction = bt_sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	sigaction(SIGSEGV, &sa, NULL);

	if (argc != NULL && *argc != 0)
	{program_name = std::string(*argv[0]);}

	// Initialize math pre-computation tables
	openfpm::math::init_getFactorization();

	ofp_initialized = true;

#ifdef CUDA_GPU

	// Initialize temporal memory
	mem_tmp.incRef();

	exp_tmp.incRef();

	for (int i = 0 ; i < MAX_NUMER_OF_PROPERTIES ; i++)
	{
		exp_tmp2[i].incRef();
	}


#endif
}

size_t openfpm_vcluster_compilation_mask()
{
	size_t compiler_mask = CUDA_ON_BACKEND;

	return compiler_mask;
}

/*! \brief Finalize the library
 *
 * This function MUST be called at the end of the program
 *
 */
void openfpm_finalize()
{
#if defined (ENABLE_NUMERICS) && defined (HAVE_PETSC)

	PetscFinalize();

#endif

	delete_global_v_cluster_private();
	ofp_initialized = false;

#ifdef CUDA_GPU

	// Release memory
	mem_tmp.destroy();
	mem_tmp.decRef();

	exp_tmp.destroy();
	exp_tmp.decRef();

	for (int i = 0 ; i < MAX_NUMER_OF_PROPERTIES ; i++)
	{
		exp_tmp2[i].destroy();
		exp_tmp2[i].decRef();
	}

#endif
}
