// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

void reconnect() {
    DEB(F("Connecting to MQTT broker: "));

    // Attempt to connect
    if (MosqClient.connect(MQTT_CLIENT_NAME)) {
        DEBLN(F("connected!"));
        // Once connected, publish an announcement...
        String str(MQTT_CLIENT_NAME);
        str += F(" - reconnected!");
        MosqClient.publish(MQTT_ARD_DEVICES_CONTROL, str.c_str());
        // ... and resubscribe
        MosqClient.subscribe(MQTT_DEVICES_CMNDS);
    }
    else {
        DEB(F("ERR: failed, rc="));
        DEB(MosqClient.state());
        DEBLN(F(", trying again soon"));
    }
}
