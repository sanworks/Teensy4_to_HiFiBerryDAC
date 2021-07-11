Notes about the Teensy4 HiFiBerry Breakout board

Version 1.1 STRONGLY recommended!

There are two versions of the board:
1. rev 1.X-T mounts to the HiFiBerry DAC's male GPIO header from above, 
using adafruit part# 1979: https://www.adafruit.com/product/1979


2. rev 1.X-B mounts to the HiFiBerry DAC's female GPIO header from below, 
using adafruit part# 3662: https://www.adafruit.com/product/3662

Other components are:
C1: 0.01uf ceramic cap, 0805 size, Yageo MPN# CC0805KRX7R9BB103 or equivalent 
C2: 0.1uf ceramic cap, 0805 size,  Taio Yuden MPN# UMK212B7104KG-T or equivalent   
C3: 4.7uf ceramic cap, 1206 size, Kemet MPN# C1206C475K5RACTU or equivalent
FB1: Ferrite bead, 0603 size, Taio Yuden MPN# FBTH1608HL300-T or equivalent 

Ordering:
The boards can be ordered from OSHPark, here:
rev 1.X-T: https://oshpark.com/shared_projects/HEg1rLr4
rev 1.X-B: https://oshpark.com/shared_projects/9LjQGttw

Notes: Both boards can be used with either Teensy 4.0 or 4.1.
       In rev 1.X-T, a tall pin header is used to clear the RCA jacks, increasing total height
       Three-pin header holes are available for mechanical support at the end of T4.1 (optional)
       In rev 1.X-B, HiFiBerry will block the top of the Teensy board (reset button, etc). 


      