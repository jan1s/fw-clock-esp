/*----------------------------------------------------------------------------------------------------------------------------------------
 * http.cpp - http server
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
#include <WString.h>
#include "base.h"
#include "vars.h"
#include "wifi.h"
#include "ntp.h"
#include "version.h"

#define WCLOCK24H   1

const char *                wdays_en[7] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
const char *                wdays_de[7] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };

WiFiServer                  http_server(80);                                        // create an instance of the server on Port 80
static WiFiClient           http_client;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * http parameters
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define MAX_LINE_LEN                                256                                     // max. length of line_buffer
#define MAX_PATH_LEN                                20                                      // max. length of path
#define MAX_HTTP_PARAMS                             16                                      // max. number of http parameters

typedef struct
{
    char *  name;
    char *  value;
} HTTP_PARAMETERS;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * globals
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static HTTP_PARAMETERS                              http_parameters[MAX_HTTP_PARAMS];
static int                                          bgcolor_cnt;

#define MAX_KEY_LEN                                 64
#define MAX_IP_LEN                                  15
#define MAX_TIMEZONE_LEN                            3
#define MAX_DATE_LEN                                10
#define MAX_TIME_LEN                                5
#define MAX_BRIGHTNESS_LEN                          2
#define MAX_COLOR_VALUE_LEN                         2

#define MAIN_HEADER_COLS                            2
#define DATETIME_HEADER_COLS                        7
#define TZ_HEADER_COLS                              7
#define TICKER_HEADER_COLS                          2
#define NETWORK_HEADER_COLS                         3
#define DISPLAY_HEADER_COLS                         3
#define TIMERS_HEADER_COLS                          8

String  sHTTP_Response   = "";

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * display flags:
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#define DISPLAY_FLAGS_NONE                          0x00                    // no display flag
#define DISPLAY_FLAGS_PERMANENT_IT_IS               0x01                    // show "ES IST" permanently
#define DISPLAY_FLAGS_SYNC_AMBILIGHT                0x02                    // synchronize display and ambilight

/*--------------------------------------------------------------------------------------------------------------------------------------
 * possible modes of ESP8266
 *--------------------------------------------------------------------------------------------------------------------------------------
 */
#define ESP8266_CLIENT_MODE                         0
#define ESP8266_AP_MODE                             1

#define ESP8266_MAX_FIRMWARE_SIZE                   16
#define ESP8266_MAX_ACCESSPOINT_SIZE                32
#define ESP8266_MAX_IPADDRESS_SIZE                  16
#define ESP8266_MAX_HTTP_GET_PARAM_SIZE             256
#define ESP8266_MAX_CMD_SIZE                        32
#define ESP8266_MAX_TIME_SIZE                       16

static struct tm tm;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * flush output buffer
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_flush (void)
{
    while (sHTTP_Response.length() > 2048)
    {
        String s = sHTTP_Response.substring (0, 2048);
        http_client.print(s);
        sHTTP_Response = sHTTP_Response.substring (2048);
    }

    http_client.print(sHTTP_Response);
    sHTTP_Response   = "";
    http_client.flush ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * send string
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_send (const char * s)
{
    sHTTP_Response += s;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * set parameters from list
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_set_params (char * paramlist)
{
    char *  p;                                              // ap=access&pw=secret&action=saveap
    int     idx = 0;

    if (paramlist && *paramlist)
    {
        http_parameters[idx].name = paramlist;

        for (p = paramlist; idx < MAX_HTTP_PARAMS - 1 && *p; p++)
        {
            if (*p == '=')
            {
                *p = '\0';
                http_parameters[idx].value = p + 1;
            }
            else if (*p == '&')
            {
                *p = '\0';
                idx++;
                http_parameters[idx].name = p + 1;
            }
        }
        idx++;
    }

    while (idx < MAX_HTTP_PARAMS)
    {
        http_parameters[idx].name = (char *) 0;
        idx++;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * get a parameter
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static char *
http_get_param (const char * name)
{
    static char empty[] = "";
    int     idx;

    for (idx = 0; idx < MAX_HTTP_PARAMS && http_parameters[idx].name != (char *) 0; idx++)
    {
        if (! strcmp (http_parameters[idx].name, name))
        {
            return http_parameters[idx].value;
        }
    }

    return empty;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * get a parameter by index
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static char *
http_get_param_by_idx (const char * name, int idx)
{
    char    name_buf[16];
    char *  rtc;

    sprintf (name_buf, "%s%d", name, idx);
    rtc = http_get_param (name_buf);

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * get a checkbox parameter
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
http_get_checkbox_param (const char * name)
{
    char *  value = http_get_param (name);
    int     rtc;

    if (! strcmp (value, "active"))
    {
        rtc = 1;
    }
    else
    {
        rtc = 0;
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * get a checkbox parameter by index
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
http_get_checkbox_param_by_idx (const char * name, int idx)
{
    char *  value = http_get_param_by_idx (name, idx);
    int     rtc;

    if (! strcmp (value, "active"))
    {
        rtc = 1;
    }
    else
    {
        rtc = 0;
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * send http and html header
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_header (const char * title)
{
    const char * p = "HTTP/1.0 200 OK";

    http_send (p);
    http_send ("\r\n\r\n");
    http_send ("<html>\r\n");
    http_send ("<head>\r\n");
    http_send ("<meta charset=\"utf-8\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    http_send ("<title>");
    http_send (title);
    http_send ("</title>\r\n");
    http_send ("<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" integrity=\"sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u\" crossorigin=\"anonymous\">");
    http_send ("</head>\r\n");
    http_send ("<body>\r\n");
    //http_send ("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.12.4/jquery.min.js\"></script>");
    http_send ("<H1>\r\n");
    http_send (title);
    http_send ("</H1>\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * send html trailer
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_trailer (void)
{
    http_send ("</body>\r\n");
    http_send ("</html>\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * send table header columns
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_header (const char ** columns, int cols)
{
    int i;

    http_send ("<table border=0>\r\n");
    http_send ("<tr>\r\n");

    for (i = 0; i < cols; i++)
    {
        http_send ("<th><font color=blue>");
        http_send (columns[i]);
        http_send ("</font></th>\r\n");
    }

    http_send ("</tr>\r\n");
    bgcolor_cnt = 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * begin form
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
begin_form (const char * page)
{
    http_send ("<form method=\"GET\" action=\"/");
    http_send (page);
    http_send ("\">\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * end form
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
end_form (void)
{
    http_send ("</form>\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * begin table row
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
begin_table_row (void)
{
    bgcolor_cnt++;

    if (bgcolor_cnt & 0x01)
    {
        http_send ("<tr bgcolor=#f0f0f0>\r\n");
    }
    else
    {
        http_send ("<tr>\r\n");
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * begin table row as form
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
begin_table_row_form (const char * page)
{
    begin_table_row ();
    begin_form (page);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * end table row
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
end_table_row (void)
{
    http_send ("</tr>\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * end table row as form
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
end_table_row_form (void)
{
    end_form ();
    end_table_row ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * checkbox field
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
checkbox_field (const char * id, const char * desc, int checked)
{
    http_send (desc);
    http_send ("&nbsp;<input type=\"checkbox\" name=\"");
    http_send (id);
    http_send ("\" value=\"active\" ");

    if (checked)
    {
        http_send ("checked");
    }

    http_send (">");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * checkbox column
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
checkbox_column (const char * id, const char * desc, int checked)
{
    http_send ("<td>");
    checkbox_field (id, desc, checked);
    http_send ("</td>");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * text column
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
text_column (const char * text)
{
    http_send ("<td>");
    http_send (text);
    http_send ("</td>");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * input field
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
input_field (const char * id, const char * desc, const char * value, int maxlength, int maxsize)
{
    char maxlength_buf[8];
    char maxsize_buf[8];

    sprintf (maxlength_buf, "%d", maxlength);
    sprintf (maxsize_buf, "%d", maxsize);

    if (desc && *desc)
    {
        http_send (desc);
        http_send ("&nbsp;");
    }

    http_send ("<input type=\"text\" id=\"");
    http_send (id);
    http_send ("\" name=\"");
    http_send (id);
    http_send ("\" value=\"");
    http_send (value);
    http_send ("\" maxlength=\"");
    http_send (maxlength_buf);
    http_send ("\" size=\"");
    http_send (maxsize_buf);
    http_send ("\">");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * input column
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
input_column (const char * id, const char * desc, const char * value, int maxlength, int maxsize)
{
    http_send ("<td>");
    input_field (id, desc, value, maxlength, maxsize);
    http_send ("</td>");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * select field
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
select_field (const char * id, const char ** text, int selected_value, int max_values)
{
    char    buf[3];
    int     i;

    http_send ("<select id=\"");
    http_send (id);
    http_send ("\" name=\"");
    http_send (id);
    http_send ("\">\r\n");

    for (i = 0; i < max_values; i++)
    {
        sprintf (buf, "%d", i);
        http_send ("<option value=\"");
        http_send (buf);
        http_send ("\"");

        if (i == selected_value)
        {
            http_send (" selected");
        }

        http_send (">");
        http_send ((char *) text[i]);
        http_send ("</option>\r\n");
    }

    http_send ("</select>\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * select column
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
select_column (const char * id, const char ** text, int selected_value, int max_values)
{
    http_send ("<td>");
    select_field (id, text, selected_value, max_values);
    http_send ("</td>");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * slider field
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
slider_field (const char * id, const char * text, const char * value, const char * min, const char * max, const char * pixelwidth)
{
    if (text && *text)
    {
        http_send (text);
        http_send ("&nbsp;");
    }

    http_send ("<input type=\"range\" id=\"");
    http_send (id);
    http_send ("\" name=\"");
    http_send (id);
    http_send ("\" value=\"");
    http_send (value);
    http_send ("\" min=\"");
    http_send (min);
    http_send ("\" max=\"");
    http_send (max);

    if (pixelwidth && *pixelwidth)
    {
        http_send ("\" style=\"width:");
        http_send (pixelwidth);
        http_send ("px;");
    }

    http_send ("\" oninput=\"");
    http_send (id);
    http_send ("_output.value=");
    http_send (id);
    http_send (".value\">");
    http_send ("<output name=\"");
    http_send (id);
    http_send ("_output\">");
    http_send (value);
    http_send ("</output>");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * slider column
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
slider_column (const char * id, const char * value, const char * min, const char * max)
{
    http_send ("<td>");
    slider_field (id, "", value, min, max, "");
    http_send ("</td>\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * submit button field
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
button_field (const char * id, const char * text)
{
    http_send ("<button type=\"submit\" name=\"action\" value=\"");
    http_send (id);
    http_send ("\">");
    http_send (text);
    http_send ("</button>&nbsp;");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * submit column field
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
button_column (const char * id, const char * text)
{
    http_send ("<td>");
    button_field (id, text);
    http_send ("</td>\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * save column
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
save_column (const char * id)
{
    http_send ("<td><button type=\"submit\" name=\"action\" value=\"save");
    http_send (id);
    http_send ("\">Save</button>");
    http_send ("</td>\r\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * table row
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_row (const char * col1, const char * col2, const char * col3)
{
    begin_table_row ();
    text_column (col1);
    text_column (col2);
    text_column (col3);
    end_table_row ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * table row with input
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_row_input (const char * page, const char * text, const char * id, const char * value, int maxlength)
{
    begin_table_row_form (page);
    text_column (text);
    input_column (id, "", value, maxlength, maxlength);
    save_column (id);
    end_table_row_form ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * table row with multiple inputs
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_row_inputs (const char * page, const char * id, int n, const char ** ids, const char ** desc, const char ** value, int * maxlength, int * maxsize)
{
    int     i;

    begin_table_row_form (page);

    for (i = 0; i < n; i++)
    {
        http_send ("<td>");
        input_field (ids[i], desc[i], value[i], maxlength[i], maxsize[i]);
        http_send ("</td>");
    }

    save_column (id);
    end_table_row_form ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * table row with checkbox
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_row_checkbox (const char * page, const char * text, const char * id, const char * desc, int checked)
{
    begin_table_row_form (page);
    text_column (text);
    checkbox_column (id, desc, checked);
    save_column (id);
    end_table_row_form ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * table row with selection
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_row_select (const char * page, const char * text1, const char * id, const char ** text2, int selected_value, int max_values)
{
    begin_table_row_form (page);
    text_column (text1);
    select_column (id, text2, selected_value, max_values);
    save_column (id);
    end_table_row_form ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * table row with slider
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_row_slider (const char * page, const char * text1, const char * id, const char * text2, const char * min, const char * max)
{
    begin_table_row_form (page);
    text_column (text1);
    slider_column (id, text2, min, max);
    save_column (id);
    end_table_row_form ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * table row with n sliders
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_row_sliders (const char * page, const char * text1, const char * id, int n, const char ** ids, const char ** desc, const char * const * text2, const char ** min, const char ** max)
{
    int i;

    begin_table_row_form (page);
    text_column (text1);

    http_send ("<td>\r\n");

    for (i = 0; i < n; i++)
    {
        slider_field (ids[i], desc[i], text2[i], min[i], max[i], "64");
        http_send ("&nbsp;&nbsp;&nbsp;\r\n");
    }

    save_column (id);
    end_table_row_form ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * table trailer
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
table_trailer (void)
{
    http_send ("</table><P>\r\n");
}

static void
menu_entry (const char * page, const char * entry)
{
    http_send ("<li><a href=\"/");
    http_send (page);
    http_send ("\">");
    http_send (entry);
    http_send ("</a></li>");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
http_menu (void)
{
    http_send("<nav class=\"navbar navbar-inverse navbar-fixed-top\">");
    http_send("<div class=\"container\">");
    http_send("<div class=\"navbar-header\">");
    http_send("<button type=\"button\" class=\"navbar-toggle collapsed\" data-toggle=\"collapse\" data-target=\"#navbar\" aria-expanded=\"false\" aria-controls=\"navbar\">");
    http_send("<span class=\"sr-only\">Toggle navigation</span>");
    http_send("<span class=\"icon-bar\"></span>");
    http_send("<span class=\"icon-bar\"></span>");
    http_send("<span class=\"icon-bar\"></span>");
    http_send("</button><a class=\"navbar-brand\" href=\"#\">NixieClock</a></div>");
    http_send("<div id=\"navbar\" class=\"collapse navbar-collapse\"><ul class=\"nav navbar-nav\">");
    menu_entry ("", "Main");
    menu_entry ("network", "Network");
    menu_entry ("display", "Display");
    //menu_entry ("timers", "Timers");
    http_send("</ul></div>");
    http_send("</div></nav>");
    http_send("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.12.4/jquery.min.js\"></script>");
    http_send ("<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js\" integrity=\"sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa\" crossorigin=\"anonymous\"></script>");
    http_send ("<P>");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * main page
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
http_main (void)
{
    const char *    thispage = "";
    const char *    header_cols[MAIN_HEADER_COLS]               = { "Name", "Value" };
    const char *    datetime_header_cols[DATETIME_HEADER_COLS]  = { "YYYY", "MM", "DD", "hh", "mm", "ss", "Action" };
    const char *    datetime_ids[6]                             = { "year", "month", "day", "hour", "min", "sec" };
    const char *    datetime_desc[6]                            = { "", "", "", "", "", "" };
    int             datetime_maxlen[6]                          = { 4, 2, 2, 2, 2, 2};
    int             datetime_maxsize[6]                         = { 4, 2, 2, 2, 2, 2 };
    char            datetime_year_str[5];
    char            datetime_mon_str[3];
    char            datetime_day_str[3];
    char            datetime_hour_str[3];
    char            datetime_minutes_str[3];
    char            datetime_seconds_str[3];
    const char *    datetime_values[6]                          = { datetime_year_str, datetime_mon_str, datetime_day_str, datetime_hour_str, datetime_minutes_str, datetime_seconds_str };
    const char *    tz_cols[MAIN_HEADER_COLS]                   = { "Name", "Value" };
    const char *    tz_header_cols[TZ_HEADER_COLS]              = { "offset", "hour", "dow", "week", "month", "Action" };
    const char *    tz_ids[5]                                   = { "offset", "hour", "dow", "week", "month"};
    const char *    tz_desc[5]                                  = { "", "", "", "", "" };
    int             tz_maxlen[5]                                = { 4, 2, 1, 1, 2 };
    int             tz_maxsize[5]                               = { 4, 2, 1, 1, 2 };
    char            tzstd_offset_str[5];
    char            tzstd_hour_str[3];
    char            tzstd_dow_str[2];
    char            tzstd_week_str[2];
    char            tzstd_month_str[3];
    const char *    tzstd_values[5]                             = { tzstd_offset_str, tzstd_hour_str, tzstd_dow_str, tzstd_week_str, tzstd_month_str };
    char            tzdst_offset_str[5];
    char            tzdst_hour_str[3];
    char            tzdst_dow_str[2];
    char            tzdst_week_str[2];
    char            tzdst_month_str[3];
    const char *    tzdst_values[5]                             = { tzdst_offset_str, tzdst_hour_str, tzdst_dow_str, tzdst_week_str, tzdst_month_str };
    char *          action;
    const char *    message                                     = (const char *) 0;
    STR_VAR *       sv;
    char *          version;
    char *          eeprom_version;
    uint_fast8_t    rtc                                         = 0;
    struct tm *     tmp;
    uint_fast8_t    eeprom_is_up;

//    tmp = get_tm_var (CURRENT_TM_VAR);
//
//    if (tmp->tm_year >= 0 && tmp->tm_mon >= 0 && tmp->tm_mday >= 0 && tmp->tm_hour >= 0 && tmp->tm_min >= 0 &&
//        tmp->tm_year <= 1200 && tmp->tm_mon <= 12 && tmp->tm_mday <= 31 && tmp->tm_hour < 24 && tmp->tm_min < 60)
//    {                                                               // check values to avoid buffer overflow
//        sprintf (year_str,      "%4d",  tmp->tm_year + 1900);
//        sprintf (mon_str,       "%02d", tmp->tm_mon + 1);
//        sprintf (day_str,       "%02d", tmp->tm_mday);
//        sprintf (hour_str,      "%02d", tmp->tm_hour);
//        sprintf (minutes_str,   "%02d", tmp->tm_min);
//    }
//    else
//    {
//        year_str[0]     = '\0';
//        mon_str[0]      = '\0';
//        day_str[0]      = '\0';
//        hour_str[0]     = '\0';
//        minutes_str[0]  = '\0';
//    }

    sprintf (datetime_year_str,         "%4d", 1900);
    sprintf (datetime_mon_str,          "%02d",   1);
    sprintf (datetime_day_str,          "%02d",   1);
    sprintf (datetime_hour_str,         "%02d",   0);
    sprintf (datetime_minutes_str,      "%02d",   0);
    sprintf (datetime_seconds_str,      "%02d",   0);

    sprintf (tzstd_offset_str,          "%4d",   60);
    sprintf (tzstd_hour_str,            "%02d",   1);
    sprintf (tzstd_dow_str,             "%01d",   1);
    sprintf (tzstd_week_str,            "%01d",   0);
    sprintf (tzstd_month_str,           "%02d",   0);

    sprintf (tzdst_offset_str,          "%4d",   60);
    sprintf (tzdst_hour_str,            "%02d",   1);
    sprintf (tzdst_dow_str,             "%01d",   1);
    sprintf (tzdst_week_str,            "%01d",   0);
    sprintf (tzdst_month_str,           "%02d",   0);

    action = http_get_param ("action");

    if (action)
    {
        if (! strcmp (action, "poweron"))
        {
            message = "Switching power on...";
            set_numvar (DISPLAY_POWER_NUM_VAR, 1);
        }
        else if (! strcmp (action, "poweroff"))
        {
            message = "Switching power off...";
            set_numvar (DISPLAY_POWER_NUM_VAR, 0);
        }
        else if (! strcmp (action, "savedatetime"))
        {
            int year    = atoi (http_get_param ("year"));
            int month   = atoi (http_get_param ("month"));
            int day     = atoi (http_get_param ("day"));
            int hour    = atoi (http_get_param ("hour"));
            int minutes = atoi (http_get_param ("min"));
            int seconds = atoi (http_get_param ("sec"));

            cmd_rtc_write(year, month, day, hour, minutes, seconds);
        }
        else if (! strcmp (action, "savetzstd"))
        {
            int offset    = atoi (http_get_param ("offset"));
            int hour    = atoi (http_get_param ("hour"));
            int dow     = atoi (http_get_param ("dow"));
            int week    = atoi (http_get_param ("week"));
            int month   = atoi (http_get_param ("month"));

            cmd_tz_write(true, offset, hour, dow, week, month);
        }
        else if (! strcmp (action, "savetzdst"))
        {
            int offset    = atoi (http_get_param ("offset"));
            int hour    = atoi (http_get_param ("hour"));
            int dow     = atoi (http_get_param ("dow"));
            int week    = atoi (http_get_param ("week"));
            int month   = atoi (http_get_param ("month"));

            cmd_tz_write(false, offset, hour, dow, week, month);
        }
    }

    http_header ("NixieClock");
    http_menu ();


    table_header (datetime_header_cols, DATETIME_HEADER_COLS);
    table_row_inputs (thispage, "datetime", 6, datetime_ids, datetime_desc, datetime_values, datetime_maxlen, datetime_maxsize);
    table_trailer ();

    table_header (tz_header_cols, TZ_HEADER_COLS);
    table_row_inputs (thispage, "tzstd", 5, tz_ids, tz_desc, tzstd_values, tz_maxlen, tz_maxsize);
    table_row_inputs (thispage, "tzdst", 5, tz_ids, tz_desc, tzdst_values, tz_maxlen, tz_maxsize);
    table_trailer ();

    begin_form (thispage);
    button_field ("poweron", "Power On");
    button_field ("poweroff", "Power Off");
    end_form ();

    if (message)
    {
        http_send ("<P>\r\n<font color=green>");
        http_send (message);
        http_send ("</font>\r\n");
    }

    http_trailer ();
    http_flush ();

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * network page
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
http_network (void)
{
    const char *                thispage = "network";
    const char *                wlan_mode_names[2] = { "WLAN Client", "AP" };
    const char *                header_cols[NETWORK_HEADER_COLS] = { "Name", "Value", "Action" };
    char *                      action;
    const char *                message = (const char *) 0;
    const char *                alert_message = (const char *) 0;
    const char *                esp_firmware_version;
    char                        timezone_str[MAX_TIMEZONE_LEN+1];
    uint_fast16_t                 utz;
    int_fast16_t                 tz;
    STR_VAR *                   sv;
    uint_fast8_t                rtc             = 0;

    esp_firmware_version        = ESP_VERSION;

    utz = get_numvar (TIMEZONE_NUM_VAR);

    tz = utz & 0xFF;

    if (utz & 0x100)
    {
        tz = -tz;
    }

    action = http_get_param ("action");

    if (action)
    {
        if (! strcmp (action, "savewlan"))
        {
            uint_fast8_t wlan_mode = atoi (http_get_param ("wlanmode"));
            char * ssid = http_get_param ("ssid");
            char * key  = http_get_param ("key");

            if (wlan_mode == ESP8266_CLIENT_MODE)
            {
                http_header ("NixieClock Network");
                http_send ("<B>Connecting to Access Point, try again later.</B>");
                http_trailer ();
                http_flush ();

                wifi_connect (ssid, key);
                return 0;
            }
            else
            {
                if (strlen (key) < 10)
                {
                    alert_message = "Minimum length of key is 10!";
                }
                else
                {
                    http_header ("NixieClock Network");

                    http_send ("<B>Stating as Access Point, try again later.</B>");
                    http_trailer ();
                    http_flush ();

                    wifi_ap (ssid, key);
                    return 0;
                }
            }
        }
        else if (! strcmp (action, "savetimeserver"))
        {
            char * newtimeserver = http_get_param ("timeserver");

            set_strvar (TIMESERVER_STR_VAR, newtimeserver);
            message = "Timeserver successfully changed.";
        }
        else if (! strcmp (action, "nettime"))
        {
            message = "Getting net time";
            ntp_get_time();
            //rpc (GET_NET_TIME_RPC_VAR);
        }
    }


    http_header ("NixieClock Network");
    http_menu ();
    table_header (header_cols, NETWORK_HEADER_COLS);

    table_row ("IP address", wifi_ip_address, "");
    table_row ("ESP8266 firmware", esp_firmware_version, "");

    begin_table_row_form (thispage);
    text_column ("WLAN");
    http_send ("<td>");
    select_field ("wlanmode", wlan_mode_names, wifi_ap_mode ? 1 : 0, 2);
    input_field ("ssid", "SSID", wifi_ssid, WIFI_MAX_SSID_LEN, 15);
    http_send ("&nbsp;&nbsp;&nbsp;");
    input_field ("key", "Key", "", MAX_KEY_LEN, 20);
    http_send ("</td>");
    save_column ("wlan");
    end_table_row_form ();

    sv = get_strvar (TIMESERVER_STR_VAR);

    table_row_input (thispage, "Time server", "timeserver", sv->str, MAX_IP_LEN);
    table_trailer ();

    begin_form (thispage);
    button_field ("nettime", "Get net time");
    end_form ();

    if (alert_message)
    {
        http_send ("<P>\r\n<font color=red><B>");
        http_send (alert_message);
        http_send ("</B></font>\r\n");
    }
    else if (message)
    {
        http_send ("<P>\r\n<font color=green>");
        http_send (message);
        http_send ("</font>\r\n");
    }

    http_trailer ();
    http_flush ();

    return rtc;
}


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * display page
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
http_display (void)
{
    const char *        thispage = "display";
    const char *        header_cols[DISPLAY_HEADER_COLS] = { "Name", "Value", "Action" };
    static int          already_called  = 0;
    char *              action;
    const char *        message         = (const char *) 0;
    const char *        display_mode_names[MAX_DISPLAY_MODE_VARIABLES] = { "None", "1", "2", "3" };
    int                 max_display_modes = MAX_DISPLAY_MODE_VARIABLES;
    int                 display_mode = 0;
    const char *        display_type_names[MAX_DISPLAY_TYPE_VARIABLES] = { "None", "1", "2", "3" };
    int                 max_display_types = MAX_DISPLAY_TYPE_VARIABLES;
    int                 display_type = 0;
    uint_fast8_t        display_flags;
    uint_fast8_t        idx;
    uint_fast8_t        rtc = 0;

    //display_mode                    = get_numvar (DISPLAY_MODE_NUM_VAR);

    action = http_get_param ("action");

    if (action)
    {
        if (! strcmp (action, "savedisplaymode"))
        {
            display_mode = atoi (http_get_param ("displaymode"));
            cmd_nixie_setmode ((uint8_t)display_mode);
        }
        else if (! strcmp (action, "savedisplaytype"))
        {
            display_type = atoi (http_get_param ("displaytype"));
            cmd_nixie_settype ((uint8_t)display_type);
        }
        else if (! strcmp (action, "testdisplay"))
        {
            message = "Testing display...";
            rpc (TEST_DISPLAY_RPC_VAR);
        }
        else if (! strcmp (action, "poweron"))
        {
            message = "Switching power on...";
            set_numvar (DISPLAY_POWER_NUM_VAR, 1);
        }
        else if (! strcmp (action, "poweroff"))
        {
            message = "Switching power off...";
            set_numvar (DISPLAY_POWER_NUM_VAR, 0);
        }
    }

    http_header ("NixieClock Display");
    http_menu ();
    table_header (header_cols, DISPLAY_HEADER_COLS);
    table_row_select (thispage, "Display Mode", "displaymode", display_mode_names, display_mode, max_display_modes);
    table_row_select (thispage, "Display Typee", "displaytype", display_type_names, display_type, max_display_types);
    table_trailer ();

    begin_form (thispage);
    button_field ("poweron", "Power On");
    button_field ("poweroff", "Power Off");
    button_field ("testdisplay", "Test display");
    end_form ();

    if (message)
    {
        http_send ("<P>\r\n<font color=green>");
        http_send (message);
        http_send ("</font>\r\n");
    }

    http_trailer ();
    http_flush ();

    return rtc;
}


static void
table_row_timers (const char * page, int idx)
{
    char            id[8];
    char            idx_buf[3];
    char            hour_buf[3];
    char            minute_buf[3];
    char            act_id[8];
    char            on_id[8];
    char            hour_id[8];
    char            min_id[8];
    char            day_id[8];

    NIGHT_TIME *    nt = get_night_time_var ((NIGHT_TIME_VARIABLE) idx);

    sprintf (hour_buf,   "%02d", nt->minutes / 60);
    sprintf (minute_buf, "%02d", nt->minutes % 60);

    begin_table_row_form (page);

    sprintf (idx_buf, "%d",   idx);
    sprintf (id,      "id%d", idx);
    sprintf (act_id,  "a%d",  idx);
    sprintf (on_id,   "o%d",  idx);
    sprintf (hour_id, "h%d",  idx);
    sprintf (min_id,  "m%d",  idx);

    text_column (idx_buf);

    checkbox_column (act_id, "", (nt->flags & NIGHT_TIME_FLAG_ACTIVE) ? 1 : 0);
    checkbox_column (on_id, "", (nt->flags & NIGHT_TIME_FLAG_SWITCH_ON) ? 1 : 0);

    sprintf (day_id, "f%d", idx);
    select_column (day_id, wdays_en, (nt->flags & NIGHT_TIME_FROM_DAY_MASK) >> 3, 7);

    sprintf (day_id, "t%d", idx);
    select_column (day_id, wdays_en, (nt->flags & NIGHT_TIME_TO_DAY_MASK), 7);

    input_column (hour_id, "",  hour_buf, 2, 2);
    input_column (min_id, "",  minute_buf, 2, 2);

    save_column (id);
    end_table_row_form ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * timers page
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
http_timers (void)
{
    const char *        header_cols[TIMERS_HEADER_COLS] = { "Slot", "Active", "On", "From", "To", "Hour", "Min", "Action" };
    char *              action;
    uint_fast8_t        idx;
    int                 rtc = 0;

    action = http_get_param ("action");

    if (action)
    {
        if (! strncmp (action, "saveid", 6))
        {
            char id[16];
            uint_fast8_t idx = atoi (action + 6);

            if (idx < MAX_NIGHT_TIME_VARIABLES)
            {
                NIGHT_TIME * nt = get_night_time_var ((NIGHT_TIME_VARIABLE) idx);
                uint_fast8_t    from_day;
                uint_fast8_t    to_day;
                uint_fast16_t   minutes;
                uint_fast8_t    flags = nt->flags;

                if (http_get_checkbox_param_by_idx ("a", idx))
                {
                    flags |= NIGHT_TIME_FLAG_ACTIVE;
                }
                else
                {
                    flags &= ~NIGHT_TIME_FLAG_ACTIVE;
                }

                if (http_get_checkbox_param_by_idx ("o", idx))
                {
                    flags |= NIGHT_TIME_FLAG_SWITCH_ON;
                }
                else
                {
                    flags &= ~NIGHT_TIME_FLAG_SWITCH_ON;
                }

                sprintf (id, "f%d", idx);
                from_day = atoi (http_get_param (id));
                sprintf (id, "t%d", idx);
                to_day = atoi (http_get_param (id));

                flags &= ~(NIGHT_TIME_FROM_DAY_MASK | NIGHT_TIME_TO_DAY_MASK);
                flags |= NIGHT_TIME_FROM_DAY_MASK & (from_day << 3);
                flags |= NIGHT_TIME_TO_DAY_MASK & (to_day);

                minutes = atoi (http_get_param_by_idx ("h", idx)) * 60 + atoi (http_get_param_by_idx ("m", idx));

                set_night_time_var ((NIGHT_TIME_VARIABLE) idx, minutes, flags);
            }
        }
    }

    http_header ("WordClock Timers");
    http_menu ();
    table_header (header_cols, TIMERS_HEADER_COLS);

    for (idx = 0; idx < MAX_NIGHT_TIME_VARIABLES; idx++)
    {
        table_row_timers ("timers", idx);
    }

    table_trailer ();

    http_trailer ();
    http_flush ();

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * http server
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
http (const char * path, const char * const_param)
{
    char param[256];
    strncpy (param, const_param, 255);
    param[255] = '\0';

    char *          p;
    char *          s;
    char *          t;
    int             rtc = 0;

    // log_printf ("http path: '%s'\r\n", path);

    for (p = param; *p; p++)
    {
        if (*p == '%')
        {
            *p = htoi (p + 1, 2);

            t = p + 1;
            s = p + 3;

            while (*s)
            {
                *t++ = *s++;
            }
            *t = '\0';
        }
        else if (*p == '+')                                     // plus must be mapped to space if GET method
        {
            *p = ' ';
        }
    }

    // log_printf ("http parameters: '%s'\r\n", param);

    if (param)
    {
        http_set_params (param);
    }
    else
    {
        http_set_params ((char *) NULL);
    }

    if (! strcmp (path, "/"))
    {
        rtc = http_main ();
    }
    else if (! strcmp (path, "/network"))
    {
        rtc = http_network ();
    }
    //else if (! strcmp (path, "/temperature"))
    //{
    //    rtc = http_temperature ();
    //}
    //else if (! strcmp (path, "/weather"))
    //{
    //    rtc = http_weather ();
    //}
    //else if (! strcmp (path, "/ldr"))
    //{
    //   rtc = http_ldr ();
    //}
    else if (! strcmp (path, "/display"))
    {
        rtc = http_display ();
    }
    //else if (! strcmp (path, "/animations"))
    //{
    //    rtc = http_animations ();
    //}
    //else if (! strcmp (path, "/ambilight"))
    //{
    //    rtc = http_ambilight ();
    //}
    else if (! strcmp (path, "/timers"))
    {
        rtc = http_timers ();
    }
    else
    {
        const char * p = "HTTP/1.0 404 Not Found";
        http_send (p);
        http_send ("\r\n\r\n");
        http_send ("404 Not Found\r\n");
        http_flush ();
    }

    return rtc;
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http server
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
http_server_loop (void)
{
//    char    answer[ANSWER_BUFFER_SIZE];
    String  sPath       = "";
    String  sParam      = "";
    String  sCmd        = "";
    String  sGetstart   = "GET ";
    String  sResponse   = "";
    int     start_position;
    int     end_position_space;
    int     end_position_question;
    int     i = 0;

    http_client = http_server.available();                                      // check if a client has connected
  
    if (!http_client)                                                           // wait until the client sends some data
    {
      return;
    }
  
    Serial.println("- new client");
    Serial.flush ();

    unsigned long ultimeout = millis() + 250;
    
    while (! http_client.available() && (millis() < ultimeout) )
    {
        delay(1);
    }
  
    if (millis() > ultimeout) 
    {
        Serial.println ("- client connection time-out!");
        Serial.flush ();
        return; 
    }
  
    String sRequest = http_client.readStringUntil ('\r');                        // read the first line of the request
    http_client.flush();
    
    if (sRequest == "")                                                     // stop client, if request is empty
    {
        Serial.println ("- empty http request");
        Serial.flush ();
        http_client.stop();
        return;
    }

    start_position = sRequest.indexOf(sGetstart);
  
    if (start_position >= 0)
    {
        start_position+=+sGetstart.length ();
        end_position_space = sRequest.indexOf (" ", start_position);
        end_position_question = sRequest.indexOf ("?", start_position);
        
        if (end_position_space > 0)                                         // parameters?
        {
            if (end_position_question > 0)
            {                                                               // yes
                sPath  = sRequest.substring(start_position,end_position_question);
                sParam = sRequest.substring(end_position_question + 1,end_position_space);
            }
            else
            {                                                               // no
                sPath  = sRequest.substring(start_position,end_position_space);
            }
        }
    }
  
    http (sPath.c_str(), sParam.c_str());

    http_client.stop();                                                     // stop client
}

/*----------------------------------------------------------------------------------------------------------------------------------------
 * http server begin
 *----------------------------------------------------------------------------------------------------------------------------------------
 */
void
http_server_begin (void)
{
    http_server.begin();
}
