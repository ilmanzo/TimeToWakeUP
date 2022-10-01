#include <WakeOnLan.h>

#include <WiFi.h>
#include <WiFiUdp.h>
#include "time.h"
#include <esp_sleep.h>

// #define DEBUG


#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(a, b) Serial.printf(a, b)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(a, b)
#endif

const char* ssid = "<INSERT_SSID_HERE>";
const char* password = "<INSERT_PASSWORD_HERE>";

const char* MACAddress = "D8:D3:85:AF:7D:05";

const char* ntpServer1 = "162.159.200.1";
const char* ntpServer2 = "135.125.165.133";
const char* ntpServer3 = "51.255.210.208";

/*
 * 
> host it.pool.ntp.org
it.pool.ntp.org has address 95.110.254.234
it.pool.ntp.org has address 162.159.200.123
it.pool.ntp.org has address 135.125.165.133
it.pool.ntp.org has address 162.159.200.1

europe.pool.ntp.org has address 194.57.169.1
europe.pool.ntp.org has address 135.181.163.94
europe.pool.ntp.org has address 89.234.64.77
europe.pool.ntp.org has address 46.227.200.72
*/


const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP_SHORT 1200  /* Time ESP32 will go to sleep (in seconds) 1200 = 20 minutes */
#define TIME_TO_SLEEP_LONG 3600   /* Time ESP32 will go to sleep (in seconds) 3600 = 1 hour */


/*
  Method to print the reason by which ESP32
  has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: DEBUG_PRINTLN("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: DEBUG_PRINTLN("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: DEBUG_PRINTLN("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: DEBUG_PRINTLN("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: DEBUG_PRINTLN("Wakeup caused by ULP program"); break;
    default: DEBUG_PRINTF("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}


int getLocalTime()  // return 0-23 current hour
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    DEBUG_PRINTLN("Failed to obtain time");
    return -1;
  }
#ifdef DEBUG
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
#endif
  return timeinfo.tm_hour * 100 + timeinfo.tm_min;
}


bool connectWiFi() {
  //connect to WiFi
  int retrycount = 0;
  DEBUG_PRINTF("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
    retrycount++;
    if (retrycount >= 20) return false;
  }
  DEBUG_PRINTLN(" CONNECTED");
  return true;
}

void setup() {


#ifdef DEBUG
  Serial.begin(115200);
  //if we are debugging, print the wakeup reason for ESP32
  print_wakeup_reason();
#endif

  btStop();  // no need for bluetooth

  bool connected = connectWiFi();
  int hour = -1;

  if (connected) {
    //init and try to get the time from any of 3 ntp servers
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1);
    if ((hour = getLocalTime()) == -1) configTime(gmtOffset_sec, daylightOffset_sec, ntpServer2);
    if ((hour = getLocalTime()) == -1) configTime(gmtOffset_sec, daylightOffset_sec, ntpServer3);
    DEBUG_PRINTF("The hour from NTP is: %d\n", hour);
  }

  if (hour >= 515 && hour <= 705) {  // short sleep
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_SHORT * uS_TO_S_FACTOR);
    DEBUG_PRINTLN("Setup ESP32 to sleep for " + String(TIME_TO_SLEEP_SHORT) + " Seconds");
    //at 5-6-7 AM magic happens
    //NB if wifi router timer is not yet started, hour == -1 so we won't enter here
    if (connected) {
      DEBUG_PRINTF("Sending magic packet to %s\n", MACAddress);
      WiFiUDP UDP;
      WakeOnLan WOL(UDP);     // Pass WiFiUDP class
      WOL.setRepeat(4, 100);  // Repeat the packet four times with 100ms delay between
      WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
      WOL.sendMagicPacket(MACAddress);
    }
  } else {  // long sleep
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_LONG * uS_TO_S_FACTOR);
    DEBUG_PRINTLN("Setup ESP32 to sleep for " + String(TIME_TO_SLEEP_LONG) + " Seconds");
  }
  if (connected) {
    // now disconnect WiFi as it's no longer needed
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
  DEBUG_PRINTLN("Going to sleep now");
#ifdef DEBUG
  Serial.flush();
#endif
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();
  DEBUG_PRINTLN("This will never be printed");
}

void loop() {
  delay(100);
  DEBUG_PRINTLN("This is not going to be called");
}