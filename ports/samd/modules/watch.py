import machine, time, sys, os
from machine import Pin, PWM
import SLCD


rtc = machine.RTC()

class Debounce:
	def __init__(self, pin):
		self.pin = pin
		self.value = pin.value()
		self.rising = False
		self.falling = False
		self.count = 0
	def update(self):
		value = self.pin.value()
		if value and self.value:
			self.count += 1
		else:
			self.count = 0
		self.rising = value and not self.value
		self.falling = not value and self.value
		self.value = value
		
btn_mode = Debounce(Pin(Pin.cpu.PA23, mode=Pin.IN, pull=Pin.PULL_DOWN))
btn_start = Debounce(Pin(Pin.cpu.PA02, mode=Pin.IN, pull=Pin.PULL_DOWN))
btn_light = Debounce(Pin(Pin.cpu.PA22, mode=Pin.IN, pull=Pin.PULL_DOWN))

#led_red = PWM(Pin(Pin.cpu.PA20), freq=1000)
#led_green = PWM(Pin(Pin.cpu.PA21), freq=1000)
led_red = Pin(Pin.cpu.PA20, mode=Pin.OUT)
led_green = Pin(Pin.cpu.PA21, mode=Pin.OUT)

if machine.reset_cause() != machine.DEEPSLEEP_RESET:
  for i in range(0,2):
	#led_red.duty_u16(10000);
	led_red.value(1)
	time.sleep(0.1);
	led_red.value(0)
	led_green.value(1)
	#led_green.duty_u16(10000);
	#led_red.duty_u16(0);
	time.sleep(0.1);
	led_green.value(0)
	#led_green.duty_u16(0);

# thermister for temperature feedback is only on lite boards
#ts_en = Pin(Pin.cpu.PB04, mode=Pin.OUT)
#ts_en.value(0)
#ts = machine.ADC(Pin.cpu.PB02)
vbus = machine.Pin(Pin.cpu.PB05, mode=Pin.IN, pull=Pin.PULL_DOWN)
#buzzer = machine.ADC(Pin.cpu.PA27)

usb_connected = vbus.value()

day_code = [ "MO", "TU", "WD", "TH", "FR", "SA", "SU", ]
day_list = [ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 ]
military_time = True

def days_in_month(year,mon):
	if mon != 2:
		return day_list[mon-1]
	if year % 4 == 0 and year % 400 != 0:
		return 29
	else:
		return 28

def showtime(force):
	led_green.value(btn_light.pin.value())
	now = rtc.datetime()

	# update the seconds one digit everytime
	sec = now[6]
	sec_low = sec % 10
	SLCD.digit(9, sec_low)

	# update the seconds ten digit if necessary
	if force or sec_low == 0:
		SLCD.digit(8, sec // 10)

	# only update the other digits if necessary
        if not force and sec != 0:
		return True

	day = now[2]
	wday = day_code[now[3]]
	hours = now[4]
	minutes = now[5]

	if not military_time:
		SLCD.ampm(hours > 12)
		hours %= 12
	SLCD.colon(1)
	#SLCD.h24(military_time)
	SLCD.digit(4, hours // 10)
	SLCD.digit(5, hours %  10)
	SLCD.digit(6, minutes // 10)
	SLCD.digit(7, minutes %  10)

	if day < 10:
		SLCD.digit(2, 10)  # blank
		SLCD.digit(3, day)
	else:
		SLCD.digit(2, day // 10)
		SLCD.digit(3, day % 10)

	# day of week
	SLCD.char(0, wday[0])
	SLCD.char(1, wday[1])


	return True

def show_usec(force):
	now = time.ticks_us() // 1000
	for i in range(7,3,-1):
		SLCD.digit(i, now % 10)
		now //= 10
	sec = time.localtime()[5]
	SLCD.digit(8, sec // 10)
	SLCD.digit(9, sec %  10)
	return False

set_field = 0
counter = 0

def digit(pos, value, dont_show):
	if dont_show:
		SLCD.digit(pos+0, 10)
		SLCD.digit(pos+1, 10)
		return

	# special case the day display
	if pos == 2 and value < 10:
		SLCD.digit(pos, 10)
	else:
		SLCD.digit(pos+0, value // 10)
	SLCD.digit(pos+1, value %  10)

def settime(force):
	global set_field, counter

	# flash the current field being modified
	counter = (counter + 1) & 0xFFFF
	now = list(rtc.datetime())
	year = now[0]
	month = now[1]
	day = now[2]
	wday = now[3]
	hour = now[4]
	minute = now[5]
	second = now[6]

	if force:
		set_field = 0
	if btn_start.rising:
		force = True
		set_field = (set_field + 1) % 6
	if force:
		SLCD.slcd.clear()

	blink = (counter & 7) == 0

	if set_field < 3:
		# hms
		digit(4, hour, set_field == 0 and blink)
		digit(6, minute, set_field == 1 and blink)
		digit(8, second, set_field == 2 and blink)
		SLCD.h24(1)
		SLCD.colon(1)
	else:
		# ymd
		digit(4, year // 100, set_field == 3 and blink)
		digit(6, year %  100, set_field == 3 and blink)
		digit(8, month, set_field == 4 and blink)
		digit(2, day, set_field == 5 and blink)

		wday = day_code[now[3]]
		SLCD.char(0, wday[0])
		SLCD.char(1, wday[1])

	# adjust the current field and update the RTC
	# add some speed ramp up
	advance = btn_light.rising
	if btn_light.count < 20:
		if btn_light.count & 3 == 3:
			advance = True
	else:
		advance = True

	if advance:
		if set_field == 0:
			hour = (hour + 1) % 24
		elif set_field == 1:
			minute = (minute + 1) % 60
		elif set_field == 2:
			second = 0
		elif set_field == 3:
			if year < 2024 or year == 2030:
				year = 2024
			else:
				year += 1
		elif set_field == 4:
			if month == 12:
				month = 1
			else:
				month += 1
			day = min(day, days_in_month(year,month))
		elif set_field == 5:
			if day == days_in_month(year,month):
				day = 1
			else:
				day += 1

		# wday is ignored in rtc set
		rtc.datetime((year, month, day, 0, hour, minute, second, 0))


	if usb_connected:
		time.sleep(0.05)

	# Don't deep sleep
	return False

# create interrupt handlers for the three buttons so that
# they will wake from the deep sleep
#btn_mode.irq(handler=False, trigger=Pin.IRQ_RISING, hard=True)
#btn_start.irq(handler=False, trigger=Pin.IRQ_RISING, hard=True)
#btn_light.irq(handler=False, trigger=Pin.IRQ_RISING, hard=True)

modes = [
	showtime,
	settime,
]

def run():
	mode = 0
	last_mode = 0
	while True:
		if mode == 0 and not btn_mode.pin.value():
			goto_sleep = showtime(False)
		else:
			btn_mode.update()
			btn_light.update()
			btn_start.update()
		
			force = False
			if btn_mode.rising:
				mode = (mode + 1) % len(modes)

				# update the full display
				SLCD.slcd.clear()
				force = True
				print("new mode", mode)

			goto_sleep = modes[mode](force)

		if goto_sleep:
			if usb_connected:
				# simulate the wait until RTC tick
				sec = rtc.datetime()[6]
				while sec == rtc.datetime()[6]:
					pass
			else:
				machine.deepsleep()

if usb_connected:
	# just once and force it
	showtime(True)
else:
	run()


# NOTE: to set the RTC you have to provide some sillyness
# the weekday is 1 - 7, the subsec is 0 - 255
# rtc.datetime((year,mon,day,weekday,hour,min,sec,subsec))
