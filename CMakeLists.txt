if(CONFIG_SHIELD_TORABO_TSUKI_LP_LEFT OR CONFIG_SHIELD_TORABO_TSUKI_LP_RIGHT)
  zephyr_library()
  zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${APPLICATION_SOURCE_DIR}/include)

  # Stability build:
  # Do not compile src/board.c. That file dynamically renegotiates the
  # split-BLE connection interval after 5/15/30 seconds of inactivity.
  # Keep the split link on ZMK's standard fixed connection parameters.
  zephyr_library_sources(
    src/mini_trackpad_init_reg.c
    src/behavior_scroll_direction_toggle.c
    src/input_processor_scroll_reverse.c
  )

  # Keymap behaviors execute on the split central. Peripheral-only builds do
  # not link the keycode event implementation used by this behavior.
  zephyr_library_sources_ifdef(CONFIG_ZMK_SPLIT_ROLE_CENTRAL src/behavior_single_double_tap.c)
endif()