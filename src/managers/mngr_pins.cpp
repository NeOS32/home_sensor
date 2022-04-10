// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#ifdef DEBUG
#define DEBUG_LOCAL 1
#endif

#define MAX_PIN_ASSIGNMENTS (12)
#define MAX_PINS (100)
static pin_group_t PINS[MAX_PIN_ASSIGNMENTS] = {};
static u8 count_pin_groups = 0;

static bool doCheckIfNewModIsOk(getPhysicalPinFromLogical_f i_fTransform,
    int i_Count,
    const __FlashStringHelper* i_pModule,
    int i_LogicalFirstPin) {
    u8 uModFirstPhysicalPinNumber = 0, uModLastPhysicalPinNumber = 0;
    u8 uFirstPhysicalPinNumber = 0, uLastPhysicalPinNumber = 0;

    // checking physiacl pin numbers
    if (false == i_fTransform(i_LogicalFirstPin, uFirstPhysicalPinNumber))
        return false;
    if (false == i_fTransform(i_LogicalFirstPin + i_Count - 1, uLastPhysicalPinNumber))
        return false;

    _FOR(i, 0, count_pin_groups) {
        if (0 == PINS[i].m_Fun)
            return false;

        if (false == PINS[i].m_Fun(PINS[i].m_log_first_pin, uModFirstPhysicalPinNumber))
            return false;
        if (false == PINS[i].m_Fun(PINS[i].m_log_first_pin + PINS[i].m_pins_count - 1, uModLastPhysicalPinNumber))
            return false;

        PINS[count_pin_groups].m_module_name = i_pModule;

        if ((uFirstPhysicalPinNumber >= uModFirstPhysicalPinNumber && uFirstPhysicalPinNumber <= uModLastPhysicalPinNumber) ||
            (uLastPhysicalPinNumber >= uModFirstPhysicalPinNumber && uLastPhysicalPinNumber <= uModLastPhysicalPinNumber)) {
                {
                    String str(F("ERR: A few of physical pin numbers: "));
                    str += uFirstPhysicalPinNumber;
                    str += F("..");
                    str += uLastPhysicalPinNumber;
                    str += F(" (from module '");
                    str += i_pModule;
                    str += F("'), have already been registered with module: 'Name='");
                    str += PINS[i].m_module_name;
                    str += F("'\n");
                    MSG_Publish(String(MQTT_DEBUG).c_str(), str.c_str());
#if 1 == DEBUG_LOCAL
                    DEBLN(str);
#endif
                    return (false);
                }
        }
    }

    return (true);
}

bool PIN_RegisterPins(getPhysicalPinFromLogical_f i_fTransform, int i_Count,
    const __FlashStringHelper* i_pModule,
    int i_LogicalFirstPin) {
        
    if (false == doCheckIfNewModIsOk(i_fTransform, i_Count, i_pModule, i_LogicalFirstPin))
        return false;

    if (count_pin_groups < MAX_PIN_ASSIGNMENTS) {
        PINS[count_pin_groups].m_Fun = i_fTransform;
        PINS[count_pin_groups].m_pins_count = i_Count;
        PINS[count_pin_groups].m_log_first_pin = i_LogicalFirstPin;
        PINS[count_pin_groups].m_module_name = i_pModule;
        count_pin_groups++;
        return true;
    }
    else
        return false;
}

void PIN_DisplayAssignments(void) {
    String str;

    {
        String str_header(F("Pin assignments:\n-=-=-=-=-="));
        MSG_Publish_Debug(str_header.c_str());
    }

    u8 PhysicalPinNumber, uTotalPins = 0;
    _FOR(i, 0, count_pin_groups) {
        str = F(" ");
        str += i + 1;
        str += F(": Name=");
        str += PINS[i].m_module_name;
        str += F(", Count=");
        str += PINS[i].m_pins_count;
        str += F(", Physical=[");

        // if ( 0x0 != PINS[i].m_Fun(i) )
        _FOR(j, 0, PINS[i].m_pins_count) {
            if (true == PINS[i].m_Fun(j + PINS[i].m_log_first_pin, PhysicalPinNumber))
                str += PhysicalPinNumber;
            else
                str += F("??");

            if (j < PINS[i].m_pins_count - 1)
                str += F(", ");
        }
        str += F("]");
        uTotalPins += PINS[i].m_pins_count;

        MSG_Publish(String(MQTT_DEBUG).c_str(), str.c_str());
#if 1 == DEBUG_LOCAL
        DEBLN(str);
#endif
    }
    str = F("Total: ");
    str += uTotalPins;
    str += F(" pins assigned.");
    MSG_Publish(String(MQTT_DEBUG).c_str(), str.c_str());
}

void PIN_GetPinGroupInfo(pint_type_e i_PinType, u16& i_uFirstPing,
    u16& i_uPingCount) {}