#include <stdio.h>
#include <stdlib.h>


int main(){

    FILE *fp = fopen("input.txt", "w");

    int n = 100;
    fprintf(fp, "%d\n", n);
    for(int i = 0; i<100; i++){
        for(int j = 0; j<100; j++){
            int val = rand() % 1000;
            if(j == 99){
                fprintf(fp, "%d\n", val);
            }
            else{
                fprintf(fp, "%d ", val);
            }
        }
    }

    return 0;
}