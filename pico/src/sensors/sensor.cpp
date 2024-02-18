#include "sensor.h"

#include "pico/time.h"
#include <string.h>


using std::tie;

map<int, Sensor::JsonSerializer> Sensor::sJSONSerializerMap;

Sensor::Sensor(uint8_t sensorType, JsonSerializer serializer) :
    mSensorType(sensorType),
    mUpdateWatchdogTimeout(nil_time)
{
    sJSONSerializerMap[mSensorType] = serializer;
}

void Sensor::update(absolute_time_t currentTime) {
    uint8_t sensorData[SensorDataBuffer::SENSOR_DATA_BUFFER_SIZE];
    uint8_t dataSize;

    tie(mCachedData.mStatus, dataSize) = doUpdate(currentTime, sensorData, SensorDataBuffer::SENSOR_DATA_BUFFER_SIZE);

    switch(mCachedData.mStatus) {
        case SENSOR_OK:
            // We got fresh data, everything is good. Cache the data.
            memcpy(mCachedData.mDataBytes, sensorData, dataSize);
            mCachedData.mDataLen = dataSize;
            mCachedData.mDataExpiryTime = make_timeout_time_ms(SENSOR_DATA_CACHE_TIME_MS);
            resetUpdateWatchdogTimer();
            break;

        case SENSOR_OK_NO_DATA:
            // No new data, but that's ok. We will continue transmitting the cached
            // data until it becomes stale
            if(absolute_time_diff_us(mCachedData.mDataExpiryTime, currentTime) > 0) {
                mCachedData.mDataLen = 0;
                mCachedData.mDataExpiryTime = nil_time;
            }
            resetUpdateWatchdogTimer();
            break;
        
        case SENSOR_MALFUNCTIONING:
            // We are getting errors from the underlying sensor. If we have been getting it for too long,
            // we will try bouncing the sensor
            if(absolute_time_diff_us(mUpdateWatchdogTimeout, currentTime) > 0) {
                reset();
                resetUpdateWatchdogTimer();
            }
            mCachedData.mDataLen = 0;
            break;

        case SENSOR_INACTIVE:
            // Not a lot we can do here, either we weren't initialized or the init failed.
            mCachedData.mDataLen = 0;
            break;
    }
}

int Sensor::getDataAsJSON(uint8_t sensorTypeID, uint8_t* data, uint8_t dataLength, char* jsonBuffer, int jsonBufferSize) {
    if(auto serializer = sJSONSerializerMap.find(sensorTypeID); serializer != sJSONSerializerMap.end()) {
        return serializer->second(data, dataLength, jsonBuffer, jsonBufferSize);
    }

    return 0;
}

void Sensor::registerJSONSerializer(int sensorTypeID, Sensor::JsonSerializer serializer) {
    sJSONSerializerMap[sensorTypeID] = serializer;
}

void Sensor::resetUpdateWatchdogTimer() {
    mUpdateWatchdogTimeout = make_timeout_time_ms(UPDATE_WATCHDOG_TIMEOUT_MS);
}