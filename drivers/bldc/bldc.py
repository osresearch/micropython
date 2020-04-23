# Brushless three phase motor driver for the AirBreak devices
# testpoints:
#	TIM1_CH1N: T520 
#	TIM1_CH2N: T518
#	TIM1_CH3N: T516
from pyb import Timer, ADC, Pin, bldc_init, bldc_step
from time import sleep_ms
from math import sin, pi

class bldc:
	pwm_pins = [
		"MOTOR_CL", 
		"MOTOR_CH",
		"MOTOR_BL",
		"MOTOR_BH",
		"MOTOR_AL",
		"MOTOR_AH",
	]

	analog_pins = ["C0", "C1", "C2", "C3", "PRESSURE"]

	def __init__(self, period=4096, pwm_max=3000):
		# start with the speed at 0
		self.running = False

		# configure high frequency timer 1 in periodic mode for PWM
		# add some deadtime so that the complementary channels don't
		# overcurrent the MOSFETs.
		# TIM_Cmd(&TIM1,DISABLE);
		# TIM1.CR1 = 4; // only counter overflow generates update interruprs
		# TIM1.CR2 = 0x4011; // preloaded, waiting for commutation event, mms=1: enable counter, oc4 output idle state 1.
		# TIM1.CCER = 0x1444;	// output 4 enable, complementary outputs 1-3 enable
		# TIM_SetCCR4(&TIM1,0xa80);	// what is TIM1.CH4 used for?
		# TIM1.EGR = TIM1.EGR | 0x20;
		# TIM1.CCMR2 = TIM1.CCMR2 | 0x7000; // enable output compare 4
		# TIM_SetAutoreload(&TIM1,0xd20); // 3360?
		# TIM_SetCCR1(&TIM1,0);
		# TIM_SetCCR2(&TIM1,0);
		# TIM_SetCCR3(&TIM1,0);
		# TIM1.BDTR = 0x9846; // dead time 0x46, Main Output Enable, Break Enable, no locks
		# tim_dier_something(&TIM1,0x80,1); // enable break interrupt?
		# TIM_Cmd(&TIM1,ENABLE);
		self.hf_timer = Timer(1,
			prescaler	= 0,
			period		= period,
			deadtime	= 0x46,
		)

		# configure the six output pins as AF mode
		self.pwm = [Pin(pin, mode=Pin.AF_PP, alt=Pin.AF1_TIM1)
			for pin in self.pwm_pins]

		# configure the back EMF and voltage pins
		self.analog = [ADC(Pin(pin, mode=Pin.ANALOG))
			for pin in self.analog_pins]
		self.samples = [0 for pin in self.analog_pins]

		# force a timer re-init
		self.stop()
		bldc_init(pwm_max)

		print("HF timer:", self.hf_timer)
		#print("LF timer:", self.lf_timer)

	def stop(self):
		# configure the output channels for the three PWM values
		# the six pins have been configured in complementary mode,
		# so each pair shares one PWM timer.
		self.running = False
		self.hf_timer.channel(1, mode=Timer.PWM, pulse_width=0)
		self.hf_timer.channel(2, mode=Timer.PWM, pulse_width=0)
		self.hf_timer.channel(3, mode=Timer.PWM, pulse_width=0)


	def _advance(self,step):
		if self.running:
			bldc_step(step)

	def emf(self):
		self.raw = [pin.read() for pin in self.analog]
		for i in range(0,len(self.samples)):
			self.samples[i] = (self.samples[i] * 3 + self.raw[i]) / 4.0
		return self.samples

	def run(self, freq=100, step=1):
		# configure low frequency timer 3 in periodic mode to advance based on the speed
		# the parameters are the same as the vendor firmware
 		self.lf_timer = Timer(3,
			freq		= freq,
			callback	= lambda x: self._advance(step),
		)

		self.running = True
		while self.running:
			emf = self.emf()
			print(
				int(self.raw[0]), int(emf[1]), int(emf[2]), int(emf[3]), int(emf[4]),
				self.hf_timer.channel(1).pulse_width(),
				self.hf_timer.channel(2).pulse_width(),
				self.hf_timer.channel(3).pulse_width(),
			)
			if emf[0] < 50:
				self.running = False
		self.stop()

def bang():
	# this totally fails due to overcurrent
	pins = [
		[ "MOTOR_CH", 0, 1, 1, 0, 0, 0 ],
		[ "MOTOR_CL", 0, 0, 0, 0, 1, 1 ],
		[ "MOTOR_BH", 1, 0, 0, 0, 0, 1 ],
		[ "MOTOR_BL", 0, 0, 1, 1, 0, 0 ],
		[ "MOTOR_AH", 0, 0, 0, 1, 1, 0 ],
		[ "MOTOR_AL", 1, 1, 0, 0, 0, 0 ],
		#[ "MOTOR_AL", 0, 0, 0, 0, 0, 0 ],
		#[ "MOTOR_BL", 0, 0, 0, 0, 0, 0 ],
		#[ "MOTOR_CL", 0, 0, 0, 0, 0, 0 ],
	]

	for pin in pins:
		pin[0] = Pin(pin[0], mode=Pin.OUT)
		pin[0].value(0)

	analog = [ADC(Pin(_, mode=Pin.ANALOG))
		for _ in ["C0", "C1", "C2", "C3"]]

	phase = 0

	while analog[0].read() > 80:
		for pin in pins:
			pin[0].value(pin[phase + 1])
		phase = (phase + 1) % 6
		print(phase, [pin.read() for pin in analog])
		sleep_ms(10)

	for pin in pins:
		pin[0].value(0)
