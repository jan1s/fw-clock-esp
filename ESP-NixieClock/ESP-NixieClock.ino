/*----------------------------------------------------------------------------------------------------------------------------------------
 * ESP-WordClock.ino - some ESP8266 network routines with communication interface via UART to WordClock (STM32)
 *
 * Copyright (c) 2016 Frank Meyer - frank(at)fli4l.de
 *              modified by jan1s - jan1s.coding(at)gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arduino Settings ("Werkzeuge"):
 *     Platine:                 Generic ESP8266 Module
 *     Flash Mode:              DIO
 *     Flash Frequency          40 MHz
 *     Upload Using             Serial
 *     CPU Frequency            80 MHz
 *     Flash Size               512K (64K SPIFFS)
 *     Reset Method             nodemcu, if connected to STM32 (wordclock)
 *     Upload Speed             115200
 * 
 * Commands:
 *    cap apname,key            - connect to AP
 *    ap apname,key             - start local AP
 *    time [timeserver]         - get time from timeserver
 *    weather appid,city        - get weather of city
 *    weather appid,lon,lat     - get weather of location with coordinates (lon/lat)
 *
 * Return values:
 *    OK [string]               - Ok
 *    ERROR [string]            - Error
 * 
 * Examples:
 *    cap "fm7320","4711471147114711"
 *    ap "wordclock","1234567890"
 *    time "129.6.15.28"
 *    weather "123456789012345678901234567890","koeln"
 *    weather "123456789012345678901234567890","6.957","50.937"
 *    var "..."
 * 
 * Asynchronous Messages terminated with CR LF:
 *    - string                - Debug message, should be ignored
 *    FIRMWARE x.x.x          - Firmware version
 *    AP ssid                 - SSID of AP connected or own AP ssid
 *    MODE client             - working as WLAN client
 *    MODE ap                 - working as AP
 *    IPADDRESS x.x.x.x       - IP address of module is x.x.x.x
 *    TIME sec                - Time in seconds since 1900
 *    CMD xx ...              - Got command followed by parameters (printed in hex)
 *    HTTP GET path [param]   - HTTP request with path and optional params. Waits for answer lines until single dot (".") arrives.
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "base.h"
#include "wifi.h"
#include "http.h"
#include "udpsrv.h"
#include "ntp.h"
#include "vars.h"
#include "version.h"

extern "C" {
#include "user_interface.h"
}

#define CMD_BUFFER_SIZE     128                                             // maximum size of command buffer

bool ntp_success;

/*----------------------------------------------------------------------------------------------------------------------------------------
 * global setup
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
setup() 
{
    Serial.begin(115200);
    delay(1);
    Serial.println ("");
    Serial.flush ();
    delay(1000);

    ntp_success = false;

    pinMode(4, OUTPUT);    
  
    ntp_setup ();
    udp_server_setup ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * main loop
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
loop() 
{
    static char cmd_buffer[CMD_BUFFER_SIZE];
    static int  cmd_len = 0;

    wifi_check_if_started ();
    
    http_server_loop ();
    udp_server_loop ();
    ntp_poll_time ();                                                       // poll NTP

    static uint32_t last_ntp_update = 0;
    uint32_t esp_rtc = system_get_time() / 1000000;
    if (!ntp_success)
    {
        if (wifi_connected() && esp_rtc - last_ntp_update > 1)             // auto poll NTP
        {
            digitalWrite(4, HIGH);
            ntp_get_time ();
            last_ntp_update = esp_rtc;
            digitalWrite(4, LOW);
        }
    }
    else
    {
        if (wifi_connected() && esp_rtc - last_ntp_update > 600)           // auto poll NTP
        {
            digitalWrite(4, HIGH);
            ntp_get_time ();
            last_ntp_update = esp_rtc;
            digitalWrite(4, LOW);
        }
    }
    

    while (Serial.available())
    {
        char ch = Serial.read ();
    
        if (ch == '\n')
        {
            cmd_buffer[cmd_len] = '\0';

            if (! strncmp (cmd_buffer, "var ", 4))
            {
                char * parameter;

                parameter = cmd_buffer + 4;

                var_set_parameter (parameter);
                Serial.println (".");                                       // "silent" OK
                Serial.flush ();
            }
            else if (! strcmp (cmd_buffer, "time"))
            {
                ntp_get_time ();
            }
            else if (! strncmp (cmd_buffer, "time ", 5))
            {
                int       syntax_ok = false;
                char *    p = cmd_buffer + 5;
                char *    pp;

                if (*p == '"')
                {
                    pp = strchr (p + 1, '"');

                    if (pp)
                    {
                        *pp = '\0';

                        ntp_get_time (p + 1);
                        syntax_ok = true;
                    }
                }

                if (! syntax_ok)
                {
                    Serial.print ("ERROR syntax error");
                    Serial.flush ();
                }
            }
            else if (! strncmp (cmd_buffer, "ap ", 3))
            {
                int     syntax_ok = false;
                char *  ssid;
                char *  key;
                char *  p = cmd_buffer + 3;
                char *  pp;

                if (*p == '"')
                {
                    pp = strchr (p + 1, '"');

                    if (pp)
                    {
                        *pp = '\0';
                        ssid = p + 1;
                        p = pp + 1;

                        if (*p == ',' && *(p + 1) == '"')
                        {
                            pp = strchr (p + 2, '"');

                            if (pp)
                            {
                                syntax_ok = true;
                                *pp = '\0';
                                key = p + 2;

                                wifi_ap (ssid, key);
                            }
                        }
                    }
                }

                if (! syntax_ok)
                {
                    Serial.print ("ERROR syntax error");
                    Serial.flush ();
                }
            }
            else if (! strncmp (cmd_buffer, "cap ", 4))
            {
                int     syntax_ok = false;
                char *  ssid;
                char *  key;
                char *  p = cmd_buffer + 4;
                char *  pp;

                if (*p == '"')
                {
                    pp = strchr (p + 1, '"');

                    if (pp)
                    {
                        *pp = '\0';
                        ssid = p + 1;
                        p = pp + 1;
                        if (*p == ',' && *(p + 1) == '"')
                        {
                            pp = strchr (p + 2, '"');

                            if (pp)
                            {
                                syntax_ok = true;
                                *pp = '\0';
                                key = p + 2;

                                wifi_connect (ssid, key);
                            }
                        }
                    }
                }

                if (! syntax_ok)
                {
                    Serial.print ("ERROR syntax error");
                    Serial.flush ();
                }
            }
            else
            {
                Serial.print ("ERROR invalid command");
                Serial.flush ();
            }

            cmd_buffer[0] = '\0';
            cmd_len = 0;
        }
        else
        {
            if (ch != '\r')
            {
                if (cmd_len < CMD_BUFFER_SIZE - 1)
                {
                    cmd_buffer[cmd_len++] = ch;
                }
            }
        }
    }
}
