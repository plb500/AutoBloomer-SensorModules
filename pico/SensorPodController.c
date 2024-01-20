#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

// Menu files
#include "menu/menu_interface.h"
#include "menu/main_menu.h"

// Hardware files
#include "sensor/sensor_i2c_interface.h"
#include "sensor_pod.h"
#include "userdata/userdata.h"


// Hardware defines
#define I2C_PORT                        (i2c1)
static const uint8_t I2C_SDA            = 6;
static const uint8_t I2C_SCL            = 3;
static const uint SENSOR_I2C_BAUDRATE   = (25 * 1000);

static const uint8_t LED_PIN            = 25;

static const uint8_t UART_TX_PIN        = 12;
static const uint8_t UART_RX_PIN        = 13;


I2CInterface mainInterface = {
    I2C_PORT,
    SENSOR_I2C_BAUDRATE,
    I2C_SDA,
    I2C_SCL,
    DEFAULT_MULTIPLEXER_ADDRESS
};

UserData userData;

SensorPod sensorPod = {
    .mInterface = &mainInterface,
    .mSCD30Address = SCD30_I2C_ADDRESS,
    .mSoilSensorAddress = SOIL_SENSOR_3_ADDRESS
};

int main() {
    char scd30Serial[SCD30_SERIAL_BYTE_SIZE];
    uint8_t scd30Firmware[2];

    // Setup UART
#if LIB_PICO_STDIO_UART
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
#endif

    // Setup I2C
    init_sensor_bus(&mainInterface);

    // Initialise I/O
    stdio_init_all(); 

    // Grab user data
    if(!read_userdata_from_flash(&userData)) {
        init_userdata(&userData);
    }


    sleep_ms(2000);

    // Initialize soil sensor
    printf("Initializing soil sensor....");
    if(init_soil_sensor(&sensorPod)) {x
        if(reset_soil_sensor(&sensorPod)) {
            uint32_t ver = get_soil_sensor_version(&sensorPod);
            printf("\nSoil sensor initialized. Version: 0x%04X\n", ver);
        } else {
            printf(("Failed to reset soil sensor\n"));
        }
    } else {
        printf(("Failed to init soil sensor\n"));
    }
    printf("\n");

    // Initialize SCD30 sensor
    printf("Initializing SCD30....");
    if(read_scd30_serial(&sensorPod, scd30Serial) && read_scd30_firmware_version(&sensorPod, scd30Firmware)) {
        printf("\nSCD30 initialized\n");
        printf("  Serial: %s\n", scd30Serial);
        printf("  Firmware: 0x%02X  : 0x%02X\n", scd30Firmware[0], scd30Firmware[1]);
        printf("\n");
    } else {
        printf(("Failed to init SCD30\n"));
    }


    MainMenuObject menuObject = {
        .mLEDPin = LED_PIN,
        .mSensorPod = &sensorPod
    };

    // Instantiate main menu and loop
    while(1) {
        do_menu(MAIN_MENU_TEXT, main_menu_handler, &menuObject);
    }
}