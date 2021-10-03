/*
 * EXTERNAL.CPP: definitions and functions for the AD9833 function generator
 *
 * Created: June 2016
 *          2021/10/01 possibility to set output level in Vrms added
 * Author : ThJ (yellobyte@bluewin.ch)
*/

#include <Arduino.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include "ad9833.h"
#include "external.h"

#define PIN_RELAY         PIN_PB0
#define PIN_BUZZER        PIN_PC0
#define PIN_ENCODERA      PIN_PD2
#define PIN_ENCODERB      PIN_PD3
#define PIN_ENCODERBUTTON PIN_PD4
#define PIN_PUSHBUTTON    PIN_PD7
#define PIN_AD5452_CS     PIN_PB1
#define SPI_AD5452        SPISettings(2000000, MSBFIRST, SPI_MODE2)

extern LiquidCrystal_I2C  lcd;
extern uint8_t            outputLevelStepSize;

volatile uint8_t rotaryImpulseLeft = 0;
volatile uint8_t rotaryImpulseRight = 0;

//
// functions for displaying frequency, waveform and output level on 16x2 LCD display
//
void EXTDisplayFrequency(uint32_t frequ, uint8_t leadingZeros)
{
  uint8_t  i;
  uint8_t	 digit[7];
  uint32_t temp;

  temp = (frequ < 10000000) ? frequ : 9999999;
  for (i = 0; i < 7; i++) digit[i] = 0; // clear array
  for (i = 0; temp > 0; i++) {
    digit[6-i] = (uint8_t)(temp % 10);
    temp = temp / 10;
  }
  lcd.setCursor(4,0);
  for (i = 0; i < 7; i++) {
    if (i == 0) {
      if ( digit[0] != 0 ) {
        lcd.print((char)(48+digit[0]));
        lcd.print((char)'.'); 
      }
      else {
         lcd.print((char)' ');
         lcd.print((char)' ');
      }
    }
    else if ( i == 1 && digit[0] == 0 && digit[1] == 0 && !(leadingZeros == 2) ) {
       lcd.print((char)' ');
    }
    else if ( i == 2 && digit[0] == 0 && digit[1] == 0 && digit[2] == 0 && leadingZeros == 0 ) {
       lcd.print((char)' ');
    }
    else if (i == 3) {
      lcd.print((char)(48+digit[i]));
      lcd.print((char)'.'); 
    }
    else {
      lcd.print((char)(48+digit[i])); 
    }
  }
  lcd.print((char)'k');
  lcd.print((char)'H');
  lcd.print((char)'z');
}

void EXTDisplayLevel(uint16_t outputLevel, uint8_t outputLevelMode)
{
  uint8_t  i, digits[3];
  uint16_t temp = outputLevel;

  for (i=0; i<3; i++) digits[i] = 0;
  digits[2] = (uint8_t)(temp % 10);
  temp /= 10;
  digits[1] = (uint8_t)(temp % 10);
  temp /= 10;
  digits[0] = (uint8_t)(temp % 10);
  lcd.setCursor(9,1);
  lcd.print((char)(48+digits[0]));
  lcd.print((char)'.');
  lcd.print((char)(48+digits[1]));
  lcd.print((char)(48+digits[2]));

  lcd.setCursor(13,1);
  lcd.print(outputLevelMode ? "Vpp" : "Vrm");
}

void EXTDisplayWaveform(uint8_t waveform)
{
  switch (waveform){
    case SINUS:
      lcd.setCursor(0,1);
      lcd.print("SINUS   "); 
      break;
    case SQUARE:
      lcd.setCursor(0,1);
      lcd.print("SQUARE    5V-TTL"); 
      break;
    case TRIANGLE:
      lcd.setCursor(0,1);
      lcd.print("TRIANGLE");
      break;
    default:
      break;
  }	
}

//
// functions for handling the relais
//
void EXTRelaisInit(void)
{
  pinMode(PIN_RELAY, OUTPUT);   
  digitalWrite(PIN_RELAY, LOW);     // relais off
}

void EXTRelaisOnOff(uint8_t setting)
{
  if ( setting == RELAIS_ON )
    digitalWrite(PIN_RELAY, HIGH);  // relais on
  else
    digitalWrite(PIN_RELAY, LOW);   // relais off
}

//
// functions for handling the buzzer
//
void EXTBuzzerInit(void)
{
  pinMode(PIN_BUZZER, OUTPUT);   
  digitalWrite(PIN_BUZZER, LOW);   // buzzer off
}

void EXTBuzzerRing(uint16_t ms)
{
  digitalWrite(PIN_BUZZER, HIGH);  // buzzer on
  delay(ms);					             // Buzzer x ms aktiv
  digitalWrite(PIN_BUZZER, LOW);   // buzzer off
}

//
// functions for handling the select button
//
void EXTSelectSwitchInit(void)
{
  pinMode(PIN_PUSHBUTTON, INPUT_PULLUP);   
}

uint8_t EXTSelectSwitchCheck(void)
{
  // function returns: 1...button was pressed, 0...otherwise
  static uint8_t oldStatus = 1;                         // default - switch not pressed
  uint8_t	status, ret = 0;
 
  status = digitalRead(PIN_PUSHBUTTON);
  if (!status && oldStatus) {                           // any input change ?
    for (uint8_t n = 0; n < 5; ) {
      delayMicroseconds(200);
      n = !digitalRead(PIN_PUSHBUTTON) ? n + 1 : 0;
    }
    // since 5 * 200us = 1ms no change at input
    ret = 1;
    oldStatus = status;                                 // remember last stable signal
  }
  else {
    oldStatus = status;                                 // remember last reading
  }  
  return(ret);
}

//
// functions handling the rotary encoder & button
//
static void impulsIrq()
{
  // PIN_ENCODERB==1 -> left turn,  PIN_ENCODERB==0 -> right turn
  if (digitalRead(PIN_ENCODERB)) {
    rotaryImpulseLeft++;
  }
  else {
    rotaryImpulseRight++;
  }
  // trailing spikes will be ignored for some time
  delayMicroseconds(50);
}

void EXTRotaryInit(void)
{
  pinMode(PIN_ENCODERBUTTON, INPUT_PULLUP);
  pinMode(PIN_ENCODERB, INPUT_PULLUP);
  pinMode(PIN_ENCODERA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODERA),impulsIrq,FALLING);
}

buttonEvent EXTRotaryButtonCheck(void)
{
  // function returns: 1...short press (<0.5s), 2...long press (>0.5s-3s), 0...otherwise
  static uint8_t oldStatus = 1;                             // default - button not pressed
  uint8_t	status;
  buttonEvent event = idle;
 
  status = digitalRead(PIN_ENCODERBUTTON);
  if (!status && oldStatus) {                               // falling edge ?
    for (uint8_t n = 0; n < 5; ) {                          // wait for stable low
      delayMicroseconds(200);
      n = !digitalRead(PIN_ENCODERBUTTON) ? n + 1 : 0;
    }
    for (uint16_t n = 1; status == LOW && n < 50; ) {       // wait for rising edge
      delay(100);
      if (!(status = digitalRead(PIN_ENCODERBUTTON))) n++;  // input still low -> knob still pressed
      else event = (n < 5) ? shortPress : longPress;        // input high again -> knob was released
    }
    oldStatus = status;                                     // remember last stable signal
  }
  else {
    oldStatus = status;                                     // remember last reading
  }
  return(event);
}

int8_t EXTRotaryImpulseCheck(void)
{
  // function returns: -1...knob was turned left, 1...knob was turned right, 0...knob was idle
  uint8_t ret = 0;
  if (rotaryImpulseLeft) {
    ret = -1;
    rotaryImpulseLeft--;
  }
  else if (rotaryImpulseRight) {
    ret = 1;
    rotaryImpulseRight--;
  }
  return(ret);
}

//
// functions handling the DAC AD5452 for setting output voltage level when SINUS or TRIANGLE
// (the DAC R-2R ladder is used to set the amplification ratio of OpAmp IC6, the output level range
// is +/- 3V (6Vpp) at maximum, adjusted with R11 at LSP2)
//
void EXTDacInit(void)
{
  SPI.begin();
}

void EXTDacSetLevel(uint16_t outputLevel, uint8_t outputWaveform, uint8_t outputLevelMode)
{
  uint16_t temp1, regist;
  double temp2;

  if (outputLevelMode) {
    // output level already represents Vpp
    temp1 = outputLevel;
  }
  else {
    // conversion Vrms to Vpp required (adding 0.5 for minimizing conversion error)
    temp1 = (uint16_t)(((double)outputLevel * 2 * (double)((outputWaveform == SINUS) ? SQRT2 : SQRT3)) + 0.5);
    // TEST TEST
    if (temp1 > 600) EXTBuzzerRing(1000);
    if (temp1 > 601) {delay(200); EXTBuzzerRing(1000);}
    if (temp1 > 602) {delay(200); EXTBuzzerRing(1000);}
    //
  }
  // final check 
  temp1 = (OUTPUT_LEVEL_VPP_MAX < temp1) ? OUTPUT_LEVEL_VPP_MAX : temp1;

  // at this point temp1 always represents the Vpp output level which must be converted
  // into a value between 0 and 4095 (DAC R-2R ladder setting)
  temp2 = ((double)temp1/(double)OUTPUT_LEVEL_VPP_MAX);
  regist = (uint16_t)(temp2 * (double)0xFFF);
  regist = regist << 2;
  regist &= 0x3FFF;                       // clearing control bits C1/C0 in AD5452

  // loading a word to the DAC (D15/D14...control bits, D13-D2...data bits, D1/D0...not used))
  SPI.beginTransaction(SPI_AD5452);
  digitalWrite(PIN_AD5452_CS, LOW);       // set select signal LOW for DAC AD5452
  SPI.transfer((uint8_t)(regist >> 8));
  SPI.transfer((uint8_t)(regist & 255));
  digitalWrite(PIN_AD5452_CS, HIGH);      // set select signal HIGH for DAC AD5452
  SPI.endTransaction();  
}
