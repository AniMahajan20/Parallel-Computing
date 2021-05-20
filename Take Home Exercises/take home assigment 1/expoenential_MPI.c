#include <stdio.h>
#include <mpi.h>

#define TERMS 100000

int main(int argc, char** argv){	
	
	double start_time, end_time;
	long double x, curr_sum = 0.0, final_sum = 0.0;
	long double  curr_val;
	int start_val;
	
	int curr_rank, comm_size;
	
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &curr_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    
    int range = (TERMS / comm_size);   // range*x + (range+1)*(comm_size - x) = TERMS;  range*comm_size - TERMS + comm_size = x
    
    int xVal = range*comm_size - TERMS + comm_size;
    
    int no_of_terms = (curr_rank + 1) <= xVal ? range : range + 1;
	
    
    if(curr_rank == 0){
		printf("Enter the value of x: ");
		scanf("%Lf", &x);
	}
    
	MPI_Bcast(&x, 1, MPI_LONG_DOUBLE, 0, MPI_COMM_WORLD);
	
	MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
	
	if(curr_rank <= xVal){
		start_val = range * (curr_rank);		
	}
	else{
		start_val = range * (xVal) + (range+1)*(curr_rank-xVal);
	}

	curr_val = 1;
	
	for(int i = 1; i<start_val; i++){
		curr_val *= x/i;
	}
	
	for(int i = 0; i<no_of_terms; i++){
		if(i+start_val != 0)
			curr_val *= x/(i+start_val);
		curr_sum += curr_val;
	}
	
	MPI_Reduce(&curr_sum, &final_sum, 1, MPI_LONG_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    end_time = MPI_Wtime();

	if(curr_rank == 0){
        printf("\nValue calculated of e^x is : %LE\n", final_sum);
        printf("Execution time is : %lf\n", (end_time-start_time));
    }
    
	MPI_Finalize();
	return 0;
}


