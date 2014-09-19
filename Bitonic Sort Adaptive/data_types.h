#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdint.h>

typedef int data_t;
typedef uint32_t uint_t;
typedef int32_t int_t;

// Key value pair used for sorting
struct Element {
    uint_t key;
    uint_t val;
};
struct Interval {
    uint32_t offset;
    uint32_t len;
};
typedef struct Element el_t;
typedef struct Interval interval_t;

#endif
