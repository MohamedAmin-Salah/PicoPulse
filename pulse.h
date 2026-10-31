
#ifndef PULSE_H
#define PULSE_H

#include "measurement.h"

typedef enum {
    PULSE_WAITING,
    PULSE_NO_FINGER,
    PULSE_BAD_RATE,
    PULSE_MOTION,
    PULSE_WEAK_SIGNAL,
    PULSE_UNSTABLE,
    PULSE_VALID
} PulseStatus;

typedef struct {
    PulseStatus status;
    float bpm;
    unsigned int peaks;
    float sample_rate_hz;
} PulseResult;

PulseResult pulse_analyze(const MeasurementBuffer *buffer,
                          float sample_rate_hz);

const char *pulse_status_text(PulseStatus status);

#endif
