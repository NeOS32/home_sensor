// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static debug_level_t uDebugLevel = DEBUG_WARN;

static void handler_Ethernet(void) { Ethernet.maintain(); }

void digitalClockDisplay() {
    //digital clock display of the time
#if 1 == N32_SERIAL_ENABLED
    DEB(hour());
    printDigits(minute());
    printDigits(second());
    DEB(F(" "));
    DEB(day());
    DEB(F(" "));
    DEB(month());
    DEB(F(" "));
    DEB(year());
    DEBLN();
#endif
}

void printDigits(int digits) {
    // utility function for digital clock display: prints preceding colon and
    // leading 0
#if 1 == N32_SERIAL_ENABLED
    DEB(F(":"));
    if (digits < 10)
        DEB('0');
    DEB(digits);
#endif
}


void loop() {
    static time_t prev_t;

#if 1==N32_CFG_WATCHDOG_ENABLED
    wdt_reset();
#endif // N32_CFG_WATCHDOG_ENABLED

    IF_DEB_L() {
        static u32 i = 0;
        i++;
        if (i % 20 == 0)
            MosqClient.publish(MQTT_DEBUG, String(F("Tick!")).c_str());
    }

#if 1==N32_CFG_ETH_ENABLED
    // low level ethernet section
    handler_Ethernet();
#endif // N32_CFG_ETH_ENABLED

    // MQTT section
    if (!MosqClient.connected())
        MQTT_reconnect();
    else
        MosqClient.loop();

    // TIME handling section
    time_t t = now(); // blocking funtion, that eventually calls NTP for curret
                      // time (via getNtpTime())

    // we just want to check it once a second
    if (prev_t != t) {
        TIMER_ProcessAllTimers();

        if (timeStatus() != timeSet) {
            IF_DEB_W() {
                MosqClient.publish(MQTT_DEBUG, String(F("ERR: NTP: time not fetched!")).c_str());
            }
        }
        else {
            IF_DEB_T() {
                digitalClockDisplay();
            }

            // UpTime updated
            if (0x0 == gUpTime)
                gUpTime = now();
        }

        prev_t = t;
    }

    // sleep 200ms
    Alarm.delay(200);
}
