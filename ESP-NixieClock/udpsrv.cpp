/*----------------------------------------------------------------------------------------------------------------------------------------
 * udpsrv.cpp - UDP server for Android App
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
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include "udpsrv.h"
#include "vars.h"

#define MAX_UDP_PACKET_SIZE        100

#define LISTENER_SET_COLOR_CODE                 'C'                             // set display color
#define LISTENER_SET_AMBILIGHT_COLOR_CODE       'c'                             // set ambilight color
#define LISTENER_POWER_CODE                     'P'                             // power on/off
#define LISTENER_DISPLAY_MODE_CODE              'D'                             // set display mode
#define LISTENER_AMBILIGHT_MODE_CODE            'd'                             // set ambilight mode
#define LISTENER_ANIMATION_MODE_CODE            'A'                             // set animation mode
#define LISTENER_COLOR_ANIMATION_MODE_CODE      'F'                             // set color animation mode
#define LISTENER_DISPLAY_TEMPERATURE_CODE       'W'                             // display temperature
#define LISTENER_SET_DISPLAY_FLAGS_CODE         'G'                             // set display flags
#define LISTENER_SET_BRIGHTNESS_CODE            'B'                             // set brightness
#define LISTENER_SET_AMBILIGHT_BRIGHTNESS_CODE  'b'                             // set ambilight brightness
#define LISTENER_SET_AUTOMATIC_BRIHGHTNESS_CODE 'L'                             // automatic brightness control on/off
#define LISTENER_TEST_DISPLAY_CODE              'X'                             // test display
#define LISTENER_SET_DATE_TIME_CODE             'T'                             // set date/time
#define LISTENER_SET_NET_DATE_TIME_CODE         't'                             // set date/time (timeserver)
#define LISTENER_GET_NET_TIME_CODE              'N'                             // Get net time
#define LISTENER_IR_LEARN_CODE                  'I'                             // IR learn
#define LISTENER_SET_NIGHT_TIME                 'J'                             // set night off time
#define LISTENER_SAVE_DISPLAY_CONFIGURATION     'S'                             // save display configuration
#define LISTENER_PRINT_TICKER_CODE              'p'                             // print ticker

static WiFiUDP         server_udp;
static unsigned int    server_udp_local_port = 2424;

/*----------------------------------------------------------------------------------------------------------------------------------------
 * setup udp service
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
udp_server_setup (void)
{
    Serial.println("- setup server UDP");
    server_udp.begin(server_udp_local_port);
    Serial.print("- local port: ");
    Serial.println(server_udp.localPort());
    Serial.flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * check for incoming udp packet
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
udp_server_loop (void)
{
    char udp_server_packet_buffer[MAX_UDP_PACKET_SIZE];

    int noBytes = server_udp.parsePacket();
 
    if (noBytes)
    {
#ifdef DEBUG
        Serial.print ("- ");
        Serial.print(millis() / 1000);
        Serial.print("sec: Packet of ");
        Serial.print(noBytes);
        Serial.print(" bytes received from ");
        Serial.print(server_udp.remoteIP());
        Serial.print(":");
        Serial.println(server_udp.remotePort());
        Serial.flush ();
#endif

        if (noBytes <= MAX_UDP_PACKET_SIZE)
        {
            server_udp.read (udp_server_packet_buffer, noBytes);            // read packet

            switch (udp_server_packet_buffer[0])
            {
                case LISTENER_DISPLAY_MODE_CODE:                            // set display mode
                {
                    if (noBytes == 2)
                    {
                        set_numvar (DISPLAY_MODE_NUM_VAR, udp_server_packet_buffer[1]);
                    }
                    break;
                }

                case LISTENER_SET_DATE_TIME_CODE:                           // set Date/Time
                {
                    if (noBytes == 7)
                    {
                        tm t;

                        t.tm_year  = udp_server_packet_buffer[1] + 100;           // tm: year since 1900  | UDP packet buffer: year since 2000
                        t.tm_mon   = udp_server_packet_buffer[2] - 1;             // tm: month 0..11      | UDP packet buffer: month 1..12
                        t.tm_mday  = udp_server_packet_buffer[3];                 // tm: day 1..31        | UDP packet buffer: day 1..31
                        t.tm_hour  = udp_server_packet_buffer[4];                 // tm: hour 1..24       | UDP packet buffer: hour 1..31
                        t.tm_min   = udp_server_packet_buffer[5];                 // tm: minute 1..59     | UDP packet buffer: minute 1..31
                        t.tm_sec   = udp_server_packet_buffer[6];                 // tm: sec 1..59        | UDP packet buffer: sec 1..59

                        //set_tm_var (CURRENT_TM_VAR, &tm);
                    }
                    break;
                }

                case LISTENER_SET_DISPLAY_FLAGS_CODE:                       // set display flags
                {
                    if (noBytes == 2)
                    {
                        set_numvar (DISPLAY_FLAGS_NUM_VAR, udp_server_packet_buffer[1]);
                    }
                    break;
                }

                case LISTENER_POWER_CODE:                                   // power on/off
                {
                    if (noBytes == 2)
                    {
                        set_numvar (DISPLAY_POWER_NUM_VAR, udp_server_packet_buffer[1]);
                    }
                    break;
                }

                case LISTENER_GET_NET_TIME_CODE:                            // get net time
                {
                    if (noBytes == 1)
                    {
                        rpc (GET_NET_TIME_RPC_VAR);
                    }
                    break;
                }

                case LISTENER_PRINT_TICKER_CODE:                            // print ticker
                {
                    udp_server_packet_buffer[MAX_TICKER_TEXT_LEN + 1] = '\0';           // terminate ticker text!
                    set_strvar (TICKER_TEXT_STR_VAR, udp_server_packet_buffer + 1);
                }
            }
        }
        else
        {
            Serial.print ("- UDP packet too large: ");
            Serial.print (noBytes);
            Serial.print (" bytes, max is ");
            Serial.println (MAX_UDP_PACKET_SIZE);
        }

        Serial.flush ();
    }
}

