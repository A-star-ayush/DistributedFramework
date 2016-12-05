# DistributedFramework
A sample distributed framework based on UNIX sockets and multi-threading (POSIX threads).

Mirroring the classical meaning, the "Zookeeper" is responsible for management and distribution of work.
The work is carried on by the "Worker(s)". Only a single node must be running the Zookeeper's code. Multiple
nodes may run the Worker's code.

To compile, simply type make. Note that there are different makefiles for the two.

## Current capabilities:
1) File sharing - all files under the "shared" folder on each node are considered public to the framework  
2) Failure detection - simple central heartbeat approach to keep track of active nodes  
3) GCP - a small routine is available to solve the graph coloring problem (gcp) by distributing the input graph data  
4) Logging - all activities are logged on each node containing the necessary connection information  

## Threading:
Basically there are two major threads running on each node: the control thread and the data thread. The control thread on the zookeeper maintains information about the workers: the set of workers that are active, the files they have in their shared folder, their port information etc. The control threads on the workers supply the stated to the zookeeper.  

The data thread on a worker waits to receive the instruction from the zookeeper to perform a task. On receipt of such an
instruction, it finds the solution to the part of the problem that is assigned to it and returns the result to the zookeeper.
The control thread and the data thread respectively handle the control port (cport) and data port (dport) on each node.  

## Sample run:
>Only the cport of the zookeeper along with it's ip address is to be known to all the workers.
>On Zookeeper:  ./zookeeper
>On Worker: ./worker 127.0.0.1 40000 20500 30500   "[ <ip of zookeeper> <zookeeper's cport> <self cport> <self dport> ]"
	
