/*
 * battery_monitor.c
 *
 * ZMK Non-LiPo Battery Monitor
 * - Polls battery SoC from non_lipo_battery_management.c
 * - Emits zmk_battery_state_changed event on change
 * - Optionally logged via ZMK_LOG_LEVEL
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

LOG_MODULE_REGISTER(battery_monitor, CONFIG_ZMK_LOG_LEVEL);

/* 外部バッテリ管理API */
extern int non_lipo_battery_get_soc(void);

/* バッテリ監視スレッド */
static void battery_monitor_thread(void)
{
    int prev_soc = -1;

    LOG_INF("Battery monitor thread started");

    while (1) {
        int soc = non_lipo_battery_get_soc();

        if (soc < 0) {
            LOG_WRN("Failed to get battery SoC (err=%d)", soc);
        } else if (soc != prev_soc) {
            /* ZMKイベント発行 */
            struct zmk_battery_state_changed ev = {
                .state_of_charge = soc
            };
            ZMK_EVENT_RAISE(new_zmk_battery_state_changed(ev));

            LOG_INF("Battery SoC changed: %d%%", soc);
            prev_soc = soc;
        }

        /* 設定値（例: 10秒）ごとに監視 */
        k_sleep(K_SECONDS(CONFIG_ZMK_BATTERY_MONITOR_INTERVAL_SEC));
    }
}

/* スレッド定義 */
K_THREAD_DEFINE(battery_monitor_task,
                CONFIG_ZMK_BATTERY_MONITOR_STACK_SIZE,
                battery_monitor_thread, NULL, NULL, NULL,
                5, 0, 0);

/* 初期化関数 */
int battery_monitor_init(void)
{
    LOG_INF("Battery Monitor initialized (ZMK event linked)");
    return 0;
}
