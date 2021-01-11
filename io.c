#include "io.h"


//==========================================================================================================================================
//= Read Graph File
//==========================================================================================================================================


static
void 
read_graph_file(char * file, long ** ret_offsets, long ** ret_edges_out)
{
	struct file_atoms * A;
	long * offsets;
	long * edges_out;
	long base;

	A = get_words_from_file(file, 0);

	base = 1;
	N = atol(A->atoms[base++]);
	E = atol(A->atoms[base++]);
	alloc(N + 1, offsets);
	alloc(E, edges_out);

	_Pragma("omp parallel")
	{
		long i;
		PRAGMA(omp for schedule(static))
		for (i=0;i<N;i++)
			offsets[i] = atol(A->atoms[base + i]);
	}
	offsets[N] = E;      // Remove the boundary special case, to make calculating offset differences (number of edges) easier.

	base += N;

	_Pragma("omp parallel")
	{
		long i;
		PRAGMA(omp for schedule(static))
		for (i=0;i<E;i++)
			edges_out[i] = atol(A->atoms[base + i]);
	}

	*ret_offsets = offsets;
	*ret_edges_out = edges_out;
	file_atoms_destructor(&A);
}


//==========================================================================================================================================
//= Create In Edges
//==========================================================================================================================================


static
void
create_in_edges(long * offsets_out, long * edges_out, long ** ret_offsets_in, long ** ret_edges_in)
{
	int num_threads = omp_get_max_threads();
	long block_size = (N + num_threads - 1) / num_threads;
	long i;

	long * offsets_in;
	long * edges_in;
	long * degrees;

	struct {
		long b_s;
		long b_e;
		long offset;
		long sum;
	} tpd[num_threads];

	alloc(N + 1, offsets_in);
	alloc(N, degrees);

	alloc(E, edges_in);

	_Pragma("omp parallel")
	{
		int tnum = omp_get_thread_num();
		typeof(&tpd[0]) d = &tpd[tnum];
		long i, sum, pos;

		d->b_s = tnum * block_size;
		d->b_e = d->b_s + block_size;
		if (d->b_e > N)
			d->b_e = N;

		/* First touch - initialize. */
		PRAGMA(omp for schedule(static))
		for (i=0;i<N;i++)
			degrees[i] = 0;

		/* Calculate in-degrees. */
		PRAGMA(omp for schedule(static))
		for (i=0;i<E;i++)
		{
			pos = edges_out[i];
			__atomic_add_fetch(&degrees[pos], 1, __ATOMIC_RELAXED);
		}

		/* Calculate relative offsets. */
		sum = 0;
		for (i=d->b_s;i<d->b_e;i++)
		{
			offsets_in[i] = sum;
			sum += degrees[i];
		}

		d->sum = sum;
	}
	offsets_in[N] = E;

	/* Calculate absolute offsets. */

	long total = 0;
	for (i=0;i<num_threads;i++)
	{
		tpd[i].offset = total;
		total += tpd[i].sum;
	}

	_Pragma("omp parallel")
	{
		int tnum = omp_get_thread_num();
		typeof(&tpd[0]) d = &tpd[tnum];
		long i, j, degree_out, node_id, pos;

		for (i=d->b_s;i<d->b_e;i++)
			offsets_in[i] += d->offset;

		_Pragma("omp barrier")

		/* Calculate in-edges. */
		PRAGMA(omp for schedule(hierarchical, GRAIN_SIZE))
		for (i=0;i<N;i++)
		{
			degree_out = offsets_out[i+1] - offsets_out[i];
			for (j=0;j<degree_out;j++)
			{
				node_id = edges_out[offsets_out[i] + j];
				pos = __atomic_add_fetch(&degrees[node_id], -1, __ATOMIC_RELAXED);
				edges_in[offsets_in[node_id] + pos] = i;
			}
		}
	}

	*ret_offsets_in = offsets_in;
	*ret_edges_in = edges_in;
	free(degrees);
}


//==========================================================================================================================================
//= Partitioning
//==========================================================================================================================================


static
long
binary_search_l(long * offsets, long target)
{
	long s, e, m;
	s = 0;
	e = N - 1;
	while (1)
	{
		m = (s + e) / 2;
		if (m == s)
			break;
		if (offsets[m] < target)
			s = m;
		else
			e = m;
	}
	return (target - offsets[s] < offsets[e] - target) ? s : e;
}


// Calculate the vertices partitions of the groups, 
// by balancing the number of edges per partition.
static
void
calculate_partitions(long * offsets)
{
	_Pragma("omp parallel")
	{
		int                           tnum               = omp_get_thread_num();
		int                           group_master_tnum  = omp_get_thread_group_master_num();
		int                           tgnum              = omp_get_thread_group_num();
		struct thread_group_data    * tgd                = &TGD[tgnum];
		struct graph_partition_data * gpart              = tgd->graph_partition;

		long balancing_target = E * tgnum / num_thread_groups;
		if (tnum == group_master_tnum)
			gpart->node_id_s = binary_search_l(offsets, balancing_target);

		// Ideally we want barrier only for the masters, but this function is comparably too fast to care.
		_Pragma("omp barrier")

		if (tnum == group_master_tnum)
		{
			if (tgnum == num_thread_groups - 1)
				gpart->node_id_e = N;
			else
				gpart->node_id_e = TGD[tgnum+1].graph_partition->node_id_s;
			gpart->num_local_nodes = gpart->node_id_e - gpart->node_id_s;
		}
	}
}


static
void
create_partitions(long * offsets_out, long * offsets_in, long * edges_out, long * edges_in)
{
	int num_threads = omp_get_max_threads();
	struct {
		long b_s;
		long b_e;
		long offset_out;
		long offset_in;
		long sum_out;
		long sum_in;
	} tpd[num_threads];

	long hierarchical_stealing = omp_get_hierarchical_stealing();
	omp_set_hierarchical_stealing(0);

	_Pragma("omp parallel")
	{
		int                           tnum               = omp_get_thread_num();
		int                           tgpos              = omp_get_thread_group_pos();
		int                           group_size         = omp_get_thread_group_size();
		int                           group_master_tnum  = omp_get_thread_group_master_num();
		int                           tgnum              = omp_get_thread_group_num();
		struct thread_group_data    * tgd                = &TGD[tgnum];
		struct graph_partition_data * gpart              = tgd->graph_partition;

		typeof(&tpd[0])               d                  = &tpd[tnum];

		long i, j, offset, pos, degree, src, trg;

		long block_size = (N + group_size - 1) / group_size;
		d->b_s = tgpos * block_size;
		d->b_e = d->b_s + block_size;
		if (d->b_e > N)
			d->b_e = N;

		if (tnum == group_master_tnum)
		{
			gpart->num_edges_out = offsets_in[gpart->node_id_e] - offsets_in[gpart->node_id_s];
			gpart->num_edges_in = offsets_out[gpart->node_id_e] - offsets_out[gpart->node_id_s];

			alloc(N + 1, gpart->nodes_out);
			alloc(gpart->num_edges_out, gpart->edges_out);

			alloc(N + 1, gpart->nodes_in);
			alloc(gpart->num_edges_in, gpart->edges_in);

			alloc(gpart->num_local_nodes, gpart->local_node_data);

			alloc(N, gpart->frontier_curr->dense);
			alloc(N, gpart->frontier_next->dense);
		}

		_Pragma("omp barrier")

		/* Initialization. */
		for (i=d->b_s;i<d->b_e;i++)
		{
			gpart->nodes_out[i].num_edges = 0;
			gpart->nodes_in[i].num_edges = 0;
		}

		_Pragma("omp barrier")

		/* Count local degrees (i.e. only counting the edges the group owns). */
		omp_set_loop_partitioner(loop_partitioner_owner_group);
		PRAGMA(omp for schedule(hierarchical, GRAIN_SIZE))
		for (i=0;i<N;i++)
		{
			offset = offsets_in[i];
			degree = offsets_in[i + 1] - offset;
			for (j=0;j<degree;j++)
			{
				src = edges_in[offset + j];
				__atomic_add_fetch(&gpart->nodes_out[src].num_edges, 1, __ATOMIC_RELAXED);
			}

			offset = offsets_out[i];
			degree = offsets_out[i + 1] - offset;
			for (j=0;j<degree;j++)
			{
				trg = edges_out[offset + j];
				__atomic_add_fetch(&gpart->nodes_in[trg].num_edges, 1, __ATOMIC_RELAXED);
			}
		}

		/* Calculate relative offsets (for the threads in the group).
		   Incorporate the base pointer to the edge array. */
		long sum_out = 0;
		long sum_in = 0;
		for (i=d->b_s;i<d->b_e;i++)
		{
			gpart->nodes_out[i].edges = gpart->edges_out + sum_out;
			sum_out += gpart->nodes_out[i].num_edges;

			gpart->nodes_in[i].edges = gpart->edges_in + sum_in;
			sum_in += gpart->nodes_in[i].num_edges;
		}
		d->sum_out = sum_out;
		d->sum_in = sum_in;
		// printf("%d: sum_out:%ld, sum_in:%ld\n", tnum, d->sum_out, d->sum_in);

		_Pragma("omp barrier")

		/* Calculate the base absolute offset for each thread. */
		if (tnum == group_master_tnum)
		{
			long total_out = 0;
			long total_in = 0;
			for (i=0;i<group_size;i++)
			{
				long member = tnum + i;
				tpd[member].offset_out = total_out;
				total_out += tpd[member].sum_out;

				tpd[member].offset_in = total_in;
				total_in += tpd[member].sum_in;
			}

			// Remove the boundary special case, to make calculating differences (number of edges) easier.
			gpart->nodes_out[N].edges = gpart->edges_out + gpart->num_edges_out;
			gpart->nodes_in[N].edges = gpart->edges_in + gpart->num_edges_in;
		}

		_Pragma("omp barrier")

		// if (tnum == 0)
			// for (i=0;i<num_threads;i++)
				// printf("%ld: b_s=%ld , b_e=%ld , offset_out:%ld, offset_in:%ld\n", i, tpd[i].b_s, tpd[i].b_e, tpd[i].offset_out, tpd[i].offset_in);

		/* Calculate absolute offsets (add the base offset of each thread to it's relative offsets). */
		for (i=d->b_s;i<d->b_e;i++)
		{
			gpart->nodes_out[i].edges += d->offset_out;
			gpart->nodes_in[i].edges += d->offset_in;
		}

		_Pragma("omp barrier")

		/* Copy edges. Use degrees as atomic counters, to enable intra-group stealing. */
		omp_set_loop_partitioner(loop_partitioner_owner_group);
		PRAGMA(omp for schedule(hierarchical, GRAIN_SIZE))
		for (i=0;i<N;i++)
		{
			offset = offsets_in[i];
			degree = offsets_in[i + 1] - offset;
			for (j=0;j<degree;j++)
			{
				src = edges_in[offset + j];
				pos = __atomic_add_fetch(&gpart->nodes_out[src].num_edges, -1, __ATOMIC_RELAXED);
				gpart->nodes_out[src].edges[pos].node_id = i;
			}

			offset = offsets_out[i];
			degree = offsets_out[i + 1] - offset;
			for (j=0;j<degree;j++)
			{
				trg = edges_out[offset + j];
				pos = __atomic_add_fetch(&gpart->nodes_in[trg].num_edges, -1, __ATOMIC_RELAXED);
				gpart->nodes_in[trg].edges[pos].node_id = i;
			}
		}

		/* Restore degrees. */
		for (i=d->b_s;i<d->b_e;i++)
		{
			gpart->nodes_out[i].num_edges = gpart->nodes_out[i+1].edges - gpart->nodes_out[i].edges;
			gpart->nodes_in[i].num_edges = gpart->nodes_in[i+1].edges - gpart->nodes_in[i].edges;
		}

		/* Partitions lookup table. */
		if (tnum == group_master_tnum)
		{
			for (i=0;i<num_thread_groups;i++)
				gpart->partitions_start_nodes[i] = TGD[i].graph_partition->node_id_s;
			gpart->partitions_start_nodes[num_thread_groups] = N;
		}

		/* Node data. */

		omp_set_loop_partitioner(loop_partitioner_owner_group);
		PRAGMA(omp for schedule(hierarchical, GRAIN_SIZE))
		for (i=0;i<N;i++)
		{
			j = i - gpart->node_id_s;
			gpart->local_node_data[j].rank_out = offsets_out[i+1] - offsets_out[i];
			gpart->local_node_data[j].rank_in = offsets_in[i+1] - offsets_in[i];
		}
	}

	omp_set_hierarchical_stealing(hierarchical_stealing);
}


//==========================================================================================================================================
//= Create Graph Structures
//==========================================================================================================================================


static
void
print_graph_structs()
{
	long i, j, k;
	printf("\n");
	for (i=0;i<num_thread_groups;i++)
	{
		__auto_type gpart = TGD[i].graph_partition;
		printf("tgnum:%-10ld s:%-10ld e:%-10ld nodes:%-10ld out-edges:%-10ld in-edges:%-10ld\n", 
				i, gpart->node_id_s, gpart->node_id_e, gpart->node_id_e - gpart->node_id_s, gpart->num_edges_out, gpart->num_edges_in);
	}
	return;

	printf("\n");
	for (i=0;i<num_thread_groups;i++)
	{
		__auto_type gpart = TGD[i].graph_partition;
		char buf[200];
		char * iter;
		printf("tgnum:%ld\n", i);
		printf("Local nodes: %ld\n", gpart->num_local_nodes);
		for (j=0;j<gpart->num_local_nodes;j++)
		{
			k = gpart->node_id_s + j;
			printf("node:%-10ld rank-out:%-10ld rank-in:%-10ld\n", k, gpart->local_node_data[j].rank_out, gpart->local_node_data[j].rank_in);
		}
		printf("Node agents:\n");
		for (j=0;j<N;j++)
		{
			printf("node:%-10ld out-edges:%-10ld in-edges:%-10ld", j, gpart->nodes_out[j].num_edges, gpart->nodes_in[j].num_edges);
			printf("out-edges: ");
			buf[0] = '\0';
			iter = buf;
			for (k=0;k<gpart->nodes_out[j].num_edges;k++)
				iter += sprintf(iter, "%ld, ", gpart->nodes_out[j].edges[k].node_id);
			if (k != 0)
				iter[-2] = '\0';
			printf("%-20s in-edges: ", buf);
			for (k=0;k<gpart->nodes_in[j].num_edges;k++)
				printf("%ld, ", gpart->nodes_in[j].edges[k].node_id);
			if (k != 0)
				printf("\b\b  ");
			printf("\n");
		}
		printf("\n");
	}
}


static
void
print_graph_edges(long N, long * offsets, long * edges)
{
	for (long i=0;i<N;i++) printf("%ld: %ld\n", i, offsets[i]);
	for (long i=0;i<N;i++)
	{
		printf("\nnode %ld:\n", i);
		for (long j=offsets[i];j<offsets[i+1];j++)
			printf("%ld\n", edges[j]);
	}
	printf("\n");
}


void
create_graph_structures(char * file, int balance_in_edges)
{
	long * offsets_out;
	long * edges_out;

	long * offsets_in;
	long * edges_in;

	double time;

	// TODO: look at ligra mmap options.

	num_thread_groups = omp_get_num_thread_groups();
	alloc(num_thread_groups, TGD);
	_Pragma("omp parallel")
	{
		int tnum = omp_get_thread_num();
		int group_master_tnum = omp_get_thread_group_master_num();
		if (tnum == group_master_tnum)
		{
			int tgnum = omp_get_thread_group_num();
			thread_group_data_init(&TGD[tgnum]);
		}
	}

	time = time_it(1,
		read_graph_file(file, &offsets_out, &edges_out);
	);
	printf("-> read_graph_file time: %lf\n", time);
	printf("N:%ld E:%ld\n", N, E);
	// print_graph_edges(N, offsets_out, edges_out);

	time = time_it(1,
		create_in_edges(offsets_out, edges_out, &offsets_in, &edges_in);
	);
	printf("-> create_in_edges time: %lf\n", time);
	// print_graph_edges(N, offsets_in, edges_in);

	/* Reverse logic:
	 *     out edges will be partitioned based on their target
	 *     in edges will be partitioned based on their source
	 */
	time = time_it(1,
		if (balance_in_edges)
			calculate_partitions(offsets_out);
		else
			calculate_partitions(offsets_in);
	);
	printf("-> calculate_partitions time: %lf\n", time);

	time = time_it(1,
		create_partitions(offsets_out, offsets_in, edges_out, edges_in);
	);
	printf("-> create_partitions time: %lf\n", time);


	print_graph_structs();


	free(offsets_out);
	free(offsets_in);
	free(edges_out);
	free(edges_in);
}


