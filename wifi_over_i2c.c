#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <pico/i2c_slave.h>
#include <pico/cyw43_arch.h>
#include <hardware/vreg.h>
#include <hardware/clocks.h>

#include "wifi_scan.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_BAUDRATE 400*1000
#define I2C_ADDRESS 0x41

uint8_t i2c_reg = 0;
uint8_t i2c_flags = 0;
uint8_t i2c_buf_idx = 0;
uint8_t i2c_buf[256] = {};
bool i2c_busy = false;
bool i2c_reset = false;

void i2c_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: {
        uint8_t x = i2c_read_byte_raw(i2c0);
        if (i2c_buf_idx < 256) i2c_buf[i2c_buf_idx++] = 'r';
        if (i2c_buf_idx < 256) i2c_buf[i2c_buf_idx++] = x;
        break;
    }
    case I2C_SLAVE_REQUEST: {
        i2c_write_byte_raw(i2c0, 0xFF);
        if (i2c_buf_idx < 256) i2c_buf[i2c_buf_idx++] = 'w';
        break;
    }
    case I2C_SLAVE_FINISH: {
        if (i2c_buf_idx < 256) i2c_buf[i2c_buf_idx++] = 'c';
        i2c_reset = true;
        break;
    }
    default: break;
    }
}

int main() {
    stdio_init_all();

    sleep_ms(2000);

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // I2C Initialisation. Using it at 400Khz.
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    i2c_slave_init(i2c0, I2C_ADDRESS, &i2c_handler);

    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    // Example to turn on the Pico W LED
    // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    cyw43_arch_enable_sta_mode();
    int ssid_count = 0;
    char *ssids_found = NULL;
    scan_ssids(&ssid_count, &ssids_found);
    cyw43_arch_disable_sta_mode();
    cyw43_arch_deinit();

    printf("%d\n", ssid_count);
    char ssid[SSID_WIDTH+1] = {};
    for (int i = 0; i < SSID_WIDTH+1; i++) ssid[i] = 0;
    for (int i = 0; i < ssid_count; i++) {
        strncpy(ssid, ssids_found + SSID_WIDTH*sizeof(char)*i, SSID_WIDTH);
        printf("%s\n", ssid);
    }
    printf("---\n");

    while (true) {
        if (i2c_reset) {
            uint8_t buf[256] = {};
            memcpy(buf, i2c_buf, i2c_buf_idx);
            for (int i = 0; i < i2c_buf_idx; i++) {
                printf("%02x ", buf[i]);
                if (i % 16 == 0) printf("\n");
            }
            i2c_reset = false;
        }
        sleep_ms(10);
    }
    return 0;
}
