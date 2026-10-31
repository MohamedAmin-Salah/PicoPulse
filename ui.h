
#ifndef UI_H
#define UI_H

#include "pulse.h"
#include "spo2.h"

void ui_init(void);

void ui_update(const PulseResult *pulse,
               const Spo2Result *oxygen,
               unsigned int fifo_errors);

#endif
