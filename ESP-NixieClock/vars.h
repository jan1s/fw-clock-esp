/*----------------------------------------------------------------------------------------------------------------------------------------
 * vars.h - synchronization of variables between STM32 and ESP8266
 *
 * Copyright (c) 2016 Frank Meyer - frank(at)fli4l.de
*              modified by jan1s - jan1s.coding@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
#ifndef VARS_H
#define VARS_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#define COLOR_ANIMATION_MODE_NONE                   0                   // no color animation
#define COLOR_ANIMATION_MODE_RAINBOW                1                   // rainbow

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * remote procedure calls:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
typedef enum
{
    LDR_MIN_VALUE_RPC_VAR,                                              // store LDR value as minimum value
    LDR_MAX_VALUE_RPC_VAR,                                              // store LDR value as maximum value
    LEARN_IR_RPC_VAR,                                                   // learn ir remote control
    GET_NET_TIME_RPC_VAR,                                               // get net time
    DISPLAY_TEMPERATURE_RPC_VAR,                                        // display current temperature
    TEST_DISPLAY_RPC_VAR,                                               // test display
    GET_WEATHER_RPC_VAR,                                                // get weather
    MAX_RPC_VARIABLES                                                   // must be the last member
} RPC_VARIABLE;

extern unsigned int     rpc (RPC_VARIABLE);

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * numeric variables:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
typedef enum
{
    DISPLAY_USE_RGBW_NUM_VAR,
    EEPROM_IS_UP_NUM_VAR,
    RTC_IS_UP_NUM_VAR,
    DISPLAY_POWER_NUM_VAR,
    DISPLAY_MODE_NUM_VAR,
    MAX_DISPLAY_MODES_NUM_VAR,
    DISPLAY_BRIGHTNESS_NUM_VAR,
    DISPLAY_FLAGS_NUM_VAR,
    DISPLAY_AUTOMATIC_BRIGHTNESS_ACTIVE_NUM_VAR,
    DISPLAY_TEMPERATURE_INTERVAL_NUM_VAR,
    ANIMATION_MODE_NUM_VAR,
    AMBILIGHT_MODE_NUM_VAR,
    AMBILIGHT_LEDS_NUM_VAR,
    AMBILIGHT_OFFSET_NUM_VAR,
    AMBILIGHT_BRIGHTNESS_NUM_VAR,
    COLOR_ANIMATION_MODE_NUM_VAR,
    LDR_RAW_VALUE_NUM_VAR,
    LDR_MIN_VALUE_NUM_VAR,
    LDR_MAX_VALUE_NUM_VAR,
    TIMEZONE_NUM_VAR,
    DS18XX_IS_UP_NUM_VAR,
    RTC_TEMP_INDEX_NUM_VAR,
    RTC_TEMP_CORRECTION_NUM_VAR,
    DS18XX_TEMP_INDEX_NUM_VAR,
    DS18XX_TEMP_CORRECTION_NUM_VAR,
    DISPLAY_HEART_INTERVAL_NUM_VAR,
    DISPLAY_XMAS_TREE_INTERVAL_NUM_VAR,
    MAX_NUM_VARIABLES                                                   // must be the last member
} NUM_VARIABLE;

extern unsigned int     numvars[MAX_NUM_VARIABLES];
extern unsigned int     get_numvar (NUM_VARIABLE);
extern unsigned int     set_numvar (NUM_VARIABLE, unsigned int);

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * string variables:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define MAX_TICKER_TEXT_LEN             32
#define MAX_VERSION_TEXT_LEN            8
#define MAX_EEPROM_VERSION_TEXT_LEN     8
#define MAX_ESP8266_VERSION_TEXT_LEN    8
#define MAX_TIMESERVER_NAME_LEN         16
#define MAX_WEATHER_APPID_LEN           32
#define MAX_WEATHER_CITY_LEN            32
#define MAX_WEATHER_LON_LEN             8
#define MAX_WEATHER_LAT_LEN             8

typedef struct
{
    char *          str;
    uint_fast16_t   maxlen;
} STR_VAR;

typedef enum
{
    TICKER_TEXT_STR_VAR,
    VERSION_STR_VAR,
    EEPROM_VERSION_STR_VAR,
    ESP8266_VERSION_STR_VAR,
    TIMESERVER_STR_VAR,
    WEATHER_APPID_STR_VAR,
    WEATHER_CITY_STR_VAR,
    WEATHER_LON_STR_VAR,
    WEATHER_LAT_STR_VAR,
    MAX_STR_VARIABLES                                                   // must be the last member
} STR_VARIABLE;

extern STR_VAR          strvars[MAX_STR_VARIABLES];
extern STR_VAR *        get_strvar (STR_VARIABLE);
extern unsigned int     set_strvar (STR_VARIABLE, char *);

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * tm variables:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
typedef struct tm TM;

typedef enum
{
    CURRENT_TM_VAR,
    MAX_TM_VARIABLES                                       // must be the last member
} TM_VARIABLE;

extern TM               tmvars[MAX_TM_VARIABLES];
extern TM *             get_tm_var (TM_VARIABLE);
extern unsigned int     set_tm_var (TM_VARIABLE, TM *);

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * display mode variables:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define MAX_DISPLAY_MODE_NAME_LEN   32

typedef struct
{
    char name[MAX_DISPLAY_MODE_NAME_LEN + 1];
} DISPLAY_MODE;

typedef enum
{
    DISPLAY_MODE_0_VAR,
    DISPLAY_MODE_1_VAR,
    DISPLAY_MODE_2_VAR,
    DISPLAY_MODE_3_VAR,
    MAX_DISPLAY_MODE_VARIABLES                                       // must be the last member
} DISPLAY_MODE_VARIABLE;

typedef enum
{
    DISPLAY_TYPE_0_VAR,
    DISPLAY_TYPE_1_VAR,
    DISPLAY_TYPE_2_VAR,
    DISPLAY_TYPE_3_VAR,
    MAX_DISPLAY_TYPE_VARIABLES                                       // must be the last member
} DISPLAY_TYPE_VARIABLE;

extern DISPLAY_MODE      displaymodevars[MAX_DISPLAY_MODE_VARIABLES];
extern DISPLAY_MODE *    get_display_mode_var (DISPLAY_MODE_VARIABLE);
extern unsigned int      set_display_mode (DISPLAY_MODE_VARIABLE);
extern unsigned int      set_display_type (DISPLAY_TYPE_VARIABLE);

extern uint8_t cmd_rtc_write (uint16_t yr, uint8_t mon, uint8_t day, uint8_t hr, uint8_t minute, uint8_t sec);
extern uint8_t cmd_tz_write (bool std, uint16_t offset, uint8_t hour, uint8_t dow, uint8_t week, uint8_t month);
extern uint8_t cmd_nixie_setmode (uint8_t var);
extern uint8_t cmd_nixie_settype (uint8_t var);


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * night time variables:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */

#define NIGHT_TIME_FLAG_ACTIVE      0x80                                    // flag if entry is active (1) or not (0)
#define NIGHT_TIME_FLAG_SWITCH_ON   0x40                                    // flag if entry switches on (1) or off (0)
#define NIGHT_TIME_FROM_DAY_MASK    0x38                                    // 3 bits for from day
#define NIGHT_TIME_TO_DAY_MASK      0x07                                    // 3 bits for to day

typedef struct
{
    uint_fast8_t        flags;                                              // flags
    uint_fast16_t       minutes;                                            // time in minutes 0 - 1439
} NIGHT_TIME;

typedef enum
{
    NIGHT0_NIGHT_TIME_VAR,
    NIGHT1_NIGHT_TIME_VAR,
    NIGHT2_NIGHT_TIME_VAR,
    NIGHT3_NIGHT_TIME_VAR,
    NIGHT4_NIGHT_TIME_VAR,
    NIGHT5_NIGHT_TIME_VAR,
    NIGHT6_NIGHT_TIME_VAR,
    NIGHT7_NIGHT_TIME_VAR,
    MAX_NIGHT_TIME_VARIABLES                                                // must be the last member
} NIGHT_TIME_VARIABLE;

extern NIGHT_TIME           nighttimevars[MAX_NIGHT_TIME_VARIABLES];
extern NIGHT_TIME *         get_night_time_var (NIGHT_TIME_VARIABLE);
extern unsigned int         set_night_time_var (NIGHT_TIME_VARIABLE, uint_fast16_t, uint_fast8_t);

extern void                 var_set_parameter (char *);
#endif
