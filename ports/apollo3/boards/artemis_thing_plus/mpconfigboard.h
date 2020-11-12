#define MICROPY_HW_BOARD_NAME "Artemis Thing Plus"
#define MICROPY_HW_MCU_NAME "APOLLO3B"

// I2C defines
#define AP3_I2C_INTERFACES 2

#define AP3_I2C0_IOMASTER 4 // Specify that I2C-0 uses IOMaster instance 4
#define AP3_I2C1_IOMASTER 3 // Specify that I2C-1 uses IOMaster instance 3
#define AP3_I2C_GPIO_FUNCSEL 4
// I2C pads
#define I2C_0_DEFAULT_SCL (39)
#define I2C_0_DEFAULT_SDA (40)
#define I2C_1_DEFAULT_SCL (42)
#define I2C_1_DEFAULT_SDA (43)

// SPI Defines
#define AP3_SPI_INTERFACES 1

#define AP3_SPI0_IOMASTER 0  // Specify that SPI uses IOMaster 0
#define AP3_SPI_GPIO_FUNCSEL 1
// SPI pads
#define SPI_0_DEFAULT_SCK   (5)
#define SPI_0_DEFAULT_MISO  (6)
#define SPI_0_DEFAULT_MOSI  (7)


// 1 := battery pressent (don't set time automatically)
#define AP3_RTC_BATTERY 0

// Define if board has SDCARD
//#define AP3_ENABLE_SDCARD 1
