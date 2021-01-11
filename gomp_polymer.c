#include "gomp_polymer.h"



int main(int argc, char **argv)
{
	char * file;

	if (argc < 2)
		error("Wrong number of arguments.");
	file = argv[1];

	int balance_in_edges = 1;
	// int balance_in_edges = 0;
	create_graph_structures(file, balance_in_edges);
	return 0;

	clear_next_frontiers();

	// _Pragma("omp parallel")
	// {
		// int tnum = omp_get_thread_num();
		// int tgpos = omp_get_thread_group_pos();
		// int group_master_tnum = omp_get_thread_group_master_num();
		// int tgnum = omp_get_thread_group_num();
		// struct thread_group_data    * tgd   = &TGD[tgnum];
		// struct graph_partition_data * gpart = tgd->graph_partition;
		// if (tnum == group_master_tnum)
			// gpart->frontier_next->dense[0] = 1;
	// }

	init(N);
	// print_parents();

	for (long i=0;i<num_thread_groups;i++) {
		__auto_type gpart = TGD[i].graph_partition;
		char buf[200];
		char * iter;
		printf("tgnum:%ld\n", i);
		printf("Frontier: Next, Dense\n");
		for (long j=0;j<gpart->num_local_nodes;j++)
		{
			long k = gpart->node_id_s + j;
			printf("node:%-10ld val:%-10d\n", k, gpart->frontier_next->dense[j]);
		}
		printf("\n");
	}


	return 0;
}


