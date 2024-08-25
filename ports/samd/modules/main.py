import machine, time, sys, os
from machine import Pin, PWM
print("Hello, world!", time.ticks_us())
print(machine.unique_id().hex())

import watch
watch.run()
