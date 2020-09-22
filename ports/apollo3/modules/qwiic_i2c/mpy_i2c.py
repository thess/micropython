#-----------------------------------------------------------------------------
# mpy_i2c.py
#
# Encapsulate MicroPython I2C interface
#------------------------------------------------------------------------


from .i2c_driver import I2CDriver

import sys
import os


_PLATFORM_NAME = "MicroPython"

#-----------------------------------------------------------------------------
# Internal function to connect to the systems I2C bus.
#
# Attempts to fail elegantly. Put this in a central place to support
# error handling  -- especially on non-embedded platforms
#
def _connectToI2CBus():

	try:
		import machine
	except Exception as ee:
		print("Error: Unable to load the I2C subsystem modules")
		return None

	daBus = None

	error=False

	# Connect - catch errors

	try:
		daBus =  machine.I2C(0)
	except Exception as ee:
		if type(ee) is RuntimeError:
			print("Error:\tUnable to connect to I2C bus. %s" % (ee))
			print("\t\tEnsure a board is connected to the %s board." % (os.uname().machine))
		else:
			print("Error:\tFailed to connect to I2C bus. Error: %s" % (ee))

		# We had an error....
		return None

	return daBus


# notes on determining platform
#
# - os.uname().sysname == artemis
#
#
class MicroPythonI2C(I2CDriver):

	# Constructor
	name = _PLATFORM_NAME

	_i2cbus = None

	def __init__(self):

		# Call the super class. The super calss will use default values if not
		# proviced
		I2CDriver.__init__(self)



	# Okay, are we running on a mpy system?
	@classmethod
	def isPlatform(cls):

		# Sparkfun Apollo3/Artemis systems , ESP32 Thing Plus
		return os.uname().sysname in ('artemis', 'esp32')

#-------------------------------------------------------------------------
	# General get attribute method
	#
	# Used to intercept getting the I2C bus object - so we can perform a lazy
	# connect ....
	#
	def __getattr__(self, name):

		if(name == "i2cbus"):
			if( self._i2cbus == None):
				self._i2cbus = _connectToI2CBus()
			return self._i2cbus

		else:
			# Note - we call __getattribute__ to the super class (object).
			return super(I2CDriver, self).__getattribute__(name)

	#-------------------------------------------------------------------------
	# General set attribute method
	#
	# Basically implemented to make the i2cbus attribute readonly to users
	# of this class.
	#
	def __setattr__(self, name, value):

		if(name != "i2cbus"):
			super(I2CDriver, self).__setattr__(name, value)

	#----------------------------------------------------------
	# read Data Command

	def readWord(self, address, commandCode):

		self.i2cbus.writeto(address, bytes([commandCode]), False)

		buffer = bytearray(2)

		self.i2cbus.readfrom_into(address, buffer)

		# build and return a word
		return (buffer[1] << 8 ) | buffer[0]

	#----------------------------------------------------------
	def readByte(self, address, commandCode):

		self.i2cbus.writeto(address, bytes([commandCode]), False)

		buffer = bytearray(1)

		self.i2cbus.readfrom_into(address, buffer)

		return buffer[0]

	#----------------------------------------------------------
	def readBlock(self, address, commandCode, nBytes):

		self.i2cbus.writeto(address, bytes([commandCode]), False)

		buffer = bytearray(nBytes)

		self.i2cbus.readfrom_into(address, buffer)

		return buffer


	#--------------------------------------------------------------------------
	# write Data Commands
	#
	# Send a command to the I2C bus for this device.
	#
	# value = 16 bits of valid data..
	#

	def writeCommand(self, address, commandCode):

		self.i2cbus.writeto(address, bytes([commandCode]))

	#----------------------------------------------------------
	def writeWord(self, address, commandCode, value):

		buffer = bytearray(3)
		buffer[0] = commandCode
		buffer[1] = value & 0xFF
		buffer[2] = (value >> 8) & 0xFF

		self.i2cbus.writeto(address, buffer)


	#----------------------------------------------------------
	def writeByte(self, address, commandCode, value):

		self.i2cbus.writeto(address, bytes([commandCode, value]))

	#----------------------------------------------------------
	def writeBlock(self, address, commandCode, value):

		buffer = bytearray(len(value) + 1)
		buffer[0] = commandCode
		buffer[1:] = bytes(value) if isinstance(value, list) else value

		self.i2cbus.writeto(address, buffer)


	#-----------------------------------------------------------------------
	# scan()
	#
	# Scans the I2C bus and returns a list of addresses that have a devices connected
	#
	@classmethod
	def scan(cls):
		""" Returns a list of addresses for the devices connected to the I2C bus."""

		# Just call the system build it....

		if cls._i2cbus == None:
			cls._i2cbus = _connectToI2CBus()

		if cls._i2cbus == None:
			return []

		return cls._i2cbus.scan()

