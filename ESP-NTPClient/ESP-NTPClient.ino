#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ctime>

const char *ssid     = "Nostromo";
const char *password = "P1mm3lk0pf";

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup(){
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    //Serial.print ( "." );
  }

  timeClient.begin();
}

void loop() {
  timeClient.update();

  uint32_t secsSince1970 = timeClient.getEpochTime();
  time_t epoch = secsSince1970;
  tm *t = localtime(&epoch);

  Serial.print("\r\n");
  Serial.flush();
  Serial.printf ("rtc_write %u %u %u %u %u %u\r\n", (unsigned int) t->tm_year + 1900, (unsigned int) t->tm_mon + 1, (unsigned int) t->tm_mday, (unsigned int) t->tm_hour, (unsigned int) t->tm_min, (unsigned int) t->tm_sec);
  Serial.flush();

  digitalWrite(5, HIGH);

  delay(1000);

  ESP.deepSleep(3.6e9); // deep-sleep for an hour
}
