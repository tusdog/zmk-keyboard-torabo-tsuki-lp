// SPDX-License-Identifier: GPL-3.0-or-later

#define DT_DRV_COMPAT zmk_behavior_scroll_direction_toggle

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#if IS_ENABLED(CONFIG_ZMK_RUNTIME_INPUT_PROCESSOR)
#include <zmk/pointing/input_processor_runtime.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct scroll_direction_toggle_config {
    const char *processor_name;
    const char *secondary_processor_name;
};

static int on_scroll_direction_toggle_pressed(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

#if IS_ENABLED(CONFIG_ZMK_RUNTIME_INPUT_PROCESSOR)
    const struct device *behavior = zmk_behavior_get_binding(binding->behavior_dev);
    const struct scroll_direction_toggle_config *cfg = behavior->config;

    const struct device *primary =
        zmk_input_processor_runtime_find_by_name(cfg->processor_name);
    if (!primary) {
        LOG_ERR("Scroll processor '%s' not found", cfg->processor_name);
        return -ENODEV;
    }

    struct zmk_input_processor_runtime_config current = {0};
    int ret = zmk_input_processor_runtime_get_config(primary, NULL, &current);
    if (ret < 0) {
        LOG_ERR("Failed to read scroll processor config: %d", ret);
        return ret;
    }

    const bool next_invert = !current.y_invert;

    ret = zmk_input_processor_runtime_set_y_invert(
        primary, next_invert, ZMK_INPUT_PROCESSOR_RUNTIME_WRITE_MODE_MEMORY);
    if (ret < 0) {
        LOG_ERR("Failed to toggle primary scroll direction: %d", ret);
        return ret;
    }

    const struct device *secondary =
        zmk_input_processor_runtime_find_by_name(cfg->secondary_processor_name);
    if (secondary) {
        ret = zmk_input_processor_runtime_set_y_invert(
            secondary, next_invert, ZMK_INPUT_PROCESSOR_RUNTIME_WRITE_MODE_MEMORY);
        if (ret < 0) {
            LOG_ERR("Failed to sync scroll-key direction: %d", ret);
            return ret;
        }
    } else {
        LOG_WRN("Secondary scroll processor '%s' not found",
                cfg->secondary_processor_name);
    }

    LOG_INF("Vertical scroll direction: %s",
            next_invert ? "reversed" : "normal");
#endif

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_scroll_direction_toggle_released(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static const struct behavior_parameter_metadata metadata = {
    .sets_len = 0,
    .sets = NULL,
};
#endif

static const struct behavior_driver_api scroll_direction_toggle_driver_api = {
    .binding_pressed = on_scroll_direction_toggle_pressed,
    .binding_released = on_scroll_direction_toggle_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

#define SCROLL_DIRECTION_TOGGLE_INST(n)                                                            \
    static const struct scroll_direction_toggle_config scroll_direction_toggle_config_##n = {       \
        .processor_name = DT_INST_PROP(n, processor_name),                                         \
        .secondary_processor_name = DT_INST_PROP(n, secondary_processor_name),                     \
    };                                                                                              \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &scroll_direction_toggle_config_##n, POST_KERNEL, \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                    \
                            &scroll_direction_toggle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCROLL_DIRECTION_TOGGLE_INST)
