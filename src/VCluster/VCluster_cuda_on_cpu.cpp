#define PRINT_STACKTRACE

#include "VCluster.hpp"
#ifndef __CYGWIN__
#include <execinfo.h>
#endif

#include "util/print_stack.hpp"
#include "util/math_util_complex.hpp"

Vcluster<> * global_v_cluster_private_heap = NULL;
Vcluster<CudaMemory> * global_v_cluster_private_cuda = NULL;

//
std::vector<int> sieve_spf;

// number of vcluster instances
size_t n_vcluster = 0;
bool ofp_initialized = false;

size_t tot_sent = 0;
size_t tot_recv = 0;

//! NBX has a potential pitfall that must be addressed,
//! NBX Send all the messages and probe for incoming messages,
//! if there is an incoming message it receive it producing
//! an acknowledge notification on the sending processor.
//! When all the sends has been acknowledged, the processor call the MPI_Ibarrier
//! when all the processors call MPI_Ibarrier all send has been received.
//! While the processors are waiting for the MPI_Ibarrier to complete, all processors
//! are still probing for incoming message, Unfortunately some processor
//! can quit the MPI_Ibarrier before others and this mean that some
//! processor can exit the probing status before others, these processors can in theory
//! start new communications while the other processor are still in probing status producing
//! a wrong send/recv association to
//! resolve this problem an incremental NBX_cnt is used as message TAG to distinguish that the
//! messages come from other send or subsequent NBX procedures
size_t NBX_cnt = 0;

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

	exit(0);
}

double time_spent = 0.0;

/*! \brief Initialize a global instance of Runtime Virtual Cluster Machine
 *
 * Initialize a global instance of Runtime Virtual Cluster Machine
 *
 */

void init_global_v_cluster_private(int *argc, char ***argv, init_options option)
{
    if (option == init_options::in_situ_visualization)
    {
        initialize_in_situ(argc, argv);
    }

    //PETSC initialize?
    if (global_v_cluster_private_heap == NULL)
    {global_v_cluster_private_heap = new Vcluster<>(argc,argv);}

    if (global_v_cluster_private_cuda == NULL)
    {global_v_cluster_private_cuda = new Vcluster<CudaMemory>(argc,argv);}
}

void delete_global_v_cluster_private()
{
    delete global_v_cluster_private_heap;
    delete global_v_cluster_private_cuda;
}


/*! \brief Initialize the library
 *
 * This function MUST be called before any other function
 *
 */
void openfpm_init_vcl(int *argc, char ***argv)
{
#ifdef HAVE_PETSC

	PetscInitialize(argc,argv,NULL,NULL);

#endif

	init_global_v_cluster_private(argc,argv);

#ifdef SE_CLASS1
	std::cout << "OpenFPM is compiled with debug mode LEVEL:1. Remember to remove SE_CLASS1 when you go in production" << std::endl;
#endif

#ifdef SE_CLASS2
	std::cout << "OpenFPM is compiled with debug mode LEVEL:2. Remember to remove SE_CLASS2 when you go in production" << std::endl;
#endif

#ifdef SE_CLASS3
	std::cout << "OpenFPM is compiled with debug mode LEVEL:3. Remember to remove SE_CLASS3 when you go in production" << std::endl;
#endif

#ifdef CUDA_ON_CPU
	init_wrappers();
#endif

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
	size_t compiler_mask = 0;

	#ifdef CUDA_ON_CPU
	compiler_mask |= 0x1;
	#endif


	#ifdef CUDA_GPU
	compiler_mask |= 0x04;
	#endif

	return compiler_mask;
}

/*! \brief Finalize the library
 *
 * This function MUST be called at the end of the program
 *
 */
void openfpm_finalize()
{
#ifdef HAVE_PETSC

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
