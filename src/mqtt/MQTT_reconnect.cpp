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
#if 1 == N32_CFG_MQTT_SECURE
        char l_user[MQTT_MAX_USER_LENGTH + 1];
        char l_passwd[MQTT_MAX_PASSWD_LENGTH + 1];
        strcpy_P(l_user, (char*)pgm_read_word(&g_mqtt_user));
        strcpy_P(l_passwd, (char*)pgm_read_word(&g_mqtt_passwd));

        if (gClient_Mosq.connect(MQTT_CLIENT_NAME, l_user, l_passwd)) {
#else
        if (gClient_Mosq.connect(MQTT_CLIENT_NAME)) {
#endif
            // Once connected, publish an announcement...
            IF_DEB_L() {
                String str(MQTT_CLIENT_NAME);
                str += F(" - connected!");
                MSG_Publish_State( str.c_str());
                DEB_L(str);
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
