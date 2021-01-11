#!/bin/bash

home='/home/users/dgal'

# export LD_LIBRARY_PATH="/various/common_tools/gcc-5.3.0/lib64:/various/common_tools/papi-5.5.1/lib"
# export LD_LIBRARY_PATH="/various/common_tools/gcc-6.4.0/lib64:/various/common_tools/papi-5.5.1/lib"
# export LD_LIBRARY_PATH="/various/dgal/gcc-8.3.0/lib64:/various/common_tools/papi-5.5.1/lib"
export LD_LIBRARY_PATH="${HOME}/Documents/gcc_current/lib64:/various/common_tools/papi-5.5.1/lib"


host="$(hostname)"
if [[ "$host" == "broady2" ]] || [[ "$host" == "broady3" ]]; then
    cores=20
    cpu_node_size=10
    graph_dir="${home}/graphs"
elif [[ "$host" == "sandman" ]]; then
    cores=32
    cpu_node_size=8
    graph_dir="${home}/graphs"
else
    export LD_LIBRARY_PATH="${HOME}/Documents/gcc_current/lib64"
    # cores=16
    cores=8
    # cores=4
    # cores=1
    # cpu_node_size=8    # one node
    cpu_node_size=4    # 2 nodes of 4 threads each
    # cpu_node_size=2    # 4 nodes of 2 threads each
    # cpu_node_size=1    # all are nodes
    graph_dir="$HOME/Data/graphs"
    # graph_dir="/home/jim/Documents"
fi


# file='small_graph'
file='small_graph_polymer'
# file='rMatGraph_J_5_100'
# file='RMat24_5_1_1_1600000_99000000'

data=("${graph_dir}/${file}")


export OMP_MAX_THREAD_GROUP_SIZE="${cpu_node_size}"

# Encourages idle threads to spin rather than sleep.
export OMP_WAIT_POLICY='active'
# Don't let the runtime deliver fewer threads than those we asked for.
export OMP_DYNAMIC='false'
export OMP_NUM_THREADS="${cores}"
export GOMP_CPU_AFFINITY="0-$((${cores}-1))"


export OMP_HIERARCHICAL_STEALING='true'
# export OMP_HIERARCHICAL_STEALING='false'

export OMP_HIERARCHICAL_STEALING_SCORES='true'
# export OMP_HIERARCHICAL_STEALING_SCORES='false'

# export OMP_HIERARCHICAL_STEALING_CPU_NODE_LOCALITY_PASS='true'
export OMP_HIERARCHICAL_STEALING_CPU_NODE_LOCALITY_PASS='false'

# export OMP_HIERARCHICAL_STATIC='true'
export OMP_HIERARCHICAL_STATIC='false'



./bfs.exe "${data[@]}"


