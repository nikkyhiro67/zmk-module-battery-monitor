#include "battery_monitor.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <non_lipo_battery_management/battery_management.h> // 外部モジュールAPI

static uint8_t current_level = 100;
static int current_mv = 3300;
static bool low_flag = false;

// === 内部関数 ===
static uint8_t mv_to_percent(int mv) {
    if (mv >= 3300) return 100;
    if (mv <= 2700) return 0;
    return (uint8_t)((mv - 2700) * 100 / (3300 - 2700));
}

// === 公開関数群 ===
void battery_monitor_update(void) {
    int mv = non_lipo_battery_get_voltage_mv(); // 外部API（実装済み）
    if (mv <= 0) {
        printk("battery_monitor: invalid voltage read\n");
        return;
    }

    current_mv = mv;
    current_level = mv_to_percent(mv);
    low_flag = (mv < 3000);
}

uint8_t battery_monitor_get_level(void) {
    return current_level;
}

int battery_monitor_get_voltage_mv(void) {
    return current_mv;
}

bool battery_monitor_is_low(void) {
    return low_flag;
}
