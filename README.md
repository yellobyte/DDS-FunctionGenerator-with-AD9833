# DDS Function Generator with AD9833 module #

While doing some work on Audio Amplifiers and Light Organs in 2016 I needed a simple (and not neccessarily absolutely precise) **Function Generator** to generate sinus signals in the frequency range of 10Hz...30kHz and voltage levels of about 0.7...1Vrms. All devices I needed to test had input impedances in the range of ~2kOhm...20kOhm. 
  
Unfortunately I didn't call a function generator my own. So in order to have some additional fun, putting together one by myself instead of buying one on the internet was the only option. I ended up with a device able to generate output signals with the following specifications:

 - **Sinus/Triangle, amplitude 0.01 to 6.00 Vpp, frequency 1Hz to 0.5MHz**
 - **TTL, amplitude 5 Vpp, frequency 1Hz to 5.0 MHz**
 
 Switching waveform, output level and frequency is done with 2 front panel knobs: a pushbutton switch labelled "Select" and a simple rotary encoder with push switch labelled "Modify". The display is a standard 16x2 LCD (HD44780) having a small I2C-LCD adapter module (PCF8574 I/O expander) mounted piggyback.

All components were put into an enclosure UM 52011-L from [Bopla](https://www.bopla.de/en/enclosure-technology/product/ultramas/enclosure-with-air-vents/um-52011-l.html) which had exactly the size needed.
  
### Front side with 2 controls, 16x2 LCD display and BNC output socket: ###
  
![github](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/FrontDisplay-Vrms.jpg)
  
The two controls allow to change signal [waveform](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingWaveform.mp4), output [level](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingLevel+SwitchingBetweenVppVrms.mp4) (Vpp resp. Vrms for sinus/triangle) and [frequency](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingFrequency.mp4) very quickly. Have a look at folder [**Doc**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/Doc) to find more details. 
  
## General technical infos ##

The analog part of the circuitry is shielded in tin plate (an old coffee tin proved ideal for this purpose, you will recognize my favorite Italian coffee brand) to keep the output signal as clear as possible and reduce EMI. 
  
The settings for waveform/level/frequency get stored in EEPROM of the Atmega168A and therefore stay permanent even after switching the device off/on.

By default, turning the encoder knob will change the output signal immediately. For example, changing the output level from 0.5Vpp up to 6.00Vpp requires a few full turns of the knob and therefore will take a few seconds. Means the output level will rise steadily.
  
However, sometimes, when testing the automatic gain control (AGC) of audio input stages for example, it requires a signal that rises instantaneously . No problem! Switch off the device and have the encoder knob pressed for two seconds while switching power back on. After that, changing a single setting (waveform, level or frequency) on the display will not affect the output signal. Only after pressing the knob shortly (<0.5s) will the new setting appear at the output.
    
![github](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/OpenCase.jpg)
  
### Infos to the Schematic Diagram ###
    
The DDS module (available for only a few dollars on Ebay/Ali/etc.) uses a sophisticated programmable waveform generator AD9833 with a 0...12.5MHz output frequency rate. 
  
Unfortunately the output pin of the AD9833 chip on the DDS module shipped from Asia was galvanically isolated from the modules output pin (due to C8/C9). The quickest remedy was to bridge one of the two capacitors with a short wire, as you can see on the [**board**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/Board_V1.0_top.jpg) or in the modules [**schematic**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/AD9833-Modul-with-Modification.jpg). Apart from that the module was perfect for the task.
  
The AD9833 is written to via SPI interface. The internet provides ready to use libraries for it but I decided to use some small, quick and dirty code for I only use two features: setting frequency & waveform.
  
The output signal of the AD9833 is unipolar and has a constant amplitude  of +38mV...+650mV over the full frequency range. The combination of Opamps IC3 + IC5 converts it to a bipolar, symmetrical signal with constant amplitude +/- 3.0V (6Vpp). The combination DAC1/IC6/IC7 is then used to attenuate this signal to the desired output level.

The DAC AD5452 contains a 12-bit R-2R-ladder and simply sets the amplification factor (better to say attenuation factor) of the combination IC6 (Opamp) + IC7 (unity gain buffer). To set the output level the selected value (0.01...6.00Vpp resp. its equivalent in Vrms) gets translated into a 12-bit value (range 0...4095) and then written into the DAC via SPI bus.
  
The LMH6321 acts as output driver/buffer with gain=1 (unity gain) and provides an  "adjustable current limit". Resistor R16 sets this limit to roughly 110mA for thermal protection in case of a short at the output.
  
Capacitors C20/C22/C38 are not populated because the Opamp NE5534 showed sufficient results without them. In case you want to increase the maximum analog output frequency and try different Opamps for best results you might need them.

The two 100Ohm resistors R9/R14 in parallel at the IC7 driver output were designed into the schematic and soldered on the board but shorted with wire in the end. The device doesn't generate high frequency analog signals and was designed to only be used in an audio environment anyway, means it will mostly be connected to devices with a few 1..10kOhm input impedance. It will probably never see low impedance loads, transmission lines, reflections and other effects of RF hell. BTW: Having those two resistors in parallel and feeding devices with low input impedance (e.g. 50Ohm) would halve the usable signal amplitude (!) and subsequently force you to correct the displayed signal level on LCD, etc. Things would get complicated. 
  
Wiring the IC7 driver output directly to CON2 (connected to BNC output socket) gives the function generator a very low output impedance and is appropriate under these circumstances. 

The rotary encoders two outer pins go to IMPULS-A/IMPULS-B and the middle one to ground GND. It's two switch pins go to IMPULS-SW and GND. The two pins of the separate "Select" switch go to SELECT-SW and GND. The four pins of socket "LCD 16x2" go to the matching pins on the small [**I2C-LCD board**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/I2C-LCD-module.jpg) that is mounted piggyback on the 16x2 LCD front display.
  
For properly adjusting the analog part an oscilloscope connected to output (BNC socket) is recommended. Trimmer R11 sets the maximum amplitude and trimmer R2 adjusts the offset for getting a symmetrical signal.
  
Set the values on the LCD display to Sinus, 1.000kHz, 6.00Vpp and adjust the output signal as shown [**here**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/AdjustingOutputLevel.jpg). If your osci can display Cmean, have an eye on it. Try to get it to +/-0V as close as possible for a nice symmetrical output signal.

The logic gate IC8D is a low-power-Schottky Schmitt-Trigger-Inverter (74LS14) and acts as buffer between DDS chip and BNC output socket when a TTL output signal is selected. Its TTL signal is slightly better than the one generated by AD9833. Furthermore, a short at the output will destroy this socketed, easy replacable chip and not the tiny DDS chip (MSOP package), soldered on the DDS module.
  
The device is powered by a standard 230V(prim)/2x6V(sec)/12VA transformer attached to CON1. 
  
The firmware for the device was done with VSCode/PlatformIO and is located in folder [**Software**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/Software) together with all needed fuse settings. For programming the Atmega168A I used this in-circuit [**Programmer**](https://github.com/yellobyte/USB-Atmel-In-Circuit-Programmer), connected to the 10-pin ISP socket.
  
![github](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/EagleFiles/Schematic_V1.1.jpg)
  
## Eagle Schematic and PCB Files ##

All relevant Eagle files (schematic, board and Gerber production files Rev 1.1) are located in folder [**EagleFiles**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/EagleFiles). The necessary Gerber files for production are zipped into file "FG with AD9833.zip".  

The pictures in folder [**Doc**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/Doc) still show board revision 1.0.   Revision 1.1 is almost identical and contains only cosmetic changes.
   
