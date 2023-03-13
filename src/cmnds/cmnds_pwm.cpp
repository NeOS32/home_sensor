// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#if 1==N32_CFG_PWM_ENABLED

#define PWM_FADE_IN_DELAY_IN_MS (3)
#define PWM_FADE_OUT_DELAY_IN_MS (20)
#define PWM_MAX (0xFF)

#ifdef DEBUG
//#define DEBUG_LOCAL 1
#endif


static bool pwm_getPinFromChannelNum(u8 i_ChannelNumber, u8& o_PinNumber) {
    if (i_ChannelNumber < PWM_NUM_OF_AVAIL_CHANNELS) {
        o_PinNumber = PWM_PIN_NUM_FIRST + i_ChannelNumber;
        return true;
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

static void pwm_cmnd_FADE_IN(actions_context_t& i_rActionsContext) {
    u8 PhysicalPin;
    if (false == pwm_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPin)) {
        THROW_ERROR();
        return; // error, means function failed to execute command
    }
    if (0xFF != i_rActionsContext.var2)
        while (i_rActionsContext.var2 < 255) {
            i_rActionsContext.var2++;
            analogWrite(PhysicalPin, i_rActionsContext.var2);
            delay(PWM_FADE_IN_DELAY_IN_MS); // 50ms?
        }
    else
        analogWrite(PhysicalPin, i_rActionsContext.var2);
}
static void pwm_Panic(u8 i_PhysicalPin) {
    analogWrite(i_PhysicalPin, 0);
}

static void pwm_cmnd_FADE_OUT(actions_context_t& i_rActionsContext) {
    u8 PhysicalPin;
    if (false == pwm_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPin)) {
        pwm_Panic(PhysicalPin);
        THROW_ERROR();
        return; // error, means function failed to execute command
    }

    if (i_rActionsContext.var2 > 0)
        while (i_rActionsContext.var2 > 0) {
            i_rActionsContext.var2--;
            analogWrite(PhysicalPin, i_rActionsContext.var2);
            delay(PWM_FADE_IN_DELAY_IN_MS); // 50ms?
        }
    else
        analogWrite(PhysicalPin, i_rActionsContext.var2);
}
static void pwm_cmnd_ON_RANDOM(actions_context_t& i_rActionsContext) {
    u8 PhysicalPin;
    if (false == pwm_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPin)) {
        pwm_Panic(PhysicalPin);
        THROW_ERROR();
        return; // error, means function failed to execute command
    }

    i_rActionsContext.var2 = 128; // TODO: get random value here
    analogWrite(PhysicalPin, i_rActionsContext.var2);
}
static void pwm_cmnd_ON(actions_context_t& i_rActionsContext) {
    u8 PhysicalPin;
    if (false == pwm_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPin)) {
        pwm_Panic(PhysicalPin);
        THROW_ERROR();
        return; // error, means function failed to execute command
    }

    i_rActionsContext.var2 = 0xFF;
    analogWrite(PhysicalPin, i_rActionsContext.var2);
}
static void pwm_cmnd_OFF(actions_context_t& i_rActionsContext) {
    u8 PhysicalPin;
    if (false == pwm_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPin)) {
        pwm_Panic(PhysicalPin);
        THROW_ERROR();
        return; // error, means function failed to execute command
    }

    i_rActionsContext.var2 = 0x0;
    analogWrite(PhysicalPin, i_rActionsContext.var2);
}

// TODO: finish for all pins
static void pwm_cmnd_ALL_ON(actions_context_t& i_rActionsContext) {
    u8 PhysicalPin;
    if (false == pwm_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPin)) {
        pwm_Panic(PhysicalPin);
        THROW_ERROR();
        return; // error, means function failed to execute command
    }

    i_rActionsContext.var2 = 0xFF;
    analogWrite(PhysicalPin, i_rActionsContext.var2);
}
static void pwm_cmnd_ALL_OFF(actions_context_t& i_rActionsContext) {
    u8 PhysicalPin;
    if (false == pwm_getPinFromChannelNum(i_rActionsContext.var1, PhysicalPin)) {
        pwm_Panic(PhysicalPin);
        THROW_ERROR();
        return; // error, means function failed to execute command
    }

    i_rActionsContext.var2 = 0x0;
    analogWrite(PhysicalPin, i_rActionsContext.var2);
}

void pwm_SetupChannel(u8 i_uLogChannel) {
    u8 PhysicalPin;
    if (false == pwm_getPinFromChannelNum(i_uLogChannel, PhysicalPin)) {
        pwm_Panic(PhysicalPin);
        THROW_ERROR();
        return; // error, means function failed to execute command
    }

    pinMode(PhysicalPin, OUTPUT);
    analogWrite(PhysicalPin, LOW);
}

void PWM_ModuleInit(void) {
    _FOR(channel, 0, PWM_NUM_OF_AVAIL_CHANNELS)
        pwm_SetupChannel(channel);

    PIN_RegisterPins(pwm_getPinFromChannelNum, PWM_NUM_OF_AVAIL_CHANNELS, F("PWM"));
}

/**
 * PA1xxxNS - Channel "A" is on (== 100%) for NS secs
 * PA2xxxNS - Fade in in channel "A" ( from current to fully light).
 *            Wait NS secs and then fade out.
 * PA3BBBNS - Set PWM in channel "A" to value "BBB" (max value is 100) in
 * BCD for NS secs Px4xxxNS - All channels are ON for NS secs PA5xxxNS - Set
 * random value in channel "A" for NS secs Px6xxxNS - Set random value in
 * all channels for NS secs
 */
bool PWM_ExecuteCommand(const state_t& s) {

    actions_t Actions; //{bin_ChannelTurnON, bin_ChannelTurnOFF};
    actions_context_t ActionsContext;
    ActionsContext.slot = s.c.p.channel;
    ActionsContext.timer_id = 0;
    ActionsContext.var1 = 0;
    ActionsContext.var2 = 0; // in var2 is current PWM value
    u8 slot;

    if (CMNDS_NULL != (slot = CMNDS_GetSlotNumber(s))) {
        switch (s.command) {
        case CMND_PWM_CHANNEL_ON_FOR_NS:
            Actions.fun_start = pwm_cmnd_ON;
            Actions.fun_stop = pwm_cmnd_OFF;
            return (CMNDS_ScheduleAction(s, Actions, ActionsContext));

        case CMND_PWM_ALL_CHANNELS_ON_FOR_NS:
            Actions.fun_start = pwm_cmnd_ALL_ON;
            Actions.fun_stop = pwm_cmnd_ALL_OFF;
            return (CMNDS_ScheduleAction(s, Actions, ActionsContext));

        case CMND_PWM_SET_RANDOM_IN_CHANNEL:
            Actions.fun_start = pwm_cmnd_ON_RANDOM;
            Actions.fun_stop = pwm_cmnd_OFF;
            return (CMNDS_ScheduleAction(s, Actions, ActionsContext));

        case CMND_PWM_SET_RANDOM_IN_CHANNELS:
            return (false); // NOT IMPLEMENTED YET

        case CMND_PWM_FADE: // done
            Actions.fun_start = pwm_cmnd_FADE_IN;
            Actions.fun_stop = pwm_cmnd_FADE_OUT;
            return (CMNDS_ScheduleAction(s, Actions, ActionsContext));

        case CMND_PWM_FADE_ALL_CHANNELS: // NOT IMPLEMENTED YET
            return (false);              // NOT IMPLEMENTED YET

        case CMND_PWM_SET_CHANNEL: // DONE
            analogWrite(s.c.p.pin, map(s.c.p.percentage, 0, 100, 0, 255));
            // CHANNELS_States[s.c.p.channel].last_cmd = s.c.p.command;
            return (true);

        case CMND_PWM_RESET_CHANNEL: // DONE
            return (CMNDS_ResetSlotState(slot));

        case CMND_PWM_RESET_ALL_CHANNELS: // DONE
            return (CMNDS_ResetAllSlotStates(s));
        }
    }

    THROW_ERROR();
    return false; // error
}

/**
 * P1AxxxNS - Channel "A" is on (== 100%) for NS secs
 * P2AxxxNS - Fade in in channel "A" ( from current to fully light).
 *            Wait NS secs and then fade out.
 * P3AxxxNS - Fade in in all channels ( from current to fully light).
 *            Wait NS secs and then fade out.
 * P4ABBBNS - Set PWM in channel "A" to value "BBB" (max value is 100) in
 * BCD for NS secs P5xxxxNS - All channels are ON for NS secs P6AxxxNS - Set
 * random value in channel "A" for NS secs P7xxxxNS - Set random value in
 * all channels for NS secs P8Axxxyy - Reset channel "A" to default state
 * P9Axxxyy - Reset all channels to default state
 */
bool decode_CMND_P(const byte* payload, state_t& s, u8* o_CmndLen) {
    const byte* cmndStart = payload;

    s.command = (*payload++) - '0';     // [0..8] - command
    s.c.p.channel = (*payload++) - '0'; // [0..9] - Channel

    s.c.p.percentage = 100 * ((*payload++) - '0'); // [0..1] - percentage
    s.c.p.percentage += 10 * ((*payload++) - '0'); // [0..9] - percentage
    s.c.p.percentage += (*payload++) - '0';        // [0..9] - percentage

    char number = (*payload++) - '0'; // [0..9] - number
    char scale = *payload++; // [SMTH] - Seconds/Minutes/TenMinutes/Hours
    s.count = getSecondsFromNumberAndScale(number, scale);

    bool sanity_ok = false;

    if (true == pwm_getPinFromChannelNum(s.c.p.channel, s.c.p.pin)) {
        s.sum = (*payload++) - '0'; // sum = 1

        if (s.command >= 0 && s.command <= CMND_PWM_MAX_VALUE)
            if (s.c.p.channel >= 0 && s.c.p.channel < PWM_NUM_OF_AVAIL_CHANNELS)
                if (s.c.p.percentage >= 0 && s.c.p.percentage <= 100)
                    if (number >= 0 && number <= 9)
                        if (true == isSumOk(s))
                            sanity_ok = true;
    }

    // Info display
#if 1==DEBUG_LOCAL
    u8 i = s.c.p.channel;
    String str(F("PWM: channel: "));
    str += i;
    str += F(", Cmd: ");
    i = (s.command);
    str += i;
    str += F(", Sanity: ");
    str += sanity_ok;
    str += F(", Secs: ");
    str += (s.count);
    str += F(", Sum: ");
    str += (s.sum + '0');
    //DEBLN(str);
    MSG_Publish_Debug(str.c_str());
#endif // DEBUG_LOCAL

    // setting decoded and valid cmnd length
    if (NULL != o_CmndLen)
        *o_CmndLen = payload - cmndStart;

    return (sanity_ok);
}

module_caps_t PWM_getCapabilities(void) {
    module_caps_t mc = {
        .m_is_input = false,
        .m_number_of_channels = PWM_NUM_OF_AVAIL_CHANNELS,
        .m_module_name = F("PWM"),
        .m_mod_init = PWM_ModuleInit,
        .m_cmnd_decoder = decode_CMND_P,
        .m_cmnd_executor = PWM_ExecuteCommand
    };

    return(mc);
}
#endif // 1==N32_CFG_PWM_ENABLED
