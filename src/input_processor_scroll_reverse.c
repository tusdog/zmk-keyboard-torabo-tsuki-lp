// SPDX-License-Identifier: GPL-3.0-or-later

#define DT_DRV_COMPAT zmk_input_processor_scroll_reverse

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/input_processor.h>
#include <torabo/scroll_reverse.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static bool scroll_reversed;

bool torabo_scroll_reverse_toggle(void) {
    scroll_reversed = !scroll_reversed;
    LOG_INF("Vertical scroll direction: %s", scroll_reversed ? "reversed" : "normal");
    return scroll_reversed;
}

bool torabo_scroll_reverse_get(void) { return scroll_reversed; }

static int scroll_reverse_handle_event(const struct device *dev, struct input_event *event,
                                       uint32_t param1, uint32_t param2,
                                       struct zmk_input_processor_state *state) {
    ARG_UNUSED(dev);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    ARG_UNUSED(state);

    if (scroll_reversed && event->type == INPUT_EV_REL && event->code == INPUT_REL_WHEEL) {
        event->value = -event->value;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api scroll_reverse_driver_api = {
    .handle_event = scroll_reverse_handle_event,
};

#define SCROLL_REVERSE_INST(n)                                                                    \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                 \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &scroll_reverse_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCROLL_REVERSE_INST)
