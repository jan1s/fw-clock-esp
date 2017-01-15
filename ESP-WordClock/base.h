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
#ifndef BASE_H
#define BASE_H

#include "WString.h"
#include "Arduino.h"

extern int              dayofweek (int, int, int);
extern uint16_t         htoi (char *, uint8_t);
extern bool             ipstr_to_ipno (int *, const char *);
extern void             strsubst (char *, int, int);
extern unsigned char *  convert_utf8_to_iso8859 (unsigned char *);
extern void				statusmsg (const char *, const char *);
extern void             debugmsg (String);
extern void             debugmsg (const char *);
extern void             debugmsg (const char *, const char *);

#endif