#include <stdio.h>
#include <math.h>
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
Initializes device memory.
*/
void memoryDataInit(el_t *h_table, el_t **d_table, uint_t tableLen) {
    cudaError_t error;

    error = cudaMalloc(d_table, tableLen * sizeof(**d_table));
    checkCudaError(error);
    error = cudaMemcpy(*d_table, h_table, tableLen * sizeof(**d_table), cudaMemcpyHostToDevice);
    checkCudaError(error);
}

/*
Sorts sub-blocks of input data with bitonic sort.
*/
void runBitoicSortKernel(el_t *table, uint_t tableLen, uint_t subBlockSize, bool orderAsc) {
    cudaError_t error;
    LARGE_INTEGER timer;

    // Every thread loads and sorts 2 elements
    dim3 dimGrid(tableLen / subBlockSize, 1, 1);
    dim3 dimBlock(subBlockSize / 2, 1, 1);

    startStopwatch(&timer);
    bitonicSortKernel<<<dimGrid, dimBlock, subBlockSize * sizeof(*table)>>>(
        table, orderAsc
    );
    /*error = cudaDeviceSynchronize();
    checkCudaError(error);
    endStopwatch(timer, "Executing bitonic sort kernel");*/
}

/*
Runs multistep kernel, which uses registers.
*/
void runMultiStepRegistersKernel(el_t *table, uint_t tableLen, uint_t phase, uint_t step, uint_t degree,
                                 bool orderAsc) {
    cudaError_t error;
    LARGE_INTEGER timer;

    uint_t partitionSize = tableLen / (1 << degree);
    uint_t maxThreadBlockSize = MAX_THREADS_PER_MULTISTEP;
    uint_t threadBlockSize = min(partitionSize, maxThreadBlockSize);
    dim3 dimGrid(partitionSize / threadBlockSize, 1, 1);
    dim3 dimBlock(threadBlockSize, 1, 1);

    startStopwatch(&timer);
    if (degree == 1) {
        multiStep1Kernel << <dimGrid, dimBlock >> >(table, phase, step, degree, orderAsc);
    } else if (degree == 2) {
        multiStep2Kernel << <dimGrid, dimBlock >> >(table, phase, step, degree, orderAsc);
    } else if (degree == 3) {
        multiStep3Kernel << <dimGrid, dimBlock >> >(table, phase, step, degree, orderAsc);
    } else if (degree == 4) {
        multiStep4Kernel << <dimGrid, dimBlock >> >(table, phase, step, degree, orderAsc);
    }

    /*error = cudaDeviceSynchronize();
    checkCudaError(error);
    endStopwatch(timer, "Executing multistep kernel using registers");*/
}

void runBitoicMergeKernel(el_t *table, uint_t tableLen, uint_t subBlockSize, uint_t phase, bool orderAsc) {
    cudaError_t error;
    LARGE_INTEGER timer;

    // Every thread loads and sorts 2 elements
    dim3 dimGrid(tableLen / subBlockSize, 1, 1);
    dim3 dimBlock(subBlockSize / 2, 1, 1);

    startStopwatch(&timer);
    bitonicMergeKernel<<<dimGrid, dimBlock, subBlockSize * sizeof(*table)>>>(
        table, phase, orderAsc
    );
    /*error = cudaDeviceSynchronize();
    checkCudaError(error);
    endStopwatch(timer, "Executing bitonic sort kernel");*/
}

void sortParallel(el_t *h_input, el_t *h_output, uint_t tableLen, bool orderAsc) {
    el_t *d_table;
    // Every thread loads and sorts 2 elements in first bitonic sort kernel
    uint_t subBlockSize = min(tableLen, 2 * THREADS_PER_MERGE);
    int_t phasesAll = log2((double)tableLen);
    int_t phasesSharedMem = log2((double)subBlockSize);

    LARGE_INTEGER timer;
    cudaError_t error;

    //cudaDeviceSetCacheConfig(cudaFuncCachePreferEqual);
    memoryDataInit(h_input, &d_table, tableLen);

    startStopwatch(&timer);
    runBitoicSortKernel(d_table, tableLen, subBlockSize, orderAsc);

    for (uint_t phase = phasesSharedMem + 1; phase <= phasesAll; phase++) {
        int_t step = phase;

        // Multisteps
        for (uint_t degree = MAX_MULTI_STEP; degree > 0; degree--) {
            for (; step >= phasesSharedMem + degree; step -= degree) {
                runMultiStepRegistersKernel(d_table, tableLen, phase, step, degree, orderAsc);
            }
        }

        runBitoicMergeKernel(d_table, tableLen, subBlockSize, phase, orderAsc);
    }

    error = cudaDeviceSynchronize();
    checkCudaError(error);
    endStopwatch(timer, "Executing parallel bitonic sort.");

    error = cudaMemcpy(h_output, d_table, tableLen * sizeof(*h_output), cudaMemcpyDeviceToHost);
    checkCudaError(error);

    cudaFree(d_table);
}
