/*----------------------------------------------------------------------------------------------------------------------------------------
 * base.h - extern declarations for base functions
 *
 * Copyright (c) 2016 Frank Meyer - frank(at)fli4l.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
#include <stdint.h>
#include "base.h"

 /*--------------------------------------------------------------------------------------------------------------------------------------
 * get day of week (0=Sunday, 1=Monday, ... 6=Saturday)
 *
 *  day         - day of month
 *  month       - month beginning with 1
 *  year        - greater than 2000
 *
 *  example:    int rtc = dayofweek (tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
 *--------------------------------------------------------------------------------------------------------------------------------------
 */
int
dayofweek (int d, int m, int y)
{
   return (d += m < 3 ? y-- : y - 2 , 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * hex to integer
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint16_t
htoi (char * buf, uint8_t max_digits)
{
    uint8_t     i;
    uint8_t     x;
    uint16_t    sum = 0;

    for (i = 0; i < max_digits && *buf; i++)
    {
        x = buf[i];

        if (x >= '0' && x <= '9')
        {
            x -= '0';
        }
        else if (x >= 'A' && x <= 'F')
        {
            x -= 'A' - 10;
        }
        else if (x >= 'a' && x <= 'f')
        {
            x -= 'a' - 10;
        }
        else
        {
            x = 0;
        }
        sum <<= 4;
        sum += x;
    }

    return (sum);
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * ipstr_to_ipno() - convert an ipaddress string into numbers
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
bool
ipstr_to_ipno (int * ipno, const char * address)
{
    uint16_t sum = 0;
    uint8_t  dots = 0;

    while (*address)
    {
        char ch = *address++;

        if (ch >= '0' && ch <= '9')
        {
            sum = sum * 10 + (ch - '0');

            if (sum > 255)
            {
                return false;
            }
        }
        else if (ch == '.')
        {
            if (dots == 3)
            {
                return false;
            }

            ipno[dots++] = sum;
            sum = 0;
        }
        else
        {
            return false;                                                   // invalid character
        }
    }

    if (dots != 3)
    {
        return false;
    }

    ipno[3] = sum;
    return true;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * substitute characters
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
strsubst (char * s, int oldchar, int newchar)
{
    while (*s)
    {
        if (*s == oldchar)
        {
            *s = newchar;
        }

        s++;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * convert_utf8_to_iso8859 () - convert string to iso8859-1
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
unsigned char *
convert_utf8_to_iso8859 (unsigned char * buf)
{
    unsigned char *         s;                          // source ptr
    unsigned char *         t;                          // target ptr

    s = buf;
    t = buf;

    while (*s)
    {
        if (*s == 0xC3)
        {
            s++;
            *t++ = *s++ + 0x40;
        }
        else if (*s == 0xC2)
        {
            s++;
            *t++ = *s++;
        }
        else if (*s >0xC0)                              // unknown codepages
        {
            s += 2;
        }
        else
        {
            *t++ = *s++;
        }
    }

    *t = '\0';                                          // terminate target

    return (buf);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * send status message to WordClock
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
statusmsg (const char * msg, const char * status)
{
    Serial.print (msg);
    Serial.print (" ");
    Serial.println (status);
    Serial.flush ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * print debug message via UART connected to WordClock
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
debugmsg (String msg)
{
    Serial.print ("- ");
    Serial.println (msg);
    Serial.flush ();
}

void
debugmsg (const char * msg)
{
    if (msg && * msg)
    {
        Serial.print ("- ");
        Serial.println (msg);
        Serial.flush ();
    }
}

void
debugmsg (const char * str, const char * msg)
{
    if (str && *str && msg && * msg)
    {
        Serial.print ("- ");
        Serial.print (str);
        Serial.print (": ");
        Serial.println (msg);
        Serial.flush ();
    }
}
