#include <stdio.h>
#include "wifi_scan.h"
#include "pico/cyw43_arch.h"

char scan_results_ssid[SSID_WIDTH * SSID_COUNT_MAX] = {};
int scan_results_n = 0;

static bool scan_results_contains(const char *value) {
    int l = 0;
    int u = scan_results_n;
    while (1) {
        int i = (l + u) / 2;
        if (i >= u) return false;

        char *ri = scan_results_ssid + SSID_WIDTH*sizeof(char)*i;
        int m = strncmp(ri, value, SSID_WIDTH);
        if (m == 0) return true;
        if (m < 0) {
            if (l == i) {
                l = i + 1;
            } else {
                l = i;
            }
        } else {
            u = i;
        }
    }
}

static void scan_results_sort() {
    char *ri, *rj;
    char tmp[SSID_WIDTH] = {};
    for (int i = 0; i < scan_results_n-1; i++) {
        ri = scan_results_ssid + SSID_WIDTH*sizeof(char)*i;
        for (int j = i+1; j < scan_results_n; j++) {
            rj = scan_results_ssid + SSID_WIDTH*sizeof(char)*j;
            if (strncmp(ri, rj, SSID_WIDTH) > 0) {
                strncpy(tmp, ri, SSID_WIDTH);
                strncpy(ri, rj, SSID_WIDTH);
                strncpy(rj, tmp, SSID_WIDTH);
            }
        }
    }
}

static void scan_results_insert(const char *value) {
    if (strnlen(value, SSID_WIDTH) == 0) return;
    if (scan_results_contains(value)) return;
    char *dst = scan_results_ssid + SSID_WIDTH*sizeof(char)*scan_results_n;
    strncpy(dst, value, SSID_WIDTH);
    scan_results_n++;
    scan_results_sort();
}

static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (result && scan_results_n < SSID_COUNT_MAX) {
        scan_results_insert(result->ssid);
    }
    return 0;
}

void scan_ssids(int *n, char **ssids) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    scan_results_n = 0;
    for (int i = 0; i < SSID_WIDTH * SSID_COUNT_MAX; i++) scan_results_ssid[i] = 0;

    cyw43_wifi_scan_options_t scan_options = {0};
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);
    if (err) {
        printf("Failed to start wifi scan\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        return;
    }

    printf("Performing wifi scan\n");

    absolute_time_t scan_time = make_timeout_time_ms(10000);
    absolute_time_t now = nil_time;
    bool scan_in_progress = true;
    while (scan_in_progress) {
        if (!cyw43_wifi_scan_active(&cyw43_state)) {
            printf("Finished wifi scan\n");
            scan_in_progress = false;
        }

        if (absolute_time_diff_us(now, scan_time) < 0) {
            printf("Timeout wifi scan\n");
            scan_in_progress = false;
        }

        if (scan_in_progress) sleep_ms(10);
    }

    *n = scan_results_n;
    *ssids = scan_results_ssid;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}
