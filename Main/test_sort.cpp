#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <memory>

#include "../Utils/data_types_common.h"
#include "../Utils/sort_interface.h"
#include "../Utils/host.h"
#include "../Utils/file.h"
#include "../Utils/generator.h"
#include "constants.h"


void stopwatchSorts(Sort sort, data_dist_t distribution, data_t *keys, data_t *values, uint_t arrayLength)
{
    readArrayFromFile(FILE_UNSORTED_ARRAY, keys, arrayLength);
}

void testSorts(
    std::vector<Sort*> sorts, std::vector<data_dist_t> distributions, uint_t arrayLenStart, uint_t arrayLenEnd,
    uint_t testRepetitions, uint_t interval
)
{
    createFolder(FOLDER_SORT_STATS);
    createFolder(FOLDER_SORT_TEMP);

    for (std::vector<data_dist_t>::iterator dist = distributions.begin(); dist != distributions.end(); dist++)
    {
        for (uint_t arrayLength = arrayLenStart; arrayLength <= arrayLenEnd; arrayLength *= 2)
        {
            data_t *keys = (data_t*)malloc(arrayLength * sizeof(*keys));
            checkMallocError(keys);
            data_t *values = (data_t*)malloc(arrayLength * sizeof(*values));
            checkMallocError(values);

            fillArrayKeyOnly(keys, arrayLength, interval, *dist);

            for (uint_t iter = 0; iter < testRepetitions; iter++)
            {
                printf("> Test repetition: %d\n", iter);
                printf("> Distribution: %s\n", getDistributionName(*dist));
                printf("> Array length: %d\n", arrayLength);
                printf("> Data type: %s\n", typeid(data_t).name());

                saveArrayToFile(FILE_UNSORTED_ARRAY, keys, arrayLength);

                for (std::vector<Sort*>::iterator sort = sorts.begin(); sort != sorts.end(); sort++)
                {
                    stopwatchSorts(**sort, *dist, keys, values, arrayLength);
                }

                printf("\n");
            }

            free(keys);
            free(values);
        }

        printf("\n\n");
    }
}


///*
//Prints out table vertical splitter if only keys are sorted.
//*/
//void printTableSplitterKeyOnly()
//{
//    printf("===================================================================================================================\n");
//}
//
///*
//Prints out table header if only keys are sorted.
//*/
//void printTableHeaderKeysOnly()
//{
//    printTableSplitterKeyOnly();
//    printf("||     # ||             PARALLEL              ||             SEQUENTIAL            ||            CORRECT         ||\n");
//    printTableSplitterKeyOnly();
//    printf("||     # ||     time    |      rate    |  ok  ||     time    |      rate    |  ok  ||     time    |      rate    ||\n");
//    printTableSplitterKeyOnly();
//}
//
///*
//Prints out table line with data if only keys are sorted.
//*/
//void printTableLineKeysOnly()
//{
//
//}

///*
//Prints out table vertical splitter if key-value pairs are sorted.
//*/
//void printTableSplitterKeyValue()
//{
//    printf("=====================================================================================================================================\n");
//}
//
///*
//Prints out table header if key-value pairs are sorted.
//*/
//void printTableHeaderKeyValue()
//{
//    printTableSplitterKeyValue();
//    printf("||     # ||                  PARALLEL                  ||                  SEQUENTIAL                ||            CORRECT         ||\n");
//    printTableSplitterKeyValue();
//    printf("||     # ||     time    |      rate    |  ok  | stable ||     time    |      rate    |  ok  | stable ||     time    |      rate    ||\n");
//    printTableSplitterKeyValue();
//}
//
///*
//Prints out table line with data if key-value pairs are sorted.
//*/
//void printTableLineKeyValue(
//    double **timers, uint_t iter, uint_t tableLen, bool areEqualParallel, bool areEqualSequential,
//    bool isStableParallel, bool isStableSequential
//)
//{
//    printf(
//        "|| %5d || %8.2lf ms | %8.2lf M/s | %s  |   %s  || %8.2lf ms | %8.2lf M/s | %s  |   %s  || %8.2lf ms | %8.2lf M/s ||\n",
//        iter + 1,
//        timers[SORT_PARALLEL][iter], tableLen / 1000.0 / timers[SORT_PARALLEL][iter],
//        areEqualParallel ? "YES" : " NO", isStableParallel ? "YES" : " NO",
//        timers[SORT_SEQUENTIAL][iter], tableLen / 1000.0 / timers[SORT_SEQUENTIAL][iter],
//        areEqualSequential ? "YES" : " NO", isStableSequential ? "YES" : " NO",
//        timers[SORT_CORRECT][iter], tableLen / 1000.0 / timers[SORT_CORRECT][iter]
//    );
//}
//
///*
//Prints out statistics of sort if only keys are sorted.
//*/
//void printStatisticsKeysOnly(double *timers, uint_t testRepetitions, uint_t tableLen, bool sortsCorrectly)
//{
//    double timeSum = 0;
//
//    for (uint_t i = 0; i < testRepetitions; i++)
//    {
//        timeSum += timers[i];
//    }
//
//    double avgTime = timeSum / (double)testRepetitions;
//    timeSum = 0;
//
//    for (uint_t i = 0; i < testRepetitions; i++)
//    {
//        timeSum += pow(avgTime - timers[i], 2);
//    }
//
//    double deviationTime = sqrt(timeSum);
//
//    printf("Average sort time:  %8.2lf ms\n", avgTime);
//    printf("Average sort rate:  %8.2lf M/s\n", tableLen / 1000.0 / avgTime);
//    printf("Standard deviation: %8.2lf ms  (%.2lf%%)\n", deviationTime, deviationTime / avgTime * 100);
//    printf("Sorting correctly:  %8s\n", sortsCorrectly ? "YES" : "NO");
//}
//
//void printStatisticsKeyValue(
//    double *timers, uint_t testRepetitions, uint_t tableLen, bool sortsCorrectly, bool isStable
//)
//{
//    printStatisticsKeysOnly(timers, testRepetitions, tableLen, sortsCorrectly);
//    printf("Is sort stable:     %8s\n", isStable ? "YES" : "NO");
//}
//
///*
//Returns a speedup for 2 provided sort types.
//*/
//double getSpeedup(double **timers, sort_type_t sortType1, sort_type_t sortType2, uint_t testRepetitions)
//{
//    double totalTimeSortType1 = 0;
//    double totalTimeSortType2 = 0;
//
//    for (uint_t i = 0; i < testRepetitions; i++)
//    {
//        totalTimeSortType1 += timers[sortType1][i];
//        totalTimeSortType2 += timers[sortType2][i];
//    }
//
//    return totalTimeSortType1 / totalTimeSortType2;
//}
//
///*
//Checks if sorted values are unique.
//Not so trivial in some sorts - for example quicksort.
//*/
//void checkValuesUniqueness(data_t *values, uint_t tableLen)
//{
//    uint_t *uniquenessTable = (uint_t*)malloc(tableLen * sizeof(*uniquenessTable));
//
//    for (uint_t i = 0; i < tableLen; i++)
//    {
//        uniquenessTable[i] = 0;
//    }
//
//    for (uint_t i = 0; i < tableLen; i++)
//    {
//        if (values[i] < 0 || values[i] > tableLen - 1)
//        {
//            printf("Value out of range: %d\n", values[i]);
//            getchar();
//            exit(EXIT_FAILURE);
//        }
//        else if (++uniquenessTable[values[i]] > 1)
//        {
//            printf("Duplicate value: %d\n", values[i]);
//            getchar();
//            exit(EXIT_FAILURE);
//        }
//    }
//
//    free(uniquenessTable);
//}
//
///*
//Determines if array is sorted in stable manner.
//*/
//bool isSortStable(data_t *keys, data_t *values, uint_t tableLen)
//{
//    if (tableLen == 1)
//    {
//        return true;
//    }
//
//    // Generally not needed
//    // checkValuesUniqueness(values, tableLen);
//
//    for (uint_t i = 1; i < tableLen; i++)
//    {
//        // For same keys, values have to be ordered in ascending order.
//        if (keys[i - 1] == keys[i] && values[i - 1] > values[i])
//        {
//            return false;
//        }
//    }
//
//    return true;
//}