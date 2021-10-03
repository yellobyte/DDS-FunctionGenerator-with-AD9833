/*
 *  AD9833.H 
*/

#ifndef AD9833_H_
#define AD9833_H_

// some definitions
#define DDS_OFF   0
#define SINUS     1
#define SQUARE    2
#define SQUARE2   3
#define TRIANGLE  4
#define FREQ0     18
#define FREQ1     28


// 16bit control register bits (according to spec)
#define MODE      1
#define DIV2      3
#define OPBITEN   5
#define SLEEP12   6
#define SLEEP1    7
#define RESET     8
#define PSELECT   10
#define FSELECT   11
#define HLB       12
#define B28       13

// function declarations 
void DDSOff(void);
void DDSInit(void);
void DDSSignal (uint8_t signal);
void DDSFreq(uint32_t frequenz);

#endif
