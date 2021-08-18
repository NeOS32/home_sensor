// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#if 1==N32_CFG_HISTERESIS_ENABLED

#define DEFAULT_DELAY_FOR_CONVERSION_IN_S 5
#define TEMP_MAX (0xFF)

/*0C  - 42k
25C - 15k
50C - 6k*/

//#define K (1000)
//#define TEMP_RESISTANCE_0C (42L * (K))
//#define TEMP_RESISTANCE_20C (18L * (K))
//#define TEMP_RESISTANCE_25C (15L * (K))
//#define TEMP_RESISTANCE_50C (6L * (K))

#define TEMP_COEFFICIENT_20C (60)
#define TEMP_COEFFICIENT_25C (260)

#define HIST_MIN_TEMP_IN_C (10)
#define HIST_MAX_TEMP_IN_C (30)

#define HIST_MAX_TEMP_READ_IN_C (35)
//#define HIST_TIME_SLOT_LENGTH 60
#define HIST_TIME_SLOT_LENGTH 20

// 25C - 10k + 15K => 25K | 10/25 x/1024 => x= 10240/25= 410
// 627/1024  9900/x => x= 9900 * 1024 / 636 = 16k3

static debug_level_t uDebugLevel = DEBUG_TRACE;
#define MY_DEBUG 1
#if 1==MY_DEBUG
#define DEBUG_LOCAL 1
#endif

#define HIST_SLOT_ALLOCATED 1

typedef u8 hist_slot_number_t;
typedef struct {
    u8 temp_low;
    u8 temp_high;
    //u8 channel;
    u8 state;
    u8 timer_id;
    u32 time_end;
} HIST_channel_state_t;
static HIST_channel_state_t HIST_States[HIST_NUM_OF_AVAIL_CHANNELS] = { 0 };

static u8 hyst_getADCChannelFromSlot(u8 i_HystSlotNumber) {
    return i_HystSlotNumber; // here translation may occur
}

static bool hyst_getPinFromChannelNum(u8 i_ChannelNumber, u8& o_PinNumber) {

    if (i_ChannelNumber < HIST_LINE_DH_COUNT) {
        o_PinNumber = HIST_DH_FIRST_PIN_NUM + i_ChannelNumber;
        return true;
    }
    else if (i_ChannelNumber < HIST_NUM_OF_AVAIL_CHANNELS) {
        o_PinNumber = HIST_DL_FIRST_PIN_NUM + i_ChannelNumber - HIST_LINE_DH_COUNT;
        return true;
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

static bool hyst_isSlotActive(hist_slot_number_t i_SlotNumber) {
    return(0x0 != (HIST_States[i_SlotNumber].state & HIST_SLOT_ALLOCATED));
}

static bool hyst_activateSlot(hist_slot_number_t i_SlotNumber) {

    if (true == hyst_isSlotActive(i_SlotNumber))
        IF_DEB_W() {
        String str = F(" WARN: hist: slot=");
        str += i_SlotNumber;
        str += F(" has already been activated!");
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    HIST_States[i_SlotNumber].temp_low = HIST_States[i_SlotNumber].temp_high = 0;
    HIST_States[i_SlotNumber].time_end = 0;
    HIST_States[i_SlotNumber].state = HIST_SLOT_ALLOCATED;
    HIST_States[i_SlotNumber].timer_id = TIMERS_NULL;

    return true;
}

static bool hyst_deallocateSlot(hist_slot_number_t i_SlotNumber) {
    if (
        i_SlotNumber >= HIST_NUM_OF_AVAIL_CHANNELS
        ||
        0x0 == (HIST_States[i_SlotNumber].state & HIST_SLOT_ALLOCATED)) {
        THROW_ERROR();
        return(false);
    }

    IF_DEB_L() {
        String str = F(" hyst: clearing hyst_slot=");
        str += i_SlotNumber;
        // DEBLN(str);
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    HIST_States[i_SlotNumber].state = 0x0;
    HIST_States[i_SlotNumber].temp_high = 0x0;
    HIST_States[i_SlotNumber].temp_low = 0x0;
    HIST_States[i_SlotNumber].time_end = 0x0;
    HIST_States[i_SlotNumber].timer_id = TIMERS_NULL;

    return true; // slot deallocated
}

static void hyst_SetupChannel(u8 i_ChannelNumber, u8 DefaultState = LOW) {
    u8 PhysicalPinNumber;

    if (true == hyst_getPinFromChannelNum(i_ChannelNumber, PhysicalPinNumber)) {
        pinMode(PhysicalPinNumber, OUTPUT);
        digitalWrite(PhysicalPinNumber, DefaultState);
    }
}

debug_level_t HYSTERESIS_SetDebugLevel(debug_level_t i_uNewDebugLevel) {
    debug_level_t prev = uDebugLevel;

    uDebugLevel = i_uNewDebugLevel;

    return prev;
}

void HYSTERESIS_ModuleInit(void) {
    PIN_RegisterPins(hyst_getPinFromChannelNum, HIST_NUM_OF_AVAIL_CHANNELS, F("HYST_OUT"));

    _FOR(i, 0, HIST_NUM_OF_AVAIL_CHANNELS) {
        //HIST_States[i].pin_bin_out = 0;
        //HIST_States[i].channel = 0;
        HIST_States[i].state = 0;
        HIST_States[i].temp_low = 0;
        HIST_States[i].temp_high = 0;
        HIST_States[i].time_end = 0;
        HIST_States[i].timer_id = TIMERS_NULL;
    }

    // PINs' setup
    u8 channel;
    for (channel = 0; channel < HIST_LINE_DH_COUNT; channel++)
        hyst_SetupChannel(channel, HIGH);

    for (channel = HIST_LINE_DH_COUNT; channel < HIST_NUM_OF_AVAIL_CHANNELS; channel++)
        hyst_SetupChannel(channel, LOW);
}

static u32 getTempBasedOnR(u32 Rdev) {
    u32 v_min, v_max;

    memcpy_P(&v_max, &hist_data[0], 4);
    memcpy_P(&v_min, &hist_data[HYST_R_STEPS_COUNT - 1], 4);

    /*IF_DEB_T() {
        String str(" flash T read: v_min=");
        str += v_min;
        str += ", v_max=";
        str += v_max;
        // DEBLN(str);
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }*/

    // normalization
    u32 r = constrain(Rdev, HYST_R_MIN, HYST_R_MAX);
    if (r <= HYST_R_MIN)
        return (HYST_T_MIN);
    else if (r >= v_max)
        return (HYST_R_MAX);

    for (u32 R = HYST_R_MIN, step_old = 0, step = 0; R <= HYST_R_MAX;
        step++, R += HYST_R_STEP_SIZE) {
        if (r < R) {
            memcpy_P(&v_min, &hist_data[step_old], 4);
            memcpy_P(&v_max, &hist_data[step], 4);

            u32 ret = (v_min + v_max) >> 1;
            /*     IF_DEB_T() {
                             String str("  ret: v_min=");
                             str += v_min;
                             str += ", v_max=";
                             str += v_max;
                             str += ", ret=";
                             str += ret;
                             // DEBLN(str);
                             MosqClient.publish(MQTT_DEBUG, str.c_str());
                 }*/

            return (ret);
        }
        step_old = step;
    }

    return (HYST_T_NULL);
}

u32 HYSTERESIS_getTempScaled(u8 i_SlotNumber) {
    // R2Ground/Rall = ADC/1024 => (Rall*ADC)/1024 = R2Ground
    // Rall*adc = R2G * 1024 => Rall = R2G * 1024 / adc
    // Rdev = Rall - HIST_RESISTANCE_TO_GND

    u8 ADCChannelNumber = hyst_getADCChannelFromSlot(i_SlotNumber);

    u32 adc = ANALOG_ReadChannelN(ADCChannelNumber, 5);
    u32 Rall = (u32)(HIST_RESISTANCE_TO_GND << 10) / adc;
    u32 Rdev = Rall - HIST_RESISTANCE_TO_GND;

    //IF_DEB_L() {
    String str(F(" HYST_getTemp: adc="));
    str += adc;
    str += F(", Rall=");
    str += Rall;
    str += F(", R2g=");
    str += HIST_RESISTANCE_TO_GND;
    str += F(", Rdev=");
    str += Rdev;
    str += F(", Temp=");
    str += ((u32)getTempBasedOnR(Rdev));
    // DEBLN(str);
    MosqClient.publish(MQTT_DEBUG, str.c_str());
    //}

    return (getTempBasedOnR(Rdev));
}

u32 HYSTERESIS_getTemp(u8 i_SlotNumber) {
    return (HYSTERESIS_getTempScaled(i_SlotNumber) >> 12);
}

static void hist_PinSetState(u8 i_PhysicalPin, u8 i_State) {
    IF_DEB_T() {
        String str(" hist_PinSetState: setting physicla pin: ");
        str += i_PhysicalPin;
        str += F(", to=");
        str += i_State;
        // DEBLN(str);
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    digitalWrite(i_PhysicalPin, i_State);
}

static void hist_EmergencyShutdown(void) {
    u8 PhysicalPin;

    _FOR(i, 0, HIST_NUM_OF_AVAIL_CHANNELS) {
        hyst_getPinFromChannelNum(i, PhysicalPin);

        if (i < HIST_LINE_DH_COUNT)
            hist_PinSetState(PhysicalPin, HIGH); // in DH high state is none active
        else
            hist_PinSetState(PhysicalPin, LOW); // in not DH, low state is none active
    }
}

static bool hist_ShutDownChannel(hist_slot_number_t i_SlotNumber) {
    if (false == hyst_isSlotActive(i_SlotNumber))
        IF_DEB_W() {
        String str(F(" WARN: hist: slot "));
        str += i_SlotNumber;
        str += F(" not active, while shutting down");
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    u8 PhysicalPin;
    //if ( false == hyst_getPinFromChannelNum(HIST_States[i_SlotNumber].channel, PhysicalPin) ) {
    if (false == hyst_getPinFromChannelNum(i_SlotNumber, PhysicalPin)) {
        THROW_ERROR();
        DEBLN(F("BIN: wrong ch number!"));
        return(false);
    }

    if (i_SlotNumber < HIST_LINE_DH_COUNT)
        hist_PinSetState(PhysicalPin, HIGH); // in DH low state is active
    else
        hist_PinSetState(PhysicalPin, LOW); // in not DH, high state is active

    return(true);
}

static bool hist_StartHeatingChannel(hist_slot_number_t i_SlotNumber) {
    if (false == hyst_isSlotActive(i_SlotNumber)) {
        hist_EmergencyShutdown();
        THROW_ERROR();
        return(false);
    }

    u8 PhysicalPin;
    if (false == hyst_getPinFromChannelNum(i_SlotNumber, PhysicalPin)) {
        THROW_ERROR();
        DEBLN(F("BIN: wrong ch number!"));
        return(false);
    }

    if (i_SlotNumber < HIST_LINE_DH_COUNT)
        hist_PinSetState(PhysicalPin, LOW); // in DH low state is active
    else
        hist_PinSetState(PhysicalPin, HIGH); // in not DH, high state is active

    return(true);
}

static bool hist_Control(hist_slot_number_t i_SlotNumber) {
    if (false == hyst_isSlotActive(i_SlotNumber)) {
        hist_EmergencyShutdown();
        THROW_ERROR();
        return(false);
    }

    u32 currentTemp = HYSTERESIS_getTemp(i_SlotNumber);

    IF_DEB_L() {
        String str(F("HIST: temp_low="));
        str += HIST_States[i_SlotNumber].temp_low;
        str += F(", currentTemp=");
        str += currentTemp;
        str += F(", temp_high=");
        str += HIST_States[i_SlotNumber].temp_high;
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    if (currentTemp > HIST_MAX_TEMP_READ_IN_C) {
        hist_EmergencyShutdown();
        THROW_ERROR();
        return(false);
    }

    if (currentTemp >= HIST_States[i_SlotNumber].temp_high) {
        return(hist_ShutDownChannel(i_SlotNumber));
    }
    else if (currentTemp < HIST_States[i_SlotNumber].temp_low) {
        return(hist_StartHeatingChannel(i_SlotNumber));
    }
    else
        return(true); // no change, currentTemp is between
}

static void hyst_StartProcess(actions_context_t& i_rActionsContext) {
    IF_DEB_L() {
        String str(F("HIST: tick1!"));
        //DEBLN(str);
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    hist_slot_number_t slot = i_rActionsContext.var1;
    if (true == hyst_isSlotActive(slot))
        IF_DEB_W() {
        String str(F(" WARN: hist: slot="));
        str += slot;
        str += F(" has already been started!");
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    // finally, we're executing actual (possible) heating here
    hist_Control(slot);
}

static void hyst_setNewFinishTime(hist_slot_number_t i_HistSlot, unsigned long secs) {
    HIST_States[i_HistSlot].time_end = now() + secs;
}

static void hyst_StopProcess(actions_context_t& i_rActionsContext) {

    IF_DEB_L() {
        String str(F("HIST: hyst_StopProcess()!"));
        //DEBLN(str);
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    hist_slot_number_t slot = i_rActionsContext.var1;
    if (false == hyst_isSlotActive(slot)) {
        hist_EmergencyShutdown();
        THROW_ERROR();
        return;
    }

    // heating finished, so turning off
    if (HIST_States[slot].time_end <= now()) {
        IF_DEB_L() {
            String str1(F("HIST: no more heating needed, shutting down slot"));
            //DEBLN(str1);
            MosqClient.publish(MQTT_DEBUG, str1.c_str());
        }

        if (false == hist_ShutDownChannel(slot)) {
            hist_EmergencyShutdown();
            THROW_ERROR();
            return;
        }

        if (false == hyst_deallocateSlot(slot)) {
            hist_EmergencyShutdown();
            THROW_ERROR();
            return;
        }

        return;
    }

    // conditions to get here:
    // 1. user-provided time not passed yet
    // 2. slot is ok
    // 3. timeout HIST_TIME_SLOT_LENGTH happened
    if (false == hist_Control(slot)) {
        hist_EmergencyShutdown();
        THROW_ERROR();
        return;
    }

    IF_DEB_L() {
        String str2(F("HIST: restarting timer="));
        str2 += i_rActionsContext.timer_id;
        str2 += F(", for ");
        str2 += HIST_TIME_SLOT_LENGTH;
        str2 += F(" secs. ");
        MosqClient.publish(MQTT_DEBUG, str2.c_str());
    }

    // heating slot still active, so continuing
    if (false == TIMER_ReStart(i_rActionsContext.timer_id, HIST_TIME_SLOT_LENGTH)) {
        hist_EmergencyShutdown();
        THROW_ERROR();
        return;
    }
}

static bool hyst_RetriggerOnAlreadyGoing(const state_t& s) {
    // ok, so we have a new command on already active channel

    // are we closing?
    if (0x0 == s.count) {
        IF_DEB_L() {
            String str(F("HIST: stopping the timer="));
            str += HIST_States[s.c.h.channel].timer_id;
            str += F(", on hist_slot=");
            str += s.c.h.channel;
            MosqClient.publish(MQTT_DEBUG, str.c_str());
        }

        // timer_id was set in the previous command on this channel, so we can use it here
        if (false == TIMER_Stop(HIST_States[s.c.h.channel].timer_id)) {
            hist_EmergencyShutdown();
            THROW_ERROR();
            return false; // with error
        }

        if (false == hist_ShutDownChannel(s.c.h.channel)) {
            hist_EmergencyShutdown();
            THROW_ERROR();
            return false; // with error
        }

        // the currently ongoing session is finished, right now
        hyst_setNewFinishTime(s.c.h.channel, 0);

        return true; // looks good
    }

    IF_DEB_L() {
        String str(F("HIST: restarting the timer="));
        str += HIST_States[s.c.h.channel].timer_id;
        str += F(", on hist_slot=");
        str += s.c.h.channel;
        str += F(", new_timer secs=");
        str += s.count;
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    // ok, if got here, it means it's just the slot's timer restart
    if (false == TIMER_ReStart(HIST_States[s.c.h.channel].timer_id, s.count)) {
        hist_EmergencyShutdown();
        THROW_ERROR();
        return false; // with error
    }

    // new finish time
    hyst_setNewFinishTime(s.c.h.channel, s.count);

    // hist control
    hist_Control(s.c.h.channel);

    return true; // ok
}

/**
 * H0x     - Reset all channels to default states (i.e. turn off)
 * H1ALLHH - Set temp limits to LL (decimal, min 10C) and HH (dec max 30C),
 *           in channel 'A'
 * H2x     - Show read temperatures in message broker (MQTT)
  */
bool HYST_ExecuteCommand(const state_t& s) {

    actions_t Actions = { 0 }; //{bin_ChannelTurnON, bin_ChannelTurnOFF};
    actions_context_t ActionsContext = { 0 };
    state_t _s = s;

    ActionsContext.slot = s.c.h.channel;
    ActionsContext.timer_id = 0;
    ActionsContext.var1 = 0;
    ActionsContext.var2 = 0;

    hist_slot_number_t HistSlot = s.c.h.channel;
    u8 cmnd_slot;
    String str;
    if (CMNDS_NULL != (cmnd_slot = CMNDS_GetSlotNumber(s))) {
        switch (s.command) {
        case CMND_HIST_H0_RESET_ALL_SETTINGS:
            return true;

        case CMND_HIST_H1_START_HEATING:
            // is it just stopping?
            if (0 == s.count) {
                if (true == hyst_isSlotActive(s.c.h.channel))
                    hyst_deallocateSlot(s.c.h.channel);

                return(hist_ShutDownChannel(s.c.h.channel));
            }

            // is some action already going on that slot?
            if (true == hyst_isSlotActive(s.c.h.channel))
                return(hyst_RetriggerOnAlreadyGoing(s));

            // no previous actions, so let's start some heating :o)
            hyst_activateSlot(HistSlot); // here slot clearing happens!

            Actions.fun_start = hyst_StartProcess;
            Actions.fun_stop = hyst_StopProcess;

            HIST_States[HistSlot].temp_low = s.c.h.low;
            HIST_States[HistSlot].temp_high = s.c.h.high;
            hyst_setNewFinishTime(HistSlot, s.count); // s.count is whole action length
            _s.count = HIST_TIME_SLOT_LENGTH; // but we want to restart each HIST_TIME_SLOT_LENGTH

            ActionsContext.var1 = HistSlot;
            ActionsContext.var2 = 0; // not used

            if (false == CMNDS_ScheduleAction(_s, Actions, ActionsContext)) {
                hyst_deallocateSlot(HistSlot);
                THROW_ERROR();
                return(false);
            }

            HIST_States[HistSlot].timer_id = ActionsContext.timer_id;

            IF_DEB_L() {
                str = F("HIST: command accepted, slot=");
                str += HistSlot;
                str += F(", timer_id=");
                str += HIST_States[HistSlot].timer_id;

                MosqClient.publish(MQTT_DEBUG, str.c_str());
            }
            return(true);

        case CMND_HIST_H3_STOP_HEATING:
            if (true == hyst_isSlotActive(s.c.h.channel))
                hyst_deallocateSlot(s.c.h.channel);
            return(hist_ShutDownChannel(s.c.h.channel));

        default:
        case CMND_HIST_H2_SHOW_TEMP:
            break;
        }
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

/**
 * H0x     - Reset all channels to default states (i.e. turn off)
 * H1ALLHH - Set temp limits to LL (decimal, min 10C) and HH (dec max 30C),
 *           in channel 'A'
 * H2x     - Show read temperatures in message broker (MQTT)
 * H3A     - Stop heating in channel 'A'
 */
bool decode_CMND_H(const byte* payload, state_t& s) {
    bool sanity_ok = false;

    s.command = (*payload++) - '0';
    s.c.h.channel = (*payload++) - '0';

    u8 number = 0, scale = 0;

    switch (s.command) {
    case CMND_HIST_H1_START_HEATING:
        s.c.h.low = 10 * ((*payload++) - '0'); // LL reading
        s.c.h.low += 1 * ((*payload++) - '0');
        s.c.h.high = 10 * ((*payload++) - '0'); // HH reading
        s.c.h.high += 1 * ((*payload++) - '0');

        number = (*payload++) - '0'; // [0..9] - number
        scale = *payload++; // [SMTH] - Seconds/Minutes/TenMinutes/Hours
        s.count = getSecondsFromNumberAndScale(number, scale);
        goto CONTINUE;

    case CMND_HIST_H0_RESET_ALL_SETTINGS:
    case CMND_HIST_H2_SHOW_TEMP:
    case CMND_HIST_H3_STOP_HEATING:
    default:
        goto ERROR;
    }

CONTINUE:
    if (false == hyst_getPinFromChannelNum(s.c.h.channel, s.c.h.pin))
        goto ERROR;

    if (s.c.h.low >= s.c.h.high) // at least one 1C needed
        goto ERROR;

    s.sum = (*payload++) - '0'; // sum = 1

    if (s.c.h.low >= HIST_MIN_TEMP_IN_C && s.c.h.high <= HIST_MAX_TEMP_IN_C)
        if (s.command >= 0 && s.command < CMND_HIST_MAX_VALUE)
            if (s.c.h.channel >= 0 && s.c.h.channel < HIST_NUM_OF_AVAIL_CHANNELS)
                if (true == isSumOk(s))
                    sanity_ok = true;

ERROR:

    // Info display
    IF_DEB_L() {
        u8 i = s.c.h.channel;
        String str(F("HIST: channel: "));
        str += i;
        str += F(", Cmd: ");
        i = (s.command);
        str += i;
        str += F(", Pin: ");
        i = (s.c.h.pin);
        str += i;
        str += F(", Sanity: ");
        str += sanity_ok;
        str += F(", Sum: ");
        str += (s.sum + '0');
        //DEBLN(str);
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    return (sanity_ok);
}

void HIST_DisplayAssignments(void) {
    String str;

    {
        String str1(F("\nHIST slots assignments:\n-=-=-=-=-="));
        // DEBLN(str);
        MosqClient.publish(MQTT_DEBUG, str1.c_str());
    }

    _FOR(i, 0, HIST_NUM_OF_AVAIL_CHANNELS) {
        str = F(" ");
        str += i;
        str += F(": T(");
        str += HIST_States[i].temp_low;
        str += F("..");
        str += HIST_States[i].temp_high;
        str += F("), state=");
        str += HIST_States[i].state;
        str += F(", timer_id=");
        str += HIST_States[i].timer_id;
        str += F(", is_timer_active=");
        str += TIMER_IsActive(HIST_States[i].timer_id);

        // DEBLN(str);
        MosqClient.publish(MQTT_DEBUG, str.c_str());
    }

    str = F("Total hist_slots: ");
    str += HIST_NUM_OF_AVAIL_CHANNELS;
    // DEBLN(str);
    MosqClient.publish(MQTT_DEBUG, str.c_str());
}

#endif // N32_CFG_HISTERESIS_ENABLED
