#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define MAX 100

double** arr;	//input matrix
double** transpose;
double** lowerTriangular; 	//output matrix 

int main(int argc, char** argv)
{
	int my_rank, comm_sz;
	double start, end;

	//read input from file
	char* fileName = argv[1];
	FILE* f = fopen(fileName, "r");
	if(f == NULL){
		printf("Couldn't open file.\n");
		exit(0);
	}
	int N; 	//size of N x N matrix
	fscanf(f, "%d", &N);
	arr = (double**)malloc(N*sizeof(double*));
	transpose = (double**)malloc(N*sizeof(double*));
	int i,j;
	for(i=0; i<N; i++){
		arr[i] = (double*)malloc(N*sizeof(double));
		transpose[i] = (double*)malloc(N*sizeof(double));
		for(j=0; j<N; j++){
			fscanf(f, "%lf", &arr[i][j]);
			transpose[i][j] = 0.0;
		}
	}

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime();

	int k;
	for (k=0; k<N; k++) {
		if (k % comm_sz == my_rank) {
			for (i=0; i<k; i++) {
				arr[k][k] = arr[k][k] - transpose[k][i] * transpose[k][i];
			}
			transpose[k][k] = sqrt(arr[k][k]);
		}
		
		//broadcast the column (by broadcasting the row from transpose matrix)
		MPI_Bcast(&transpose[k][0], N, MPI_DOUBLE, k % comm_sz, MPI_COMM_WORLD);
		//updating matrix below diagonal.
		for (i=k+1; i<N; i++) {
			if (i % comm_sz == my_rank) {
				for (j = 0; j < k; j++) {
					arr[i][k] -= transpose[i][j] * transpose[k][j];
				}
				transpose[i][k] = arr[i][k] / transpose[k][k];
			}
		}
		
	}
	MPI_Barrier(MPI_COMM_WORLD); 
	end = MPI_Wtime();
	
	//printing the result matrix.
	if(my_rank == 0){
		printf("Decomposed Matrix: \n");
		for(i=0; i<N; i++){
			for(j=0; j<N; j++){
				printf("%lf\t" , transpose[i][j]);
			}
			printf("\n");
		}
		double diff = (end-start);
		printf("Execution Time: %lf\n", diff);	
	}
	MPI_Finalize();
	return 0;				
}




