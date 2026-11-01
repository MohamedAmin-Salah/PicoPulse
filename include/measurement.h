
#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "max30102.h"

#define MEASUREMENT_CAPACITY 500

typedef struct {
    Max30102Sample samples[MEASUREMENT_CAPACITY];
    size_t next;
    size_t count;
} MeasurementBuffer;

typedef struct {
    uint32_t red_mean;
    uint32_t ir_mean;
    uint32_t red_range;
    uint32_t ir_range;
} MeasurementStats;

void measurement_clear(MeasurementBuffer *buffer);
void measurement_push(MeasurementBuffer *buffer, Max30102Sample sample);
bool measurement_stats(const MeasurementBuffer *buffer, MeasurementStats *stats);

#endif
