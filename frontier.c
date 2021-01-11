#include "frontier.h"


void
frontier_dense_to_sparse(struct frontier_data * frontier)
{
	long hierarchical_stealing = omp_get_hierarchical_stealing();
	omp_set_hierarchical_stealing(0);

	_Pragma("omp parallel")
	{
		int tnum = omp_get_thread_num();
		int tgpos = omp_get_thread_group_pos();
		int group_master_tnum = omp_get_thread_group_master_num();
		int tgnum = omp_get_thread_group_num();
		struct thread_group_data    * tgd   = &TGD[tgnum];
		struct graph_partition_data * gpart = tgd->graph_partition;
		long i;
		long sum;

		omp_set_loop_partitioner(loop_partitioner_entire_field);
		PRAGMA(omp for schedule(hierarchical, GRAIN_SIZE))
		for (i=0;i<N;i++)
		{
			// if ()
				// sum++;
		}
	}

	omp_set_hierarchical_stealing(hierarchical_stealing);
}


void
clear_next_frontiers()
{
	long hierarchical_stealing = omp_get_hierarchical_stealing();
	omp_set_hierarchical_stealing(0);

	_Pragma("omp parallel")
	{

		// int tnum = omp_get_thread_num();
		// int tgpos = omp_get_thread_group_pos();
		// int tgnum = omp_get_thread_group_num();
		// int group_size = omp_get_thread_group_size();
		// struct thread_group_data    * tgd   = &TGD[tgnum];
		// struct graph_partition_data * gpart = tgd->graph_partition;
		// long block_size = (gpart->num_local_nodes + group_size - 1) / group_size;
		// long b_s;
		// long b_e;
		// long i, j;
		// b_s = tgpos * block_size;
		// b_e = b_s + block_size;
		// if (b_e > N)
			// b_e = N;
		// for (i=b_s;i<b_e;i++)
		// {
			// gpart->frontier_next->dense[i] = tnum;
		// }

		int tnum = omp_get_thread_num();
		int tgnum = omp_get_thread_group_num();
		struct thread_group_data    * tgd   = &TGD[tgnum];
		struct graph_partition_data * gpart = tgd->graph_partition;
		long i, j;
		omp_set_loop_partitioner(loop_partitioner_owner_group);
		PRAGMA(omp for schedule(hierarchical, GRAIN_SIZE))
		for (i=0;i<N;i++)
		{
			j = i - gpart->node_id_s;
			gpart->frontier_next->dense[j] = tnum;
		}

	}

	omp_set_hierarchical_stealing(hierarchical_stealing);
}


