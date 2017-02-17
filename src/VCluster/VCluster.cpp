#include "VCluster.hpp"
#include <execinfo.h>

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
	void *trace[16];
	char **messages = NULL;
	int i, trace_size = 0;

	if (sig == SIGSEGV)
		std::cout << "Got signal " << sig << " faulty address is %p, " << info->si_addr << " from " << info->si_pid << std::endl;
	else
		std:: cout << "Got signal " << sig << std::endl;

	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	trace[1] = (void *)info->si_addr;
	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");
	for (i=1; i<trace_size; ++i)
	{
		printf("[bt] #%d %s\n", i, messages[i]);
	}

	exit(0);
}
