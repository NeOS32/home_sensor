// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

//static debug_level_t uDebugLevel = DEBUG_WARN;

bool SERIAL_publish( const char* payload) {
    DEBLN(payload);
    return true;
}

bool MQTT_publish(const char* topic, const char* payload) {
    return gClient_Mosq.publish(topic, payload);
}

bool MSG_Publish(const char* topic, const char* payload) {
    return MQTT_publish(topic, payload);
}

bool MSG_Publish_State(const char* payload) {
    return MSG_Publish(MQTT_DEV_STATE, payload);
}

bool MSG_Publish_State_Errors(const char* payload) {
    return MSG_Publish(MQTT_DEV_STATE_ERRORS, payload);
}

bool MSG_Publish_State_Buildtime(const char* payload) {
    return MSG_Publish(MQTT_DEV_STATE_BUILDTIME, payload);
}

bool MSG_Publish_Presence(const char* payload) {
    return MSG_Publish(MQTT_DEV_PRESENCE, payload);
}

bool MSG_Publish_Command(const char* payload) {
    return MSG_Publish(MQTT_DEVICES_CMNDS, payload);
}

bool MSG_Publish_Debug(const char* payload) {
    return MSG_Publish(MQTT_DEBUG, payload);
}
