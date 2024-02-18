#ifndef _SENSOR_H_
#define _SENSOR_H_

#include "pico/types.h"

#include <map>
#include <tuple>


using std::map;
using std::tuple;

class Sensor {
    public:
        enum SensorStatus {
            SENSOR_OK,
            SENSOR_OK_NO_DATA,
            SENSOR_INACTIVE,
            SENSOR_MALFUNCTIONING
        };

        struct SensorDataBuffer {
            static constexpr uint32_t SENSOR_DATA_BUFFER_SIZE       = (64);

            SensorStatus mStatus;
            uint8_t mDataBytes[SENSOR_DATA_BUFFER_SIZE];
            uint8_t mDataLen;
            absolute_time_t mDataExpiryTime;
        };

        typedef int (*JsonSerializer)(uint8_t*, uint8_t, char*, int);


        Sensor(uint8_t sensorType, JsonSerializer serializer);

        // Must be unique per-sensor type
        uint8_t getSensorTypeID() const { return mSensorType; };      

        // Perform all required hardware initialization
        virtual void initialize() = 0;

        // Fully reset the sensor hardware (will be used if sensor stops responding for a period of time)
        virtual void reset() = 0;

        // Completely disable the sensor
        virtual void shutdown() = 0;                

        void update(absolute_time_t currentTime);
        const SensorDataBuffer& getCachedData() const { return mCachedData; }

        static int getDataAsJSON(uint8_t sensorTypeID, uint8_t* data, uint8_t dataLength, char* jsonBuffer, int jsonBufferSize);
        static void registerJSONSerializer(int sensorTypeID, JsonSerializer serializer);

    protected:
        typedef tuple<SensorStatus, uint8_t> SensorUpdateResponse;

        // Update the underlying sensor hardware, serializing any current data into the supplied buffer
        virtual SensorUpdateResponse doUpdate(absolute_time_t currentTime, uint8_t *dataStorageBuffer, size_t bufferSize) = 0;

    private:
        inline void resetUpdateWatchdogTimer();

        static constexpr uint32_t UPDATE_WATCHDOG_TIMEOUT_MS    = (15 * 1000);      // Reset sensor if it hasn't responded in 15s
        static constexpr uint32_t SENSOR_DATA_CACHE_TIME_MS     = (5 * 1000);       // Keep old sensor data around for 5s

        static map<int, JsonSerializer> sJSONSerializerMap;

        const uint8_t mSensorType;
        absolute_time_t mUpdateWatchdogTimeout;
        SensorDataBuffer mCachedData;
};

#endif      // _SENSOR_H_