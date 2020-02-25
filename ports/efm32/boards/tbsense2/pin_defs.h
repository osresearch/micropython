    { {&machine_pin_type}, gpioPortD, 11,  0, PWM(0,0) }, // PWM4
    { {&machine_pin_type}, gpioPortD, 12,  1, PWM(1,0) }, // PWM3
    { {&machine_pin_type}, gpioPortD, 12, 2, PWM(2,5) }, // PWM2, warm
    { {&machine_pin_type}, gpioPortB, 13, 3, PWM(3,5) }, // PWM1, on LED10W

    { {&machine_pin_type}, gpioPortD, 8, 4, NO_PWM }, // RED LED
    { {&machine_pin_type}, gpioPortD, 9, 5, NO_PWM }, // Green LED
    { {&machine_pin_type}, gpioPortD, 14, 6, NO_PWM }, // BTN0
    { {&machine_pin_type}, gpioPortD, 15, 7, NO_PWM }, // BT1
    { {&machine_pin_type}, gpioPortF, 0,  8, NO_PWM }, // SWCLK
    { {&machine_pin_type}, gpioPortF, 1,  9, NO_PWM }, // SWD
    { {&machine_pin_type}, gpioPortF, 2,  10, NO_PWM }, // SWO
    { {&machine_pin_type}, gpioPortF, 3,  11, NO_PWM }, // ?

    // internal connections
    { {&machine_pin_type}, gpioPortK, 1,  12, NO_PWM }, // spi cs
    { {&machine_pin_type}, gpioPortF, 7,  13, NO_PWM }, // spi sck
    { {&machine_pin_type}, gpioPortK, 2,  14, NO_PWM }, // spi miso
    { {&machine_pin_type}, gpioPortK, 0,  15, NO_PWM }, // spi mosi

