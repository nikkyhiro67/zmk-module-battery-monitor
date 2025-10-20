#include "led_indicator.h"
#include "../battery_monitor/battery_monitor.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zmk/keymap.h>
#include <zmk/layers.h>
#include <zephyr/sys/printk.h>

#define STRIP_NODE DT_NODELABEL(sk6812_led)
static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);
struct led_rgb colors[1];

// GPIO駆動LED設定
static const struct device *gpio_dev;
#define LED_EN_PIN 9
#define LED_PIN    10

static void set_led_color(uint8_t r, uint8_t g, uint8_t b) {
    colors[0] = (struct led_rgb){ .r = r, .g = g, .b = b };
    led_strip_update_rgb(strip, colors, 1);
}

static void gpio_led_init(void) {
    gpio_dev = device_get_binding("GPIO_0");
    if (!gpio_dev) {
        printk("GPIO device not found!\n");
        return;
    }
    gpio_pin_configure(gpio_dev, LED_EN_PIN, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure(gpio_dev, LED_PIN, GPIO_OUTPUT_INACTIVE);
}

static void gpio_led_set(bool on) {
    if (!gpio_dev) return;
    gpio_pin_set(gpio_dev, LED_PIN, on ? 1 : 0);
}

// === 初期化 ===
void led_indicator_init(void) {
    if (!device_is_ready(strip)) {
        printk("LED strip not ready\n");
        return;
    }
    gpio_led_init();
    printk("LED indicator initialized.\n");
}

// === 周期処理 ===
void led_indicator_loop(void) {
    static bool bt_prev_connected = false;
    static int prev_layer = -1;
    static uint8_t prev_batt = 255;

    while (1) {
        battery_monitor_update();

        int mv = battery_monitor_get_voltage_mv();
        uint8_t level = battery_monitor_get_level();
        bool low = battery_monitor_is_low();

        bool bt_ready = bt_is_ready();
        struct bt_conn *conns[CONFIG_BT_MAX_CONN];
        int conn_count = bt_ready ? bt_conn_get_connections(conns, CONFIG_BT_MAX_CONN) : 0;

        if (low) {
            set_led_color(255, 0, 0);
            gpio_led_set(true);
            k_sleep(K_SECONDS(1));
            gpio_led_set(false);
            continue;
        }

        // --- バッテリ変動時のみ点灯 ---
        if (level != prev_batt) {
            if (level >= 80)
                set_led_color(0, 255, 0);
            else if (level >= 50)
                set_led_color(255, 165, 0);
            else
                set_led_color(255, 0, 0);

            gpio_led_set(true);
            k_sleep(K_SECONDS(2));
            gpio_led_set(false);
            set_led_color(0, 0, 0);
            prev_batt = level;
        }

        // --- Bluetooth接続状態 ---
        if (conn_count == 0) {
            set_led_color(0, 255, 0);
            gpio_led_set(true);
            k_sleep(K_MSEC(300));
            gpio_led_set(false);
            set_led_color(0, 0, 0);
            k_sleep(K_MSEC(700));
        } else {
            set_led_color(0, 0, 0);
            gpio_led_set(false);
            k_sleep(K_SECONDS(5));
        }

        // --- 接続/切断トリガ表示 ---
        if (conn_count > 0 && !bt_prev_connected) {
            set_led_color(0, 255, 0);
            gpio_led_set(true);
            k_sleep(K_SECONDS(1));
            gpio_led_set(false);
            set_led_color(0, 0, 0);
        } else if (conn_count == 0 && bt_prev_connected) {
            set_led_color(255, 0, 0);
            gpio_led_set(true);
            k_sleep(K_SECONDS(1));
            gpio_led_set(false);
            set_led_color(0, 0, 0);
        }
        bt_prev_connected = (conn_count > 0);

        // --- レイヤー表示 ---
        uint8_t layer = zmk_keymap_layer_active();
        if (layer != prev_layer) {
            if (layer == 0) set_led_color(255, 255, 255);
            else if (layer == 1) set_led_color(0, 0, 255);
            else if (layer == 2) set_led_color(255, 255, 0);
            else if (layer == 6) set_led_color(0, 255, 0);
            else set_led_color(0, 0, 0);

            gpio_led_set(true);
            k_sleep(K_SECONDS(1));
            gpio_led_set(false);
            set_led_color(0, 0, 0);
            prev_layer = layer;
        }

        k_sleep(K_SECONDS(5));
    }
}
