/*
  ----------------------------------------------------------------------------

  This file is part of the Sanworks Teensy4_To_HiFiBerryDAC repository
  Copyright (C) 2021 Sanworks LLC, Rochester, New York, USA

  ----------------------------------------------------------------------------

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3.

  This program is distributed  WITHOUT ANY WARRANTY and without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

// NOTE: Configure user macros below to set Teensy as I2S master or slave, to enable or disable the DAC2 Pro headphone amp, or to set the freq range of the sine wave sweep.
// COMPATIBILITY: Teensy 4.0-4.1, HIFiBerry DAC+, DAC2 Pro. **DOES NOT** work with HiFiBerry DAC2 HD.

#include "Wire.h"
#include <Audio.h>
#include <utility/imxrt_hw.h>
#define PCM5122_ADDRESS 0x4D
#define TPA6130A2_ADDRESS 0x60

// User Macros
#define TEENSY_I2S_MASTER 1 // Set to 1 for Teensy as I2S master. Set to 0 for HiFiBerry as I2S master using its precision clock source
#define IS_DAC2_PRO 0       // Set to 1 to enable headphone amp (DAC2 Pro only), otherwise 0
#define HEADPHONE_AMP_GAIN 10 // 0-63, DAC2 Pro only
#define SINE_FREQ_MIN 20
#define SINE_FREQ_MAX 10000

uint32_t sineFrequency = SINE_FREQ_MIN;

#if TEENSY_I2S_MASTER
  AudioOutputI2S i2s1; 
#else
  AudioOutputI2Sslave i2s1; 
#endif
AudioSynthWaveformSine   sine1;
AudioConnection          patchCord1(sine1, 0, i2s1, 0);
AudioConnection          patchCord2(sine1, 0, i2s1, 1);

void setup() {
  AudioMemory(2);
  sine1.amplitude(0.5);
  sine1.frequency(SINE_FREQ_MIN);
  Wire.begin();
  #if TEENSY_I2S_MASTER
    setup_PCM5122_AsI2SSlave();
  #else
    setup_PCM5122_AsI2SMaster();
  #endif
  if (IS_DAC2_PRO) {
    setup_TPA6130A2();
  }
}

void loop() {
  sineFrequency += 1;
  if (sineFrequency > SINE_FREQ_MAX) {
    sineFrequency = SINE_FREQ_MIN;
  }
  sine1.frequency(sineFrequency);
  delayMicroseconds(500);
}

void setup_PCM5122_AsI2SSlave() {
  //                        REG# Bits
  i2c_write(PCM5122_ADDRESS, 2,  B00010000); // Power --> Standby
  i2c_write(PCM5122_ADDRESS, 1,  B00010001); // Reset DAC
  i2c_write(PCM5122_ADDRESS, 2,  B00000000); // Power --> On
  i2c_write(PCM5122_ADDRESS, 13, B00010000); // Set PLL reference clock = BCK
  i2c_write(PCM5122_ADDRESS, 14, B00010000); // Set DAC clock source = PLL
  i2c_write(PCM5122_ADDRESS, 37, B01111101); // Ignore various errors
  i2c_write(PCM5122_ADDRESS, 61, B00110000); // Set left ch volume attenuation, 48 = 0dB
  i2c_write(PCM5122_ADDRESS, 62, B00110000); // Set right ch volume attenuation, 48 = 0dB
  i2c_write(PCM5122_ADDRESS, 65, B00000000); // Set mute off
  i2c_write(PCM5122_ADDRESS, 2,  B00000000); // Power --> On
}

void setup_PCM5122_AsI2SMaster() {
  //                        REG# Bits
  // Reset and powerup
  i2c_write(PCM5122_ADDRESS, 2,  B00010000); // Power --> Standby
  i2c_write(PCM5122_ADDRESS, 1,  B00010001); // Reset DAC
  i2c_write(PCM5122_ADDRESS, 2,  B00000000); // Power --> On
  i2c_write(PCM5122_ADDRESS, 2,  B00010000); // Power --> Standby

  // DAC GPIO setup
  i2c_write(PCM5122_ADDRESS, 8,  B00100100); // Register 8 = GPIO enable = 24 = enable ch3 + 6
  i2c_write(PCM5122_ADDRESS, 82, B00000010); // Reg 82 = GPIO ch3 config = 02 = output controlled by reg 86
  i2c_write(PCM5122_ADDRESS, 83, B00000010); // Reg 82 = GPIO ch4 config = 02 = output controlled by reg 86
  i2c_write(PCM5122_ADDRESS, 85, B00000010); // Reg 85 = GPIO ch6 config = 02 = output controlled by reg 86
  i2c_write(PCM5122_ADDRESS, 86, B00000000); // Reg 86 = GPIO write = 00 (all lines low)

  // Master mode setup
  i2c_write(PCM5122_ADDRESS, 9,  B00010001); //BCK, LRCLK configuration = 10001 = set BCK and LRCK to outputs (master mode)
  i2c_write(PCM5122_ADDRESS, 12, B00000011); //Master mode BCK, LRCLK reset = 11 = BCK and LRCK clock dividers enabled
  i2c_write(PCM5122_ADDRESS, 40, B00000000); //I2S data format, word length = 0 = 16 bit I2S
  i2c_write(PCM5122_ADDRESS, 37, B01111101); // Ignore various errors
  i2c_write(PCM5122_ADDRESS, 4,  B00000000); // Disable PLL = 0 = off
  i2c_write(PCM5122_ADDRESS, 14, B00110000); //DAC clock source selection = 011 = SCK clock
  i2c_write(PCM5122_ADDRESS, 28, B00000011); //DAC clock divider (B00001 = divide by 2, etc. 3 = divide by 4, same for next 4 dividers)
  i2c_write(PCM5122_ADDRESS, 29, B00000011); //CP clock divider
  i2c_write(PCM5122_ADDRESS, 30, B00000111); //OSR clock divider
  i2c_write(PCM5122_ADDRESS, 32, B00000111); //Master mode BCK divider
  i2c_write(PCM5122_ADDRESS, 33, B00111111); //Master mode LRCK divider
  i2c_write(PCM5122_ADDRESS, 86, B00100000); // Reg 86 = GPIO write = 36 (line 6 high) // Enable 22.5792MHz precision clock for 44.1kHz
  i2c_write(PCM5122_ADDRESS, 19, B00000001); // Clock sync request
  i2c_write(PCM5122_ADDRESS, 19, B00000000); // Clock sync start

  // Finish startup
  i2c_write(PCM5122_ADDRESS, 61, B00110000); // Set left ch volume attenuation, 48 = 0dB
  i2c_write(PCM5122_ADDRESS, 62, B00110000); // Set right ch volume attenuation, 48 = 0dB
  i2c_write(PCM5122_ADDRESS, 65, B00000000); // Set mute off
  i2c_write(PCM5122_ADDRESS, 2,  B00000000); // Power --> On
}

void setup_TPA6130A2() {
  //                         REG# Bits
  i2c_write(TPA6130A2_ADDRESS, 1, B11000000); // Set both output channels enabled
  i2c_write(TPA6130A2_ADDRESS, 2, HEADPHONE_AMP_GAIN); // 64 gain taper levels are valid in range 0-63. 10 is a comfortable initial volume.
}

void i2c_write(byte i2cAddress, byte address, byte val) {
  Wire.beginTransmission(i2cAddress);  
  Wire.write(address);
  Wire.write(val);
  Wire.endTransmission();
}
