// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

#if 1 == N32_CFG_TEMP_ENABLED

static debug_level_t uDebugLevel = DEBUG_LOG;

#define DEFAULT_DELAY_FOR_CONVERSION_IN_S 5

#define TEMP_MIN (-100)
#define TEMP_MAX (99)

static void handler_UpdateSensorsAddr(void) {
    uint8_t address[DS18B20_ADDRESS_SIZE];

    while (ds18b20_sensors.selectNext()) {

        ds18b20_sensors.getAddress(address);

        if (0x28 != address[0])
            continue;

        if (OneWire::crc8(address, 7) != address[7])
            break; // DEBLN(F("Bad address!"));

        String stringAddr;
        char hexBuf[4];
        for (byte i = 0; i < DS18B20_ADDRESS_SIZE; i++) {
            sprintf(&hexBuf[0], "%x", address[i]);
            stringAddr += hexBuf;
            if (i < 7)
                stringAddr += F(",");
        }
        DEB_L(stringAddr);

        MSG_Publish(MQTT_SENSORS_ADDR, stringAddr.c_str());
    }
}

static void HANDLER_OneWire(void) {

    if (ds18b20_sensors.getNumberOfDevices()) {
        uint8_t address[DS18B20_ADDRESS_SIZE]; // a buffer for sensor address
        for (byte i = 0; i < DS18B20_SENSORS_COUNT; i++) {
            memcpy_P(&address[0], &ds18b20_addrs[i], DS18B20_ADDRESS_SIZE);

            ds18b20_sensors.select(&address[0]);

            float temperature_prev = ds18b20_sensors.getTempC();
            float temperature = constrain(temperature_prev, TEMP_MIN, TEMP_MAX);

            String stringPath(MQTT_SENSORS_T);
            stringPath += i;

            String stringTemp;
            stringTemp += temperature;
            MSG_Publish(stringPath.c_str(), stringTemp.c_str());
            DEBLN(stringTemp);

            IF_DEB_L() {
                String str(F(" 1-Wire: i="));
                str += i;
                str += F(", Tprev=");
                str += temperature_prev;
                str += F(", T=");
                str += temperature;
                DEBLN(str);
                MSG_Publish(MQTT_DEBUG, str.c_str());
                DEBLN(stringPath + " : " + stringTemp);
            }
        }

        ds18b20_sensors.doConversion();
    }
    else
        DEB_W(F(" No Temp sensors found!"));
}

static void temp_AlarmFun(void) { HANDLER_OneWire(); }

static bool ds18b20_getPinFromChannelNum(u8 i_ChannelNumber, u8& o_PinNumber) {
    if (i_ChannelNumber < DS18B20_NUM_OF_AVAIL_CHANNELS) {
        o_PinNumber = ONEWIRE_FIRST_PIN_NUM + i_ChannelNumber;
        return true;
    }

    THROW_ERROR();
    return false; // error, means function failed to execute command
}

void TEMP_ModuleInit(void) {
    // begin conversions
    ds18b20_sensors.doConversion();

    // make it cyclic
    Alarm.timerRepeat(15, temp_AlarmFun);

    // module registry
    PIN_RegisterPins(ds18b20_getPinFromChannelNum,
        DS18B20_NUM_OF_AVAIL_CHANNELS, F("DS18B20"));
}

// ------------- EFFECTS -------------

/**
 * T0x - Reset all channels to default states
 * T1A - Show status in messaage broker (MQTT)
 * T2A - Show read temperatures in messaage broker (MQTT)
 * T3A - Show sensor addresses in messaage broker (MQTT)
 * T4A - Pause/Restart temperature conversion
 */
static void temp_StartConversion(actions_context_t& i_rActionsContext) {
    if (ds18b20_sensors.getNumberOfDevices()) // available()
        ds18b20_sensors.doConversion(); // request()
}
static void temp_SendResults(actions_context_t& i_rActionsContext) {
    // normal temperature reading
    uint8_t address[DS18B20_ADDRESS_SIZE]; // a buffer for sensor address

    for (byte i = 0; i < DS18B20_SENSORS_COUNT; i++) {
        memcpy_P(&address[0], &ds18b20_addrs[i], DS18B20_ADDRESS_SIZE);

        ds18b20_sensors.select(&address[0]);
        float temperature = ds18b20_sensors.getTempC();

        String stringPath(MQTT_SENSORS_T);
        stringPath += i;

        String stringTemp;
        stringTemp += temperature;
        MSG_Publish(stringPath.c_str(), stringTemp.c_str());

        DEB_L(stringPath + F(" : ") + stringTemp);
    }
}

bool TEMP_ExecuteCommand(const state_t& s) {

    actions_t Actions; //{bin_ChannelTurnON, bin_ChannelTurnOFF};
    actions_context_t ActionsContext;

    ActionsContext.slot = s.c.t.channel;
    ActionsContext.timer_id = 0;
    ActionsContext.var1 = 0;
    ActionsContext.var2 = 0; // in var2 is current PWM value

    u8 slot;

    if (CMNDS_NULL != (slot = CMNDS_GetSlotNumber(s))) {
        switch (s.command) {
        case CMND_TEMP_RESET_ALL_INTERFACES:
            ds18b20_sensors.doConversion(); //request();
            return true;

        case CMND_TEMP_DISPLAY_TEMP:
            Actions.fun_start = temp_StartConversion;
            Actions.fun_stop = temp_SendResults;
            return (CMNDS_ScheduleAction(s, Actions, ActionsContext));

        case CMND_TEMP_DISPLAY_ADDRESSES:
            handler_UpdateSensorsAddr();
            return true;

        case CMND_TEMP_SET_REFRESH_PERIOD:
            return false;

        case CMND_TEMP_DISPLAY_STATUS:
            return false;
        }
    }

    THROW_ERROR();
    return false; // error
}

/**
 * T0x - Reset all channels to default states
 * T1A - Show status in messaage broker (MQTT)
 * T2A - Show read temperatures in messaage broker (MQTT)
 * T3A - Show sensor addresses in messaage broker (MQTT)
 * T4A - Pause/Restart temperature conversion
 */
bool decode_CMND_T(const byte* payload, state_t& s) {
    s.command = (*payload++) - '0'; // 0..4
    s.c.t.channel = (*payload++) - '0'; // 0..DS18B20_NUM_OF_AVAIL_CHANNELS

    s.count = DEFAULT_DELAY_FOR_CONVERSION_IN_S;
    s.sum = (*payload++) - '0'; // sum = 1

    bool sanity_ok = false;
    if (s.command >= 0 && s.command <= CMND_TEMP_MAX_VALUE)
        if (s.c.t.channel >= 0 && s.c.t.channel < DS18B20_NUM_OF_AVAIL_CHANNELS)
            if (true == isSumOk(s))
                sanity_ok = true;

    // Info display
    IF_DEB_L() {
        u8 i = s.c.t.channel;
        String str(F("TEMP: channel: "));
        str += i;
        str += F(", Cmd: ");
        i = (s.command);
        str += i;
        str += F(", Sanity: ");
        str += sanity_ok;
        str += F(", Sum: ");
        str += (s.sum + '0');
        DEBLN(str);
    }

    return (sanity_ok);
}
#endif // N32_CFG_LED_W2918_ENABLED
