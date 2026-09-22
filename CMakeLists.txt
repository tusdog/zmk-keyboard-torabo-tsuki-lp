if(CONFIG_SHIELD_TORABO_TSUKI_LP_LEFT OR CONFIG_SHIELD_TORABO_TSUKI_LP_RIGHT)
  zephyr_library()
  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

  # Stability build:
  # Do not compile src/board.c. That file dynamically renegotiates the
  # split-BLE connection interval after 5/15/30 seconds of inactivity.
  # Keep the split link on ZMK's standard fixed connection parameters.
  zephyr_library_sources(src/mini_trackpad_init_reg.c)
endif()