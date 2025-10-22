#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>

/**
 * @file led_indicator.h
 * @brief Interface for ZMK LED Indicator module
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LED indicator manually (optional)
 *
 * Normally initialized automatically at boot by SYS_INIT().
 */
int led_indicator_init(void);

/**
 * @brief Update LED indicator manually (optional)
 *
 * Can be used to force refresh after custom state changes.
 */
void led_indicator_update(void);

#ifdef __cplusplus
}
#endif
