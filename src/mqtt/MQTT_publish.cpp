// SPDX-FileCopyrightText: 2021 Sebastian Serewa <neos32.project@gmail.com>
//
// SPDX-License-Identifier: Apache-2.0

#include "my_common.h"

static debug_level_t uDebugLevel = DEBUG_LOG;

bool MSG_Publish(const char* topic, const char* payload) {
    return MQTT_publish(topic, payload);
}

bool MQTT_publish(const char* topic, const char* payload) {

    return gClient_Mosq.publish(topic, payload);
}
