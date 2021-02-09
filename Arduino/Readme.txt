Known issues in current release:
-Harmonic distortion is above the board spec.
-Setting a different sine signal to the right audio channel with AudioConnection results in a corrupted signal on both channels

Compatability:
-Teensy As I2S Master --> HiFiBerry DAC+ and HiFiBerry DAC2 Pro
-Teensy As I2S Slave  --> HiFiBerry DAC2 Pro only

Description:
This example code uses the Teensy Audio Library's AudioOutputI2S object to output sound to the PCM5122 DAC. It outputs a sine wave sweep on both channels, and optionally uses the DAC2 Pro's headphone amp. A block of user-configurable macros near the top of the script are used to set the frequency sweep, I2S mode and headphone amp gain.

In Teensy-as-master mode, the DAC is initialized with PLL reference clock = bit clock, and DAC clock source = PLL. The PCM5122 auto-configures its clock dividers, unless this function is explicitly disabled. 

In Teensy-as-slave mode, the DAC is initialized with both PLL and clock auto-config disabled, and DAC clock source set to SCK. Clock dividers are explicitly programmed, using the values in Table 133 of the PCM5122 datasheet. The DAC2 Pro has two onboard precision clock sources - one at 22.5792MHz (for sampling rates that are multiples of 44.1k) and one at 24.576MHz (for sampling rates that are multiples of 16k). These clock sources are disabled by default allowing the board to function as an I2S slave, but either can be enabled depending on which sampling rate is desired. Enabling the clocks is accomplished by setting GPIO output pins of the DAC: GPIO6 to enable the 22.5792MHz clock, and GPIO3 to enable the 24.576MHz clock. See comments in the example script for GPIO config details.