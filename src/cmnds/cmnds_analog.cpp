// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#ifdef N32_CFG_ANALOG_IN_ENABLED
static bool analog_getPinFromChannelNum(u8 i_ChannelNumber, u8& o_PinNumber) {
    switch (i_ChannelNumber) {
    case 0:
        o_PinNumber = PIN_ANALOG_ADC0;
        return true;
    case 1:
        o_PinNumber = PIN_ANALOG_ADC1;
        return true;
    case 2:
        o_PinNumber = PIN_ANALOG_ADC2;
        return true;
    case 3:
        o_PinNumber = PIN_ANALOG_ADC3;
        return true;
    case 4:
        o_PinNumber = PIN_ANALOG_ADC4;
        return true;
    case 5:
        o_PinNumber = PIN_ANALOG_ADC5;
        return true;
    case 6:
        o_PinNumber = PIN_ANALOG_ADC6;
        return true;
    case 7:
        o_PinNumber = PIN_ANALOG_ADC7;
        return true;
    default:
        break;
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

void ANALOG_SetupPin(u8 i_PhysicalPin) {
    pinMode(i_PhysicalPin, INPUT);
}

int ANALOG_ReadChannel(u8 ChannelNumber) {
    switch (ChannelNumber) {
    case 0:
        return (analogRead(PIN_ANALOG_ADC0));
    case 1:
        return (analogRead(PIN_ANALOG_ADC1));
    case 2:
        return (analogRead(PIN_ANALOG_ADC2));
    case 3:
        return (analogRead(PIN_ANALOG_ADC3));
    case 4:
        return (analogRead(PIN_ANALOG_ADC4));
    case 5:
        return (analogRead(PIN_ANALOG_ADC5));
    case 6:
        return (analogRead(PIN_ANALOG_ADC6));
    case 7:
        return (analogRead(PIN_ANALOG_ADC7));
    default:
        DEBLN(F("ERR: bad ANALOG channel num!"));
        THROW_ERROR();

        return (0xFFFF);
    }
}

void ANALOG_ModuleInit() {
    // analogReference(INTERNAL2V56);

    u8 PhysicalPin;

    _FOR(pin, 0, ANALOG_NUM_OF_AVAIL_CHANNELS) {
        if (true == analog_getPinFromChannelNum(pin, PhysicalPin))
            ANALOG_SetupPin(PhysicalPin);
        else
        {
            THROW_ERROR();
            break;
        }

    }

    PIN_RegisterPins(analog_getPinFromChannelNum, ANALOG_NUM_OF_AVAIL_CHANNELS, F("ANALOG"));
}

int ANALOG_ReadChannelN(u8 ChannelNumber, u8 Count) {
    int sum = 0;
    u8 i = 0;

    for (i = 0; i < Count; i++)
        sum += ANALOG_ReadChannel(ChannelNumber);

    return (sum / Count);
}

module_caps_t ANALOG_getCapabilities(void) {
    module_caps_t mc = {
        .m_is_input = false,
        .m_number_of_channels = ANALOG_NUM_OF_AVAIL_CHANNELS,
        .m_module_name = F("ANALOG"),
        .m_mod_init = ANALOG_ModuleInit,
        .m_cmnd_decoder = NULL,
        .m_cmnd_executor = NULL
    };

    return(mc);
}

#endif // N32_CFG_ANALOG_IN_ENABLED
