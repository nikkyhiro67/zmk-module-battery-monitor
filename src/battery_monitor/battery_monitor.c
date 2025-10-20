#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(battery_monitor, LOG_LEVEL_INF);

#if defined(CONFIG_ZMK_BATTERY_MONITOR)

/* DeviceTree instance for our battery-monitor node */
#define BATTERY_MONITOR_NODE DT_INST(0, zmk_battery_monitor)
BUILD_ASSERT(DT_NODE_HAS_STATUS(BATTERY_MONITOR_NODE, okay), "Battery monitor DT node not found");

/* Get ADC channel info */
#define IO_CHANNELS_NODE DT_PHANDLE(BATTERY_MONITOR_NODE, io_channels)
#define IO_CHANNELS_INPUT DT_IO_CHANNELS_INPUT(BATTERY_MONITOR_NODE)

/* Full/Empty voltage thresholds (optional properties) */
#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, full_mv)
#define FULL_MV DT_PROP(BATTERY_MONITOR_NODE, full_mv)
#else
#define FULL_MV 4200
#endif

#if DT_NODE_HAS_PROP(BATTERY_MONITOR_NODE, empty_mv)
#define EMPTY_MV DT_PROP(BATTERY_MONITOR_NODE, empty_mv)
#else
#define EMPTY_MV 3300
#endif

/* -------------------------------------------------------------------------- */
/* 1. 外部API存在確認 (weakフォールバック)                                    */
/* -------------------------------------------------------------------------- */
#if defined(CONFIG_ZMK_FEATURE_NON_LIPO)
extern int non_lipo_battery_get_voltage_mv(void);
#else
__weak int non_lipo_battery_get_voltage_mv(void)
{
    /* この関数は、non-lipo-battery-managementが存在しない場合のフォールバック */
    return -ENOTSUP;
}
#endif

/* -------------------------------------------------------------------------- */
/* 2. センサー経由で電圧取得                                                  */
/* -------------------------------------------------------------------------- */
static int read_voltage_via_sensor(void)
{
    const struct device *adc_dev = DEVICE_DT_GET(IO_CHANNELS_NODE);
    if (!device_is_ready(adc_dev)) {
        LOG_WRN("ADC device not ready");
        return -ENODEV;
    }

    struct adc_channel_cfg ch_cfg = {
        .gain             = ADC_GAIN_1,
        .reference        = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .channel_id       = IO_CHANNELS_INPUT,
    };

    if (adc_channel_setup(adc_dev, &ch_cfg)) {
        LOG_ERR("ADC channel setup failed");
        return -EIO;
    }

    int16_t raw;
    struct adc_sequence sequence = {
        .channels    = BIT(IO_CHANNELS_INPUT),
        .buffer      = &raw,
        .buffer_size = sizeof(raw),
        .resolution  = 12,
    };

    if (adc_read(adc_dev, &sequence)) {
        LOG_ERR("ADC read failed");
        return -EIO;
    }

    /* 仮にVref=3.3V、12bitとして電圧をmV単位で換算 */
    int32_t mv = (raw * 3300) / 4095;
    return mv;
}

/* -------------------------------------------------------------------------- */
/* 3. 統合バッテリー電圧取得関数                                             */
/* -------------------------------------------------------------------------- */
int battery_monitor_get_voltage_mv(void)
{
    int mv = read_voltage_via_sensor();

    if (mv > 0) {
        LOG_DBG("Voltage via ADC: %d mV", mv);
        return mv;
    }

    /* センサーで取得できなかった場合はnon-lipoフォールバック */
    int mv2 = non_lipo_battery_get_voltage_mv();
    if (mv2 > 0) {
        LOG_DBG("Voltage via non-lipo API: %d mV", mv2);
        return mv2;
    }

    LOG_ERR("Battery voltage read failed via both paths");
    return -ENODATA;
}

/* -------------------------------------------------------------------------- */
/* 4. 初期化ルーチン                                                         */
/* -------------------------------------------------------------------------- */
static int battery_monitor_init(void)
{
    LOG_INF("Battery monitor init start");

    int mv = battery_monitor_get_voltage_mv();
    if (mv > 0) {
        LOG_INF("Initial battery voltage: %d mV", mv);
    } else {
        LOG_WRN("Initial battery voltage unavailable");
    }

    return 0;
}

SYS_INIT(battery_monitor_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_BATTERY_MONITOR */
