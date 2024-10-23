#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "wifi_scan.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

int main() {
    stdio_init_all();

    sleep_ms(2000);

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    // Example to turn on the Pico W LED
    // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    int ssid_count = 0;
    char *ssids_found = NULL;
    scan_ssids(&ssid_count, &ssids_found);
    printf("%d\n", ssid_count);
    char ssid[SSID_WIDTH+1] = {};
    for (int i = 0; i < SSID_WIDTH+1; i++) ssid[i] = 0;
    for (int i = 0; i < ssid_count; i++) {
        strncpy(ssid, ssids_found + SSID_WIDTH*sizeof(char)*i, SSID_WIDTH);
        printf("%s\n", ssid);
    }
    printf("---\n");

    while (true) {
        sleep_ms(1000);
    }
    return 0;
}
