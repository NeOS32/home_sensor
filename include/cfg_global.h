// SPDX-FileCopyrightText: Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#define C_WRAPPER(STR) String(F(STR)).c_str()

#define MQTT_PART_SENSORS_PART "sensors"
#define MQTT_PART_CONTROL "control"
#define MQTT_PART_COMMANDS "commands"
#define MQTT_PART_DEVICES5 "devices"
#define MQTT_PART_STATE "state"
#define MQTT_PART_DEBUG "debug"
#define MQTT_PART_ERRORS "errors"
#define MQTT_PART_BUILDTIME "buildtime"
#define MQTT_PART_PRESENCE "presence"
#define MQTT_SENSORS_T_MATA C_WRAPPER("mata")

#define APP_VERSION C_WRAPPER( "ardControl: 0v14" )

#define ARD_PREFIX C_WRAPPER( "ard:" )

// board-specific macros
#define MQTT_ALARM_SECURITY C_WRAPPER("ard/" MQTT_CLIENT_SHORT_NAME "/alarm/sec")
#define MQTT_ALARM_SYS_ERR  C_WRAPPER("ard/" MQTT_CLIENT_SHORT_NAME "/alarm/err")
#define MQTT_DEBUG          C_WRAPPER("ard/" MQTT_CLIENT_SHORT_NAME "/debug")
#define MQTT_DEVICES_CMNDS  C_WRAPPER("ard/" MQTT_CLIENT_SHORT_NAME "/control/commands")

#define MQTT_SENSORS_ANALOG C_WRAPPER("ard/" MQTT_CLIENT_SHORT_NAME "/sensors/A")
#define MQTT_SENSORS_ADDR   C_WRAPPER("ard/" MQTT_CLIENT_SHORT_NAME "/sensors/T/addr/")
#define MQTT_SENSORS_T      C_WRAPPER("ard/" MQTT_CLIENT_SHORT_NAME "/sensors/T/values/")
#define MQTT_SENSORS_BIN_IN C_WRAPPER("ard/" MQTT_CLIENT_SHORT_NAME "/sensors/bin_in/")

#define MQTT_DEV_STATE           C_WRAPPER( "devices/" MQTT_PART_STATE "/" MQTT_CLIENT_SHORT_NAME "/" )
#define MQTT_DEV_STATE_ERRORS    C_WRAPPER( "devices/" MQTT_PART_STATE "/" MQTT_PART_ERRORS "/" )
#define MQTT_DEV_STATE_BUILDTIME C_WRAPPER( "devices/" MQTT_PART_STATE "/" MQTT_PART_BUILDTIME "/" )
#define MQTT_DEV_PRESENCE        C_WRAPPER( "devices/" MQTT_PART_STATE "/" MQTT_PART_PRESENCE "/" )

// some rationales

#if N32_CFG_BIN_OUT_ENABLED == 0 & BIN_OUT_LINE_DH_COUNT != 0
#error Set BIN_OUT_LINE_DH_COUNT and BIN_OUT_LINE_DL_COUNT to zero, to save RAM
#endif
