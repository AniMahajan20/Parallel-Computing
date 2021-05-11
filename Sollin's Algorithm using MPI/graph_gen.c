#include <stdio.h>
#include <stdlib.h>


int main(int argc, char** argv){

    FILE *fp = fopen("input.txt", "w");

    if(argc < 1){
        printf("Invalid Input.\n");
        exit(0);
    }

    int vertices = atoi(argv[1]);
    int edges = (vertices*(vertices-1)/2);


    fprintf(fp, "%d %d\n", vertices, edges);
    int i,j;
    for(i = 0; i<vertices; i++){
        for(j = 0; j<vertices; j++){
            if(i != j)
                fprintf(fp, "%d %d %d\n", i, j, rand());
        }    
    }

    printf("Graph generated.\n");

    return 0;
}
