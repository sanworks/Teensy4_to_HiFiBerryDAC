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

// This example file plays a 20Hz-80kHz frequency sweep sampled at 192kHz using HiFiBerry DAC2 HD.
// Send any byte over the serial terminal to start playback of a single sweep.
// This example does not use the Audio library - instead, the sine wave is synthesized directly in the I2C DMA channel's ISR

#include "Wire.h"
#include <Audio.h>
#include <utility/imxrt_hw.h>
#define PCM1796_ADDRESS 0x4C
#define SI5351_ADDRESS 0x62
#define RESET_PIN 9

#define SRATE 192000
#define SINE_FREQ_MIN 20
#define SINE_FREQ_MAX 80000

// Set up codec and DMA channel
DMAMEM __attribute__((aligned(32))) static int16_t i2s_tx_buffer[2] = {0};
static DMAChannel dma;

uint32_t SF = SRATE;
double waveFreq = SINE_FREQ_MIN;
double waveTime = 0;
boolean activeInterrupt = true;
int16_t sineVal = 0;
const double twoPi = 6.28318530717958;
boolean isRunning = false;

void setup() {
  startDMA();
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  Wire.begin();
  setup_SI5351();
  setSamplingRate(SRATE);
  setup_PCM1796();
}

void loop() {
  if (isRunning) {
    waveFreq++;
    if (waveFreq == SINE_FREQ_MAX) {
      waveFreq = SINE_FREQ_MIN;
      isRunning = false;
    }
  }
  if (Serial.available() > 0) { 
    byte inByte = Serial.read();
    isRunning = true;
  }
  delayMicroseconds(100);
}

void i2c_write(byte i2cAddress, byte address, byte val) {
  Wire.beginTransmission(i2cAddress);  
  Wire.write(address);
  Wire.write(val);
  Wire.endTransmission();
}

void config_i2s() {
  CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

  // if either transmitter or receiver is enabled, do nothing
  if (I2S1_TCSR & I2S_TCSR_TE) return;
  if (I2S1_RCSR & I2S_RCSR_RE) return;

  // not using MCLK in slave mode - hope that's ok?
  //CORE_PIN23_CONFIG = 3;  // AD_B1_09  ALT3=SAI1_MCLK
  CORE_PIN21_CONFIG = 3;  // AD_B1_11  ALT3=SAI1_RX_BCLK
  CORE_PIN20_CONFIG = 3;  // AD_B1_10  ALT3=SAI1_RX_SYNC
  IOMUXC_SAI1_RX_BCLK_SELECT_INPUT = 1; // 1=GPIO_AD_B1_11_ALT3, page 868
  IOMUXC_SAI1_RX_SYNC_SELECT_INPUT = 1; // 1=GPIO_AD_B1_10_ALT3, page 872

  // configure transmitter
  I2S1_TMR = 0;
  I2S1_TCR1 = I2S_TCR1_RFW(1);  // watermark at half fifo size
  I2S1_TCR2 = I2S_TCR2_SYNC(1) | I2S_TCR2_BCP;
  I2S1_TCR3 = I2S_TCR3_TCE;
  I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF
    | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_RCR4_FSD;
  I2S1_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

  // configure receiver
  I2S1_RMR = 0;
  I2S1_RCR1 = I2S_RCR1_RFW(1);
  I2S1_RCR2 = I2S_RCR2_SYNC(0) | I2S_TCR2_BCP;
  I2S1_RCR3 = I2S_RCR3_RCE;
  I2S1_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF
    | I2S_RCR4_FSE | I2S_RCR4_FSP;
  I2S1_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);
}

void startDMA() {
  dma.begin(true);
  config_i2s();
  CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0
  dma.TCD->SADDR = i2s_tx_buffer;
  dma.TCD->SOFF = 2;
  dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
  dma.TCD->NBYTES_MLNO = 2;
  dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
  dma.TCD->DOFF = 0;
  dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
  dma.TCD->DLASTSGA = 0;
  dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
  dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0 + 2);
  dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);
  dma.enable();

  I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
  I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

  dma.attachInterrupt(isr);
}

void isr(void) // DMA interrupt, called twice per sample. Audio data is pushed into the DMA channel source array on every second call.
{
  uint32_t saddr = (uint32_t)(dma.TCD->SADDR);
  if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
    if (isRunning) {
      sineVal = round(sin(waveTime)*32767);
      waveTime += twoPi/((double)SF/waveFreq);
      i2s_tx_buffer[0] = sineVal; // Left Channel
      i2s_tx_buffer[1] = sineVal; // Right Channel
    } else {
      i2s_tx_buffer[0] = 0;
      i2s_tx_buffer[1] = 0;
    }
  } else {
    
  }
  dma.clearInterrupt(); // Putting this line where it is in Audio library (after saddr = (dma.TCD...)) makes samples clocked out half speed
  arm_dcache_flush_delete(i2s_tx_buffer, sizeof(i2s_tx_buffer));
}

void setup_SI5351() {
  byte REGs[] = {0x02,0x03,0x07,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x1C,0x1D,0x1F,0x2A,0x2C,0x2F,0x30,0x31,0x32,0x34,0x37,0x38,0x39,0x3A,0x3B,0x3E,0x3F,0x40,0x41,0x5A,0x5B,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0xA2,0xA3,0xA4,0xB7,0xB1,0x1A,0x1B,0x1E,0x20,0x21,0x2B,0x2D,0x2E,0x33,0x35,0x36,0x3C,0x3D,0xB1};
  byte Vals[] = {0x53,0x00,0x20,0x00,0x0D,0x1D,0x0D,0x8C,0x8C,0x8C,0x8C,0x8C,0x2A,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x92,0xAC,0x0C,0x35,0xF0,0x09,0x50,0x02,0x10,0x40,0x01,0x22,0x80,0x22,0x46,0xAC};

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
  i2c_write(PCM1796_ADDRESS, 16, B11110101); // -5 DB
  i2c_write(PCM1796_ADDRESS, 17, B11110101); 
  i2c_write(PCM1796_ADDRESS, 18, B11000000); 
  i2c_write(PCM1796_ADDRESS, 19, B00000000); 
}

void setSamplingRate(uint32_t samplingRate) {
  byte REGs[] = {0x1A,0x1B,0x1E,0x20,0x21,0x2B,0x2D,0x2E,0x33,0x35,0x36,0x3C,0x3D,0xB1};
  #if SRATE == 44100
      byte Vals[] = {0x3D,0x09,0xF3,0x13,0x75,0x04,0x11,0xE0,0x01,0x9D,0x00,0x42,0x7A,0xAC};
  #elif SRATE == 48000
      byte Vals[] = {0x0C,0x35,0xF0,0x09,0x50,0x02,0x10,0x40,0x01,0x90,0x00,0x42,0x46,0xAC};
  #elif SRATE == 96000
      byte Vals[] = {0x0C,0x35,0xF0,0x09,0x50,0x02,0x10,0x40,0x01,0x47,0x00,0x32,0x46,0xAC};
  #elif SRATE == 192000
      byte Vals[] = {0x0C,0x35,0xF0,0x09,0x50,0x02,0x10,0x40,0x01,0x22,0x80,0x22,0x46,0xAC};
  #endif
  for (int i = 0; i < sizeof(REGs); i++) {
    i2c_write(SI5351_ADDRESS, REGs[i], Vals[i]);
  }
}
