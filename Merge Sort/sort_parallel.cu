#include <stdio.h>
#include <Windows.h>

#include <cuda.h>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "data_types.h"
#include "constants.h"
#include "utils_cuda.h"
#include "utils_host.h"
#include "kernels.h"


/*
Initializes memory needed for parallel implementation of merge sort.
*/
void memoryInit(el_t *h_input, el_t **d_input, el_t **d_output, el_t **d_buffer, uint_t tableLen) {
    cudaError_t error;

    error = cudaMalloc(d_input, tableLen * sizeof(**d_input));
    checkCudaError(error);
    error = cudaMalloc(d_output, tableLen * sizeof(**d_output));
    checkCudaError(error);
    error = cudaMalloc(d_buffer, tableLen * sizeof(**d_buffer));
    checkCudaError(error);

    error = cudaMemcpy(*d_input, h_input, tableLen * sizeof(**d_input), cudaMemcpyHostToDevice);
    checkCudaError(error);
}

/*
Sorts data blocks of size sortedBlockSize with bitonic sort.
*/
void runBitonicSortKernel(el_t *input, el_t *output, uint_t tableLen, bool orderAsc) {
    cudaError_t error;
    LARGE_INTEGER timer;

    uint_t sharedMemSize = min(tableLen, MAX_SHARED_MEM_SIZE);
    dim3 dimGrid((tableLen - 1) / sharedMemSize + 1, 1, 1);
    dim3 dimBlock(sharedMemSize / 2, 1, 1);

    startStopwatch(&timer);
    bitonicSortKernel<<<dimGrid, dimBlock, sharedMemSize * sizeof(*input)>>>(
        input, output, orderAsc
    );
    error = cudaDeviceSynchronize();
    checkCudaError(error);
    endStopwatch(timer, "Executing Bitonic sort Kernel");
}

/*
Generates ranks of sub-blocks that need to be merged.
*/
void runGenerateRanksKernel(data_t* data, uint_t* ranks, uint_t dataLen, uint_t sortedBlockSize,
                            uint_t subBlockSize) {
    cudaError_t error;
    LARGE_INTEGER timer;

    uint_t ranksPerSharedMem = MAX_SHARED_MEM_SIZE / sizeof(sample_el_t);
    uint_t numAllRanks = dataLen / subBlockSize;
    uint_t threadBlockSize = min(ranksPerSharedMem, numAllRanks);

    dim3 dimGrid((numAllRanks - 1) / threadBlockSize + 1, 1, 1);
    dim3 dimBlock(threadBlockSize, 1, 1);

    startStopwatch(&timer);
    generateRanksKernel<<<dimGrid, dimBlock, threadBlockSize * sizeof(sample_el_t)>>>(
        data, ranks, dataLen, sortedBlockSize, subBlockSize
    );
    error = cudaDeviceSynchronize();
    checkCudaError(error);
    //endStopwatch(timer, "Executing Generate ranks kernel");
}

/*
Executes merge kernel, which merges all consecutive sorted blocks in data.
*/
void runMergeKernel(data_t* inputData, data_t* outputData, uint_t* ranks, uint_t dataLen,
                    uint_t ranksLen, uint_t sortedBlockSize, uint_t tabSubBlockSize) {
    cudaError_t error;
    LARGE_INTEGER timer;

    uint_t subBlocksPerMergedBlock = sortedBlockSize / tabSubBlockSize * 2;
    uint_t numMergedBlocks = dataLen / (sortedBlockSize * 2);
    uint_t sharedMemSize = tabSubBlockSize * sizeof(*inputData) * 2;
    dim3 dimGrid(subBlocksPerMergedBlock + 1, numMergedBlocks, 1);
    dim3 dimBlock(tabSubBlockSize, 1, 1);

    startStopwatch(&timer);
    mergeKernel<<<dimGrid, dimBlock, sharedMemSize>>>(
        inputData, outputData, ranks, dataLen, ranksLen, sortedBlockSize, tabSubBlockSize
    );
    error = cudaDeviceSynchronize();
    checkCudaError(error);
    //endStopwatch(timer, "Executing merge kernel");
}

void sortParallel(el_t *h_input, el_t *h_output, uint_t tableLen, bool orderAsc) {
    el_t *d_input, *d_output, *d_buffer;
    cudaError_t error;

    memoryInit(h_input, &d_input, &d_output, &d_buffer, tableLen);
    runBitonicSortKernel(d_input, d_output, tableLen, orderAsc);

    //// TODO verify, if ALL (also up) device syncs are necessary
    //for (; sortedBlockSize < dataLen; sortedBlockSize *= 2) {
    //    runGenerateRanksKernel(inputDataDevice, ranksDevice, dataLen, sortedBlockSize, subBlockSize);

    //    runMergeKernel(inputDataDevice, outputDataDevice, ranksDevice, dataLen, ranksLen,
    //                   sortedBlockSize, subBlockSize);

    //    data_t* temp = inputDataDevice;
    //    inputDataDevice = outputDataDevice;
    //    outputDataDevice = temp;
    //}

    error = cudaMemcpy(h_output, d_output, tableLen * sizeof(*h_output), cudaMemcpyDeviceToHost);
    checkCudaError(error);

    //return outputDataHost;
}
