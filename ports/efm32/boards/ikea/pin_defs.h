    { {&machine_pin_type}, gpioPortA, 0,  0, PWM(0,0) }, // pcb label PWM4
    { {&machine_pin_type}, gpioPortA, 1,  1, PWM(1,0) }, // pcb label PWM3
    { {&machine_pin_type}, gpioPortB, 12, 2, PWM(2,5) }, // pcb label PWM2
    { {&machine_pin_type}, gpioPortB, 13, 3, PWM(3,5) }, // pcb label PWM1

    // right side going up
    { {&machine_pin_type}, gpioPortB, 15, 4, NO_PWM },
    { {&machine_pin_type}, gpioPortB, 14, 5, NO_PWM },
    { {&machine_pin_type}, gpioPortC, 12, 6, NO_PWM }, // TX
    { {&machine_pin_type}, gpioPortC, 11, 7, NO_PWM }, // RX
    { {&machine_pin_type}, gpioPortF, 0,  8, NO_PWM }, // SWCLK
    { {&machine_pin_type}, gpioPortF, 1,  9, NO_PWM }, // SWD
    { {&machine_pin_type}, gpioPortF, 2,  10, NO_PWM }, // ?
    { {&machine_pin_type}, gpioPortF, 3,  11, NO_PWM }, // ?

    // internal connections
    { {&machine_pin_type}, gpioPortB, 11,  12, NO_PWM }, // spi cs
    { {&machine_pin_type}, gpioPortD, 13,  13, NO_PWM }, // spi sck
    { {&machine_pin_type}, gpioPortD, 14,  14, NO_PWM }, // spi miso
    { {&machine_pin_type}, gpioPortD, 15,  15, NO_PWM }, // spi mosi

