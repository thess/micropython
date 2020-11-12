#define MICROPY_HW_BOARD_NAME "Openlog Artemis"
#define MICROPY_HW_MCU_NAME "APOLLO3B"

// I2C defines
#define AP3_I2C_INTERFACES 1

// Specify that I2C-0 uses IOMaster instance 1
// Specify GPIO uFuncSel
#define AP3_I2C0_IOMASTER 1
#define AP3_I2C_GPIO_FUNCSEL 0
// I2C pads
#define I2C_0_DEFAULT_SCL (8)
#define I2C_0_DEFAULT_SDA (9)

// SPI Defines
#define AP3_SPI_INTERFACES 1

#define AP3_SPI_IOMASTER 0  // Specify that SPI uses IOMaster 0
#define AP3_SPI_GPIO_FUNCSEL 1
// SPI pads
#define SPI_0_DEFAULT_SCK   (5)
#define SPI_0_DEFAULT_MISO  (6)
#define SPI_0_DEFAULT_MOSI  (7)

// 1 := battery pressent (don't set time automatically)
#define AP3_RTC_BATTERY 1

// Define if board has SDCARD
#define AP3_ENABLE_SDCARD 1
