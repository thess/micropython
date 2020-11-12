// ap3_pinmap.c
//
// Map of board pins to Artemis pad numbers
//
const uint8_t ap3_pin2pad[] = {
    //~ := PWM, A := ADC
    // <Apollo3 Pad>  Board Pin - functions
    25,               //0     - ~RX1/SDA2/MISO2
    24,               //1     - ~TX1/32kHz/SWO
    44,               //2     - ~MOSI4
    35,               //3/A6  - ~A/TX1/I2SDAT/PDMCLK
     4,               //4     - ~RX1/SLINT
    22,               //5     - ~PDMCLK/SWO
    23,               //6     - ~I2SWCLK/CMPOUT
    27,               //7     - ~SCL2/SCK2
    28,               //8     - ~MOSI2/I2SWCLK
    32,               //9/A7  - ~A/SCCIO
    14,               //10    -  SWCH/ADCD1N/TX1/PDMCLK
     7,               //11    - ~MOSI0/CLKOUT
     6,               //12    - ~MISO0/SDA0/I2SDAT
     5,               //13    - ~SCK0/SCL0
    40,               //14    -  SDA4/MISO4/RX1
    39,               //15    - ~SCL4/SCK4/TX1
    43,               //16    - ~SDA3/MISO3/RX1
    42,               //17    - ~SCL3/SCK3/TX1
    26,               //18    - ~LED/SCCRST
    33,               //19/A0 - ~A/SWO/32KHZ
    13,               //20/A1 - ~A/I2SBCLK/RX1
    11,               //21/A2 - ~A/PDMDATA
    29,               //22/A3 - ~A/PDMDATA
    12,               //23/A4 - ~A/PDMCLK/TX1
    31,               //24/A5 - ~A/SCCCLK
    48,               //25    - Not exposed, TX0
    49,               //26    - Not exposed, RX0
    36,               //27    - Not exposed, PDMDATA of Mic
    37,               //28    - Not exposed, PDMCLK of Mic
};

//
// Pin object index table
//
STATIC const machine_pin_obj_t machine_pin_obj[] = {
    {{&machine_pin_type}, 0},    //25     - ~RX1/SDA2/MISO2
    {{&machine_pin_type}, 1},    //24     - ~TX1/32kHz/SWO
    {{&machine_pin_type}, 2},    //44     - ~MOSI4
    {{&machine_pin_type}, 3},    //35/A6  - ~A/TX1/I2SDAT/PDMCLK
    {{&machine_pin_type}, 4},    //4      - ~RX1/SLINT
    {{&machine_pin_type}, 5},    //22     - ~PDMCLK/SWO
    {{&machine_pin_type}, 6},    //23     - ~I2SWCLK/CMPOUT
    {{&machine_pin_type}, 7},    //27     - ~SCL2/SCK2
    {{&machine_pin_type}, 8},    //29     - ~MOSI2/I2SWCLK
    {{&machine_pin_type}, 9},    //32/A7  - ~A/SCCIO
    {{&machine_pin_type}, 10},   //14     -  SWCH/ADCD1N/TX1/PDMCLK
    {{&machine_pin_type}, 11},   //7      - ~MOSI0/CLKOUT
    {{&machine_pin_type}, 12},   //6      - ~MISO0/SDA0/I2SDAT
    {{&machine_pin_type}, 13},   //5      - ~SCK0/SCL0
    {{&machine_pin_type}, 14},   //40     -  SDA4/MISO4/RX1
    {{&machine_pin_type}, 15},   //39     - ~SCL4/SCK4/TX1
    {{&machine_pin_type}, 16},   //43     - ~SDA3/MISO3/RX1
    {{&machine_pin_type}, 17},   //42     - ~SCL3/SCK3/TX1
    {{&machine_pin_type}, 18},   //26     - ~LED/SCCRST
    {{&machine_pin_type}, 19},   //33/A0  - ~A/SWO/32KHZ
    {{&machine_pin_type}, 20},   //13/A1  - ~A/I2SBCLK/RX1
    {{&machine_pin_type}, 21},   //11/A2  - ~A/PDMDATA
    {{&machine_pin_type}, 22},   //29/A3  - ~A/PDMDATA
    {{&machine_pin_type}, 23},   //12/A4  - ~A/PDMCLK/TX1
    {{&machine_pin_type}, 24},   //31/A5  - ~A/SCCCLK
    {{&machine_pin_type}, 25},   //25     - Not exposed, TX0
    {{&machine_pin_type}, 26},   //26     - Not exposed, RX0
    {{&machine_pin_type}, 27},   //27     - Not exposed, PDMDATA of Mic
    {{&machine_pin_type}, 28},   //28     - Not exposed, PDMCLK of Mic
};
