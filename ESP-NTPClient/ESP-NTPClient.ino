#include <ESP8266WiFi.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ctime>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <Ticker.h>
Ticker ticker;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void tick()
{
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

void configModeCallback (WiFiManager *myWiFiManager) 
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach(0.2, tick);
}

void setup()
{
  Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect("AutoConnectAP", "defaultPW")) 
  {
    ESP.reset();
    delay(1000);
  }

  ticker.detach();
  digitalWrite(BUILTIN_LED, LOW);

  timeClient.begin();
}

void loop()
{  
  if(timeClient.update())
  {
    uint32_t secsSince1970 = timeClient.getEpochTime();
    time_t epoch = secsSince1970;
    tm *t = localtime(&epoch);
  
    Serial.printf("\r\nrtc_write %u %u %u %u %u %u\r\n", (unsigned int) t->tm_year + 1900, (unsigned int) t->tm_mon + 1, (unsigned int) t->tm_mday, (unsigned int) t->tm_hour, (unsigned int) t->tm_min, (unsigned int) t->tm_sec);
    Serial.flush();
  
    digitalWrite(5, HIGH);
    delay(1000);

    ESP.deepSleep(3.6e9); // deep-sleep for an hour
  }
  
  delay(1000);
}
