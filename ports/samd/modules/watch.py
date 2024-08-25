import machine, time, sys, os
from machine import Pin, PWM
import SLCD

rtc = machine.RTC()
btn_mode = Pin(Pin.cpu.PA23, mode=Pin.IN, pull=Pin.PULL_DOWN)
btn_start = Pin(Pin.cpu.PA02, mode=Pin.IN, pull=Pin.PULL_DOWN)
btn_light = Pin(Pin.cpu.PA22, mode=Pin.IN, pull=Pin.PULL_DOWN)

led_red = PWM(Pin(Pin.cpu.PA20), freq=1000)
led_green = PWM(Pin(Pin.cpu.PA21), freq=1000)

# thermister for temperature feedback, not on green board?
ts_en = Pin(Pin.cpu.PB04, mode=Pin.OUT)
ts_en.value(0)
ts = machine.ADC(Pin.cpu.PB02)
vbus = machine.Pin(Pin.cpu.PB05, mode=Pin.IN, pull=Pin.PULL_DOWN)
#buzzer = machine.ADC(Pin.cpu.PA27)

def showtime():
	now = time.localtime()
	SLCD.digit(4, now[3] // 10)
	SLCD.digit(5, now[3] %  10)
	SLCD.digit(6, now[4] // 10)
	SLCD.digit(7, now[4] %  10)
	SLCD.digit(8, now[5] // 10)
	SLCD.digit(9, now[5] %  10)

	day = now[2]
	if day > 10:
		SLCD.digit(2, day // 10)
	SLCD.digit(3, day % 10)

def run():
	last_light_pressed = 0
	while True:
		showtime()
		light_pressed = btn_light.value()
		if light_pressed != last_light_pressed:
			last_light_pressed = light_pressed
			if light_pressed:
				led_red.duty_u16(10000)
				led_green.duty_u16(12000)
			else:
				led_red.duty_u16(0)
				led_green.duty_u16(0)
		machine.lightsleep(100)
		#time.sleep(1)


# NOTE: to set the RTC you have to provide some sillyness
# the weekday is 1 - 7, the subsec is 0 - 255
# rtc.datetime((year,mon,day,weekday,hour,min,sec,subsec))
