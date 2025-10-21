# CMakeLists.txt (module root)

if (CONFIG_ZMK_BATTERY_MONITOR)
    zephyr_library()
    zephyr_library_sources(src/battery_monitor.c)
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
