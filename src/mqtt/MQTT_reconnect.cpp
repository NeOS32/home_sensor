// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static debug_level_t uDebugLevel = DEBUG_LOG;

void MQTT_reconnect() {

    DEB(F("Connecting to MQTT broker: "));

    // looping until we're connected
    while (!gClient_Mosq.connected()) {

        // Attempt to connect
        if (gClient_Mosq.connect(MQTT_CLIENT_NAME)) {

            // Once connected, publish an announcement...
            IF_DEB_L() {
                String str(MQTT_CLIENT_NAME);
                str += F(" - connected!");
                DEB_L(str);
                MSG_Publish(MQTT_ARD_DEVICES_STATUS, str.c_str());
            }

            // ... and resubscribe
            gClient_Mosq.subscribe(MQTT_DEVICES_CMNDS);
        }
        else {
            DEB_W(F("ERR: failed, rc="));
            DEB_W(gClient_Mosq.state());
            DEB_W(F(", trying again soon\n"));
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
