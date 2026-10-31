
#include "measurement.h"

void measurement_clear(MeasurementBuffer *buffer)
{
    buffer->next = 0;
    buffer->count = 0;
}

void measurement_push(MeasurementBuffer *buffer, Max30102Sample sample)
{
    buffer->samples[buffer->next] = sample;
    buffer->next = (buffer->next + 1) % MEASUREMENT_CAPACITY;
    if (buffer->count < MEASUREMENT_CAPACITY) {
        buffer->count++;
    }
}

bool measurement_stats(const MeasurementBuffer *buffer, MeasurementStats *stats)
{
    if (buffer->count == 0) {
        return false;
    }

    uint64_t red_sum = 0;
    uint64_t ir_sum = 0;
    uint32_t red_min = UINT32_MAX, ir_min = UINT32_MAX;
    uint32_t red_max = 0, ir_max = 0;

    for (size_t i = 0; i < buffer->count; i++) {
        Max30102Sample sample = buffer->samples[i];
        red_sum += sample.red;
        ir_sum += sample.ir;
        if (sample.red < red_min) red_min = sample.red;
        if (sample.red > red_max) red_max = sample.red;
        if (sample.ir < ir_min) ir_min = sample.ir;
        if (sample.ir > ir_max) ir_max = sample.ir;
    }

    stats->red_mean = (uint32_t)(red_sum / buffer->count);
    stats->ir_mean = (uint32_t)(ir_sum / buffer->count);
    stats->red_range = red_max - red_min;
    stats->ir_range = ir_max - ir_min;
    return true;
}
