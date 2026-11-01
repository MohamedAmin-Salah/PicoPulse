
#include "pulse.h"

#define SMOOTH_RADIUS       2
#define BASELINE_RADIUS    25

#define MIN_IR_LEVEL    10000.0f
#define MIN_PULSE_HEIGHT  40.0f

#define MIN_BPM           45.0f
#define MAX_BPM          170.0f

#define MAX_RATE_ERROR     0.10f
#define MAX_IR_RANGE       0.12f
#define MAX_IR_STEP        0.04f
#define MAX_IR_DRIFT       0.08f

PulseResult pulse_analyze(const MeasurementBuffer *buffer,
                          float sample_rate_hz)
{
    PulseResult result = {
        PULSE_WAITING, 0.0f, 0, sample_rate_hz
    };

    if (buffer->count < MEASUREMENT_CAPACITY) {
        return result;
    }

    // Kontrollera att mottagningshastigheten ar rimlig.
    if (sample_rate_hz < 100.0f * (1.0f - MAX_RATE_ERROR) ||
        sample_rate_hz > 100.0f * (1.0f + MAX_RATE_ERROR)) {

        result.status = PULSE_BAD_RATE;
        return result;
    }

    float ir[MEASUREMENT_CAPACITY];
    float smooth[MEASUREMENT_CAPACITY];
    float signal[MEASUREMENT_CAPACITY];

    float total = 0.0f;
    float minimum = 262143.0f;
    float maximum = 0.0f;
    float max_step = 0.0f;
    float begin_sum = 0.0f;
    float end_sum = 0.0f;

    // Las cirkelbufferten i kronologisk ordning.
    for (size_t i = 0; i < MEASUREMENT_CAPACITY; i++) {

        size_t index =
            (buffer->next + i) % MEASUREMENT_CAPACITY;

        ir[i] = (float)buffer->samples[index].ir;
        total += ir[i];

        if (ir[i] < minimum) minimum = ir[i];
        if (ir[i] > maximum) maximum = ir[i];

        if (i < 25) {
            begin_sum += ir[i];
        }

        if (i >= MEASUREMENT_CAPACITY - 25) {
            end_sum += ir[i];
        }

        if (i > 0) {
            float delta = ir[i] - ir[i - 1];

            if (delta < 0.0f) {
                delta = -delta;
            }

            if (delta > max_step) {
                max_step = delta;
            }
        }
    }

    float mean = total / (float)MEASUREMENT_CAPACITY;

    // Kontrollera fingerkontakt.
    if (mean < MIN_IR_LEVEL || minimum < MIN_IR_LEVEL) {
        result.status = PULSE_NO_FINGER;
        return result;
    }

    // Jamfor signalens borjan och slut.
    float drift = end_sum / 25.0f - begin_sum / 25.0f;

    if (drift < 0.0f) {
        drift = -drift;
    }

    // Upptack kraftiga tryckforandringar.
    if ((maximum - minimum) > mean * MAX_IR_RANGE ||
        max_step > mean * MAX_IR_STEP ||
        drift > mean * MAX_IR_DRIFT) {

        result.status = PULSE_MOTION;
        return result;
    }

    // Steg 1: Glidande medelvarde for brusreducering.
    for (size_t i = SMOOTH_RADIUS;
         i < MEASUREMENT_CAPACITY - SMOOTH_RADIUS; i++) {

        float sum = 0.0f;

        for (int j = -SMOOTH_RADIUS;
             j <= SMOOTH_RADIUS; j++) {

            sum += ir[(int)i + j];
        }

        smooth[i] =
            sum / (2 * SMOOTH_RADIUS + 1);
    }

    // Steg 2: Ta bort langsamt varierande baslinje.
    const size_t first =
        BASELINE_RADIUS + SMOOTH_RADIUS;

    const size_t last =
        MEASUREMENT_CAPACITY - first;

    float highest = 0.0f;

    for (size_t i = first; i < last; i++) {

        float baseline = 0.0f;

        for (int j = -BASELINE_RADIUS;
             j <= BASELINE_RADIUS; j++) {

            baseline += smooth[(int)i + j];
        }

        baseline /= (2 * BASELINE_RADIUS + 1);
        signal[i] = smooth[i] - baseline;

        if (signal[i] > highest) {
            highest = signal[i];
        }
    }

    // Steg 3: Bestam en troskel for pulstoppar.
    float threshold = highest * 0.40f;

    if (threshold < MIN_PULSE_HEIGHT) {
        result.status = PULSE_WEAK_SIGNAL;
        return result;
    }

    int min_distance =
        (int)(60.0f * sample_rate_hz / MAX_BPM);

    int max_distance =
        (int)(60.0f * sample_rate_hz / MIN_BPM);

    int peaks[16];
    unsigned int count = 0;

    // Steg 4: Hitta lokala toppar.
    for (size_t i = first + 1; i + 1 < last; i++) {

        if (signal[i] <= threshold ||
            signal[i] <= signal[i - 1] ||
            signal[i] < signal[i + 1]) {

            continue;
        }

        if (count > 0 &&
            (int)i - peaks[count - 1] < min_distance) {

            if (signal[i] > signal[peaks[count - 1]]) {
                peaks[count - 1] = (int)i;
            }

        } else if (count < 16) {

            peaks[count++] = (int)i;
        }
    }

    result.peaks = count;
    result.status = PULSE_UNSTABLE;

    if (count < 3) {
        return result;
    }

    // Steg 5: Berakna intervall mellan topparna.
    float interval_sum = 0.0f;

    for (unsigned int i = 1; i < count; i++) {

        int interval = peaks[i] - peaks[i - 1];

        if (interval < min_distance ||
            interval > max_distance) {

            return result;
        }

        interval_sum += (float)interval;
    }

    float average =
        interval_sum / (float)(count - 1);

    // Steg 6: Kontrollera regelbundenheten.
    for (unsigned int i = 1; i < count; i++) {

        float error =
            (float)(peaks[i] - peaks[i - 1]) - average;

        if (error < 0.0f) {
            error = -error;
        }

        if (error > average * 0.25f) {
            return result;
        }
    }

    // Steg 7: Omvandla toppintervall till BPM.
    result.bpm =
        60.0f * sample_rate_hz / average;

    result.status = PULSE_VALID;

    return result;
}

const char *pulse_status_text(PulseStatus status)
{
    switch (status) {

        case PULSE_WAITING:
            return "Samlar fem sekunder data";

        case PULSE_NO_FINGER:
            return "Ingen tydlig fingerkontakt";

        case PULSE_BAD_RATE:
            return "Samplingshastighet avviker";

        case PULSE_MOTION:
            return "Tryckforandring/rorelse - hall stilla";

        case PULSE_WEAK_SIGNAL:
            return "For svag pulssignal";

        case PULSE_UNSTABLE:
            return "Ojamn pulssignal";

        case PULSE_VALID:
            return "Puls hittad";

        default:
            return "Okand status";
    }
}
