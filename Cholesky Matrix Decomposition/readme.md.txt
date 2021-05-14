
To build, run the 'make' command.

The input matrix is read from a file named 'input.txt' that contains :
	First line contains an integer representing dimension size of the square matrix 'N'. 
	Please Note that matrix must be a positive-definite matrix.
	Then there are N lines each of which contains N space separated decimal numbers that represent each row of the matrix

To run, use the command : mpirun -np <no_of_proc> ./matrix_cholesky input.txt
	eg:- mpirun -np 1 ./matrix_cholesky input.txt
 