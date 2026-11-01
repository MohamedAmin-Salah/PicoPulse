
#include <stdio.h>

#include "ui.h"
#include "display.h"

#define UI_BACKGROUND DISPLAY_BLACK
#define UI_TEXT       DISPLAY_WHITE
#define UI_ACCENT     DISPLAY_GREEN
#define UI_WARNING    DISPLAY_YELLOW

static void clear_region(uint16_t x, uint16_t y,
                         uint16_t width, uint16_t height)
{
    display_rect(x, y, width, height, UI_BACKGROUND);
}

void ui_init(void)
{
    display_init();

    // Rita den statiska delen endast en gang.
    display_fill(UI_BACKGROUND);

    display_draw_text(8, 6, "PULSE MONITOR",
                      UI_ACCENT, UI_BACKGROUND, 1);

    display_rect(8, 20, 112, 1, DISPLAY_BLUE);

    display_draw_text(8, 29, "HEART RATE",
                      UI_TEXT, UI_BACKGROUND, 1);

    display_draw_text(78, 53, "BPM",
                      UI_ACCENT, UI_BACKGROUND, 2);

    display_rect(8, 79, 112, 1, DISPLAY_BLUE);

    display_draw_text(8, 86, "SPO2",
                      UI_TEXT, UI_BACKGROUND, 1);

    display_draw_text(8, 99, "NOT CALIBRATED",
                      UI_WARNING, UI_BACKGROUND, 1);

    display_rect(8, 128, 112, 1, DISPLAY_BLUE);

    display_draw_text(8, 44, "--",
                      UI_TEXT, UI_BACKGROUND, 3);

    display_draw_text(8, 113, "R:-----",
                      UI_TEXT, UI_BACKGROUND, 1);

    display_draw_text(8, 136, "COLLECTING",
                      UI_WARNING, UI_BACKGROUND, 1);

    display_draw_text(8, 149, "FIFO:0",
                      UI_TEXT, UI_BACKGROUND, 1);
}

void ui_update(const PulseResult *pulse,
               const Spo2Result *oxygen,
               unsigned int fifo_errors)
{
    char text[24];

    // Uppdatera endast BPM-siffrorna.
    clear_region(8, 44, 64, 26);

    if (pulse->status == PULSE_VALID) {
        snprintf(text, sizeof(text), "%.0f", pulse->bpm);

        display_draw_text(8, 44, text,
                          UI_TEXT, UI_BACKGROUND, 3);
    } else {
        display_draw_text(8, 44, "--",
                          UI_TEXT, UI_BACKGROUND, 3);
    }

    // Visa den okalibrerade RED/IR-kvoten.
    clear_region(8, 113, 112, 10);

    if (oxygen->status == SPO2_RATIO_VALID) {
        snprintf(text, sizeof(text),
                 "R:%.3f", oxygen->ratio);

        display_draw_text(8, 113, text,
                          UI_TEXT, UI_BACKGROUND, 1);
    } else {
        display_draw_text(8, 113, "R:-----",
                          UI_TEXT, UI_BACKGROUND, 1);
    }

    // Visa pulssignalens status.
    clear_region(8, 136, 112, 10);

    const char *status_text = "COLLECTING";
    uint16_t status_color = UI_WARNING;

    switch (pulse->status) {
        case PULSE_VALID:
            status_text = "SIGNAL OK";
            status_color = UI_ACCENT;
            break;

        case PULSE_NO_FINGER:
            status_text = "NO FINGER";
            break;

        case PULSE_MOTION:
            status_text = "HOLD STILL";
            break;

        case PULSE_WEAK_SIGNAL:
            status_text = "WEAK SIGNAL";
            break;

        case PULSE_UNSTABLE:
            status_text = "UNSTABLE";
            break;

        case PULSE_BAD_RATE:
            status_text = "RATE ERROR";
            break;

        default:
            status_text = "COLLECTING";
            break;
    }

    display_draw_text(8, 136, status_text,
                      status_color, UI_BACKGROUND, 1);

    // Visa upptackta FIFO-overfloden.
    clear_region(8, 149, 112, 9);

    snprintf(text, sizeof(text),
             "FIFO:%u", fifo_errors);

    display_draw_text(8, 149, text,
                      fifo_errors == 0
                          ? UI_TEXT : UI_WARNING,
                      UI_BACKGROUND, 1);
}
