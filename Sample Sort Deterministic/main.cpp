#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <array>

#include <cuda.h>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "../Utils/data_types_common.h"
#include "../Utils/host.h"
#include "../Utils/cuda.h"
#include "../Utils/generator.h"
#include "../Utils/sort_correct.h"
#include "../Utils/statistics.h"
#include "constants.h"
#include "memory.h"
#include "sort_parallel.h"
#include "sort_sequential.h"


int main(int argc, char **argv)
{
    data_t *h_input;
    data_t *h_outputParallel, *h_outputCorrect;
    // For sequential sample sort 3 arrays are needed. Data is exchanged between input and buffer array.
    // At the end result is coppied to output array.
    data_t *h_inputSequential, *h_bufferSequential, *h_outputSequential;
    // Holds samples and after samples are sorted holds splitters in sequential sample sort
    data_t *h_samples;
    // For every element in input holds bucket index to which it belogns (needed for sequential sample sort)
    uint_t *h_elementBuckets;
    data_t *d_dataTable, *d_dataBuffer;
    // LOCAL samples:  NUM_SAMPLES samples for each data block sorted by initial bitonic sort
    // GLOBAL samples: NUM_SAMPLES samples collected from sorted LOCAL samples
    data_t *d_samplesLocal, *d_samplesGlobal;
    // Sizes and offsets of local (per every tile - thread block) buckets (gained after scan on bucket sizes)
    uint_t *d_localBucketSizes, *d_localBucketOffsets;
    // Offsets of entire (whole, global) buckets, not just parts of buckets for every tile (local)
    uint_t *h_globalBucketOffsets, *d_globalBucketOffsets;
    double **timers;

    uint_t tableLen = (1 << 20);
    uint_t interval = (1 << 31);
    uint_t testRepetitions = 10;    // How many times are sorts ran
    order_t sortOrder = ORDER_ASC;  // Values: ORDER_ASC, ORDER_DESC
    data_dist_t distribution = DISTRIBUTION_UNIFORM;
    bool printMeaurements = true;

    // Determines whether paralle/sequential sort has always sorted data correctly. NOT CONFIGURABLE!
    bool parallelSortsCorrectly = true, sequentialSortsCorrectly = true;
    cudaError_t error;

    // Memory alloc
    allocHostMemory(
        &h_input, &h_outputParallel, &h_inputSequential, &h_bufferSequential, &h_outputSequential,
        &h_outputCorrect, &h_samples, &h_elementBuckets, &h_globalBucketOffsets, &timers, tableLen,
        testRepetitions
    );
    allocDeviceMemory(
        &d_dataTable, &d_dataBuffer, &d_samplesLocal, &d_samplesGlobal, &d_localBucketSizes,
        &d_localBucketOffsets, &d_globalBucketOffsets, tableLen
    );

    printf(">>> SAMPLE SORT <<<\n\n\n");
    printDataDistribution(distribution);
    printf("> Array length: %d\n", tableLen);
    if (printMeaurements)
    {
        printf("\n");
        printTableHeaderKeysOnly();
    }

    for (uint_t i = 0; i < testRepetitions; i++)
    {
        fillTableKeysOnly(h_input, tableLen, interval, distribution);

        // Sort parallel
        error = cudaMemcpy(d_dataTable, h_input, tableLen * sizeof(*d_dataTable), cudaMemcpyHostToDevice);
        checkCudaError(error);
        error = cudaDeviceSynchronize();
        checkCudaError(error);
        timers[SORT_PARALLEL][i] = sortParallel(
            h_outputParallel, d_dataTable, d_dataBuffer, d_samplesLocal, d_samplesGlobal, d_localBucketSizes,
            d_localBucketOffsets, h_globalBucketOffsets, d_globalBucketOffsets, tableLen, sortOrder
        );

        // Sort sequential
        std::copy(h_input, h_input + tableLen, h_inputSequential);
        timers[SORT_SEQUENTIAL][i] = sortSequential(
            h_inputSequential, h_bufferSequential, h_outputSequential, h_samples, h_elementBuckets,
            tableLen, sortOrder
        );

        // Sort correct
        std::copy(h_input, h_input + tableLen, h_outputCorrect);
        timers[SORT_CORRECT][i] = sortCorrect(h_outputCorrect, tableLen, sortOrder);

        bool areEqualParallel = compareArrays(h_outputParallel, h_outputCorrect, tableLen);
        bool areEqualSequential = compareArrays(h_outputSequential, h_outputCorrect, tableLen);

        parallelSortsCorrectly &= areEqualParallel;
        sequentialSortsCorrectly &= areEqualSequential;

        if (printMeaurements)
        {
            printTableLineKeysOnly(timers, i, tableLen, areEqualParallel, areEqualSequential);
        }
    }

    if (printMeaurements)
    {
        printTableSplitterKeysOnly();
    }

    // Print-out of statistics for PARALLEL sort
    printf("\n- PARALLEL SORT\n");
    printStatisticsKeysOnly(timers[SORT_PARALLEL], testRepetitions, tableLen, parallelSortsCorrectly);

    // Print-out of statistics for SEQUENTIAL sort
    printf("\n- SEQUENTIAL SORT\n");
    printStatisticsKeysOnly(timers[SORT_SEQUENTIAL], testRepetitions, tableLen, sequentialSortsCorrectly);

    printf(
        "\n- Speedup (SEQUENTIAL/PARALLEL): %.2lf\n",
        getSpeedup(timers, SORT_SEQUENTIAL, SORT_PARALLEL, testRepetitions)
    );
    printf(
        "- Speedup (CORRECT/PARALLEL):    %.2lf\n",
        getSpeedup(timers, SORT_CORRECT, SORT_PARALLEL, testRepetitions)
    );

    // Memory free
    freeHostMemory(
        h_input, h_outputParallel, h_inputSequential, h_bufferSequential, h_outputSequential, h_outputCorrect,
        h_samples, h_elementBuckets, h_globalBucketOffsets, timers
    );
    freeDeviceMemory(
        d_dataTable, d_dataBuffer, d_samplesLocal, d_samplesGlobal, d_localBucketSizes, d_localBucketOffsets,
        d_globalBucketOffsets
    );

    getchar();
    return 0;
}
