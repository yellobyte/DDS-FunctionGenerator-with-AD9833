# DDS Function Generator with AD9833 module #

While doing some work on Audio Amplifiers and Light Organs in 2016 I needed a simple (and not neccessarily absolutely precise) **Function Generator** to generate sinus signals in the range of 10Hz...30kHz and about 0.7...1Vrms. All devices I needed to test had input impedances in the range of ~2kOhm...20kOhm. Unfortunately I didn't call a function generator my own. So in order to have some additional fun, putting together one by myself instead of buying one on the internet was the only option. 

I ended up with a device able to generate **0.01 to 6.00Vpp sinus/triangle signals up to 500kHz and 5V TTL signals up to 5MHz**. Switching waveform, output level and frequency is done with 2 front panel knobs: a pushbutton switch labelled "Select" and a simple rotary encoder with push switch labelled "Modify". 
  
The display is a standard 16x2 LCD being controlled via I2C with a cheap China I2C-LCD module.

I put all components into an enclosure UM 52011-L from [Bopla](https://www.bopla.de/en/enclosure-technology/product/ultramas/enclosure-with-air-vents/um-52011-l.html) which had exactly the size I was looking for.
  
### Front side with 2 controls, 16x2 LCD display & BNC socket: ###
  
![github](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/FrontDisplay-Vrms.jpg)
  
The two controls allow to change signal [waveform](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingWaveform.mp4), output [level](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingLevel+SwitchingBetweenVppVrms.mp4) (Vpp resp. Vrms for sinus/triangle) and [frequency](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingFrequency.mp4) very quickly. Have a look at folder [**Doc**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/Doc) to find more details. 
  
## Some technical infos ##

The analog part of the circuitry is shielded in tin plate (an old coffee tin proved ideal for this purpose, you will recognize my favorite Italian coffee brand) to keep the output signal as clear as possible and reduce EMI. 
  
The settings for waveform/level/frequency get stored in EEPROM of the Atmega168A and therefore stay permanent even after switching the device off/on.

By default, turning the encoder knob will change the output signal immediately. For example, changing the output level from 0.5Vpp up to 6.00Vpp requires a few full turns of the knob and therefore will take a few seconds. Means the output level will rise steadily.
  
However, sometimes, when testing the automatic gain control (AGC) of audio input stages for example, it requires a signal that rises instantaneously . No problem! Switch off the device and have the encoder knob pressed for two seconds while switching power back on. After that, changing a single setting (waveform, level or frequency) on the display will not affect the output signal. Only after pressing the knob shortly (<0.5s) will the new setting appear at the output.
    
![github](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/OpenCase.jpg)
  
### Schematic ###
    
The DDS module (available for only a few dollars on Ebay/Ali/etc.) uses a sophisticated programmable waveform generator AD9833 with a 0...12.5MHz output frequency rate. The AD9833 is written to via SPI interface. The internet provides ready to use libraries for it but I decided to use my own small quick and dirty code for I use only two features: setting frequency & waveform.
  
The output signal of the AD9833 is unipolar and has a constant amplitude  of +38mV...+650mV over the full frequency range. A level shifter is needed to get a bipolar, symmetrical signal. Opamp IC3 serves the purpose. Opamp IC5 is further used to amplify this signal to +/- 3.0V (6Vpp). The combination DAC1/IC6/IC7 is then used to attenuate the signal to the desired output level.

Unfortunately the output of the AD9833 chip on the DDS module is galvanically isolated from the output pin (due to C8/C9) which is not what we need. The quickest remedy is to bridge one of the two capacitors with a short wire, as you can see [**here**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/Board_V1.0_top.jpg) on the board or [**here**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/AD9833-Modul-with-Modification.jpg) in the schematic.
  
The DAC AD5452 contains a 12-bit R-2R-ladder and simply sets the amplification factor (better to say attenuation factor) of the combination IC6 (Opamp) + IC7 (unity gain buffer). To set the output level the selected value (0.01...6.00Vpp resp. its equivalent in Vrms) gets translated into a 12-bit value (range 0...4095) and then written into the DAC via SPI bus.
  
The LMH6321 acts as output driver/buffer with gain=1 (unity gain) and provides an  "adjustable current limit". Resistor R16 sets this limit to roughly 110mA for thermal protection in case of a short at the output.
  
Capacitors C20/C22/C38 are not populated because the Opamp NE5534 showed sufficient results without them. In case you want to increase the maximum analog frequency and try different Opamps for best results you might need them.

The two resistors R9/R14 (100R||100R) at the driver output were designed into the schematic and soldered on the board but in the end were shorted with wire. The final device doesn't really generate high frequency analog signals and is mostly connected to inputs of audio devices with a few 10kOhm input impedance. Having those two resistors in parallel and feeding devices with low input impedance (e.g. 50Ohm) attached via a 50 Ohm transmission line like coax cable would halve the usable signal amplitude (!) and subsequently force you to correct the displayed signal level on LCD, etc. It's not a scenario the device was designed for anyway. No, the wire keeps it simple...

The rotary encoders two outer pins go to IMPULS-A/IMPULS-B and the middle one to ground GND. It's two switch pins go to IMPULS-SW and GND. The two pins of the separate "Select" switch go to SELECT-SW and GND. The four pins of socket "LCD 16x2" go to the matching pins on the small I2C-LCD board that is mounted piggyback on the 16x2 LCD front display.
  
For properly adjusting the analog part an oscilloscope connected to output (BNC socket) is recommended. Trimmer R11 sets the maximum amplitude and trimmer R2 adjusts the offset so that you get an exact symmetrical signal.
Set the output on the LCD display to 1kHz, 6.00Vpp and adjust the signal as shown [here](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/AdjustingOutputLevel.jpg). Info: Cmean is 0V for a completely symmetrical sinus signal, but this is hard to achive.
  
The device is powered by a standard 230V(prim)/2x6V(sec)/12VA transformer attached to CON1. 
  
The firmware for the device was done with VSCode/PlatformIO and is located in folder [**Software**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/Software) together with all needed fuse settings. For programming the Atmega168A I used this in-circuit [**Programmer**](https://github.com/yellobyte/USB-Atmel-In-Circuit-Programmer), connected to the 10-pin ISP socket.
  
![github](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/EagleFiles/Schematic_V1.1.jpg)
  
## Eagle Schematic and PCB Files ##

All relevant Eagle files (schematic, board and Gerber production files Rev 1.1) are located in folder [**EagleFiles**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/EagleFiles). The necessary Gerber files for production are zipped into file "FG with AD9833.zip".  

The pictures in folder [**Doc**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/Doc) still show board revision 1.0.   Revision 1.1 is almost identical and contains only cosmetic changes.
   
