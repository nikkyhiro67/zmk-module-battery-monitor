#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <led_indicator.h>

LOG_MODULE_REGISTER(led_indicator, CONFIG_ZMK_LOG_LEVEL);

/* 外部のバッテリ管理モジュールAPI（non_lipo_battery_management.c） */
extern int non_lipo_battery_get_soc(void);

/* デバイスノード参照 */
static const struct device *led_strip = DEVICE_DT_GET(DT_ALIAS(led_strip0));

/* LED表示更新スレッド */
static void led_indicator_thread(void)
{
    if (!device_is_ready(led_strip)) {
        LOG_ERR("LED strip not ready!");
        return;
    }

    while (1) {
        int soc = non_lipo_battery_get_soc();

        /* バッテリ残量をLEDカラーに変換（例: 赤～緑） */
        struct led_rgb color = {
            .r = 255 * (100 - soc) / 100,
            .g = 255 * soc / 100,
            .b = 0
        };

        led_strip_update_rgb(led_strip, &color, 1);
        LOG_INF("LED updated: %d%% (R:%d G:%d)", soc, color.r, color.g);

        k_sleep(K_MSEC(CONFIG_ZMK_LED_INDICATOR_UPDATE_INTERVAL));
    }
}

K_THREAD_DEFINE(led_indicator_task,
                CONFIG_ZMK_LED_INDICATOR_THREAD_STACK_SIZE,
                led_indicator_thread, NULL, NULL, NULL,
                5, 0, 0);

int led_indicator_init(void)
{
    LOG_INF("LED Indicator initialized");
    return 0;
}

void led_indicator_update(void)
{
    LOG_INF("Manual LED indicator refresh");
}
