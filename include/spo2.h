
#ifndef SPO2_H
#define SPO2_H

#include "measurement.h"

typedef enum {
    SPO2_WAITING,
    SPO2_NO_FINGER,
    SPO2_MOTION,
    SPO2_WEAK_SIGNAL,
    SPO2_BAD_RATIO,
    SPO2_RATIO_VALID
} Spo2Status;

typedef struct {
    Spo2Status status;
    float ratio;
    float red_ac_dc;
    float ir_ac_dc;
} Spo2Result;

Spo2Result spo2_analyze(const MeasurementBuffer *buffer);
const char *spo2_status_text(Spo2Status status);

#endif
