// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static debug_level_t uDebugLevel = DEBUG_WARN;

void alarm_15s() {
}

void alarm_30s() {

#if 1==N32_CFG_ANALOG_IN_ENABLED
    _FOR(i, 0, ANALOG_NUM_OF_AVAIL_CHANNELS) {
        String tim_str(ANALOG_ReadChannel(i));
        String path_str(MQTT_SENSORS_ANALOG);
        path_str += i;
        MSG_Publish(path_str.c_str(), tim_str.c_str());
        DEB_L(tim_str);
    }
#endif // 1==N32_CFG_ANALOG_IN_ENABLED

#if 1==N32_CFG_HISTERESIS_ENABLED
    u32 tempScaled = HYSTERESIS_getTempScaled(0);
    u32 remainder = (tempScaled % 4096) * 10;
    IF_DEB_L() {
        String str = F(" Ch0T=");
        str += tempScaled >> 12;
        str += F(".");
        str += remainder >> 12;
        str += F("C, Ch1T=");
        str += HYSTERESIS_getTemp(1);
        str += F("C");
        MSG_Publish_Debug( str.c_str());
        //DEB_L(str);
    }
    String str(tempScaled >> 12);
    str += F(".");
    str += remainder >> 12;
    String str1(MQTT_SENSORS_T);
    str1 += MQTT_SENSORS_T_MATA;
    MSG_Publish(str1.c_str(), str.c_str());
#endif
}

#define _SEC (1)
#define _MIN ((u32)60 * _SEC)
#define _HOUR ((u32)60 * _MIN)
#define _DAY ((u32)24 * _HOUR)
static String retUpTime(void) {
    time_t diff = now() - gUpTime;
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
    return str;
}

static void timers_ShowErrors(void) {
    String str(MQTT_CLIENT_NAME);
    str += F(": ");
    str += ERR_GetNumberOfGlobalErrors();
    if (false == MSG_Publish(MQTT_ARD_DEVICES_ERRORS, str.c_str())) {
        DEBLN(F("Failed with publishing! (probably to long)"));
    }
}

// once a minute, we want to read all avail nodes addresses and publish it to
// the broker
void alarm_1m() {
    String str(MQTT_CLIENT_NAME);
    str += F(": Up=");
    str += retUpTime();

    if (false == MSG_Publish_State(str.c_str()))
        DEBLN(F("Failed with publishing! (probably to long)"));

    String str1(F("Active timers: "));
    str1 += MAX_TIMERS - TIMER_GetNumberOfFreeTimers();
    str1 += F(", Errs: ");
    str1 += ERR_GetNumberOfGlobalErrors();
    if (false == MSG_Publish_State(str1.c_str())) {
        DEBLN(F("Failed with publishing! (probably to long)"));
    }

    PIN_DisplayAssignments();
    TIMER_PrintActiveTimers();
    CMNDS_DisplayAssignments();
#if 1==N32_CFG_HISTERESIS_ENABLED
    HIST_DisplayAssignments();
#endif // 1==N32_CFG_HISTERESIS_ENABLED
#if 1==N32_CFG_QUICK_ACTIONS_ENABLED
    QA_DisplayAssignments();
#endif // 1 == N32_CFG_QUICK_ACTIONS_ENABLED
}

void alarm_2m() {
    timers_ShowErrors();
#if 1==N32_CFG_WITH_ERR_STORAGE
    ERR_ShowGlobalErrors();
#endif // N32_CFG_WITH_ERR_STORAGE
}

// once a minute, we want to read all avail nodes addresses and publish it to
// the broker
void alarm_15m() {
    String str(MQTT_CLIENT_NAME);
    str += F(": ");
    str += APP_VERSION;
    str += F(", BuildTime: ");
    str += __DATE__;
    str += F(" ");
    str += __TIME__;
    if (false == MSG_Publish_State(str.c_str())) {
        DEB_W(F("Failed with publishing! (probably to long)"));
    }

    DEB_L(str);
}

void alarm_1h() {
    DEBLN(F("1h seconds timer"));
}
