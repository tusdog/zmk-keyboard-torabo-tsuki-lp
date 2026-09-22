// SPDX-License-Identifier: GPL-3.0-or-later

#define DT_DRV_COMPAT zmk_behavior_scroll_direction_toggle

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <torabo/scroll_reverse.h>

static int on_scroll_direction_toggle_pressed(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    torabo_scroll_reverse_toggle();
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
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                               \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                    \
                            &scroll_direction_toggle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCROLL_DIRECTION_TOGGLE_INST)
