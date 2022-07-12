// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static debug_level_t uDebugLevel = DEBUG_WARN;

void MQTT_callback(char* topic, byte* payload, unsigned int length) {
    IF_DEB_L() {
        DEB(F("Msg received ["));
        DEB(topic);
        DEB(F("] '"));
        _FOR(i, 0, (int)length)
            DEB((char)payload[i]);
        DEB(F("'\n"));
    }

    // do we have a match?
    if (0 == strcmp(MQTT_DEVICES_CMNDS, topic))
        CMNDS_Launch(payload);
    else {
        IF_DEB_T() {
            DEB_T(F(" ignored!"));
        }
    }
}
