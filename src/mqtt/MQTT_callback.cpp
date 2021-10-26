// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static debug_level_t uDebugLevel = DEBUG_LOG;

void MQTT_callback(char* topic, byte* payload, unsigned int length) {
    bool msg_accepted = false;

    IF_DEB_L() {
        DEB(F("Msg received ["));
        DEB(topic);
        DEB(F("] '"));
        _FOR(i, 0, (int)length)
            DEB((char)payload[i]);
        DEB(F("'\n"));
    }

    // do we have a match?
    if (0 == strcmp(MQTT_DEVICES_CMNDS, topic)) {
        state_t s;

        // command processing
        s.action = *payload++; // ACTION

        switch (s.action) {
#if 1 == N32_CFG_VALVE_ENABLED
        case 'Z': // valve
            decode_CMND_Z(payload, s);
            break;
#endif // N32_CFG_VALVE_ENABLED

#if 1 == N32_CFG_PWM_ENABLED
        case 'P': // PWM
            msg_accepted = decode_CMND_P(payload, s);
            break;
#endif // N32_CFG_PWM_ENABLED

#if 1 == N32_CFG_HISTERESIS_ENABLED
        case 'H': // HYST
            msg_accepted = decode_CMND_H(payload, s);
            break;
#endif // N32_CFG_HISTERESIS_ENABLED

#if 1 == N32_CFG_BIN_OUT_ENABLED
        case 'B': // BIN OUT
            msg_accepted = decode_CMND_B(payload, s);
            break;
#endif // N32_CFG_BIN_OUT_ENABLED

#if 1 == N32_CFG_BIN_IN_ENABLED
        case 'I': // BIN IN
            msg_accepted = decode_CMND_I(payload, s);
            break;
#endif // N32_CFG_BIN_IN_ENABLED

#if 1 == N32_CFG_LED_W2918_ENABLED
        case 'L': // LED
            msg_accepted = decode_CMND_L(payload, s);
            break;
#endif // N32_CFG_LED_W2918_ENABLED

#if 1 == N32_CFG_TEMP_ENABLED
        case 'T': // TEMP
            msg_accepted = decode_CMND_T(payload, s);
            break;
#endif // N32_CFG_TEMP_ENABLED

        default:
            DEB_L(F(" - unknown action command - ignored!"));
            break;
        }

        if (true == msg_accepted) {
            // DEBLN(" - OK!");
            if (false == CMNDS_executeCmnd(s)) {
                // DEBLN(str);
                MSG_Publish(MQTT_DEBUG, " cmnd execution failed!");
            }
        }
        else
            DEB_W(F(" - failed, message ignored!"));
    }
    else {
        DEB_T(F(" ignored!"));
    }
}
