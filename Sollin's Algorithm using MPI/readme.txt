To build, run the 'make' command.

The input graph is read from a file named 'input.txt' that contains :
	First line contains the number of vertices 'v' followed by number of edges 'e' (separated by space)
	Then there are 'e' lines containing the source, destination and weight of each edge (separated by space)


To run, use the command : mpirun -np <no_of_proc> ./mst_sollins input.txt
	eg:- mpirun -np 1 ./mst_sollins input.txt
 
