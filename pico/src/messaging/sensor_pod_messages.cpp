#include "sensor_pod_messages.h"

#include <cstring>
#include <cstdio>
#include <string>

#include "util/debug_io.h"

using std::nullopt;

constexpr const char* SENSOR_STATUS_JSON_KEY           = "Status";
constexpr const char* FEED_LEVEL_JSON_KEY              = "Feed Level";
constexpr const char* CO2_LEVEL_JSON_KEY               = "CO2 Level";
constexpr const char* TEMPERATURE_JSON_KEY             = "Temperature";
constexpr const char* HUMIDITY_JSON_KEY                = "Humidity";
constexpr const char* SOIL_MOISTURE_JSON_KEY           = "Soil Moisture";


optional<SensorPodMessages::SensorControlMessage> SensorPodMessages::mqttToControlMessage(MQTTMessage& mqttMessage) {
    char command[8];
    char value[8];
    SensorControlMessage controlMessage;

    memset(command, 0, 8);
    memset(value, 0, 8);

    // Parse command and value strings, bail if we can't even get that far
    sscanf(mqttMessage.mPayload, "%s %s", command, value);
    if(!strlen(command) || !strlen(value)) {
        // Malformed message
        return nullopt;
    }

    // Make sure the command translates into something sensible
    uint32_t commandInt = *((uint32_t*) command);
    switch(commandInt) {
        case SCD30_SET_TEMP_OFFSET:
        case SCD30_SET_FRC:
            controlMessage.mCommand = static_cast<SensorControlCommandType>(commandInt);
            memcpy(controlMessage.mCommandParams, value, 8);
            return controlMessage;

        default:
            return nullopt;
    }

    return nullopt;
}

optional<SensorPodMessages::MQTTMessage> SensorPodMessages::dataUpdateToMQTTMessage(
    const char *sensorName, 
    const char *sensorLocation,
    SensorDataUpdateMessage& dataUpdate
) {
    char soilReadingString[32];
    char scd30ReadingString[64];
    MQTTMessage mqttMsg;

    // Sanity check
    if(!sensorName || !sensorLocation) {
        return nullopt;
    }

    snprintf(mqttMsg.mTopic, MQTT_MAX_TOPIC_LENGTH, "%s/%s/%s",
        AUTOBLOOMER_TOPIC_NAME,
        sensorLocation,
        sensorName
    );

    if(dataUpdate.mHasSoilReding) {
        snprintf(soilReadingString, 32,
            ", \"%s\":%d",
            SOIL_MOISTURE_JSON_KEY, dataUpdate.mSoilSensorData
        );
    } else {
        soilReadingString[0] = 0;
    }

    if(dataUpdate.mHasSCD30Reading) {
        snprintf(scd30ReadingString, 64,
            ", \"%s\":%.2f, \"%s\":%.2f, \"%s\":%.2f",
            CO2_LEVEL_JSON_KEY, dataUpdate.mCO2Level,
            TEMPERATURE_JSON_KEY, dataUpdate.mTemperature,
            HUMIDITY_JSON_KEY, dataUpdate.mHumidity
        );
    } else {
        scd30ReadingString[0] = 0;
    }


    snprintf(mqttMsg.mPayload, MQTT_MAX_PAYLOAD_LENGTH, 
        "{\"%s\":%d%s%s}",
        SENSOR_STATUS_JSON_KEY, dataUpdate.mSensorStatus,
        scd30ReadingString,
        soilReadingString
    );

    return mqttMsg;
}

optional<SensorPodMessages::MQTTMessage> SensorPodMessages::createTestMQTTMessage(
    const char *sensorName, 
    const char *sensorLocation,
    const char *message
) {
    MQTTMessage mqttMsg;

    // // Sanity check
    if(!sensorName || !sensorLocation || !message) {
        return nullopt;
    }

    snprintf(mqttMsg.mTopic, MQTT_MAX_TOPIC_LENGTH, "%s/%s/%s",
        AUTOBLOOMER_TOPIC_NAME,
        sensorLocation,
        sensorName
    );

    int messageLength = strlen(message);
    memcpy(mqttMsg.mPayload, message, messageLength);
    mqttMsg.mPayload[messageLength] = 0;

    return mqttMsg;
}