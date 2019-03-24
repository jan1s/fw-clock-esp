/*----------------------------------------------------------------------------------------------------------------------------------------
 * ntp.cpp - NTP functions
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
#include <ctime>
#include "ntp.h"
#include "vars.h"
#include "base.h"

const int       NTP_PACKET_SIZE = 48;                                       // NTP time stamp is in the first 48 bytes of the message
byte            ntp_packet_buffer[NTP_PACKET_SIZE];                         // buffer to hold incoming and outgoing NTP packets

WiFiUDP         ntp_udp;
unsigned int    ntp_local_port = 2421;

tm tmvar;

/*----------------------------------------------------------------------------------------------------------------------------------------
 * setup ntp
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
ntp_setup (void)
{
    Serial.println("- setup UDP");
    ntp_udp.begin(ntp_local_port);
    Serial.print("- local port: ");
    Serial.println(ntp_udp.localPort());
    Serial.flush ();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * send ntp packet
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
static unsigned long
send_ntp_packet (IPAddress& address)
{
    memset(ntp_packet_buffer, 0, NTP_PACKET_SIZE);

    ntp_packet_buffer[0] = 0b11100011;                                      // LI, Version, Mode
    ntp_packet_buffer[1] = 0;                                               // Stratum, or type of clock
    ntp_packet_buffer[2] = 6;                                               // Polling Interval
    ntp_packet_buffer[3] = 0xEC;                                            // Peer Clock Precision
                                                                            // 8 bytes of zero for Root Delay & Root Dispersion
    ntp_packet_buffer[12]  = 49;
    ntp_packet_buffer[13]  = 0x4E;
    ntp_packet_buffer[14]  = 49;
    ntp_packet_buffer[15]  = 52;

    ntp_udp.flush();
    ntp_udp.beginPacket(address, 123);
    ntp_udp.write(ntp_packet_buffer, NTP_PACKET_SIZE);
    ntp_udp.endPacket();
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * poll ntp packet
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
ntp_poll_time (void)
{
    int packetsize;

    packetsize = ntp_udp.parsePacket();
        
    if (packetsize)
    {
        if (packetsize == NTP_PACKET_SIZE)
        {
            ntp_udp.read(ntp_packet_buffer, packetsize);
            
            unsigned long hi = word(ntp_packet_buffer[40], ntp_packet_buffer[41]);
            unsigned long lo = word(ntp_packet_buffer[42], ntp_packet_buffer[43]);
            unsigned long secsSince1900 = hi << 16 | lo;

            uint32_t secsSince1970 = secsSince1900 - 2208988800ULL;
            time_t epoch = secsSince1970;
            tm *t = localtime(&epoch);
            memcpy (&tmvar, t, sizeof (tm));

            ntp_success = true;

            Serial.print ("\r\n");
            Serial.printf ("rtc_write %u %u %u %u %u %u\r\n", (unsigned int) t->tm_year + 1900, (unsigned int) t->tm_mon + 1, (unsigned int) t->tm_mday, (unsigned int) t->tm_hour, (unsigned int) t->tm_min, (unsigned int) t->tm_sec);
            Serial.flush();

            digitalWrite(5, HIGH);
        }
        else
        {
            //Serial.println("- wrong packet size");
            digitalWrite(5, LOW);
            ntp_udp.flush();
            ntp_success = false;
        }
    }
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * get time from ntp server
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
ntp_get_time (IPAddress timeserver)
{
    send_ntp_packet (timeserver);
}

void
ntp_get_time (char * timeserver_str)
{
    int       ipno[4];

    if (ipstr_to_ipno (ipno, timeserver_str))
    {
        IPAddress timeserver (ipno[0], ipno[1], ipno[2], ipno[3]);
        ntp_get_time (timeserver);
    }
}

void ntp_get_time (void)
{
    IPAddress timeserver (129, 6, 15, 28);                      // time.nist.gov NTP server

    ntp_get_time (timeserver);
}
