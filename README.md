Simple Linux i2c-dev BSL Flashing Tool for MSP430
Michael Brooks <mbrooks@ti.com>
1/28/2014

This code enables you to use the i2c-dev interface in Linux to update the flash of a 5-series MSP430.
It makes the following assumptions:
* You have a firmware image that has already been CRC'd.
* You are using the i2c in-band way to jump to BSL (see below).

Usage:
bsl_flasher <i2c bus number> <Location of CRC hex file>

Changes for your platform:
This example has main MSP430 code that will invalidate the BSL CRC when 0xFE is written twice from the host.
This causes the system to go to BSL on reboot. You will likely need to update jump_to_bsl() to incorporate the way you jump
to BSL (i.e. toogle the BSL_EN GPIO).

Known Issues:
* The reset command from the F522x BSL doesn't seem to actually work so it will always fail on the reset command. This requires
  a full power cycle, since a system reset will likely not cut the rails.
* This doesn't elegantly detect the end of an image. We write the entire flash with 0xFF at the end and we timeout while doing this.
  This project simply looks for that timeout.

Improvements Planned:
* CRC for the main system image.
* More intelligent end to the flashing
* Correct MSP430 reboot

