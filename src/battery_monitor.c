#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

LOG_MODULE_REGISTER(battery_monitor, CONFIG_ZMK_LOG_LEVEL);

#define BATTERY_MONITOR_NODE DT_INST(0, zmk_battery_monitor)

#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, io_channels)
#include <zephyr/drivers/adc.h>
#define ADC_CHANNEL DT_IO_CHANNELS_INPUT(BATTERY_MONITOR_NODE)
#define ADC_NODE DT_IO_CHANNELS_CTLR(BATTERY_MONITOR_NODE)
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);
#endif

#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, power_gpios)
static const struct gpio_dt_spec power_gpio = GPIO_DT_SPEC_GET(BATTERY_MONITOR_NODE, power_gpios);
#endif

static int32_t battery_voltage_mv = 0;
static uint8_t battery_soc = 100;

/* ========== Fallback: use non-lipo external API if available ========== */
#ifdef CONFIG_NON_LIPO_BATTERY_MANAGEMENT
#include <non_lipo_battery.h>
static bool use_non_lipo_api = true;
#else
static bool use_non_lipo_api = false;
#endif
/* ===================================================================== */

static int battery_monitor_sample(void) {
    if (use_non_lipo_api) {
        /* 外部モジュールから取得 */
        int mv = non_lipo_battery_get_voltage_mv();
        int soc = non_lipo_battery_get_soc();
        battery_voltage_mv = mv;
        battery_soc = soc;
        return 0;
    }

#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, power_gpios)
    gpio_pin_set_dt(&power_gpio, 1);
    k_msleep(5);
#endif

#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, io_channels)
    struct adc_sequence sequence = {
        .channels = BIT(ADC_CHANNEL),
        .buffer = &battery_voltage_mv,
        .buffer_size = sizeof(battery_voltage_mv),
        .resolution = 12,
    };

    int ret = adc_read(adc_dev, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC read failed (%d)", ret);
        return ret;
    }

    /* サンプル電圧をmV換算 (簡易例: 3.3V基準、12bit) */
    battery_voltage_mv = (battery_voltage_mv * 3300) / 4095;
#endif

#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, power_gpios)
    gpio_pin_set_dt(&power_gpio, 0);
#endif

    /* 3.0V → 0%, 4.2V → 100% の線形換算 */
    int soc = (battery_voltage_mv - 3000) * 100 / (4200 - 3000);
    if (soc < 0) soc = 0;
    if (soc > 100) soc = 100;
    battery_soc = soc;

    return 0;
}

int battery_monitor_get_voltage_mv(void) {
    return battery_voltage_mv;
}

uint8_t battery_monitor_get_soc(void) {
    return battery_soc;
}

static void battery_monitor_work(struct k_work *work) {
    battery_monitor_sample();
    struct zmk_battery_state_changed ev = {
        .state_of_charge = battery_soc,
        .voltage = battery_voltage_mv
    };
    LOG_INF("Battery: %d%% (%dmV)", battery_soc, battery_voltage_mv);
    ZMK_EVENT_RAISE(new_zmk_battery_state_changed(&ev));
}

K_WORK_DELAYABLE_DEFINE(battery_monitor_work_item, battery_monitor_work);

int battery_monitor_init(void) {
#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, power_gpios)
    if (!device_is_ready(power_gpio.port)) {
        LOG_ERR("Power GPIO not ready");
        return -ENODEV;
    }
    gpio_pin_configure_dt(&power_gpio, GPIO_OUTPUT_INACTIVE);
#endif

#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, io_channels)
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device not ready");
        return -ENODEV;
    }
#endif

    LOG_INF("Battery Monitor initialized");
    k_work_schedule(&battery_monitor_work_item, K_SECONDS(5));
    return 0;
}

SYS_INIT(battery_monitor_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
