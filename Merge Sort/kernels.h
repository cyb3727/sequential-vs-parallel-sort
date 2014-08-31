#ifndef KERNELS_H
#define KERNELS_H

__global__ void bitonicSortKernel(data_t* data, uint_t dataLen, uint_t sortedBlockSize, bool orderAsc);
__global__ void generateSublocksKernel(data_t* table, uint_t* rankTable,uint_t tableLen,
                                       uint_t tabBlockSize, uint_t tabSubBlockSize);
__global__ void mergeKernel(data_t* inputDataTable, data_t* outputDataTable, uint_t* rankTable, uint_t tableLen,
                            uint_t rankTableLen, uint_t tableBlockSize, uint_t tableSubBlockSize);

#endif
