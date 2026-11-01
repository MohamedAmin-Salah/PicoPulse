
#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "max30102.h"
#include "measurement.h"
#include "pulse.h"
#include "spo2.h"
#include "ui.h"

int main(void)
{
    stdio_init_all();
    sleep_ms(3000);

    printf("Pico fungerar!\n");

    // Initiera display och rita granssnittet.
    ui_init();
    printf("Display initierad!\n");

    Max30102 sensor;
    max30102_setup_bus(&sensor, i2c0, 4, 5);

    static MeasurementBuffer measurements;
    static uint64_t sample_times[MEASUREMENT_CAPACITY];

    unsigned int fifo_overflows = 0;

    while (true) {

        Max30102Status status = max30102_start(&sensor);

        if (status != MAX30102_OK) {
            printf("Sensorstart misslyckades: %s\n",
                   max30102_status_text(status));
            sleep_ms(1000);
            continue;
        }

        measurement_clear(&measurements);

        unsigned int samples_received = 0;

        printf("MAX30102 startad.\n");

        while (true) {

            Max30102Sample sample;
            status = max30102_next_sample(&sensor, &sample);

            if (status == MAX30102_NO_DATA) {
                sleep_ms(2);
                continue;
            }

            if (status != MAX30102_OK) {

                if (status == MAX30102_FIFO_OVERFLOW) {
                    fifo_overflows++;
                }

                printf("Sensorfel: %s\n",
                       max30102_status_text(status));
                break;
            }

            // Spara tidsstampel och matvarde.
            sample_times[measurements.next] = time_us_64();
            measurement_push(&measurements, sample);
            samples_received++;

            // Uppdatera efter varje 100 mottagna sampel.
            if (samples_received % 100 != 0) {
                continue;
            }

            if (measurements.count < MEASUREMENT_CAPACITY) {
                printf("Samlar fem sekunder data...\n");
                continue;
            }

            // Berakna mottagningshastigheten.
            size_t oldest = measurements.next;

            size_t newest =
                (oldest + MEASUREMENT_CAPACITY - 1)
                % MEASUREMENT_CAPACITY;

            uint64_t elapsed_us =
                sample_times[newest] - sample_times[oldest];

            float rate_hz = elapsed_us > 0
                ? (float)(MEASUREMENT_CAPACITY - 1)
                  * 1000000.0f / (float)elapsed_us
                : 0.0f;

            // Pulsberakning.
            PulseResult pulse =
                pulse_analyze(&measurements, rate_hz);

            // RED/IR-analys for SpO2-underlag.
            Spo2Result oxygen = {
                SPO2_WAITING, 0.0f, 0.0f, 0.0f
            };

            if (rate_hz >= 90.0f && rate_hz <= 110.0f) {
                oxygen = spo2_analyze(&measurements);
            }

            // Uppdatera displayen.
            ui_update(&pulse, &oxygen, fifo_overflows);

            // Behall informationen i Serial Monitor.
            printf("Frekvens: %.1f sampel/s | FIFO: %u\n",
                   rate_hz, fifo_overflows);

            if (pulse.status == PULSE_VALID) {
                printf("Puls: %.1f BPM\n", pulse.bpm);
            } else {
                printf("Puls: %s\n",
                       pulse_status_text(pulse.status));
            }

            if (oxygen.status == SPO2_RATIO_VALID) {
                printf("SpO2-underlag: R=%.3f\n",
                       oxygen.ratio);
            } else {
                printf("SpO2-underlag: %s\n",
                       spo2_status_text(oxygen.status));
            }
        }

        sleep_ms(1000);
    }
}
