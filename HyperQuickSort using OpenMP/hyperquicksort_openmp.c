#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/time.h>

#define NO_OF_THREADS 4

typedef struct ChunkInfo{
    int start;
    int end;
} ChunkInfo;

typedef struct OutgoingInfo{
    int* arr;   //contains ONLY those elements which are leaving the chunk
    int to;
    int arrSize;
    bool isLow;
} OutgoingInfo;

int* arrRead;
int* arrWrite;
int medians[NO_OF_THREADS];
int chunkSizes[NO_OF_THREADS];
ChunkInfo infos[NO_OF_THREADS];
OutgoingInfo outInfos[NO_OF_THREADS];
int noOfElements;

/*
1st iteration -> low -> 0,3 ; high->4,7
2nd iteration -> low -> (0,1), (4,5); high -> (2,3),(6,7) 
000 infos[0] => [0..1]
001 infos[1] => [2..10]
010 infos[2] => [11..11]
011 infos[3] => [12..15]
100 infos[4] => [16..19]
101 infos[5] => [20..25]
110 infos[6] => [26..27]
111 infos[7] => [28..31]
*/

void hyperquicksort2();
void quicksort(int start, int end);

int main(int argc, char** argv){

    if(argc <= 2){
        printf("Invalid Input.\n");
        exit(0);
    }
    
    struct timeval time_seq_before, time_seq_after, time_final; 
    gettimeofday(&time_seq_before, NULL);    
    //read input into arrWrite;
    noOfElements = atoi(argv[1]);
    // noOfElements = 16;

    int choice = 0 ;

    
    choice = atoi(argv[2]);
    


    arrRead = (int *)malloc(sizeof(int)*noOfElements);
    arrWrite = (int *)malloc(sizeof(int)*noOfElements);
    int *a3 = (int *)malloc(sizeof(int)*noOfElements);

    omp_set_num_threads(NO_OF_THREADS);


    for(int i = 0; i<noOfElements; i++){
        arrWrite[i] = rand() % 100;
        // if(i < 5) arrWrite[i] = i+1;
        // else arrWrite[i] = 9;
        a3[i] = arrWrite[i];
    }

    
    if(choice){
        printf("\nInitial Array: ");
        for(int i = 0; i<noOfElements; i++){
            printf("%d ", arrWrite[i]);
        }
        printf("\n");
    }


    int chunk = (noOfElements / NO_OF_THREADS);
    int idx = 0;
    for(int i = 0; i<NO_OF_THREADS; i++){
        infos[i].start = idx;
        if(i == (NO_OF_THREADS - 1))
            infos[i].end = noOfElements-1;
        else
            infos[i].end = idx + chunk-1;
        
        if(i != (NO_OF_THREADS - 1))
            chunkSizes[i] = chunk;
        else
            chunkSizes[i] = noOfElements - idx; 
        
        idx += chunk;
    }
    gettimeofday (&time_seq_after, NULL);    
    printf("Sequential portion computation time: %ld microseconds\n", ((time_seq_after.tv_sec - time_seq_before.tv_sec)*1000000L + time_seq_after.tv_usec) - time_seq_before.tv_usec);
    double x = omp_get_wtime();
    quicksort(0, noOfElements-1);
    double y = omp_get_wtime();
    printf("Sequential quicksort timing: %lf seconds\n", (y-x));
    arrWrite = a3;
    double par_start = omp_get_wtime();
    hyperquicksort2();
    double par_end = omp_get_wtime();
    printf("HYPER QUICK SORT ENDED\n");
    

    printf("Time taken by hyperquicksort: %lf\n", par_end - par_start);
    
    gettimeofday (&time_final, NULL);    
    printf("\nTotal time: %ld microseconds\n", ((time_final.tv_sec - time_seq_before.tv_sec)*1000000L + time_final.tv_usec) - time_seq_before.tv_usec);

    if(choice){
        printf("\nFinal Array after sorting: ");
        for(int i = 0; i<noOfElements; i++){
            printf("%d ", arrWrite[i]);
        }

        printf("\n");

    }

    double hyperquicksortTime = par_end - par_start;
    double quicksortTime = y - x;
    double speedup = quicksortTime / hyperquicksortTime;
    printf("\nSpeedup obtained = %lf", speedup);

    return 0;
}

int partition(int start, int end){
    int pivot = arrWrite[end];  
    int i = (start - 1); 
  
    for (int j = start; j <= end - 1; j++) 
    { 
        if (arrWrite[j] < pivot) 
        { 
            i++; 
            int temp = arrWrite[i];
            arrWrite[i] = arrWrite[j];
            arrWrite[j] = temp;
        } 
    } 
    int temp = arrWrite[i+1];
    arrWrite[i+1] = arrWrite[end];
    arrWrite[end] = temp;
    return (i + 1); 
}

void quicksort(int start, int end){
    if(start >= end) return;
    //printf("Depth: %d\n", depth);
    if(start < end){
        int pivot = partition(start, end);
        // #pragma omp barrier
        quicksort(start, pivot-1);
        // #pragma omp barrier
        quicksort(pivot+1, end);
        // #pragma omp barrier
        //#pragma omp barrier
    }
}


void hyperquicksort2(){

    // #pragma omp parallel for num_threads(NO_OF_THREADS)
    // for(i=0; i<NO_OF_THREADS; i++){



        //for each hypercube iteration
        for(int j=0; j<(int)log2(NO_OF_THREADS); j++){
            
            // printf("\n\n********************** At j = %d\n", j);
            #pragma omp parallel shared(infos, arrWrite, arrRead, medians, outInfos, noOfElements, chunkSizes, j) default(none)
            {    
                #pragma omp barrier
                int currId = omp_get_thread_num();
                if(j == 0){
                    //quicksort for its chunk
                    int startIndex = infos[currId].start;
                    int endIndex = infos[currId].end; //(currId == (NO_OF_THREADS - 1)) ? noOfElements - 1 : startIndex + chunkSizes[0] - 1;
                    //printf("currproc: %d start: %d end: %d\n", currId, startIndex, endIndex);
                    quicksort(startIndex, endIndex);

                    #pragma omp barrier
                    // 2 3 5 6 7 9 9 9 9 99 9 9 99  9 9 9 9 9 9 99 9 9
                }
                

                bool sendLow;
                int neighbour;
                int bitPosition = (1 << ((int)(log2(NO_OF_THREADS)) - j - 1));
                if((currId & bitPosition)){
                    //current thread should deal with high list
                    sendLow = true;
                }
                else{
                    sendLow = false;
                }
                neighbour = currId ^ bitPosition;
                /*#pragma omp barrier
                 printf("\ncurrproc: %d, sendLow:%d neigh:%d bitposition:%d iteration: %d", currId, sendLow, neighbour, bitPosition, j);
                #pragma omp barrier*/
                //copy elements of current chunk from arrWrite to arrRead
                for(int k = infos[currId].start; k<=infos[currId].end; k++){
                    arrRead[k] = arrWrite[k];
                }

                #pragma omp barrier

                bool execute = true;
                //if current chunk is responsible for median
                if((currId << j) == 0){
                    //populate median in medians[currId]
                    if(infos[currId].start == -1){
                        if(infos[neighbour].start == -1){
                            execute = false;
                        }
                        else{
                            medians[currId] = arrRead[infos[neighbour].start + (infos[neighbour].end - infos[neighbour].start)/2];
                        }
                    }
                    else
                        medians[currId] = arrRead[infos[currId].start + (infos[currId].end - infos[currId].start)/2];
                }
                #pragma omp barrier

                if(execute){

                    int t2 = INT_MAX << ((int)log2(NO_OF_THREADS) - j);
                    int medianIndex =  t2  & currId;    //find this using bit tricks  will write some logic 
                    // j = 0  1000
                    // j = 1   100
                    // j = 2    10
                    
                    int medianValue = medians[medianIndex];
                    //printf("\nCurr Thread: %d Median Index: %d Iteration: %d Median Value: %d ", currId, medianIndex, j, medianValue);
                    //printf("infos.start: %d  infos.end: %d\n", infos[currId].start, infos[currId].end);
                    //partition and populate OutgoingInfo
                    int *tempArr = (int *)malloc(sizeof(int)*noOfElements);
                    int tempIdx = 0;
                    for(int k = infos[currId].start; k<=infos[currId].end;k++){
                        if(sendLow && (arrRead[k] <= medianValue)){
                            tempArr[tempIdx++] = arrRead[k]; 
                            arrRead[k] = -1;
                        }
                        else if(!sendLow && (arrRead[k] > medianValue)){
                            tempArr[tempIdx++] = arrRead[k]; 
                            arrRead[k] = -1;    
                        } 
                            
                    }
                    outInfos[currId].arr = tempArr;
                    outInfos[currId].arrSize = tempIdx;
                    outInfos[currId].isLow = sendLow;
                    outInfos[currId].to = neighbour;

                /* #pragma omp barrier
                    printf("\n CurrID %d OutInfo :", currId);
                    if (sendLow) printf("lows: ["); else printf("highs: [");
                    for (int i = 0; i < outInfos[currId].arrSize; i++) printf("%d, ", outInfos[currId].arr[i]);
                    printf("]");*/
                    #pragma omp barrier

                    //you can find out using this ki current chunk me kitne jaa rhe aur kitne aa rhe
                    //start .. end (end-start+1 size allotted tha pehle)
                    //so find baad me kitna allotted hoga
                    //
                    //then update chunkSizes[currId]
                    
                /// printf("\nCurrID: %d  Old chunck size %d", currId, chunkSizes[currId]);

                    #pragma omp barrier
                    chunkSizes[currId] -= outInfos[currId].arrSize;
                    chunkSizes[currId] += outInfos[neighbour].arrSize;
                    #pragma omp barrier
                /* #pragma omp barrier
                    printf("\nCurrID: %d  New chunck size %d", currId, chunkSizes[currId]);
                    #pragma omp barrier*/
                    
                    //now using chunkSizes array, you can find out newStart and newEnd for current chunk
                    //write new elements into arrWrite using existing elements and OutgoingInfo.. and update infos array

                    int startIdx = 0;
                    for(int idx = 0; idx < currId; idx++) startIdx += chunkSizes[idx];

                    int idx1 = infos[currId].start, idx2 = 0, idx_write = startIdx;

                    int n1 = infos[currId].end+1;
                    int n2 = outInfos[neighbour].arrSize;

                    if(chunkSizes[currId] == 0){
                        infos[currId].start = -1;
                        infos[currId].end = -2;
                    }
                    else{
                        infos[currId].start = startIdx;
                        infos[currId].end = startIdx + chunkSizes[currId]-1;
                        // printf("Merging %d\n", currId);
                    }
                    #pragma omp barrier
                    // printf("Before while iteration: %d\n",j);
                    while((idx1 < n1) && (idx2 < n2)){
                        if(arrRead[idx1] == -1){
                            idx1++;
                        }
                        else if(arrRead[idx1] >= outInfos[neighbour].arr[idx2]){
                            arrWrite[idx_write++] = outInfos[neighbour].arr[idx2];
                            idx2++;
                        }
                        else{
                            arrWrite[idx_write++] = arrRead[idx1];
                            idx1++;
                        }
                    // printf("Inside while process %d\n", currId);
                    }
                    // printf("After while iteration: %d\n",j);
                    if(idx1 < n1){
                        // printf("Inside IF-1 process %d\n", currId);
                        while(idx1 < n1){
                            if(arrRead[idx1] != -1)
                                arrWrite[idx_write++] = arrRead[idx1];
                            idx1++;    
                            // printf("Inside while IF-1 process %d\n", currId);
                        }
                    }
                    if(idx2 < n2){
                        // printf("Inside IF-2 process %d\n", currId);

                        while(idx2 < n2){
                            arrWrite[idx_write++] = outInfos[neighbour].arr[idx2++]; 
                            // printf("Inside while IF-2 process %d\n", currId);
                        }
                        // printf("Inside IF-2 process %d\n", currId);
                    }
                    // printf("Merge done %d\n", currId);
                    /*#pragma omp barrier
                    if(currId == 0){
                    printf("\narrWrite : [");
                    for (int i = 0; i < noOfElements; i++) printf("%d, ", arrWrite[i]);
                    printf("]");
                    }*/
                }
                #pragma omp barrier 
                // printf("Loop work done: %d\n", currId);
            }
        }
    //}

}

//chunkSizes : [4,4,4,4]
//chunkSizes : [1,6,7,2]