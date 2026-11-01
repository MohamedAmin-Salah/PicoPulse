
#include "max30102.h"
#include "pico/stdlib.h"

#define REG_FIFO_WR_PTR  0x04
#define REG_OVF_COUNTER  0x05
#define REG_FIFO_RD_PTR  0x06
#define REG_FIFO_DATA    0x07
#define REG_FIFO_CONFIG  0x08
#define REG_MODE_CONFIG  0x09
#define REG_SPO2_CONFIG  0x0A
#define REG_LED1_PA      0x0C
#define REG_LED2_PA      0x0D
#define REG_PART_ID      0xFF

#define EXPECTED_PART_ID 0x15
#define I2C_TIMEOUT_US   10000

static bool read_bytes(const Max30102 *sensor, uint8_t reg,
                       uint8_t *data, size_t count)
{
    int written = i2c_write_timeout_us(sensor->bus, sensor->address,
                                       &reg, 1, true, I2C_TIMEOUT_US);
    if (written != 1) {
        return false;
    }

    int received = i2c_read_timeout_us(sensor->bus, sensor->address,
                                       data, count, false, I2C_TIMEOUT_US);
    return received == (int)count;
}

static bool write_register(const Max30102 *sensor, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_write_timeout_us(sensor->bus, sensor->address,
                                data, sizeof(data), false,
                                I2C_TIMEOUT_US) == (int)sizeof(data);
}

static bool reset_fifo(const Max30102 *sensor)
{
    return write_register(sensor, REG_FIFO_WR_PTR, 0) &&
           write_register(sensor, REG_OVF_COUNTER, 0) &&
           write_register(sensor, REG_FIFO_RD_PTR, 0);
}

void max30102_setup_bus(Max30102 *sensor, i2c_inst_t *bus,
                        unsigned int sda_gpio, unsigned int scl_gpio)
{
    sensor->bus = bus;
    sensor->address = MAX30102_I2C_ADDRESS;

    i2c_init(bus, 100 * 1000);
    gpio_set_function(sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(scl_gpio, GPIO_FUNC_I2C);
    gpio_pull_up(sda_gpio);
    gpio_pull_up(scl_gpio);
}

Max30102Status max30102_start(const Max30102 *sensor)
{
    uint8_t id = 0;
    if (!read_bytes(sensor, REG_PART_ID, &id, 1)) {
        return MAX30102_I2C_ERROR;
    }
    if (id != EXPECTED_PART_ID) {
        return MAX30102_BAD_ID;
    }

    // Stanna sensorn medan vi konfigurerar den.
    if (!write_register(sensor, REG_MODE_CONFIG, 0x00) ||
        !write_register(sensor, REG_FIFO_CONFIG, 0x0F) ||
        !write_register(sensor, REG_SPO2_CONFIG, 0x27) ||
        !write_register(sensor, REG_LED1_PA, 0x1F) ||
        !write_register(sensor, REG_LED2_PA, 0x1F) ||
        !reset_fifo(sensor) ||
        !write_register(sensor, REG_MODE_CONFIG, 0x03)) {
        return MAX30102_I2C_ERROR;
    }

    return MAX30102_OK;
}

Max30102Status max30102_next_sample(const Max30102 *sensor,
                                    Max30102Sample *sample)
{
    uint8_t wr = 0;
    uint8_t rd = 0;
    uint8_t overflow = 0;

    if (!read_bytes(sensor, REG_FIFO_WR_PTR, &wr, 1) ||
        !read_bytes(sensor, REG_FIFO_RD_PTR, &rd, 1) ||
        !read_bytes(sensor, REG_OVF_COUNTER, &overflow, 1)) {
        return MAX30102_I2C_ERROR;
    }

    // Om minnet svammat över behöver vi börja om med ren FIFO.
    if (overflow != 0) {
        if (!reset_fifo(sensor)) {
            return MAX30102_I2C_ERROR;
        }
        return MAX30102_FIFO_OVERFLOW;
    }
    if ((wr & 0x1F) == (rd & 0x1F)) {
        return MAX30102_NO_DATA;
    }

    uint8_t data[6];
    if (!read_bytes(sensor, REG_FIFO_DATA, data, sizeof(data))) {
        return MAX30102_I2C_ERROR;
    }

    sample->red = ((((uint32_t)data[0] << 16) |
                    ((uint32_t)data[1] << 8) | data[2]) & 0x3FFFF);
    sample->ir  = ((((uint32_t)data[3] << 16) |
                    ((uint32_t)data[4] << 8) | data[5]) & 0x3FFFF);
    return MAX30102_OK;
}

const char *max30102_status_text(Max30102Status status)
{
    switch (status) {
        case MAX30102_OK:            return "OK";
        case MAX30102_NO_DATA:       return "Inga nya sampel";
        case MAX30102_I2C_ERROR:     return "I2C-fel";
        case MAX30102_BAD_ID:        return "Fel sensor-ID";
        case MAX30102_FIFO_OVERFLOW: return "FIFO-overflode";
        default:                     return "Okant fel";
    }
}
