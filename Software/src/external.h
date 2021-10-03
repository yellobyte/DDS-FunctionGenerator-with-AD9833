/*
 * EXTERNAL.H: definitions and functions for the AD9833 function generator
 *
 * Created: June 2016
 *          2021/10/01 possibility to set output level in Vrms added
 * Author : ThJ (yellobyte@bluewin.ch)
*/

#ifndef EXTERNAL_H_
#define EXTERNAL_H_

//
// definitions
//
#define RELAIS_OFF	0
#define RELAIS_ON	  1
#define V_RMS 0           // root mean square
#define V_P2P 1           // peak to peak

// limit set by actual hardware, the exact Vpp limit is adjusted with R11 at LSP2
#define OUTPUT_LEVEL_VPP_MAX 600  // max output level 6.00 Vpp (equal to 2.1213 Vrms[sinus] or 1.7320 Vrms[triangle])
#define OUTPUT_LEVEL_VPP_MIN 1

#define SQRT2 1.41421
#define SQRT3 1.73205
#define V_RMS_MAX_SIN (uint16_t)(OUTPUT_LEVEL_VPP_MAX / (2 * (double)SQRT2)) // Vrms[sinus] = Vpp/(2*SQRT(2))
#define V_RMS_MAX_TRI (uint16_t)(OUTPUT_LEVEL_VPP_MAX / (2 * (double)SQRT3)) // Vrms[triangle] = Vpp/(2*SQRT(3))
#define V_RMS_MIN_SIN 1
#define V_RMS_MIN_TRI 1

// for buttons/switches/etc.
enum buttonEvent { idle = 0, shortPress, longPress, fallingEdge, risingEdge, pressed };

//
// function declarations
//
void EXTDisplayFrequency(uint32_t frequ, uint8_t leadingZeros);
void EXTDisplayWaveform(uint8_t waveform);
void EXTDisplayLevel(uint16_t outputLevel,  uint8_t outpuLevelMode);

void EXTRelaisInit(void);
void EXTRelaisOnOff(uint8_t setting);

void EXTBuzzerInit(void);
void EXTBuzzerRing(uint16_t ms);

void    EXTSelectSwitchInit(void);
uint8_t EXTSelectSwitchCheck(void);

void        EXTRotaryInit(void);
buttonEvent EXTRotaryButtonCheck(void);
int8_t      EXTRotaryImpulseCheck(void);

void EXTDacInit(void);
void EXTDacSetLevel(uint16_t outputLevel, uint8_t outputWaveform, uint8_t outputLevelMode);

#endif

