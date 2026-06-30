file(GLOB_RECURSE SOURCES_C *.c)
file(GLOB_RECURSE SOURCES_CPP *.cpp)

if (CONFIG_OBD_BOARD_WS_175_AMOLED)
    list(REMOVE_ITEM SOURCES_C
        "${CMAKE_CURRENT_LIST_DIR}/bsp_obd_dsp/ads1115_oil_pressure.c"
        "${CMAKE_CURRENT_LIST_DIR}/bsp_obd_dsp/boards/board_ws_185.c"
        "${CMAKE_CURRENT_LIST_DIR}/bsp_obd_dsp/exio/TCA9554PWR.c"
        "${CMAKE_CURRENT_LIST_DIR}/bsp_obd_dsp/i2c_driver/I2C_Driver.c"
        "${CMAKE_CURRENT_LIST_DIR}/bsp_obd_dsp/lcd_driver/ST77916.c"
        "${CMAKE_CURRENT_LIST_DIR}/bsp_obd_dsp/lcd_driver/esp_lcd_st77916/esp_lcd_st77916.c"
        "${CMAKE_CURRENT_LIST_DIR}/bsp_obd_dsp/touch_driver/CST816.c"
    )
elseif (CONFIG_OBD_BOARD_WS_185)
    list(REMOVE_ITEM SOURCES_C
        "${CMAKE_CURRENT_LIST_DIR}/bsp_obd_dsp/boards/board_ws_175_amoled.c"
    )
endif()

idf_component_register(SRCS
                    ${SOURCES_C}
                    ${SOURCES_CPP}
                    INCLUDE_DIRS
                    ".")

