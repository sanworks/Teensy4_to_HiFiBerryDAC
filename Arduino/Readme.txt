Known issues in current release:
-Setting a different sine signal to the right audio channel with AudioConnection results in a corrupted signal on both channels (this issue may require mods to the Teensy Audio library)

Compatability:
-Teensy As I2S Master --> HiFiBerry DAC+ and HiFiBerry DAC2 Pro
-Teensy As I2S Slave  --> HiFiBerry DAC2 Pro, HiFiBerry DAC2 HD

Description:
This example code uses the Teensy Audio Library's AudioOutputI2S object to output sound to the PCM5122 and PCM1796 DACs on HiFiBerry DAC boards. It outputs a sine wave sweep on both channels, and optionally uses the DAC2 Pro's headphone amp. A block of user-configurable macros near the top of the script are used to set the HiFiBerry board model, frequency sweep parameters and headphone amp gain.

In Teensy-as-master mode, the DAC is initialized with PLL reference clock = bit clock, and DAC clock source = PLL. The PCM5122 auto-configures its clock dividers, unless this function is explicitly disabled. 

In Teensy-as-slave mode, there are two DACs supported - PCM5122 (DAC2 Pro) and PCM1796 (DAC2 HD). PCM5122 is initialized with both PLL and clock auto-config disabled, and DAC clock source set to SCK. Clock dividers are explicitly programmed, using the values in Table 133 of the PCM5122 datasheet. The DAC2 Pro has two onboard precision clock sources - one at 22.5792MHz (for sampling rates that are multiples of 44.1k) and one at 24.576MHz (for sampling rates that are multiples of 16k). These clock sources are disabled by default allowing the board to function as an I2S slave, but either can be enabled depending on which sampling rate is desired. Enabling the clocks is accomplished by setting GPIO output pins of the DAC: GPIO6 to enable the 22.5792MHz clock, and GPIO3 to enable the 24.576MHz clock. See comments in the example script for GPIO config details. For PCM1796, the external clock source is a SI5351 programmable clock, which must be configured for each sampling rate via its I2S. The included code programs the clock for 16-bit 44.1kHz stereo playback.