TODO
====
* add saving settings to RTC:
    * last move detection settings
    * flag for reset cause 

* publish telemetries on mqtt:
    * free memory
    * uptime
    * last reboot reason and time

* Web interface
    * enable/disable pirs
    * enable/disablt global light

* add watchdog for loop function:
    * see: OsWatchLoop OsWatchInit OsWatchTicker from tasmota
``` C
void OsWatchTicker(void)
{
  uint32_t t = millis();
  uint32_t last_run = abs(t - oswatch_last_loop_time);

#ifdef DEBUG_THEO
  int32_t rssi = WiFi.RSSI();
  AddLog_P2(LOG_LEVEL_DEBUG, PSTR(D_LOG_APPLICATION D_OSWATCH " FreeRam %d, rssi %d %% (%d dBm), last_run %d"), ESP_getFreeHeap(), WifiGetRssiAsQuality(rssi), rssi, last_run);
#endif  // DEBUG_THEO
  if (last_run >= (OSWATCH_RESET_TIME * 1000)) {
//    AddLog_P(LOG_LEVEL_INFO, PSTR(D_LOG_APPLICATION D_OSWATCH " " D_BLOCKED_LOOP ". " D_RESTARTING));  // Save iram space
    RtcSettings.oswatch_blocked_loop = 1;
    RtcSettingsSave();

//    ESP.restart();  // normal reboot
//    ESP.reset();  // hard reset

    // Force an exception to get a stackdump
    volatile uint32_t dummy;
    dummy = *((uint32_t*) 0x00000000);
  }
}

void OsWatchInit(void)
{
  oswatch_blocked_loop = RtcSettings.oswatch_blocked_loop;
  RtcSettings.oswatch_blocked_loop = 0;
  oswatch_last_loop_time = millis();
  tickerOSWatch.attach_ms(((OSWATCH_RESET_TIME / 3) * 1000), OsWatchTicker);
}

void OsWatchLoop(void)
{
  oswatch_last_loop_time = millis();
//  while(1) delay(1000);  // this will trigger the os watch
}
```