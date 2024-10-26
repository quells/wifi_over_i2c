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
#define I2C_ADDRESS 0x57
#define I2C_CHIP_ID 0x46

#define I2C_RX_SIZE 4096
#define I2C_TX_SIZE 65536

uint8_t i2c_op = 0;
uint8_t i2c_flags = 0;
bool i2c_busy = false;
bool i2c_op_written = false;

int i2c_rx_buf_idx = 0;
uint8_t i2c_rx_buf[I2C_RX_SIZE] = {};

int i2c_tx_buf_sent = 0;
int i2c_tx_buf_idx = 0;
uint8_t i2c_tx_buf[I2C_TX_SIZE] = {};

typedef enum _opcode {
    OP_PING = 0x12,
    OP_CHIP_ID = 0x1D,
} opcode;

void i2c_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: {
        uint8_t r = i2c_read_byte_raw(i2c0);
        if (i2c_busy) return;
        if (!i2c_op_written) {
            i2c_op = r;
            switch (i2c_op) {
            case OP_PING:
                i2c_busy = true;
                break;
            default:
                break;
            }
            i2c_op_written = true;
        } else {
            if (i2c_rx_buf_idx >= I2C_RX_SIZE) return;
            i2c_rx_buf[i2c_rx_buf_idx++] = r;
        }
        break;
    }
    case I2C_SLAVE_REQUEST: {
        uint8_t w = 0xFF;
        if (!i2c_busy) {
            switch (i2c_op) {
            case OP_CHIP_ID: {
                w = I2C_CHIP_ID;
                break;
            }
            default: {
                if (i2c_tx_buf_sent < i2c_tx_buf_idx) w = i2c_tx_buf[i2c_tx_buf_sent++];
                break;
            }
            }
        }
        i2c_write_byte_raw(i2c0, w);
        break;
    }
    case I2C_SLAVE_FINISH: {
        i2c_op_written = false;
        break;
    }
    default: break;
    }
}

void i2c_tx_msg(char *msg) {
    strncpy(i2c_tx_buf, msg, I2C_TX_SIZE);
    i2c_tx_buf_sent = 0;
    i2c_tx_buf_idx = strnlen(msg, I2C_TX_SIZE);
}

int main() {
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    i2c_slave_init(i2c0, I2C_ADDRESS, &i2c_handler);

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(150);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // cyw43_arch_enable_sta_mode();
    // int ssid_count = 0;
    // char *ssids_found = NULL;
    // scan_ssids(&ssid_count, &ssids_found);
    // cyw43_arch_disable_sta_mode();
    // cyw43_arch_deinit();

    // printf("%d\n", ssid_count);
    // char ssid[SSID_WIDTH+1] = {};
    // for (int i = 0; i < SSID_WIDTH+1; i++) ssid[i] = 0;
    // for (int i = 0; i < ssid_count; i++) {
    //     strncpy(ssid, ssids_found + SSID_WIDTH*sizeof(char)*i, SSID_WIDTH);
    //     printf("%s\n", ssid);
    // }
    // printf("---\n");

    while (true) {
        if (i2c_busy) {
            switch (i2c_op) {
            case OP_PING: {
                i2c_tx_msg("PONG");
                break;
            }
            default:
                break;
            }

            i2c_busy = false;
        }
        sleep_us(100);
    }
    return 0;
}
