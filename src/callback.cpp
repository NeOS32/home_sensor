/* 
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2021 Sebastian Serewa
 */

#include "my_common.h"

#define DEBUG_LOCAL 1
#ifdef DEBUG
#define DEBUG_LOCAL 1
#endif

#if 1 == N32_CFG_VALVE_ENABLED
static void turn_off_valve(unsigned int channel, unsigned int valve) {
    // decreas number of timers
    if (num_of_active_timers > 0)
        num_of_active_timers--;

    String str(" OFF: channel=");
    str += channel;
    str += ", valve=: ";
    str += valve;
    DEBLN(str);

    digitalWrite(valve, HIGH);    // sets the digital pin 13 on
    delay(VALVE_SWITCH_DELAY_MS); // waits VALVE_SWITCH_DELAY_MS ms
    digitalWrite(channel, HIGH);  // sets the digital pin 13 on

    if (num_of_active_timers <= 0) {
        delay(VALVE_SWITCH_DELAY_MS);              // aits DELAY_MS_SEQ ms
        digitalWrite(VALVE_SWITCH_DELAY_MS, HIGH); // sets the digital pin 13 on

        MosqClient.publish(MQTT_DEV_STATE, "Backup pin turned off!");
    }

    MosqClient.publish(MQTT_DEV_STATE, str.c_str()); // extra debug
}

static void turn_on_valve(unsigned int channel, unsigned int valve) {
    // increas number of timers
    num_of_active_timers++;

    String str(" ON: channel=");
    str += channel;
    str += ", valve=: ";
    str += valve;
    DEBLN(str);

    if (num_of_active_timers <= 1) {
        digitalWrite(VALVE_SWITCH_DELAY_MS, LOW); // sets the digital pin 13 on
        delay(VALVE_SWITCH_DELAY_MS); // aits VALVE_SWITCH_DELAY_MS ms

        MosqClient.publish(MQTT_DEV_STATE, "Backup pin turned on!");
    }
    digitalWrite(channel, LOW);   // sets the digital pin 13 on
    delay(VALVE_SWITCH_DELAY_MS); // aits VALVE_SWITCH_DELAY_MS ms
    digitalWrite(valve, LOW);     // sets the digital pin 13 on

    MosqClient.publish(MQTT_DEV_STATE, str.c_str()); // extra debug
}

static void set_valve_state_for_time(const state_t &s, boolean mode = INPUT) {
    if (s.seconds > 0) // means turn on
    {
        if (true == TimerStart(turn_off_valve, s.seconds, s.v1, s.v2)) // 30mins
            turn_on_valve(s.v1, s.v2); // Kanal {MKO}, Zawor [0..4]
    } else                             // means turn off
    {
        if (true == TimerStop(s.v1, s.v2)) // 30mins
            turn_off_valve(s.v1, s.v2);    // Kanal {MKO}, Zawor [0..4]
    }
}

u8 mapChannel2Pin(u8 Channel) {
    switch (Channel) {
    case 'M':
        return (PIN_VALVE_LINE_M);

    case 'K':
        return (PIN_VALVE_LINE_K);

    case 'O':
        return (PIN_VALVE_LINE_O);

    default:
        return (-1);
    }
}

u8 mapChannelValve2Pin(u8 Channel, u8 Valve) {
    if ('O' == Channel)
        switch (Valve) {
        case '0':
            return (PIN_VALVE_LINE_O_0);
        case '1':
            return (PIN_VALVE_LINE_O_1);
        case '2':
            return (PIN_VALVE_LINE_O_2);
        case '3':
            return (PIN_VALVE_LINE_O_3);
        }
    else if ('K' == Channel)
        switch (Valve) {
        case '0':
            return (PIN_VALVE_LINE_K_0);
        case '1':
            return (PIN_VALVE_LINE_K_1);
        case '2':
            return (PIN_VALVE_LINE_K_2);
        case '3':
            return (PIN_VALVE_LINE_K_3);
        }
    else if ('M' == Channel)
        switch (Valve) {
        case '0':
            return (PIN_VALVE_LINE_M_0);
        case '1':
            return (PIN_VALVE_LINE_M_1);
        case '2':
            return (PIN_VALVE_LINE_M_2);
        case '3':
            return (PIN_VALVE_LINE_M_3);
        }

    return (-1);
}

static bool decode_CMND_Z(const byte *payload, state_t &s) {
    char number;
    char scale;

    s.v1 = *payload++; // v1 = { M,K,O } - (K)anal
    s.v2 = *payload++; // v2 = [0..4] - Numer zaworu w skrzynce
    // seconds counting
    number = (*payload++) - '0'; // number = [1..9]
    scale = *payload++; // scale = [SMT] - Seconds/Minutes/TenMinutes/Hours
    s.seconds = getSecondsFromNumberAndScale(number, scale);
    s.sum = *payload++; // sum = 1

    // Info display
    String str("Valve: ");
    str += s.v1;
    str += ", Line: ";
    str += s.v2;
    str += ", Action: ";
    str += s.action;
    str += ", Sum: ";
    str += s.sum;
    DEB(str);

    s.v2 = mapChannelValve2Pin(s.v1, s.v2);
    s.v1 = mapChannel2Pin(s.v1);

    return (true);
}
#endif // N32_CFG_VALVE_ENABLED

static bool executeCmnd(state_t &s) {
    switch (s.action) {
#if 1 == N32_CFG_VALVE_ENABLED
    case 'Z': // Zawor
        return (set_valve_state_for_time(s));
#endif // N32_CFG_VALVE_ENABLED

#if 1 == N32_CFG_PWM_ENABLED
    case 'P': // PWM
        return (PWM_ExecuteCommand(s));
#endif // N32_CFG_PWM_ENABLED

#if 1 == N32_CFG_LED_W2918_ENABLED
    case 'L': // LED
        return (LED_ExecuteCommand(s));
#endif // N32_CFG_LED_W2918_ENABLED

#if 1 == N32_CFG_BIN_OUT_ENABLED
    case 'B': // Bin OUT
        return (BIN_OUT_ExecuteCommand(s));
#endif // N32_CFG_BIN_OUT_ENABLED

#if 1 == N32_CFG_BIN_IN_ENABLED
    case 'I': // Bin IN
        return (BIN_IN_ExecuteCommand(s));
#endif // N32_CFG_BIN_IN_ENABLED

#if 1 == N32_CFG_TEMP_ENABLED
    case 'T': // TEMP
        return (TEMP_ExecuteCommand(s));
#endif // N32_CFG_TEMP_ENABLED

#if 1 == N32_CFG_HISTERESIS_ENABLED
    case 'H': // HYST
        return (HYST_ExecuteCommand(s));
#endif // N32_CFG_HISTERESIS_ENABLED

    default:
        return (false);
    }
}

u32 getSecondsFromNumberAndScale(char number, char scale) {
    if ((number > 9) || (number < 0))
        number = 0;
    else
        switch (scale) {
        case 'S': // Seconds
            return ((u32)number);

        case 'M': // Minutes
            return ((u32)60 * number);

        case 'T': // TenMinutes
            return ((u32)10 * 60 * number);

        case 'H': // Hours
            return ((u32)60 * 60 * number);

        default:
            return ((u32)0);
        }

    return (number);
}

u8 getDecodedChannelNum(u8 uRawNumber) {
    if (uRawNumber <= 9)
        return (uRawNumber);

    uRawNumber += '0';
    uRawNumber -= 'A';

    if (uRawNumber <= 5)
        return (10 + uRawNumber);

    return (CMNDS_NULL); // FAILED
}

void MQTT_callback(char *topic, byte *payload, unsigned int length) {
    bool msg_accepted = false;

#if 1 == DEBUG_LOCAL
    DEB(F("Msg received ["));
    DEB(topic);
    DEB(F("] '"));
    _FOR(i, 0, (int)length)
    DEB((char)payload[i]);
    DEBLN(F("'"));
#endif

    // do we have a match?
    if (0 == strcmp(MQTT_DEVICES_CMNDS, topic)) {
        state_t s;

        // command processing
        s.action = *payload++; // ACTION

        switch (s.action) {
#if 1 == N32_CFG_VALVE_ENABLED
        case 'Z': // Zawor
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
            DEBLN(F(" - unknown action command - ignored!"));
            break;
        }

        if (true == msg_accepted) {
            // DEBLN(" - OK!");
            if (false == executeCmnd(s)) {
                // DEBLN(str);
                MosqClient.publish(MQTT_DEBUG, " cmnd execution failed!");
            }
        } else
            DEBLN(F(" - failed, message ignored!"));
    } else {
       DEB(" ignored!");
    }
}
