
#include "spo2.h"
#include <math.h>

#define BASELINE_RADIUS 25
#define MIN_DC 10000.0f
#define MIN_RELATIVE_AC 0.0003f
#define MAX_RELATIVE_STEP 0.04f
#define MAX_RELATIVE_RANGE 0.12f
#define MAX_RELATIVE_DRIFT 0.08f

Spo2Result spo2_analyze(const MeasurementBuffer *buffer)
{
    Spo2Result result = {SPO2_WAITING, 0.0f, 0.0f, 0.0f};

    if (buffer->count < MEASUREMENT_CAPACITY) {
        return result;
    }

    float red[MEASUREMENT_CAPACITY];
    float ir[MEASUREMENT_CAPACITY];

    float red_sum = 0.0f, ir_sum = 0.0f;
    float red_min = 262143.0f, ir_min = 262143.0f;
    float red_max = 0.0f, ir_max = 0.0f;
    float red_step = 0.0f, ir_step = 0.0f;
    float red_first = 0.0f, red_last = 0.0f;
    float ir_first = 0.0f, ir_last = 0.0f;

    // Las RED och IR i kronologisk ordning.
    for (size_t i = 0; i < MEASUREMENT_CAPACITY; ++i) {
        size_t pos =
            (buffer->next + i) % MEASUREMENT_CAPACITY;

        red[i] = (float)buffer->samples[pos].red;
        ir[i] = (float)buffer->samples[pos].ir;

        red_sum += red[i];
        ir_sum += ir[i];

        if (red[i] < red_min) red_min = red[i];
        if (red[i] > red_max) red_max = red[i];
        if (ir[i] < ir_min) ir_min = ir[i];
        if (ir[i] > ir_max) ir_max = ir[i];

        if (i < 25) {
            red_first += red[i];
            ir_first += ir[i];
        }

        if (i >= MEASUREMENT_CAPACITY - 25) {
            red_last += red[i];
            ir_last += ir[i];
        }

        if (i > 0) {
            float dr = fabsf(red[i] - red[i - 1]);
            float di = fabsf(ir[i] - ir[i - 1]);

            if (dr > red_step) red_step = dr;
            if (di > ir_step) ir_step = di;
        }
    }

    // DC = medelnivan for respektive ljuskanal.
    float red_dc = red_sum / (float)MEASUREMENT_CAPACITY;
    float ir_dc = ir_sum / (float)MEASUREMENT_CAPACITY;

    if (red_dc < MIN_DC || ir_dc < MIN_DC ||
        red_min < MIN_DC || ir_min < MIN_DC) {

        result.status = SPO2_NO_FINGER;
        return result;
    }

    // Kontrollera plötsliga forandringar och drift.
    float red_drift =
        fabsf((red_last - red_first) / 25.0f);

    float ir_drift =
        fabsf((ir_last - ir_first) / 25.0f);

    if ((red_max - red_min) > red_dc * MAX_RELATIVE_RANGE ||
        (ir_max - ir_min) > ir_dc * MAX_RELATIVE_RANGE ||
        red_step > red_dc * MAX_RELATIVE_STEP ||
        ir_step > ir_dc * MAX_RELATIVE_STEP ||
        red_drift > red_dc * MAX_RELATIVE_DRIFT ||
        ir_drift > ir_dc * MAX_RELATIVE_DRIFT) {

        result.status = SPO2_MOTION;
        return result;
    }

    // Berakna pulserande signal efter att baslinjen
    // tagits bort med ett glidande medelvarde.
    float red_energy = 0.0f;
    float ir_energy = 0.0f;
    size_t samples = 0;

    for (size_t i = BASELINE_RADIUS;
         i < MEASUREMENT_CAPACITY - BASELINE_RADIUS;
         ++i) {

        float red_baseline = 0.0f;
        float ir_baseline = 0.0f;

        for (int j = -BASELINE_RADIUS;
             j <= BASELINE_RADIUS; ++j) {

            red_baseline += red[(int)i + j];
            ir_baseline += ir[(int)i + j];
        }

        red_baseline /=
            (float)(2 * BASELINE_RADIUS + 1);

        ir_baseline /=
            (float)(2 * BASELINE_RADIUS + 1);

        float red_ac = red[i] - red_baseline;
        float ir_ac = ir[i] - ir_baseline;

        red_energy += red_ac * red_ac;
        ir_energy += ir_ac * ir_ac;

        ++samples;
    }

    // RMS-amplitud dividerad med DC-nivan.
    result.red_ac_dc =
        sqrtf(red_energy / (float)samples) / red_dc;

    result.ir_ac_dc =
        sqrtf(ir_energy / (float)samples) / ir_dc;

    if (result.red_ac_dc < MIN_RELATIVE_AC ||
        result.ir_ac_dc < MIN_RELATIVE_AC) {

        result.status = SPO2_WEAK_SIGNAL;
        return result;
    }

    // Ratio-of-ratios.
    result.ratio =
        result.red_ac_dc / result.ir_ac_dc;

    if (!isfinite(result.ratio) ||
        result.ratio < 0.2f ||
        result.ratio > 2.0f) {

        result.status = SPO2_BAD_RATIO;
        return result;
    }

    result.status = SPO2_RATIO_VALID;
    return result;
}

const char *spo2_status_text(Spo2Status status)
{
    switch (status) {

        case SPO2_WAITING:
            return "Samlar data";

        case SPO2_NO_FINGER:
            return "Ingen fingerkontakt";

        case SPO2_MOTION:
            return "Rorelse eller tryckforandring";

        case SPO2_WEAK_SIGNAL:
            return "Svag RED/IR-pulssignal";

        case SPO2_BAD_RATIO:
            return "RED/IR-kvot utanfor rimligt intervall";

        case SPO2_RATIO_VALID:
            return "RED/IR-kvot beraknad";

        default:
            return "Okant fel";
    }
}
