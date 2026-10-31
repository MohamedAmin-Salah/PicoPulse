
#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>
#include "hardware/i2c.h"

#define MAX30102_I2C_ADDRESS 0x57

typedef struct {
    uint32_t red;
    uint32_t ir;
} Max30102Sample;

typedef struct {
    i2c_inst_t *bus;
    uint8_t address;
} Max30102;

typedef enum {
    MAX30102_OK,
    MAX30102_NO_DATA,
    MAX30102_I2C_ERROR,
    MAX30102_BAD_ID,
    MAX30102_FIFO_OVERFLOW
} Max30102Status;

void max30102_setup_bus(Max30102 *sensor, i2c_inst_t *bus,
                        unsigned int sda_gpio, unsigned int scl_gpio);
Max30102Status max30102_start(const Max30102 *sensor);
Max30102Status max30102_next_sample(const Max30102 *sensor,
                                    Max30102Sample *sample);
const char *max30102_status_text(Max30102Status status);

#endif
