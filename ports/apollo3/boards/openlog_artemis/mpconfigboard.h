#define MICROPY_HW_BOARD_NAME "Openlog Artemis"
#define MICROPY_HW_MCU_NAME "APOLLO3B"

// I2C defines
#define AP3_I2C_INTERFACES 1

#define AP3_I2C0_IOM 1 // Secify that I2C-0 uses IOMaster instance 1

// SPI Defines
#define AP3_SPI_INTERFACES 1

#define AP3_SPI_IOM0 0                   // Specify that SPI uses IOMaster 0
#define AP3_SPI_DUP  ap3_spi_full_duplex // Specify that SPI is full-duplex

// 1 := battery pressent (don't set time automatically)
#define AP3_RTC_BATTERY 1

