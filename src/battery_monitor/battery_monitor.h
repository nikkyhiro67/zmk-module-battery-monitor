#pragma once
#include <stdint.h>
#include <stdbool.h>

// 残量取得（%）
uint8_t battery_monitor_get_level(void);

// 電圧取得（mV）
int battery_monitor_get_voltage_mv(void);

// 低電圧状態判定
bool battery_monitor_is_low(void);

// 更新（定期呼び出し）
void battery_monitor_update(void);
