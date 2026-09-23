/*
 * Configurable single/double tap behavior for DYA Studio.
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_single_double_tap

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define SINGLE_DOUBLE_TAP_MAX_ACTIVE 8
#define SINGLE_DOUBLE_TAP_POSITION_FREE UINT32_MAX

struct behavior_single_double_tap_config {
    uint32_t tapping_term_ms;
};

struct active_single_double_tap {
    uint32_t position;
    uint32_t single_key;
    uint32_t double_key;
    uint32_t selected_key;
    uint8_t tap_count;
    bool is_pressed;
    bool decided;
    int64_t release_at;
    struct k_work_delayable timer;
};

static struct active_single_double_tap active_taps[SINGLE_DOUBLE_TAP_MAX_ACTIVE];

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata single_param_values[] = {
    {
        .display_name = "Single Tap",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE,
    },
};

static const struct behavior_parameter_value_metadata double_param_values[] = {
    {
        .display_name = "Double Tap",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE,
    },
};

static const struct behavior_parameter_metadata_set parameter_sets[] = {{
    .param1_values = single_param_values,
    .param1_values_len = ARRAY_SIZE(single_param_values),
    .param2_values = double_param_values,
    .param2_values_len = ARRAY_SIZE(double_param_values),
}};

static const struct behavior_parameter_metadata parameter_metadata = {
    .sets = parameter_sets,
    .sets_len = ARRAY_SIZE(parameter_sets),
};

#endif

static struct active_single_double_tap *find_active(uint32_t position) {
    for (int i = 0; i < ARRAY_SIZE(active_taps); i++) {
        if (active_taps[i].position == position) {
            return &active_taps[i];
        }
    }

    return NULL;
}

static void clear_active(struct active_single_double_tap *tap) {
    tap->position = SINGLE_DOUBLE_TAP_POSITION_FREE;
    tap->tap_count = 0;
    tap->is_pressed = false;
    tap->decided = false;
    tap->selected_key = 0;
    tap->release_at = 0;
}

static struct active_single_double_tap *allocate_active(
    const struct zmk_behavior_binding *binding, uint32_t position) {
    for (int i = 0; i < ARRAY_SIZE(active_taps); i++) {
        struct active_single_double_tap *tap = &active_taps[i];

        if (tap->position != SINGLE_DOUBLE_TAP_POSITION_FREE) {
            continue;
        }

        tap->position = position;
        tap->single_key = binding->param1;
        tap->double_key = binding->param2;
        tap->selected_key = 0;
        tap->tap_count = 0;
        tap->is_pressed = false;
        tap->decided = false;
        tap->release_at = 0;
        return tap;
    }

    return NULL;
}

static int emit_key(uint32_t encoded_key, bool pressed, int64_t timestamp) {
    return raise_zmk_keycode_state_changed_from_encoded(encoded_key, pressed, timestamp);
}

static int decide_single(struct active_single_double_tap *tap, int64_t timestamp) {
    if (tap->decided) {
        return 0;
    }

    tap->decided = true;
    tap->selected_key = tap->single_key;

    int ret = emit_key(tap->selected_key, true, timestamp);
    if (ret < 0) {
        return ret;
    }

    if (!tap->is_pressed) {
        ret = emit_key(tap->selected_key, false, timestamp);
        clear_active(tap);
    }

    return ret;
}

static int decide_double(struct active_single_double_tap *tap, int64_t timestamp) {
    tap->decided = true;
    tap->selected_key = tap->double_key;
    return emit_key(tap->selected_key, true, timestamp);
}

static void single_double_tap_timer_handler(struct k_work *item) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(item);
    struct active_single_double_tap *tap =
        CONTAINER_OF(dwork, struct active_single_double_tap, timer);

    if (tap->position == SINGLE_DOUBLE_TAP_POSITION_FREE || tap->decided) {
        return;
    }

    decide_single(tap, tap->release_at);
}

static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_single_double_tap_config *cfg = dev->config;
    struct active_single_double_tap *tap = find_active(event.position);

    if (tap == NULL) {
        tap = allocate_active(binding, event.position);
        if (tap == NULL) {
            LOG_ERR("No free single/double tap state for position %u", event.position);
            return -ENOMEM;
        }
    }

    tap->is_pressed = true;
    k_work_cancel_delayable(&tap->timer);

    if (tap->tap_count < 2) {
        tap->tap_count++;
    }

    if (tap->tap_count >= 2) {
        return decide_double(tap, event.timestamp);
    }

    tap->release_at = event.timestamp + cfg->tapping_term_ms;
    int64_t remaining_ms = tap->release_at - k_uptime_get();
    if (remaining_ms < 1) {
        remaining_ms = 1;
    }

    k_work_schedule(&tap->timer, K_MSEC(remaining_ms));
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_binding_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);

    struct active_single_double_tap *tap = find_active(event.position);
    if (tap == NULL) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    tap->is_pressed = false;

    if (!tap->decided) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    int ret = emit_key(tap->selected_key, false, event.timestamp);
    clear_active(tap);
    return ret;
}

static int behavior_single_double_tap_init(const struct device *dev) {
    ARG_UNUSED(dev);

    static bool initialized;
    if (initialized) {
        return 0;
    }

    for (int i = 0; i < ARRAY_SIZE(active_taps); i++) {
        k_work_init_delayable(&active_taps[i].timer, single_double_tap_timer_handler);
        clear_active(&active_taps[i]);
    }

    initialized = true;
    return 0;
}

static const struct behavior_driver_api behavior_single_double_tap_driver_api = {
    .binding_pressed = on_binding_pressed,
    .binding_released = on_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &parameter_metadata,
#endif
};

#define SINGLE_DOUBLE_TAP_INST(n)                                                                  \
    static const struct behavior_single_double_tap_config single_double_tap_config_##n = {          \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                       \
    };                                                                                              \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_single_double_tap_init, NULL, NULL,                         \
                            &single_double_tap_config_##n, POST_KERNEL,                              \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                    \
                            &behavior_single_double_tap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SINGLE_DOUBLE_TAP_INST)

#endif
