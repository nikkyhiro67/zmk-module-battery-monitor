/* include/battery_monitor.h */
#ifndef ZMK_MODULE_BATTERY_MONITOR_H_
#define ZMK_MODULE_BATTERY_MONITOR_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Update internal reading from hardware (force read). */
void battery_monitor_update(void);

/** Get last cached battery percentage (0-100). */
uint8_t battery_monitor_get_level(void);

/** Get last cached battery voltage in mV (or 0 if unknown). */
int battery_monitor_get_voltage_mv(void);

/** Return true if battery is considered low. */
bool battery_monitor_is_low(void);

#ifdef __cplusplus
}
#endif

#endif /* ZMK_MODULE_BATTERY_MONITOR_H_ */
