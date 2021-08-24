// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#ifdef DEBUG
#define DEBUG_LOCAL 1
#endif

static void handler_Ethernet(void) { Ethernet.maintain(); }

void digitalClockDisplay() {
    //digital clock display of the time
#if 1 == N32_SERIAL_ENABLED
    DEB(hour());
    printDigits(minute());
    printDigits(second());
    DEB( F(" "));
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
    DEB( F(":"));
    if (digits < 10)
        DEB( '0');
    DEB(digits);
#endif
}

void loop() {
#if 1==N32_CFG_WATCHDOG_ENABLED
    wdt_reset();
#endif // N32_CFG_WATCHDOG_ENABLED

    // TIME handling section
    time_t t = now(); // blocking funtion, that eventually calls NTP for curret
                      // time (via getNtpTime())
    static time_t prev_t;

#if 1==DEBUG_LOCAL
    // DEBLN("Tick!");
#endif // DEBUG_LOCAL

    // MQTT section
    if (!MosqClient.connected())
        MQTT_reconnect();
    MosqClient.loop();

    handler_Ethernet();

    // we just want to check it once a second
    if (prev_t != t) {
        TIMER_ProcessAllTimers();

        if (timeStatus() != timeSet) {
#if 1==DEBUG_LOCAL
            MosqClient.publish( MQTT_DEBUG, String(F( "ERR: NTP: time not fetched!") ).c_str() );
#endif // DEBUG_LOCAL
        } else {
            // digitalClockDisplay();

            // UpTime updated
            if (0x0 == timeUpTime)
                timeUpTime = now();
        }

        prev_t = t;
    }

    // sleep 200ms
    Alarm.delay(200);
}
