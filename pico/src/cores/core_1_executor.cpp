#include "core_1_executor.h"
#include "util/debug_io.h"

#include "pico/multicore.h"
#include <cstdlib>


Core1Executor* Core1Executor::sExecutor = nullptr;

Core1Executor::Core1Executor(
    MulticoreMailbox& mailbox,
    optional<StemmaSoilSensor>& soilSensor,
    optional<I2CInterface>& scd30Interface
) :
    mMailbox(mailbox),
    mSensorPod(soilSensor, scd30Interface)
{}

void Core1Executor::initialize() {
    if(mSensorPod.initialize()) {
        DEBUG_PRINT("Sensor pod initialized");
        mSensorPod.startReadings();
    } else {
        DEBUG_PRINT("Sensor pod initialization failed");
    }
}

void Core1Executor::loop() {
    if(!sExecutor) {
        return;
    }

    sExecutor->doLoop();
}

void Core1Executor::setExecutor(Core1Executor& executor) {
    sExecutor = &executor;
}

void Core1Executor::doLoop() {
    multicore_lockout_victim_init();

    while(1) {
        // Check for sensor control messages
        processSensorControlCommands();

        // Update sensors and push any data to core0 if necessary
        mSensorPod.update();

        if(mSensorPod.hasValidData()) {
            const SensorPod::Data& currentData = mSensorPod.getCurrentData();

            DEBUG_PRINT("+--------------------------------+");
            DEBUG_PRINT("|         SCD30 CO2: %7.2f PPM |", currentData.mCO2Level);
            DEBUG_PRINT("| SCD30 Temperature: %5.2f °C    |", currentData.mTemperature);
            DEBUG_PRINT("|    SCD30 Humidity: %5.2f%%      |", currentData.mHumidity);
            DEBUG_PRINT("+--------------------------------+\n");

            mMailbox.sendSensorDataToCore0(currentData);
        } else {
            DEBUG_PRINT("+---------------------------+");
            DEBUG_PRINT("|           NO DATA         |");
            DEBUG_PRINT("| Soil sensor active: %d [%c] |", mSensorPod.isSoilSensorActive(), mSensorPod.hasSoilSensor() ? '*' : ' ');
            DEBUG_PRINT("|       SCD30 active: %d [%c] |", mSensorPod.isSCD30Active(), mSensorPod.hasSCD30Sensor() ? '*' : ' ');
            DEBUG_PRINT("+---------------------------+\n");
        }

        sleep_ms(500);
    }
}

void Core1Executor::processSensorControlCommands() {
    optional<SensorPodMessages::SensorControlMessage> msgOpt;

    do {
        if(msgOpt = mMailbox.getWaitingSensorControlMessage()) {
            switch(msgOpt->mCommand) {
                case SensorPodMessages::SensorControlCommandType::SCD30_SET_TEMP_OFFSET:
                    handleSetTemperatureOffsetCommand(msgOpt->mCommandParams);
                    break;

                case SensorPodMessages::SensorControlCommandType::SCD30_SET_FRC:
                    handleSetFRCCommand(msgOpt->mCommandParams);
                    break;
            }
        }
    } while(msgOpt);
}


void Core1Executor::handleSetTemperatureOffsetCommand(const char *commandParam) {
    double val;
    char *end;

    val = strtod(commandParam, &end);
    if(end == commandParam) {
        // Could not convert supplied value
        DEBUG_PRINT("Conversion error while setting temperature offset.");
        return;
    }

    DEBUG_PRINT("SETTING TEMPERATURE OFFSET (%f)", val);
    mSensorPod.setTemperatureOffset(val);
}

void Core1Executor::handleSetFRCCommand(const char *commandParam) {
    long val;
    char *end;

    val = strtol(commandParam, &end, 10);
    if(end == commandParam) {
        // Could not convert supplied value
        DEBUG_PRINT("Conversion error while setting FRC.");
        return;
    }

    DEBUG_PRINT("SETTING FRC: %d", val);
    mSensorPod.setForcedRecalibrationValue(val);
}