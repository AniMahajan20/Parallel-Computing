#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <device_functions.h>
#include <cuda_device_runtime_api.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <chrono>

#define ADDITION 1
#define MINIMUM 2
#define MAXIMUM 3

#define OPERATOR ADDITION	// can also be MINIMUM or MAXIMUM
#define ARR_SIZE 4194304

//Sample input sizes for testing:
/*
2^10 = 1024
2^12 = 4096
2^15 = 32769
2^17 = 131072
2^20 = 1048576
2^22 = 4194304
2^25 = 33554432
2^27 = 134217728
*/

//Blelloch's Parallel scan algorithm for large arrays that use multiple blocks
__global__ void blellochScanLarge(long long int* in, long long int* out, long long int* blockSums, long int arrSize) {
	__shared__ extern long long int myChunk[];
	
	int base = blockDim.x * 2 * blockIdx.x;
	int idx1 = threadIdx.x;
	int idx2 = threadIdx.x + blockDim.x;
	if (base + idx1 < arrSize)
		myChunk[idx1] = in[idx1 + base];
	else {
		if (OPERATOR == ADDITION)
			myChunk[idx1] = 0;
		else if (OPERATOR == MINIMUM)
			myChunk[idx1] = LLONG_MAX;
		else if (OPERATOR == MAXIMUM)
			myChunk[idx1] = LLONG_MIN;
	}
	if (base + idx2 < arrSize)
		myChunk[idx2] = in[idx2 + base];
	else {
		if (OPERATOR == ADDITION)
			myChunk[idx2] = 0;
		else if (OPERATOR == MINIMUM)
			myChunk[idx2] = LLONG_MAX;
		else if (OPERATOR == MAXIMUM)
			myChunk[idx2] = LLONG_MIN;
	}

	__syncthreads();

	//up-sweep
	for (int depth = 1; depth <= blockDim.x; depth <<= 1) {
		int dest = (threadIdx.x + 1) * 2 * depth - 1;
		if (dest < 2 * blockDim.x) {
			if (OPERATOR == ADDITION)
				myChunk[dest] += myChunk[dest - depth];
			else if (OPERATOR == MINIMUM)
				myChunk[dest] = myChunk[dest] > myChunk[dest - depth] ? myChunk[dest - depth] : myChunk[dest];
			else if (OPERATOR == MAXIMUM)
				myChunk[dest] = myChunk[dest] < myChunk[dest - depth] ? myChunk[dest - depth] : myChunk[dest];
		}
		__syncthreads();
	}

	//down-sweep
	for (int depth = blockDim.x >> 1; depth > 0; depth >>= 1) {
		int dest = (threadIdx.x + 1) * depth * 2 - 1;
		if (dest + depth < 2 * blockDim.x) {
			if (OPERATOR == ADDITION)
				myChunk[dest + depth] += myChunk[dest];
			else if(OPERATOR == MINIMUM)
				myChunk[dest + depth] = myChunk[dest + depth] > myChunk[dest] ? myChunk[dest] : myChunk[dest+depth];
			else if (OPERATOR == MAXIMUM)
				myChunk[dest + depth] = myChunk[dest + depth] < myChunk[dest] ? myChunk[dest] : myChunk[dest + depth];
		}
		__syncthreads();
	}

	if (base + idx1 < arrSize)
		out[idx1 + base] = myChunk[idx1];
	if (base + idx2 < arrSize)
		out[idx2 + base] = myChunk[idx2];

	__syncthreads();
	if (threadIdx.x == 0) {
		blockSums[blockIdx.x] = myChunk[2 * blockDim.x - 1];
	}
}

//Adds the cumulative block sums to all the elements in output array
__global__ void addBlockSums(long long int* out, long long int* blockSums, long int arrSize) {
	int threadId = 2 * blockDim.x * blockIdx.x + 2 * threadIdx.x;
	if (threadId + 1 < arrSize && blockIdx.x > 0) {
		if (OPERATOR == ADDITION)
			out[threadId + 1] += blockSums[blockIdx.x - 1];
		else if (OPERATOR == MINIMUM)
			out[threadId + 1] = out[threadId + 1] > blockSums[blockIdx.x - 1] ? blockSums[blockIdx.x - 1] : out[threadId + 1];
		else if (OPERATOR == MAXIMUM)
			out[threadId + 1] = out[threadId + 1] < blockSums[blockIdx.x - 1] ? blockSums[blockIdx.x - 1] : out[threadId + 1];
	
		if (OPERATOR == ADDITION)
			out[threadId] += blockSums[blockIdx.x - 1];
		else if (OPERATOR == MINIMUM)
			out[threadId] = out[threadId] > blockSums[blockIdx.x - 1] ? blockSums[blockIdx.x - 1] : out[threadId];
		else if (OPERATOR == MAXIMUM)
			out[threadId] = out[threadId] < blockSums[blockIdx.x - 1] ? blockSums[blockIdx.x - 1] : out[threadId];
	}
	else if (threadId < arrSize && blockIdx.x > 0) {
		if (OPERATOR == ADDITION)
			out[threadId] += blockSums[blockIdx.x - 1];
		else if (OPERATOR == MINIMUM)
			out[threadId] = out[threadId] > blockSums[blockIdx.x - 1] ? blockSums[blockIdx.x - 1] : out[threadId];
		else if (OPERATOR == MAXIMUM)
			out[threadId] = out[threadId] < blockSums[blockIdx.x - 1] ? blockSums[blockIdx.x - 1] : out[threadId];
	}
}

//Computes scan sequentially
void computeLinearScan(long long int* out, long long int* in, long int n) {
	out[0] = in[0];
	for (int i = 1; i < n; i++) {
		if (OPERATOR == ADDITION)
			out[i] = out[i - 1] + in[i];
		else if (OPERATOR == MINIMUM)
			out[i] = out[i - 1] > in[i] ? in[i] : out[i - 1];
		else if (OPERATOR == MAXIMUM)
			out[i] = out[i - 1] < in[i] ? in[i] : out[i - 1];
	}
}

//Verifies whether Parallel scan output is correct by comparing it with linear scan output
void verifyOutput(long long int* out1, long long int* out2, long int n) {
	int mismatches = 0;
	for (int i = 0; i < n; i++) {
		if (out1[i] != out2[i]) {
			if(mismatches == 0)
				printf("\nParallel scan first failed at i=%d. Serial output, Parallel Output : %lld, %lld", i, out1[i], out2[i]);
			mismatches++;
		}
	}
	if (mismatches == 0)
		printf("\nParallel scan output has been verified to be correct.\n");
	else
		printf("\nTotal of %d mismatches found between serial output and parallel output.\n", mismatches);
}

int main(int argc, char** argv) {

	srand(time(NULL));
	int deviceCount = 0;
	cudaGetDeviceCount(&deviceCount);
	cudaDeviceProp pr;
	if (deviceCount == 0) {
		printf("No CUDA compatible GPU exists");
		return 0;
	}
	else {
		cudaGetDeviceProperties(&pr, 0);
	}

	/*clock_t linearStart, parallelStart, linearEnd, parallelEnd, serialStart, serialEnd;*/
	//double linearTime, parallelTime, serialTime, masterTime;
	double parallelTime = 0.0;
	float elapsedTime1, elapsedTime2;
	elapsedTime1 = 0.0;
	elapsedTime2 = 0.0;

	/*
	linearTime = 0.0;
	parallelTime = 0.0;
	serialTime = 0.0;*/
	cudaEvent_t start1, stop1, start2, stop2;
	cudaEventCreate(&start1);
	cudaEventCreate(&stop1);
	cudaEventCreate(&start2);
	cudaEventCreate(&stop2);

	int threadsPerBlock = pr.maxThreadsPerBlock;
	int arrElementsPerBlock = 2 * threadsPerBlock;
	int blockCount = (int)ceil(((double)ARR_SIZE) / arrElementsPerBlock);

	//serialStart = clock();

	//host copies
	long long int* inArr, * outArr, * blockSums;
	//long long int* linearScanOutput;
	long long int allocSize1 = sizeof(long long) * (long)ARR_SIZE;
	long long int allocSize2 = sizeof(long long) * blockCount;
	long long int allocSize3 = sizeof(long long) * (2 * threadsPerBlock);

	//device copies
	long long int* devInArr, * devOutArr, * devBlockSums;

	//linearStart = clock();

	//linearScanOutput = (long long int*)malloc(allocSize1);
	inArr = (long long int*)malloc(allocSize1);
	outArr = (long long int*)malloc(allocSize1);
	blockSums = (long long int*)malloc(allocSize2);
	for (int i = 0; i < ARR_SIZE; i++) {
		//if (inArr != NULL)
			//inArr[i] = (rand() % 53) + (long long)17;
			inArr[i] = 1;
	}
	

	/*computeLinearScan(linearScanOutput, inArr, ARR_SIZE);
	serialEnd = clock();
	serialTime += (double)(serialEnd - serialStart);
	linearEnd = clock();*/
	//printf("\n%d, %d", linearStart, linearEnd);

	/*linearTime = (((double)(linearEnd - linearStart)) / (double)CLOCKS_PER_SEC) * 1000.00;

	parallelStart = clock();
	serialStart = clock();*/

	cudaEventRecord(start1);
	cudaMalloc((void**)&devInArr, allocSize1);
	cudaMalloc((void**)&devOutArr, allocSize1);
	cudaMalloc((void**)&devBlockSums, allocSize2);
	cudaMemcpy(devInArr, inArr, allocSize1, cudaMemcpyHostToDevice);
	/*serialEnd = clock();

	parallelEnd = clock();

	serialTime += (double)(serialEnd - serialStart);
	parallelTime += (((double)(parallelEnd - parallelStart)) / (double)CLOCKS_PER_SEC) * 1000.00;*/


	blellochScanLarge <<<blockCount, threadsPerBlock, allocSize3>>> (devInArr, devOutArr, devBlockSums, (long)ARR_SIZE);
	cudaEventRecord(stop1);

	cudaDeviceSynchronize();
	cudaMemcpy(blockSums, devBlockSums, allocSize2, cudaMemcpyDeviceToHost);
	cudaEventSynchronize(stop1);
	cudaEventElapsedTime(&elapsedTime1, start1, stop1);

	/*serialStart = clock();
	parallelStart = clock();*/


	
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 1; i < blockCount; i++) {
		if (OPERATOR == ADDITION)
			blockSums[i] += blockSums[i - 1];
		else if (OPERATOR == MINIMUM)
			blockSums[i] = blockSums[i] > blockSums[i - 1] ? blockSums[i - 1] : blockSums[i];
		else if (OPERATOR == MAXIMUM)
			blockSums[i] = blockSums[i] < blockSums[i - 1] ? blockSums[i - 1] : blockSums[i];
		//printf("\nblocksums[%d] = %lld", i, blockSums[i]);
	}

	auto elapsed = std::chrono::high_resolution_clock::now() - start;

	parallelTime += ((double)(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()))/1000.00;

	cudaEventRecord(start2);
	cudaMemcpy(devBlockSums, blockSums, allocSize2, cudaMemcpyHostToDevice);
	/*serialEnd = clock();
	parallelEnd = clock();
	parallelTime += (((double)(parallelEnd - parallelStart)) / (double)CLOCKS_PER_SEC) * 1000.00;
	serialTime += (double)(serialEnd - serialStart);*/

	addBlockSums <<<blockCount, threadsPerBlock>>> (devOutArr, devBlockSums, (long)ARR_SIZE);
	cudaEventRecord(stop2);

	cudaMemcpy(outArr, devOutArr, allocSize1, cudaMemcpyDeviceToHost);
	cudaEventSynchronize(stop2);
	cudaEventElapsedTime(&elapsedTime2, start2, stop2);

	//parallelTime += (double)elapsedTime1 + elapsedTime2;

	if (argc > 1 && (argv[1][0] == 'v' || argv[1][0] == 'V')) {
		int minm = (ARR_SIZE <= 100 ? ARR_SIZE : 100);
		printf("\nFirst %d elements of input and output:-", minm);
		printf("\nInput: [");
		for (int i = 0; i < minm; i++)
			printf("%lld, ", inArr[i]);
		printf("]");

		printf("\nOutput: [");
		for (int i = 0; i < minm; i++)
			printf("%lld, ", outArr[i]);
		printf("]");
	}

	//verifyOutput(linearScanOutput, outArr, ARR_SIZE);

	/*printf("\nScan Operation performed: ");
	if (OPERATOR == ADDITION) printf("Addition");
	else if (OPERATOR == MINIMUM) printf("Minimum");
	else if (OPERATOR == MAXIMUM) printf("Maximum");
	printf("\nInput array size = %lld", ARR_SIZE);
	printf("\nNo of threads per block = %d, no. of blocks = %d\n", threadsPerBlock, blockCount);
	printf("\nTime taken by CPU(linear) scan = %Lf ms", linearTime);
	printf("\nTime taken by GPU(parallel) scan = %Lf ms\n", parallelTime);*/


	free(inArr);
	free(outArr);
	free(blockSums);
	cudaFree(devInArr);
	cudaFree(devOutArr);
	cudaFree(devBlockSums);

	parallelTime += elapsedTime1 + elapsedTime2;

	//printf("\nSerialTime = %Lf ms", (serialTime / (double)CLOCKS_PER_SEC)*1000.00);
	printf("\nOverall parallel program time = %Lf ms", parallelTime);
	printf("\nNo. of blocks = %d", blockCount);
	//printf("\nSerial fraction = %Lf", (serialTime / masterTime));
	return 0;
}