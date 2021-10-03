/*
 * AD9833.CPP: Routines to communicate with DDS-Chip AD9833 using SPI
*/

#include <Arduino.h>
#include <SPI.h>
#include "ad9833.h"

#define AD9833_CS  PIN_SPI_SS
#define AD9833_SPI SPISettings(4000000, MSBFIRST, SPI_MODE2)
// AD9833 master clock (depends on module hardware)
#define AD9833_MCLK 25000000

uint16_t value = 0;
double   konst = 268435456;       //2^28 (1 << 28) 
float    konst2 = 4096;           // 2^12 (1 << 12)

// writing a 16bit word into AD9833, high byte first
static void DDSWrite(uint16_t data)
{
  SPI.beginTransaction(AD9833_SPI);
  digitalWrite(AD9833_CS, LOW);   // set select signal LOW for AD9833
  SPI.transfer((uint8_t)(data >> 8));
  SPI.transfer((uint8_t)(data & 255));
  digitalWrite(AD9833_CS, HIGH);  // set select signal HIGH for AD9833
  SPI.endTransaction();  
}

// init AD9833
void DDSInit(void)
{
  SPI.begin();
  DDSOff();
}

// reset counter & switch off AD9833
void DDSOff(void)
{
  // complete word write, counter reset, MCLK off, DAC disconnected, SINROM bypass
  value = (1 << B28) | (1 << RESET) | (1 << SLEEP1) | (1 << OPBITEN) | (1 << MODE);
  DDSWrite(value);
  value &= ~(1 << RESET);
  delay(1);
  DDSWrite(value);
}

// sets output waveform (SINUS, TRIANGLE, SQUARE, SQUARE2, OFF)
void DDSSignal (uint8_t signal) 
{
  switch (signal) {
    case DDS_OFF:
      // AD9833 no output
      DDSOff();
      break;
    case SINUS:
      // AD9833 output SINUS
      value &= ~((1 << OPBITEN) | (1 << MODE) | (1 << SLEEP1));
      break;
    case SQUARE:
      // AD9833 output SQUARE
      value |= (1 << OPBITEN) | (1 << DIV2);
      value &= ~((1 << MODE) | (1 << SLEEP1));
      break;
    case SQUARE2:
      // AD9833 output SQUARE2
      value &=  ~((1 << MODE) | (1 << DIV2) | (1 << SLEEP1));
      value |= (1 << OPBITEN);
      break;
    case TRIANGLE:
      // AD9833 output TRIANGLE
      value &= ~((1 << OPBITEN) | (1 << SLEEP1));
      value |= (1 << MODE);
      break;
    default:
      // no change
      break;
  }

  DDSWrite(value);
}

// sets selected frequency by writing to register FREQ0
void DDSFreq(uint32_t frequenz) 
{
  double temp = ((double)frequenz/(double)AD9833_MCLK);
  uint32_t regist = temp * konst;
  uint16_t write = regist & 0x3FFF;
  
  write |= (1 << 14); // (0x4000)
  DDSWrite(write);

  write = (regist & ~0x3FFF) >> 14;
  write |= (1 << 14); // (0x4000)
  DDSWrite(write);
}

