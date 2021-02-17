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

// NOTE: Configure user macros below to indicate the HiFiBerry DAC board you have attached, and to set the freq range of the sine wave sweep.
// COMPATIBILITY: Teensy 4.0-4.1, HIFiBerry DAC+, DAC2 Pro, DAC2 HD. Note: For HIFiBerry DAC+ Pro (discontinued), configure as DAC2 Pro.

#include "Wire.h"
#include <Audio.h>
#include <utility/imxrt_hw.h>

// ---------------USER MACROS--------------

// ------------Device Selection------------
// Uncomment one line in the group below to indicate your board
// #define HIFIBERRY_DACPLUS
// #define HIFIBERRY_DAC2_PRO
// #define HIFIBERRY_DAC2_HD

// ------------Playback Parameters---------
#define HEADPHONE_AMP_GAIN 10 // 0-63, DAC2 Pro only
#define SINE_FREQ_MIN 20
#define SINE_FREQ_MAX 10000

// -------------END USER MACROS------------

// Device I2C Addresses
#define PCM5122_ADDRESS 0x4D
#define TPA6130A2_ADDRESS 0x60
#define PCM1796_ADDRESS 0x4C
#define SI5351_ADDRESS 0x62

// Pins
#define RESET_PIN 9 // Only used for HiFiBerry DAC2 HD

uint32_t sineFrequency = SINE_FREQ_MIN;

#ifdef HIFIBERRY_DACPLUS
  AudioOutputI2S i2s1; 
#else
  AudioOutputI2Sslave i2s1; 
#endif

AudioSynthWaveformSine   sine1;
AudioConnection          patchCord1(sine1, 0, i2s1, 0);
AudioConnection          patchCord2(sine1, 0, i2s1, 1);

#if !defined(HIFIBERRY_DACPLUS) && !defined(HIFIBERRY_DAC2_PRO) && !defined(HIFIBERRY_DAC2_HD)
#error Error! You must uncomment a macro in the Device Selection section at the top of this sketch to indicate which HiFiBerry board you are using
#endif

void setup() {
  AudioMemory(2);
  sine1.amplitude(0.5);
  sine1.frequency(SINE_FREQ_MIN);
  Wire.begin();
  #ifdef HIFIBERRY_DACPLUS
    setup_PCM5122_AsI2SSlave();
  #endif
  #ifdef HIFIBERRY_DAC2_PRO
    setup_PCM5122_AsI2SMaster();
    setup_TPA6130A2();
  #endif
  #ifdef HIFIBERRY_DAC2_HD
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, LOW);
    setup_SI5351();
    setup_PCM1796();  
  #endif
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

void setup_SI5351() {
  // This setup configures the SI5351 clock source to supply clocks for 16-bit 44.1khz stereo sampling
  byte REGs[] = {0x02,0x03,0x07,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x1C,0x1D,0x1F,0x2A,0x2C,0x2F,0x30,0x31,0x32,0x34,0x37,0x38,0x39,0x3A,0x3B,0x3E,0x3F,0x40,0x41,0x5A,0x5B,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0xA2,0xA3,0xA4,0xB7,0xB1,0x1A,0x1B,0x1E,0x20,0x21,0x2B,0x2D,0x2E,0x33,0x35,0x36,0x3C,0x3D,0xB1};
  byte Vals[] = {0x53,0x00,0x20,0x00,0x0D,0x1D,0x0D,0x8C,0x8C,0x8C,0x8C,0x8C,0x2A,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x92,0xAC,0x3D,0x09,0xF3,0x13,0x75,0x04,0x11,0xE0,0x01,0x9D,0x00,0x42,0x7A,0xAC};
  for (int i = 0; i < sizeof(REGs); i++) {
    i2c_write(SI5351_ADDRESS, REGs[i], Vals[i]);
  }
}

void setup_PCM1796() {
  digitalWrite(RESET_PIN, HIGH);
  delay(1);
  digitalWrite(RESET_PIN, LOW);
  delay(1);
  digitalWrite(RESET_PIN, HIGH);
  delay(10);
  i2c_write(PCM1796_ADDRESS, 16, B11110101); // -5 DB - Left Ch. 255 = no attenuation. Each bit subtracts 0.5dB
  i2c_write(PCM1796_ADDRESS, 17, B11110101); // -5 DB - Right Ch 
  i2c_write(PCM1796_ADDRESS, 18, B11000000); // Use reg 16+17 for attenuation, Audio format = 16 bit I2S
  i2c_write(PCM1796_ADDRESS, 19, B00000000); 
}

void i2c_write(byte i2cAddress, byte address, byte val) {
  Wire.beginTransmission(i2cAddress);  
  Wire.write(address);
  Wire.write(val);
  Wire.endTransmission();
}
