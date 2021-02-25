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

// ***IMPORTANT***
// You must select 'Audio' from tools > USB Type before loading this sketch!
// Please also configure the macro below to select headphone or RCA output.
// *************** 

// This sketch turns Teensy 4.X + HiFiBerry DAC2 PRO into a USB sound card. 
// Note: 44.1kHz + 16-bit stereo mode only!

// ***USER OUTPUT CONFIG***
// NOTE: Uncomment one line only!
// #define OUTPUT_RCA // RCA output
#define OUTPUT_AMP // Headphone amp output
//*************************

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#define PCM5122_ADDRESS 0x4D
#define TPA6130A2_ADDRESS 0x60
#define TPA6130A2_REG1 0x1
#define TPA6130A2_REG2 0x2

#if !defined(OUTPUT_RCA) && !defined(OUTPUT_AMP)
#error Error! You must uncomment a macro in the output config section at the top of this sketch to indicate which DAC2 Pro output channels you are using
#endif

AudioInputUSB            usb1;          
AudioOutputI2Sslave      i2sslave1;
AudioConnection          patchCord1(usb1, 0, i2sslave1, 0);
AudioConnection          patchCord2(usb1, 1, i2sslave1, 1);

uint32_t samplingRate = 44100;
boolean LED_Enabled = true;
float headphoneAmpMaxTaperLevel = 30;
float PCM5122MaxTaperLevel = 207;
float lastVolume = 0;

void setup() {
  AudioMemory(8);
  Wire.begin();
  setup_PCM5122_I2SMaster();
  #ifdef OUTPUT_AMP
    setAmpPower(1);
    setAmpGain(usb1.volume());
  #endif
}

void loop () {
  float vol = usb1.volume(); // PC volume setting
  if (vol != lastVolume) {
    lastVolume = vol;
    #ifdef OUTPUT_AMP
      vol = vol * headphoneAmpMaxTaperLevel;
      setAmpGain(vol);
    #else
      vol = vol * PCM5122MaxTaperLevel;
      setPCM5122DigitalGain(vol);
    #endif
  }
  delay(100);
}

void setAmpPower(boolean powerState) {
  if (powerState == 0) {
    i2c_write(TPA6130A2_ADDRESS, TPA6130A2_REG1, 0); // 00000000 - both channels disabled
  } else {
    i2c_write(TPA6130A2_ADDRESS, TPA6130A2_REG1, 192); // 11000000 - both channels enabled
  }
}

void setAmpGain(byte taperLevel) {
  if (taperLevel > headphoneAmpMaxTaperLevel) {
    taperLevel = headphoneAmpMaxTaperLevel;
  }
  i2c_write(TPA6130A2_ADDRESS, TPA6130A2_REG2, taperLevel);
}

void setup_PCM5122_I2SMaster() {
    // Reset and powerup
  i2c_write(PCM5122_ADDRESS, 2,  B00010000); // Power --> Standby
  i2c_write(PCM5122_ADDRESS, 1,  B00010001); // Reset DAC
  i2c_write(PCM5122_ADDRESS, 2,  B00000000); // Power --> On
  i2c_write(PCM5122_ADDRESS, 2,  B00010000); // Power --> Standby

  // DAC GPIO setup
  i2c_write(PCM5122_ADDRESS, 8,  B00101100); // Register 8 = GPIO enable = 24 = enable ch3 + 6
  i2c_write(PCM5122_ADDRESS, 82, B00000010); // Reg 82 = GPIO ch3 config = 02 = output controlled by reg 86
  i2c_write(PCM5122_ADDRESS, 83, B00000010); // Reg 82 = GPIO ch4 config = 02 = output controlled by reg 86
  i2c_write(PCM5122_ADDRESS, 85, B00000010); // Reg 85 = GPIO ch6 config = 02 = output controlled by reg 86
  i2c_write(PCM5122_ADDRESS, 86, B00000000); // Reg 86 = GPIO write = 00 (all lines low)

  // Master mode setup
  i2c_write(PCM5122_ADDRESS, 9,  B00010001); //BCK, LRCLK configuration = 10001 = set BCK and LRCK to outputs (master mode)
  i2c_write(PCM5122_ADDRESS, 12, B00000011); //Master mode BCK, LRCLK reset = 11 = BCK and LRCK clock dividers enabled
  i2c_write(PCM5122_ADDRESS, 40, B00000000); //I2S data format, word length = 0 = 16 bit I2S //**DOES NOT MATTER**
  i2c_write(PCM5122_ADDRESS, 37, B01111101); // Ignore various errors
  i2c_write(PCM5122_ADDRESS, 4,  B00000000); // Disable PLL = 0 = off
  i2c_write(PCM5122_ADDRESS, 14, B00110000); //DAC clock source selection = 011 = SCK clock
  
  switch (samplingRate) {
    case 44100:
        i2c_write(PCM5122_ADDRESS, 32, B00000111); //Master mode BCK divider
        i2c_write(PCM5122_ADDRESS, 33, B00111111); //Master mode LRCK divider
        if (LED_Enabled) {
          i2c_write(PCM5122_ADDRESS, 86, B00101000); // Reg 86 = GPIO write = 36 (line 3 high) // Enable precision clock
        } else {
          i2c_write(PCM5122_ADDRESS, 86, B00100000); // Reg 86 = GPIO write = 36 (line 3 high) // Enable precision clock
        }
    break;
    case 48000:
        i2c_write(PCM5122_ADDRESS, 32, B00000111); //Master mode BCK divider
        i2c_write(PCM5122_ADDRESS, 33, B00111111); //Master mode LRCK divider
        if (LED_Enabled) {
          i2c_write(PCM5122_ADDRESS, 86, B00001100); // Reg 86 = GPIO write = 36 (line 3 high) // Enable precision clock
        } else {
          i2c_write(PCM5122_ADDRESS, 86, B00000100); // Reg 86 = GPIO write = 36 (line 3 high) // Enable precision clock
        }
    break;
    case 96000:
        i2c_write(PCM5122_ADDRESS, 32, B00000011); //Master mode BCK divider
        i2c_write(PCM5122_ADDRESS, 33, B00111111); //Master mode LRCK divider
        if (LED_Enabled) {
          i2c_write(PCM5122_ADDRESS, 86, B00001100); // Reg 86 = GPIO write = 36 (line 3 high) // Enable precision clock
        } else {
          i2c_write(PCM5122_ADDRESS, 86, B00000100); // Reg 86 = GPIO write = 36 (line 3 high) // Enable precision clock
        }
    break;
    case 192000:
        i2c_write(PCM5122_ADDRESS, 32, B00000001); //Master mode BCK divider
        i2c_write(PCM5122_ADDRESS, 33, B00111111); //Master mode LRCK divider
        if (LED_Enabled) {
          i2c_write(PCM5122_ADDRESS, 86, B00001100); // Reg 86 = GPIO write = 36 (line 3 high) // Enable precision clock
        } else {
          i2c_write(PCM5122_ADDRESS, 86, B00000100); // Reg 86 = GPIO write = 36 (line 3 high) // Enable precision clock
        }
    break;
  }
  
  i2c_write(PCM5122_ADDRESS, 19, B00000001); // Clock sync request
  i2c_write(PCM5122_ADDRESS, 19, B00000000); // Clock sync start

  // Finish startup
  i2c_write(PCM5122_ADDRESS, 61, B00110000); // Set left ch volume attenuation, 48 = 0dB
  i2c_write(PCM5122_ADDRESS, 62, B00110000); // Set right ch volume attenuation, 48 = 0dB
  i2c_write(PCM5122_ADDRESS, 65, B00000000); // Set mute off
  i2c_write(PCM5122_ADDRESS, 2,  B00000000); // Power --> On
}

void setPCM5122DigitalGain(byte taperLevel) {
    if (taperLevel > PCM5122MaxTaperLevel) {
      taperLevel = PCM5122MaxTaperLevel;
    }
    byte newTaperLevel = 255-taperLevel;
    i2c_write(PCM5122_ADDRESS, 61, newTaperLevel); // Set left ch volume attenuation, 48 = 0dB
    i2c_write(PCM5122_ADDRESS, 62, newTaperLevel); // Set right ch volume attenuation, 48 = 0dB
}

void i2c_write(byte i2cAddress, byte address, byte val) {
  Wire.beginTransmission(i2cAddress);  
  Wire.write(address);
  Wire.write(val);
  Wire.endTransmission();
}
