/******************************************************************************

  Function generator based on a China AD9833 DDS module, controlled by an Atmega168
  
  Info:
    If the encoder button is pressed during power on then you always have to press 
    this button to activate new values. A short beep will confirm that. 
    The displayed output level represents Vpp or Vrms, this is changeable with
    the encoder button by a long press (>0.5s in output level changing mode).

  Created: August 2016
  Changes: 2020/09/05 usage of lib LiquidCrystal_I2C instead of own quick and dirty code
           2021/10/01 possibility to set output level in Vrms added
           2021/10/02 watchdog added
  Author: ThJ <yellobyte@bluewin.ch>

******************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h> 
#include "ad9833.h"
#include "external.h"

#define USE_WDT           // uncomment for using watchdog
//#define USE_SERIAL        // uncomment for using serial output

// some configurable definitions
#define MAX_IDLE_TIME	30		  // 30 sec
#define MAX_FREQ		  500000	// 500kHz for sinus/triangle
#define MAX_FREQ_TTL	5000000 // 5.0MHz for TTL

#define OUTPUT_WAVEFORM_DEFAULT   SINUS
#define OUTPUT_LEVEL_MODE_DEFAULT V_P2P   // Vpp
#define OUTPUT_LEVEL_DEFAULT      200     // 2.0Vpp
#define OUTPUT_FREQU_DEFAULT      1000    // 1kHz

#define V_STEPSIZE_SMALL 1    // useful values are only 1,2 or 5

// fix definitions - don't change
#define I_NORMAL		  0		  // immediate activation of new values without pushing the turn-push-button
#define I_EXPLICIT		1		  // activation of new values requires pushing the turn-push-button
#define V_STEPSIZE_10 10

// state machine states
#define	M_IDLE			  0
#define M_WAVEFORM		1
#define M_LEVEL			  2
#define M_FREQUENCY1	3
#define M_FREQUENCY2	4

//
// global variables (set to default if required)
//
uint16_t outputLevelMaxVrms = (OUTPUT_WAVEFORM_DEFAULT == SINUS) ? V_RMS_MAX_SIN : V_RMS_MAX_TRI;
uint16_t outputLevelMinVrms = (OUTPUT_WAVEFORM_DEFAULT == SINUS) ? V_RMS_MIN_SIN : V_RMS_MIN_TRI;
uint8_t	 outputWaveform = OUTPUT_WAVEFORM_DEFAULT;
uint16_t outputLevel = OUTPUT_LEVEL_DEFAULT;          // the chosen output level
uint8_t  outputLevelMode = OUTPUT_LEVEL_MODE_DEFAULT; // defines what above value represents (Vpp or Vrms)
uint32_t outputFrequency = OUTPUT_FREQU_DEFAULT;
uint8_t	 systemState	= M_IDLE;
uint8_t  tempWaveform = OUTPUT_WAVEFORM_DEFAULT;
uint16_t tempLevel = OUTPUT_LEVEL_DEFAULT;
uint32_t tempFrequency = OUTPUT_FREQU_DEFAULT;
uint32_t tempDigit = OUTPUT_FREQU_DEFAULT;
int8_t   ret = 0;
int32_t  temp = 0;
buttonEvent event;

extern volatile uint8_t rotaryImpulseLeft;
extern volatile uint8_t rotaryImpulseRight;

volatile uint8_t systemIdleTime = 0;
volatile uint8_t timer2 = 0;

uint8_t outputLevelStepSize = V_STEPSIZE_SMALL;
uint8_t inputMode = I_NORMAL;

// for remembering settings after power off
uint8_t	 EEMEM eWaveform;
uint16_t EEMEM eLevel;
uint8_t  EEMEM eLevelMode;
uint32_t EEMEM eFrequency;

LiquidCrystal_I2C lcd(0x27,16,2); // set the LCD I2C address, 16 cols, 2 rows

#ifdef USE_WDT
//ISR (WDT_vect) {}               // could be used instead of a default WDT reset (power reset)
#endif

//
// macros & ISR function for hardware timer 2 of Atmega328
// (8-bit Timer 2 with base clock 15625Hz (F_CPU/1024): overflow every 1/61s=~16ms)
//
#define Timer2Init() {\
  TCCR2B |= (1<<CS22) | (1<<CS21) |(1<<CS20);\
}

// enable timer 2 overflow interrupt (start timer 2)
#define Timer2Start() {\
  timer2 = 0;\
  systemIdleTime = 0;\
  TIMSK2 |= (1<<TOIE2);\
}

// disable timer 2 interrupt (stop timer 2)
#define Timer2Stop() {\
  TIMSK2 &= (~(1<<TOIE2));\
}

// re-start timer 2
#define Timer2Clear() {\
  timer2 = 0;\
  systemIdleTime = 0;\
}

// timer 2 overflow interrupt routine
ISR(TIMER2_OVF_vect){
  if (timer2++ > 60) {					  // ca. 1 sec over ?
    timer2 = 0;
    systemIdleTime++;
  }
}

//
// some function definitions
//
static void SPIInit(void)
{
  digitalWrite(PIN_PB1, HIGH);    // chip select signal for DAC AD5452 set high
  pinMode(PIN_PB1, OUTPUT);       // chip select port for DAC AD5452 set to output
  SPI.begin();                    // SPI SS pin is CS signal for AD9833, will be high
}

static void setAndStoreOutputLevel()
{
  EXTDacSetLevel(outputLevel, outputWaveform, outputLevelMode);
  eeprom_busy_wait();
  eeprom_update_word(&eLevel,outputLevel);
}

static void setAndStoreOutputFrequency()
{
  DDSFreq(outputFrequency);
  eeprom_busy_wait();
  eeprom_update_dword(&eFrequency,outputFrequency);
}            

//
// runs once after power on
//
void setup() {
  // put your setup code here, to run once:
#ifdef USE_SERIAL
  Serial.begin(38400);
#endif

  EXTSelectSwitchInit();
  EXTRotaryInit();
  if (EXTRotaryButtonCheck()) {
    // encoder button was pressed during power on
    inputMode = I_EXPLICIT;
  }

  // read stored parameters from EEPROM and check validity
  eeprom_busy_wait();
  outputWaveform = eeprom_read_byte(&eWaveform);
  if (outputWaveform != SINUS && outputWaveform != TRIANGLE && outputWaveform != SQUARE) {
    outputWaveform = SINUS;
  }
  eeprom_busy_wait();
  outputFrequency = eeprom_read_dword(&eFrequency);
  if (outputFrequency < 1 || outputFrequency > ((outputWaveform == SQUARE) ? MAX_FREQ_TTL : MAX_FREQ)) {
    outputFrequency = OUTPUT_FREQU_DEFAULT;
  }
  eeprom_busy_wait();
  outputLevel = eeprom_read_word(&eLevel);
  if (/*outputLevel < 10 || */outputLevel > OUTPUT_LEVEL_VPP_MAX) {
    // set default output level
    outputLevel = OUTPUT_LEVEL_DEFAULT;
  }
  if (outputLevelStepSize == V_STEPSIZE_10) {		
    // fix level matching step size if necessary (e.g. 105 -> 100)
    outputLevel /= 10;
    outputLevel *= 10;
  }
  eeprom_busy_wait();
  outputLevelMode = eeprom_read_byte(&eLevelMode);
  if (outputLevelMode != V_RMS && outputLevelMode != V_P2P) {
    // set default output level mode
    outputLevelMode = OUTPUT_LEVEL_MODE_DEFAULT;
  }

  // Initialize LCD module, Cursor not visible
  lcd.init();
  lcd.backlight();

  // Power on message
  lcd.print("DDS Function Ge-");
  lcd.setCursor(0,1);                           // go to the next line
  lcd.print("nerator (AD9833)");
  delay(2500);

  // Display frequency range
  lcd.clear();
  lcd.print("Range:1Hz-0.5MHz");
  lcd.setCursor(0,1);
  lcd.print("TTL:  1Hz-5.0MHz");
  lcd.noCursor();
  delay(2500);

   // If button press needed to activate new values then give info
  if (inputMode == I_EXPLICIT) {
    lcd.clear();
    lcd.print("Pressing button ");
    lcd.setCursor(0,1);
    lcd.print("changes value!");
    lcd.noCursor();
    delay(2500);
  }

  // set display to default
  lcd.clear();
  lcd.noCursor();

  SPIInit();
  EXTRelaisInit();
  EXTBuzzerInit();
  EXTDacInit();
  DDSInit();
  Timer2Init();
  
  EXTDacSetLevel(outputLevel, outputWaveform, outputLevelMode);
  EXTDisplayFrequency(outputFrequency,0);
  EXTDisplayWaveform(outputWaveform);
  if (outputWaveform != SQUARE) {
    EXTDisplayLevel(outputLevel, outputLevelMode);
    EXTRelaisOnOff(RELAIS_OFF);
  }
  else {
    EXTRelaisOnOff(RELAIS_ON);
    delay(10);
  }

  DDSFreq(outputFrequency);
  //delay(1);
  DDSSignal(outputWaveform); 

#ifdef USE_WDT
  wdt_enable(WDTO_4S);				                  // Enable Watchdog (4 sek.)
  wdt_reset();
#endif
  //sei();								                      // enable global interrupts - done in framework
#ifdef USE_SERIAL
  Serial.println(F("Programmstart ok."));  
#endif
}

void loop() {
  // put your main code here, to run repeatedly:
#ifdef USE_WDT
  wdt_reset();					// reset watchdog
#endif  
  if (systemState == M_IDLE) {
    // we are idle and only check select switch
    if (EXTSelectSwitchCheck()) {
      systemState = M_WAVEFORM;
      rotaryImpulseLeft = rotaryImpulseRight = 0;
      tempWaveform = outputWaveform;
      lcd.setCursor(0,1);
      lcd.blink();
      Timer2Start();
    }
  }
  else if (systemState == M_WAVEFORM) {
    // selecting waveform
    if (systemIdleTime >= MAX_IDLE_TIME) {
      systemState = M_IDLE;
      Timer2Stop();
      lcd.noCursor();
      if (tempWaveform != outputWaveform) EXTDisplayWaveform(outputWaveform);	
      if (outputWaveform != SQUARE ) EXTDisplayLevel(outputLevel, outputLevelMode);
    }
    else if ((ret = EXTRotaryImpulseCheck()) != 0) {
      Timer2Clear();
      if (ret == -1) {
        switch (tempWaveform) {
          case SINUS:	tempWaveform = SQUARE; break;
          case SQUARE: tempWaveform = TRIANGLE; break;
          default: tempWaveform = SINUS; break;
        }
      }
      else {
        switch (tempWaveform) {
          case SINUS:	tempWaveform = TRIANGLE; break;
          case TRIANGLE: tempWaveform = SQUARE; break;
          default: tempWaveform = SINUS; break;
        }
      }
      if (inputMode == I_NORMAL) {
        goto JUMP1;
      }
      else {
        EXTDisplayWaveform(tempWaveform);
        if (tempWaveform != SQUARE) EXTDisplayLevel(outputLevel, outputLevelMode);
        lcd.setCursor(0,1);
      }
    }
    else if (EXTRotaryButtonCheck()) {
JUMP1:
      Timer2Clear();
      if (tempWaveform != outputWaveform) {
        outputWaveform = tempWaveform;
        EXTDisplayWaveform(outputWaveform);
        DDSSignal(outputWaveform); 
        if (tempWaveform != SQUARE) {
          // new selected waveform is sinus or triangle
          outputLevelMaxVrms = (outputWaveform == SINUS) ? V_RMS_MAX_SIN : V_RMS_MAX_TRI;
          outputLevelMinVrms = (outputWaveform == SINUS) ? V_RMS_MIN_SIN : V_RMS_MIN_TRI;
          if (outputLevelMode != V_P2P && outputLevelMaxVrms < outputLevel) {
            // max Vrms has been reduced (change from SINUS to TRIANGLE)
            outputLevel = outputLevelMaxVrms;
          }
          else if (outputLevelMode != V_P2P && outputLevel < outputLevelMinVrms) {
            outputLevel = outputLevelMinVrms;
          }
          tempLevel = outputLevel;
          setAndStoreOutputLevel();
          EXTDisplayLevel(outputLevel, outputLevelMode);
          if (outputFrequency > MAX_FREQ) {
            // happens when waveform switched from TTL to SINUS/TRIANGLE
            outputFrequency = MAX_FREQ;
            setAndStoreOutputFrequency();
            EXTDisplayFrequency(outputFrequency,0);	
          }
        }
        eeprom_busy_wait();
        eeprom_update_byte(&eWaveform,outputWaveform);
        EXTRelaisOnOff((outputWaveform == SQUARE)?RELAIS_ON:RELAIS_OFF);
        if (inputMode == I_EXPLICIT) EXTBuzzerRing(80);
      }
      lcd.setCursor(0,1);
    } 
    else if (EXTSelectSwitchCheck()) {
      Timer2Clear();
      if (tempWaveform != outputWaveform) EXTDisplayWaveform(outputWaveform);	
      if (outputWaveform != SQUARE) {
        // output level can only be changed for sinus and triangle waveform
        systemState = M_LEVEL;
        tempLevel = outputLevel;
        EXTDisplayLevel(outputLevel, outputLevelMode);
        lcd.setCursor((outputLevelStepSize == V_STEPSIZE_10 ? 11 : 12),1);
        lcd.blink();
      }
      else {
        // square was selected -> TTL level can't be changed
        systemState = M_FREQUENCY1;
        tempFrequency = outputFrequency;
        if (tempFrequency >= 1000000) {
          tempDigit = 100000;
          lcd.setCursor(6,0);
        }
        else {
          tempDigit = 1000;
          lcd.setCursor(8,0);
        }
        lcd.cursor();
        lcd.noBlink();
      }
    }
  }
  else if (systemState == M_LEVEL) {
    // changing output level (sinus & triangle)
    if (systemIdleTime >= MAX_IDLE_TIME) {
      systemState = M_IDLE;
      Timer2Stop();
      lcd.noCursor();
      lcd.noBlink();
      if (tempLevel != outputLevel) EXTDisplayLevel(outputLevel, outputLevelMode);	
    }
    else if ((ret = EXTRotaryImpulseCheck()) != 0) {
      Timer2Clear();
      // tempLevel gets increased or decreased by actual step size
      uint16_t tempLevel2 = tempLevel + (outputLevelStepSize * ret);
      if (tempLevel2 >= ((outputLevelMode == V_P2P) ? OUTPUT_LEVEL_VPP_MIN : outputLevelMinVrms) && 
          tempLevel2 <= ((outputLevelMode == V_P2P) ? OUTPUT_LEVEL_VPP_MAX : outputLevelMaxVrms)) {
        tempLevel = tempLevel2;
      }
      else {
        EXTBuzzerRing(10);
      }
      EXTDisplayLevel(tempLevel, outputLevelMode);
      lcd.setCursor((outputLevelStepSize == V_STEPSIZE_10 ? 11 : 12),1);
      if (inputMode == I_NORMAL && tempLevel != outputLevel) {
        outputLevel = tempLevel;
        setAndStoreOutputLevel();
      }
    }
    else if ((event = EXTRotaryButtonCheck()) != idle) {
      Timer2Clear();
      if (inputMode == I_EXPLICIT && tempLevel != outputLevel) {
        outputLevel = tempLevel;
        setAndStoreOutputLevel();
        EXTBuzzerRing(80);
      }
      else {
        if (event == longPress) {
          // toggle between Vpp & Vrms, output level might slightly change due to rounding & casting
          outputLevelMode = (outputLevelMode == V_RMS) ? V_P2P : V_RMS;
          if (outputLevelMode == V_RMS) {
            // converting output level from Vpp to Vrms
            outputLevel = (uint16_t)(outputLevel / (2 * (double)((outputWaveform == SINUS) ? SQRT2 : SQRT3)));
            if (outputLevelMaxVrms < outputLevel) outputLevel = outputLevelMaxVrms;
            if (outputLevel < outputLevelMinVrms) outputLevel = outputLevelMinVrms;
          }
          else {
            // converting output level from Vrms to Vpp (add 0.8 to minimize conversion errors),
            outputLevel = (uint16_t)((outputLevel * 2 * (double)((outputWaveform == SINUS) ? SQRT2 : SQRT3)) + 0.8);
            if (OUTPUT_LEVEL_VPP_MAX < outputLevel) outputLevel = OUTPUT_LEVEL_VPP_MAX;
            if (outputLevel < OUTPUT_LEVEL_VPP_MIN) outputLevel = OUTPUT_LEVEL_VPP_MIN;
          }
          tempLevel = outputLevel;
          EXTDisplayLevel(outputLevel, outputLevelMode);
          lcd.setCursor((outputLevelStepSize == V_STEPSIZE_10 ? 11 : 12),1);
          setAndStoreOutputLevel();
          eeprom_busy_wait();
          eeprom_update_byte(&eLevelMode,outputLevelMode);
        }
        else {
          // shortPress, we change step size
          outputLevelStepSize = (outputLevelStepSize == V_STEPSIZE_10) ? V_STEPSIZE_SMALL : V_STEPSIZE_10;
          lcd.setCursor((outputLevelStepSize == V_STEPSIZE_10 ? 11 : 12),1);
        }
      }
    }
    else if (EXTSelectSwitchCheck()) {
      systemState = M_FREQUENCY1;
      Timer2Clear();
      if (tempLevel != outputLevel) EXTDisplayLevel(outputLevel, outputLevelMode);	
      tempFrequency = outputFrequency;
      if (tempFrequency >= 1000000) {
        tempDigit = 100000;
        lcd.setCursor(6,0);
      }
      else {
        tempDigit = 1000;
        lcd.setCursor(8,0);
      }
      lcd.cursor();
      lcd.noBlink();
    }
  }
  else if (systemState == M_FREQUENCY1) {
    // changing output frequency (selecting digits)
    if (systemIdleTime >= MAX_IDLE_TIME) {
      systemState = M_IDLE;
      Timer2Stop();
      lcd.noCursor();
      if (outputFrequency == 0) {
        outputFrequency = OUTPUT_FREQU_DEFAULT;
        EXTDisplayFrequency(outputFrequency,0);
        setAndStoreOutputFrequency();
        EXTBuzzerRing(80);
      }
      else if (tempFrequency != outputFrequency) {
        EXTDisplayFrequency(outputFrequency,0);	
      }
    }
    else if ((ret = EXTRotaryImpulseCheck()) != 0) {
      Timer2Clear();
      switch (tempDigit) {
        case 1:
          if (ret == -1) { tempDigit = 10; lcd.setCursor(11,0); }
          break;
        case 10:
          if (ret == -1) { tempDigit = 100; lcd.setCursor(10,0); }
          else { tempDigit = 1; lcd.setCursor(12,0); }
          break;
        case 100:
          if (ret == -1) { tempDigit = 1000; lcd.setCursor(8,0); }
          else { tempDigit = 10; lcd.setCursor(11,0); }
          break;
        case 1000:
          if (ret == -1) { tempDigit = 10000; lcd.setCursor(7,0); }
          else { tempDigit = 100; lcd.setCursor(10,0); }
          break;
        case 10000:
          if (ret == -1) { tempDigit = 100000; lcd.setCursor(6,0); }
          else { tempDigit = 1000; lcd.setCursor(8,0); }
          break;
        case 100000:
          if (ret == 1) { tempDigit = 10000; lcd.setCursor(7,0); }
          break;
        default: break;
      }
    }
    else if (EXTRotaryButtonCheck()) {
      systemState = M_FREQUENCY2;
      Timer2Clear();
      if (tempDigit == 10000) {
        EXTDisplayFrequency(outputFrequency,1);
        lcd.setCursor(7,0);
      }
      else if (tempDigit == 100000) {
        EXTDisplayFrequency(outputFrequency,2);
        lcd.setCursor(6,0);
      }
      lcd.blink();
    }
    else if (EXTSelectSwitchCheck()) {
      systemState = M_WAVEFORM;
      Timer2Clear();
      rotaryImpulseLeft = rotaryImpulseRight = 0;
      if (outputFrequency == 0) {
        outputFrequency = OUTPUT_FREQU_DEFAULT;
        setAndStoreOutputFrequency();
        EXTBuzzerRing(80);
      }
      EXTDisplayFrequency(outputFrequency,0);
      tempWaveform = outputWaveform;
      lcd.setCursor(0,1);
      lcd.blink();
    }
  }
  else if (systemState == M_FREQUENCY2) {
    // changing output frequency (changing digits)
    if (systemIdleTime >= MAX_IDLE_TIME) {
      systemState = M_IDLE;
      Timer2Stop();
      lcd.noCursor();
      if (outputFrequency == 0) {
        outputFrequency = OUTPUT_FREQU_DEFAULT;
        setAndStoreOutputFrequency();
        EXTBuzzerRing(80);
      }
      EXTDisplayFrequency(outputFrequency,0);	
    }
    else if ((ret = EXTRotaryImpulseCheck()) != 0) {
      Timer2Clear();
      if (ret == -1) {
        temp = tempFrequency - tempDigit;
        if (temp >= 0) tempFrequency -= tempDigit;
      }
      else {
        temp = tempFrequency + tempDigit;
        if (temp <= ((outputWaveform==SQUARE)?MAX_FREQ_TTL:MAX_FREQ)) {
          tempFrequency += tempDigit;
        }
      }
      if (tempDigit == 10000) EXTDisplayFrequency(tempFrequency,1);
      else if (tempDigit == 100000) EXTDisplayFrequency(tempFrequency,2);
      else EXTDisplayFrequency(tempFrequency,0);
      lcd.blink();
      switch ( tempDigit ) {
        case 1:
          lcd.setCursor(12,0); break;
        case 10:
          lcd.setCursor(11,0); break;
        case 100:
          lcd.setCursor(10,0); break;
        case 1000:
          lcd.setCursor(8,0); break;
        case 10000:
          lcd.setCursor(7,0); break;
        case 100000:
          lcd.setCursor(6,0); break;
        default: break;
      }
      if (inputMode == I_NORMAL && tempFrequency != outputFrequency) {
        outputFrequency = tempFrequency;
        setAndStoreOutputFrequency();
      }
    }
    else if (EXTRotaryButtonCheck()) {
      systemState = M_FREQUENCY1;
      Timer2Clear();
      if (inputMode == I_EXPLICIT && tempFrequency != outputFrequency) {
        outputFrequency = tempFrequency;
        setAndStoreOutputFrequency();
        EXTBuzzerRing(80);
#ifdef USE_SERIAL
        Serial.printf(F("Frequency: %lu"),outputFrequency);
#endif
      }
      EXTDisplayFrequency(outputFrequency,0);	
      lcd.cursor();
      lcd.noBlink();
      switch ( tempDigit ) {
        case 1:
          lcd.setCursor(12,0); break;
        case 10:
          lcd.setCursor(11,0); break;
        case 100:
          lcd.setCursor(10,0); break;
        case 1000:
          lcd.setCursor(8,0); break;
        case 10000:
          lcd.setCursor(7,0); break;
        case 100000:
          lcd.setCursor(6,0); break;
        default: break;
      }
    }
    else if (EXTSelectSwitchCheck()) {
      systemState = M_WAVEFORM;
      Timer2Clear();
      rotaryImpulseLeft = rotaryImpulseRight = 0;
      if (outputFrequency == 0) {
        outputFrequency = OUTPUT_FREQU_DEFAULT;
        setAndStoreOutputFrequency();
        EXTBuzzerRing(80);
      }
      EXTDisplayFrequency(outputFrequency,0);
      tempWaveform = outputWaveform;
      lcd.setCursor(0,1);
      lcd.blink();
    }
  }
}