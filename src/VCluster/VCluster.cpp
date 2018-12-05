#define PRINT_STACKTRACE

#include "VCluster.hpp"
#ifndef __CYGWIN__
#include <execinfo.h>
#endif

#include "util/print_stack.hpp"

init_options global_option;
Vcluster * global_v_cluster_private = NULL;


// number of vcluster instances
size_t n_vcluster = 0;
bool ofp_initialized = false;

size_t tot_sent = 0;
size_t tot_recv = 0;

std::string program_name;

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
