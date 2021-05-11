#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

typedef struct Graph{
	int noOfVertex;
	int noOfEdges;
	int** edges;
}Graph;

int **edgeList;


void createGraphFromFile(Graph* graph, char fileName[]){
	FILE* fp;
	fp = fopen(fileName, "r");
	if(fp == NULL){
		printf("Couldn't open file.\n");
		exit(0);
	}

	int vertices, edges;

	fscanf(fp, "%d %d", &vertices, &edges);

	graph->noOfEdges = edges;
	graph->noOfVertex = vertices;
	graph->edges = (int **)malloc(edges*sizeof(int*));

	for(int i = 0; i<edges; i++){
		graph->edges[i] = (int *)calloc(3, sizeof(int));
	}

	for(int i = 0; i<edges; i++){
		int frm, to, wt;
		int res = fscanf(fp, "%d %d %d", &frm, &to, &wt);
		graph->edges[i][0] = frm;
		graph->edges[i][1] = to;
		graph->edges[i][2] = wt;

		if(res == EOF){
			printf("File is not properly formatted.\n");
			fclose(fp);
			exit(0);
		}
	}

	fclose(fp);
}


/************** Functions required for implementing disjoint-set union.***********/

int findSet(int* parent, int v, int depth){
	if(parent[v] != v)
		parent[v] = findSet(parent, parent[v], depth+1);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// printf("\nRank = %d, %d", rank, depth);
	return parent[v];
}

void unionByRank(int v1, int v2, int* parent, int* rank){
	int x = findSet(parent, v1, 0);
	int y = findSet(parent, v2, 0);

	if(rank[x] > rank[y]){
		parent[y] = x;
	}
	else if(rank[x] < rank[y]){
		parent[x] = y;
	}
	else{
		parent[y] = x;
		rank[x]++;
	}
	return;
}

void customCompareMPI(int* friendResult, int* myResult, int* resultLen, MPI_Datatype* dptr){
	
	for(int i = 0; i<(*resultLen); i++){
		if (*myResult == INT_MAX){
  			*myResult = *friendResult;
  		}
  		else if(*myResult != INT_MAX && *friendResult != INT_MAX){

    		if ( edgeList[*friendResult][2] < edgeList[*myResult][2]){
      			*myResult = *friendResult;
  			}
		}
		friendResult++;
		myResult++;
	}
}

void resetCheapestArray(int* arr, int vertex){
	for(int i = 0; i<vertex; i++){
		arr[i] = INT_MAX;
	}
}


int main(int* argc, char** argv){
	double start, end, average1, start1, end1, start2, end2;
	int countTimes = 0;
	edgeList = NULL;
	Graph* graph;
	graph = (Graph*)malloc(sizeof(Graph));
	createGraphFromFile(graph, argv[1]);
	// printf("\nFinished reading file");
	edgeList = graph->edges;
	MPI_Op customOP;
	MPI_Init(NULL, NULL);
	start = MPI_Wtime();
	int vertex = graph->noOfVertex;
	int edges = graph->noOfEdges;

	int rank, size;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int startIndex, endIndex;
  
	if(edges % size == 0){
		startIndex = rank * (edges/size);
		endIndex = startIndex + (edges/size - 1);
	}
	else{
		int num2 = edges/size;
		int num1 = num2 + 1;
		int extraCountProc = edges % size;
		if(rank >= extraCountProc){
			int prevNonExtraProcCount = rank-extraCountProc;
		int assignedEdges = extraCountProc * num1 + prevNonExtraProcCount * num2;
		startIndex = assignedEdges;
		endIndex = assignedEdges + num2 - 1;
		}
		else{
			startIndex = rank * num1;
		endIndex = startIndex + num1 - 1;
		}
	}

	MPI_Op_create((MPI_User_function *) customCompareMPI, true, &customOP);

	// int cheapestEdge[vertex];
	int* cheapestEdge = (int *)malloc(sizeof(int)*vertex); 
	int* parent = (int*)malloc(sizeof(int)*vertex);
	int* rankDSU = (int*)malloc(vertex * sizeof(int));
	int* recieveEdge = (int*)malloc(vertex * sizeof(int));

	int mstWeightSum = 0;

	/*Initilize Disjoint Set Union.*/
	for(int i = 0; i<vertex; i++){
		cheapestEdge[i] = INT_MAX;
		if(rank == 0){
			rankDSU[i] = 0;
			parent[i] = i;
		}
	}

	// printf("\nInitialized arrays ");
	int vertexLeft = vertex;

	for(; vertexLeft > 1; ){
		resetCheapestArray(cheapestEdge, vertex);

		/*Broadcast the current status of parent array to all the processes. */
		MPI_Bcast(parent, vertex, MPI_INT, 0, MPI_COMM_WORLD);

		// printf("\nRank = %d; cheapestEdge : [", rank);
		// int x = 0;
		// for(x = 0; x < vertex; x++){
		// 	printf("%d,", cheapestEdge[x]);
		// }
		// printf("]");

		// printf("\nrank = %d, startIndex = %d, endIndex = %d", rank, startIndex, endIndex);
		start1 = MPI_Wtime();
		for(int i = startIndex; i <= endIndex; i++){
			int s1 = findSet(parent, edgeList[i][0], 0);
			int s2 = findSet(parent, edgeList[i][1], 0);

			if(s1 != s2){
				// printf("\nRank = %d, at edge (%d,%d), s1 = %d, s2 = %d", rank, edgeList[i][0], edgeList[i][1], s1, s2);
				if(cheapestEdge[s1] != INT_MAX) {
					if(edgeList[cheapestEdge[s1]][2] > edgeList[i][2]){
						cheapestEdge[s1] = i;
					}
				}
				else{
					cheapestEdge[s1] = i;
				}
				if(cheapestEdge[s2] != INT_MAX) {
					if(edgeList[cheapestEdge[s2]][2] > edgeList[i][2]){
						cheapestEdge[s2] = i;
					}
				}
				else{
					cheapestEdge[s2] = i;
				}
				
				// if(cheapestEdge[s2] == INT_MAX || edgeList[cheapestEdge[s2]][2] > edgeList[i][2]){
				// 	cheapestEdge[s2] = i;
				// }
			} 
		}
		end1 = MPI_Wtime();
		//printf("\nRank = %d; time1 = %lf", rank, end1-start1);
	
		MPI_Reduce(cheapestEdge, recieveEdge, vertex, MPI_INT, customOP, 0, MPI_COMM_WORLD);
		int k;
		// if(rank == 0){
		// for(k=0; k<vertex; k++){
		// 	if(recieveEdge[k] == INT_MAX) printf("\nk=%d; recEdge[k] = INT_MAX", k);
		// 	else printf("\nk=%d; recEdge[k] = %d", k, recieveEdge[k]);
		// }
		// }
		// if(rank == 0){
		// 	for(k=0; k<edges; k++){
		// 		printf("k=%d, edgeList[k] = {%d,%d,%d}", k, edgeList[k][0], edgeList[k][1], edgeList[k][2]);
		// 	}
		// }
		/*Combining results of each process at rank 0 process.*/
		if(rank == 0){
			start2 = MPI_Wtime();
			//printf("\nAt iteration %d", vertexLeft);
			for(int i = 0; i<vertex; i++){
				// printf("\nAt i = %d, recEdge[%d] = %d", i, i, recieveEdge[i]);
				if(recieveEdge[i] != INT_MAX){
					int s1 = findSet(parent, edgeList[recieveEdge[i]][0], 0);
					int s2 = findSet(parent, edgeList[recieveEdge[i]][1], 0);
					if(s1 == s2){
						continue;
					}
					mstWeightSum += edgeList[recieveEdge[i]][2];
					// printf("\nHere");
					// printf("\nRank = %d; rankDSU: [", rank);
					// int x = 0;
					// for(x = 0; x < vertex; x++){
					// 	printf("%d,", rankDSU[x]);
					// }
					unionByRank(s1, s2, parent, rankDSU);
					vertexLeft--;
				}
			}
			end2 = MPI_Wtime();
			//printf("\nRank = %d, time2 = %lf", rank, (end2-start2));
			// printf("\nCompleted Iteration");
		}

		MPI_Bcast(&vertexLeft,1,MPI_INT,0, MPI_COMM_WORLD);
	} 
	end = MPI_Wtime();

	if(rank == 0){
		printf("Weight of MST : %d\n", mstWeightSum);
		printf("Execution time : %lf\n", (end-start));
	}

	MPI_Finalize();

	return 0;
}


// 4 5
// 0 1 10
// 0 2 6
// 0 3 5
// 1 3 15
// 2 3 4



// 7 21
// 0 1 2
// 0 2 8
// 0 3 5
// 0 4 1
// 0 5 10
// 0 6 5
// 1 2 9
// 1 3 9
// 1 4 3
// 1 5 5
// 1 6 6
// 2 3 6
// 2 4 2
// 2 5 8
// 2 6 2
// 3 4 2
// 3 5 6
// 3 6 3
// 4 5 8
// 4 6 7
// 5 6 2

//https://browserboard.com/whiteboard/05dce4fc-3bb8-473e-9738-612344c67cb2


