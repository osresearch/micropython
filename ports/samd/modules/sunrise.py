# Adapted from https://web.archive.org/web/20161202180207/http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
import math
from math import floor, degrees, radians
def sin(x): return math.sin(radians(x))
def cos(x): return math.cos(radians(x))
def tan(x): return math.tan(radians(x))
def asin(x): return degrees(math.asin(x))
def acos(x): return degrees(math.acos(x))
def atan(x): return degrees(math.atan(x))


# zenith: Sun's zenith for sunrise/sunset
#   offical      = 90 degrees 50'
#   civil        = 96 degrees
#   nautical     = 102 degrees
#   astronomical = 108 degrees

def sunrise(rising,latitude,longitude,year,month,day,localOffset=1,zenith=96):
	# 1. first calculate day of the year
	N1 = floor(275 * month / 9)
	N2 = floor((month + 9) / 12)
	N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3))
	N = N1 - (N2 * N3) + day - 30

	# 2. convert the longitude to hour value and calculate an approximate time
	lngHour = longitude / 15
 
	if rising:
		t = N + ((6 - lngHour) / 24)
	else:
		t = N + ((18 - lngHour) / 24)

	# 3. calculate the Sun's mean anomaly
	M = (0.9856 * t) - 3.289

	# 4. calculate the Sun's true longitude
	L = M + (1.916 * sin(M)) + (0.020 * sin(2 * M)) + 282.634
	if L > 360:
		L -= 360
	elif L < 0:
		L += 360

	# 5a. calculate the Sun's right ascension
	RA = atan(0.91764 * tan(L))
	if RA > 360:
		RA -= 360
	elif RA < 0:
		RA += 360

	# 5b. right ascension value needs to be in the same quadrant as L
	Lquadrant  = (floor( L/90)) * 90
	RAquadrant = (floor(RA/90)) * 90
	RA = RA + (Lquadrant - RAquadrant)

	# 5c. right ascension value needs to be converted into hours
	RA = RA / 15

	# 6. calculate the Sun's declination
	sinDec = 0.39782 * sin(L)
	cosDec = cos(asin(sinDec))

	# 7a. calculate the Sun's local hour angle
	cosH = (cos(zenith) - (sinDec * sin(latitude))) / (cosDec * cos(latitude))
 
	# the sun never rises on this location (on the specified date)
	if cosH >  1:
		return None
	# the sun never sets on this location (on the specified date)
	if cosH < -1:
		return None

	# 7b. finish calculating H and convert into hours
	if rising:
		H = 360 - acos(cosH)
	else:
		H = acos(cosH)
	H = H / 15

	# 8. calculate local mean time of rising/setting
	T = H + RA - (0.06571 * t) - 6.622

	# 9. adjust back to UTC
	UT = T - lngHour
	# NOTE: UT potentially needs to be adjusted into the range [0,24) by adding/subtracting 24

	# 10. convert UT value to local time zone of latitude/longitude
	localT = UT + localOffset
	if localT > 24:
		localT -= 24
	elif localT < 0:
		localT += 24
	return localT

