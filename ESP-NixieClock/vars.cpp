/*----------------------------------------------------------------------------------------------------------------------------------------
 * vars.cpp - synchronization of variables between STM32 and ESP8266
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
#include "Arduino.h"
#include "base.h"
#include "vars.h"

uint8_t cmd_rtc_write (uint16_t yr, uint8_t mon, uint8_t day, uint8_t hr, uint8_t minute, uint8_t sec)
{
    Serial.printf ("rtc_write %u %u %u %u %u %u\r\n", (unsigned int) yr, (unsigned int) mon, (unsigned int) day, (unsigned int) hr, (unsigned int) minute, (unsigned int) sec);
    Serial.flush ();

    return 1;
}

uint8_t cmd_tz_write (bool std, uint16_t offset, uint8_t hour, uint8_t dow, uint8_t week, uint8_t month)
{
    if(std)
    {
      Serial.printf ("tz_write %s %u %u %u %u\r\n", "std", (unsigned int) offset, (unsigned int) hour, (unsigned int) dow, (unsigned int) week, (unsigned int) month);
    }
    else
    {
      Serial.printf ("tz_write %s %u %u %u %u\r\n", "dst", (unsigned int) offset, (unsigned int) hour, (unsigned int) dow, (unsigned int) week, (unsigned int) month);
    }
    Serial.flush ();

    return 1;
}

uint8_t cmd_nixie_setmode (uint8_t var)
{
    unsigned int   rtc = 0;

    if (var < MAX_DISPLAY_MODE_VARIABLES)
    {
        Serial.printf ("cmd_nixie_set_mode %02x\r\n", (int) var);
        Serial.flush ();
        rtc =  1;
    }

    return rtc;
}

uint8_t cmd_nixie_settype (uint8_t var)
{
    unsigned int   rtc = 0;

    if (var < MAX_DISPLAY_TYPE_VARIABLES)
    {
        Serial.printf ("cmd_nixie_set_type %02x\r\n", (int) var);
        Serial.flush ();
        rtc =  1;
    }

    return rtc;
}

DISPLAY_MODE * get_display_mode_var (DISPLAY_MODE_VARIABLE var)
{
    DISPLAY_MODE *   rtc = (DISPLAY_MODE *) 0;

    if (var < MAX_DISPLAY_MODE_VARIABLES)
    {
        rtc = &(displaymodevars[var]);
    }

    return rtc;
}



// ---------------------------------

unsigned int
rpc (RPC_VARIABLE var)
{
    unsigned int   rtc = 0;

    if (var < MAX_RPC_VARIABLES)
    {
        Serial.printf ("CMD R%02x\r\n", (int) var);
        Serial.flush ();
        rtc = 1;
    }

    return rtc;
}

unsigned int     numvars[MAX_NUM_VARIABLES];

unsigned int
get_numvar (NUM_VARIABLE var)
{
    unsigned int   rtc = 0;

    if (var < MAX_NUM_VARIABLES)
    {
        rtc =  numvars[var];
    }

    return rtc;
}

unsigned int
set_numvar (NUM_VARIABLE var, unsigned int value)
{
    unsigned int   rtc = 0;

    if (var < MAX_NUM_VARIABLES)
    {
        numvars[var] = value;
        Serial.printf ("CMD N%02x%02x%02x\r\n", (int) var, value & 0xFF, (value >> 8) & 0xFF);
        Serial.flush ();
        rtc = 1;
    }

    return rtc;
}

static char ticker_text[MAX_TICKER_TEXT_LEN + 1];
static char version[MAX_VERSION_TEXT_LEN + 1];
static char eeprom_version[MAX_EEPROM_VERSION_TEXT_LEN + 1];
static char esp8266_version[MAX_ESP8266_VERSION_TEXT_LEN + 1];
static char timeserver_name[MAX_TIMESERVER_NAME_LEN + 1];
static char weather_appid[MAX_WEATHER_APPID_LEN + 1];
static char weather_city[MAX_WEATHER_CITY_LEN + 1];
static char weather_lon[MAX_WEATHER_LON_LEN + 1];
static char weather_lat[MAX_WEATHER_LAT_LEN + 1];

STR_VAR strvars[MAX_STR_VARIABLES] =
{
    { ticker_text,      MAX_TICKER_TEXT_LEN },
    { version,          MAX_VERSION_TEXT_LEN },
    { eeprom_version,   MAX_EEPROM_VERSION_TEXT_LEN },
    { esp8266_version,  MAX_ESP8266_VERSION_TEXT_LEN },
    { timeserver_name,  MAX_TIMESERVER_NAME_LEN },
    { weather_appid,    MAX_WEATHER_APPID_LEN },
    { weather_city,     MAX_WEATHER_CITY_LEN },
    { weather_lon,     MAX_WEATHER_LON_LEN },
    { weather_lat,     MAX_WEATHER_LAT_LEN },
};


STR_VAR *
get_strvar (STR_VARIABLE var)
{
    STR_VAR *   rtc = (STR_VAR *) 0;

    if (var < MAX_STR_VARIABLES)
    {
        rtc = &(strvars[var]);
    }

    return rtc;
}

unsigned int
set_strvar (STR_VARIABLE var, char * p)
{
    unsigned int   rtc = 0;

    if (var < MAX_STR_VARIABLES)
    {
        strncpy (strvars[var].str, p, strvars[var].maxlen);
        Serial.printf ("CMD S%02x%s\r\n", (int) var, p);
        Serial.flush ();
        rtc =  1;
    }

    return rtc;
}

TM tmvars[MAX_TM_VARIABLES];

TM *
get_tm_var (TM_VARIABLE var)
{
    TM *   rtc = (TM *) 0;

    if (var < MAX_TM_VARIABLES)
    {
        rtc = &(tmvars[var]);
    }

    return rtc;

}

unsigned int
set_tm_var (TM_VARIABLE var, TM * tm)
{
    unsigned int   rtc = 0;

    if (var < MAX_TM_VARIABLES)
    {
        memcpy (&tmvars[var], tm, sizeof (TM));
        Serial.printf ("CMD T%02x%04d%02d%02d%02d%02d%02d\r\n", (int) var, tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        Serial.flush ();
        rtc =  1;
    }

    return rtc;
}

DISPLAY_MODE displaymodevars[MAX_DISPLAY_MODE_VARIABLES];




NIGHT_TIME nighttimevars[MAX_NIGHT_TIME_VARIABLES];


NIGHT_TIME *
get_night_time_var (NIGHT_TIME_VARIABLE var)
{
    NIGHT_TIME *   rtc = (NIGHT_TIME *) 0;

    if (var < MAX_NIGHT_TIME_VARIABLES)
    {
        rtc = &(nighttimevars[var]);
    }

    return rtc;
}

unsigned int
set_night_time_var (NIGHT_TIME_VARIABLE var, uint_fast16_t minutes, uint_fast8_t flags)
{
    unsigned int   rtc = 0;

    if (var < MAX_NIGHT_TIME_VARIABLES)
    {
        nighttimevars[var].minutes = minutes;
        nighttimevars[var].flags = flags;
        Serial.printf ("CMD t%02x%02x%02x%02x\r\n", (int) var, minutes & 0xFF, (minutes >> 8) & 0xFF, flags);
        Serial.flush ();
        rtc =  1;
    }

    return rtc;
}

void
var_set_parameter (char * parameters)
{
    uint_fast8_t        cmd_code;
    uint_fast8_t        var_idx;

#ifdef DEBUG
    debugmsg ("VAR", parameters); 
#endif

    cmd_code    = *parameters++;

    switch (cmd_code)
    {
        case 'N':                                                   // numeric variable: Niillhh
        {
            uint_fast8_t    lo;
            uint_fast8_t    hi;
            uint_fast16_t   val;

            var_idx = htoi (parameters, 2);
            parameters += 2;
            lo      = htoi (parameters, 2);
            parameters += 2;
            hi      = htoi (parameters, 2);
            parameters += 2;
            val     = (hi << 8) | lo;

            if (var_idx < MAX_NUM_VARIABLES)
            {
                numvars[var_idx] = val;
            }
            break;
        }

        case 'S':                                               // string variable: Siissssssss...
        {
            var_idx = htoi (parameters, 2);
            parameters += 2;

            if (var_idx < MAX_STR_VARIABLES)
            {
                strncpy (strvars[var_idx].str, parameters, strvars[var_idx].maxlen);
            }
            break;
        }

        case 'T':
        {
            var_idx = htoi (parameters, 2);
            parameters += 2;

            if (var_idx < MAX_TM_VARIABLES)
            {
                tmvars[var_idx].tm_year = 1000 * (parameters[0]  - '0') +
                                           100 * (parameters[1]  - '0') +
                                            10 * (parameters[2]  - '0') +
                                             1 * (parameters[3]  - '0');
                tmvars[var_idx].tm_mon  =   10 * (parameters[4]  - '0') +
                                             1 * (parameters[5]  - '0');
                tmvars[var_idx].tm_mday =   10 * (parameters[6]  - '0') +
                                             1 * (parameters[7]  - '0');
                tmvars[var_idx].tm_hour =   10 * (parameters[8]  - '0') +
                                             1 * (parameters[9]  - '0');
                tmvars[var_idx].tm_min  =   10 * (parameters[10] - '0') +
                                             1 * (parameters[11] - '0');
                tmvars[var_idx].tm_sec =    10 * (parameters[12] - '0') +
                                             1 * (parameters[13] - '0');

                tmvars[var_idx].tm_wday = dayofweek (tmvars[var_idx].tm_mday, tmvars[var_idx].tm_mon + 1, tmvars[var_idx].tm_year + 1900);
            }

            break;
        }

        case 'D':                                               // display
        {
            cmd_code = *parameters++;
            var_idx = htoi (parameters, 2);
            parameters += 2;

            switch (cmd_code)
            {
                case 'N':                                       // DN: Display mode Name
                {
                    if (var_idx < MAX_DISPLAY_MODE_VARIABLES)
                    {
                        strncpy (displaymodevars[var_idx].name, parameters, MAX_DISPLAY_MODE_NAME_LEN);
                    }
                    break;
                }

             
            }
            break;
        }

        case 't':                                                               // tiimm night table minutes
        {
            uint_fast16_t minutes;
            uint_fast8_t flags;

            var_idx = htoi (parameters, 2);
            parameters += 2;
            minutes = htoi (parameters, 2) + (htoi (parameters + 2, 2) << 8);
            parameters += 4;
            flags = htoi (parameters, 2);
            parameters += 2;

            if (var_idx < MAX_NIGHT_TIME_VARIABLES)
            {
                nighttimevars[var_idx].minutes = minutes;
                nighttimevars[var_idx].flags = flags;
            }

            break;
        }
    }
}

