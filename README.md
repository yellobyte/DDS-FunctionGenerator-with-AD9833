# DDS Function Generator with AD9833 module #

Doing some work on Audio Amplifiers and Light Organs in 2016 I needed a simple function generator to generate sinus signals in the range of 10Hz...30kHz but didn't call one my own. So in order to have some fun, putting together a simple one was the only option.

I ended up with a device able to generate sinus/triangle signals up to 500kHz and 5V TTL signals up to 5MHz. Switching waveform, output level and frequency is done with 2 front panel knobs: a pushbutton switch and a simple rotary encoder with push switch. 
  
The display is a 16x2 LCD which is controlled via I2C with a cheap China I2C-LCD module.

I put all components into an enclosure UM 52011-L from [Bopla](https://www.bopla.de/en/enclosure-technology/product/ultramas/enclosure-with-air-vents/um-52011-l.html) which had exactly the size I was looking for.

### Front side with 2 controls, 16x2 LCD display & BNC socket: ###
  
![github](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/FrontDisplay-Vrms.jpg)
  
The two controls allow to change signal [waveform](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingWaveform.mp4), output [level](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingLevel+SwitchingBetweenVppVrms.mp4) (for sinus/Triangle) and [frequency](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/SettingFrequency.mp4) very quickly. Have a look at folder [**Doc**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/Doc) to find more details. 
  
## Some technical infos ##

........
  
![github](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/raw/main/Doc/OpenCase.jpg)
  


## PCB Files ##

You find the Eagle files (schematic, board and Gerber production files Rev 1.1) in folder [**EagleFiles**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/EagleFiles). I zipped all necessary Gerber files into file "FG with AD9833.zip".  

The pictures in folder [**Doc**](https://github.com/yellobyte/DDS-FunctionGenerator-with-AD9833/blob/main/Doc) still show board revision 1.0.  Revision 1.1 is almost identical and contains only cosmetic changes.
   
