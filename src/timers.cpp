// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#ifdef DEBUG
#define DEBUG_LOCAL 1
#endif

void alarm_15s() {
}

void alarm_30s() {

#if 1==N32_CFG_ANALOG_IN_ENABLED
    _FOR(i, 0, ANALOG_NUM_OF_AVAIL_CHANNELS) {
        String tim_str(ANALOG_ReadChannel(i));
        String path_str(MQTT_SENSORS_ANALOG);
        path_str += i;
        MosqClient.publish(path_str.c_str(), tim_str.c_str());
#if 1==DEBUG_LOCAL
        DEBLN(tim_str);
#endif // 1==DEBUG_LOCAL
    }
#endif // 1==N32_CFG_ANALOG_IN_ENABLED

#if 1==N32_CFG_HISTERESIS_ENABLED
    u32 tempScaled = HYSTERESIS_getTempScaled(0);
    u32 remainder = (tempScaled % 4096) * 10;
#ifdef DEBUG_LOCAL
    String str = F(" Ch0T=");
    str += tempScaled >> 12;
    str += F(".");
    str += remainder >> 12;
    str += F("C, Ch1T=");
    str += HYSTERESIS_getTemp(1);
    str += F("C");
    MosqClient.publish(MQTT_DEBUG, str.c_str());
    // DEBLN(tim_str);
#endif
    str = tempScaled >> 12;
    str += F(".");
    str += remainder >> 12;
    String str1(MQTT_SENSORS_T);
    str1 += MQTT_SENSORS_T_MATA;
    MosqClient.publish(str1.c_str(), str.c_str());
#endif
}

#define _SEC (1)
#define _MIN ((u32)60 * _SEC)
#define _HOUR ((u32)60 * _MIN)
#define _DAY ((u32)24 * _HOUR)
static String retUpTime(void) {
    time_t diff = now() - timeUpTime;
    u32 days = diff / _DAY;
    diff -= days * _DAY;
    u32 hours = diff / _HOUR;
    diff -= hours * _HOUR;
    u32 mins = diff / _MIN;
    diff -= mins * _MIN;

    String str;
    str += days;
    str += F(" days, ");
    str += hours;
    str += F(" hours, ");
    str += mins;
    str += F(" mins");
    return (str);
}

static void timers_ShowErrors(void) {
    String str(MQTT_CLIENT_NAME);
    str += F(": ");
    str += ERR_GetNumberOfGlobalErrors();
    if (false == MosqClient.publish(MQTT_ARD_DEVICES_ERRORS, str.c_str())) {
        DEBLN(F("Failed with publishing! (probably to long)"));
    }
}

// once a minute, we want to read all avail nodes addresses and publish it to
// the broker
void alarm_1m() {
    String str(MQTT_CLIENT_NAME);
    str += F(": Up=");
    str += retUpTime();

    if (false == MosqClient.publish(MQTT_DEV_STATE, str.c_str()))
        DEBLN(F("Failed with publishing! (probably to long)"));

    String str1(F("Active timers: "));
    str1 += MAX_TIMERS - TIMER_GetNumberOfFreeTimers();
    str1 += F(", Errs: ");
    str1 += ERR_GetNumberOfGlobalErrors();
    if (false == MosqClient.publish(MQTT_DEV_STATE, str1.c_str())) {
        DEBLN(F("Failed with publishing! (probably to long)"));
    }

    PIN_DisplayAssignments();
    TIMER_PrintActiveTimers();
    CMNDS_DisplayAssignments();
#if 1==N32_CFG_HISTERESIS_ENABLED
    HIST_DisplayAssignments();
#endif // 1==N32_CFG_HISTERESIS_ENABLED

}

void alarm_2m() {
    timers_ShowErrors();
    ERR_ShowGlobalErrors();
}

// once a minute, we want to read all avail nodes addresses and publish it to
// the broker
void alarm_15m() {
    String str(MQTT_CLIENT_NAME);
    str += F(": Ver:");
    str += APP_VERSION;
    str += F(", BuildTime: ");
    str += __DATE__;
    str += F(" ");
    str += __TIME__;
    if (false == MosqClient.publish(MQTT_DEV_STATE, str.c_str())) {
        DEBLN(F("Failed with publishing! (probably to long)"));
    }

#if 1==DEBUG_LOCAL
    DEBLN(str);
#endif
}

void alarm_1h() {
    DEBLN(F("1h seconds timer"));
}
