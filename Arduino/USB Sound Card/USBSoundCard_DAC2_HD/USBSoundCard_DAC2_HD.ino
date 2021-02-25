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
// *************** 

// This sketch turns Teensy 4.X + HiFiBerry DAC2 HD into a USB sound card. 
// Note: 44.1kHz + 16-bit stereo mode only!

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#define PCM1796_ADDRESS 0x4C
#define SI5351_ADDRESS 0x62
#define RESET_PIN 9

AudioInputUSB            usb1;          
AudioOutputI2Sslave      i2sslave1;
AudioConnection          patchCord1(usb1, 0, i2sslave1, 0);
AudioConnection          patchCord2(usb1, 1, i2sslave1, 1);

float dacMaxTaperLevel = 254; // -0.5dB
float lastVolume = 0;

void setup() {
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  AudioMemory(6);
  Wire.begin();
  setup_SI5351();
  setup_PCM1796();
}

void loop () {
  float vol = usb1.volume(); // PC volume setting
  if (vol != lastVolume) {
    lastVolume = vol;
    vol = vol * dacMaxTaperLevel;
    set_PCM1796_Attenuation(vol);
  }
  delay(100);
}

void setup_PCM1796() {
  digitalWrite(RESET_PIN, HIGH);
  delay(1);
  digitalWrite(RESET_PIN, LOW);
  delay(1);
  digitalWrite(RESET_PIN, HIGH);
  delay(10);
  i2c_write(PCM1796_ADDRESS, 16, B11110101); // -5 DB
  i2c_write(PCM1796_ADDRESS, 17, B11110101); 
  i2c_write(PCM1796_ADDRESS, 18, B11000000); 
  i2c_write(PCM1796_ADDRESS, 19, B00000000); 
}

void set_PCM1796_Attenuation(byte val) {
  i2c_write(PCM1796_ADDRESS, 16, val); // -0.5 DB
  i2c_write(PCM1796_ADDRESS, 17, val); 
  i2c_write(PCM1796_ADDRESS, 18, B11000000); 
  i2c_write(PCM1796_ADDRESS, 19, B00000000); 
}

void setup_SI5351() {
  byte REGs[] = {0x02,0x03,0x07,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x1C,0x1D,0x1F,0x2A,0x2C,0x2F,0x30,0x31,0x32,0x34,0x37,0x38,0x39,0x3A,0x3B,0x3E,0x3F,0x40,0x41,0x5A,0x5B,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0xA2,0xA3,0xA4,0xB7,0xB1,0x1A,0x1B,0x1E,0x20,0x21,0x2B,0x2D,0x2E,0x33,0x35,0x36,0x3C,0x3D,0xB1};
  byte Vals[] = {0x53,0x00,0x20,0x00,0x0D,0x1D,0x0D,0x8C,0x8C,0x8C,0x8C,0x8C,0x2A,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x92,0xAC,0x3D,0x09,0xF3,0x13,0x75,0x04,0x11,0xE0,0x01,0x9D,0x00,0x42,0x7A,0xAC};

  for (int i = 0; i < sizeof(REGs); i++) {
    i2c_write(SI5351_ADDRESS, REGs[i], Vals[i]);
  }
}

void i2c_write(byte i2cAddress, byte address, byte val) {
  Wire.beginTransmission(i2cAddress);  
  Wire.write(address);
  Wire.write(val);
  Wire.endTransmission();
}
