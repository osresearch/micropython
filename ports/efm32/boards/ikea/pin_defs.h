    { {&machine_pin_type}, gpioPortA, 0,  0, PWM(0,0) }, // pcb label PWM4	- PA0	Join BTN
    { {&machine_pin_type}, gpioPortA, 1,  1, PWM(1,0) }, // pcb label PWM3	- PA1	
    { {&machine_pin_type}, gpioPortB, 12, 2, PWM(2,5) }, // pcb label PWM2	- PB12	U/D BTN
    { {&machine_pin_type}, gpioPortB, 13, 3, PWM(3,5) }, // pcb label PWM1	- PB13	

    // right side going up
    { {&machine_pin_type}, gpioPortB, 15, 4, NO_PWM }, // RX			- PB15	RX
    { {&machine_pin_type}, gpioPortB, 14, 5, NO_PWM }, // TX			- PB14	TX
    { {&machine_pin_type}, gpioPortC, 10, 6, NO_PWM }, //			- PC10	U/D BTN
    { {&machine_pin_type}, gpioPortC, 11, 7, NO_PWM }, //			- PC11	LED
    { {&machine_pin_type}, gpioPortF, 0,  8, NO_PWM }, // SWCLK		//TODO: see if user can kill his debug connection
    { {&machine_pin_type}, gpioPortF, 1,  9, NO_PWM }, // SWD			//TODO: see if user can kill his debug connection
    { {&machine_pin_type}, gpioPortF, 2,  10, NO_PWM }, // ?			- F2
    { {&machine_pin_type}, gpioPortF, 3,  11, NO_PWM }, // ?			- F3	SPI FLASH VCC and HOLD

    // internal connections
    { {&machine_pin_type}, gpioPortB, 11,  12, NO_PWM }, // spi cs
    { {&machine_pin_type}, gpioPortD, 13,  13, NO_PWM }, // spi sck
    { {&machine_pin_type}, gpioPortD, 14,  14, NO_PWM }, // spi miso
    { {&machine_pin_type}, gpioPortD, 15,  15, NO_PWM }, // spi mosi

