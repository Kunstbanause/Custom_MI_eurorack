#include <string.h>

#include "sam.h"
#include "render.h"
#include "RenderTabs.h"

#include "debug.h"
extern int debug;

//return = hibyte(mem39212*mem39213) <<  1
unsigned char trans(unsigned char a, unsigned char b)
{
  return ((a * b) >> 8) << 1;
}

//timetable for more accurate c64 simulation
static const int timetable[5][5] =
{
  {162, 167, 167, 127, 128},
  {226, 60, 60, 0, 0},
  {225, 60, 59, 0, 0},
  {200, 0, 0, 54, 55},
  {199, 0, 0, 54, 54}
};

void SAM::Output(int index, unsigned char A)
{
  static unsigned oldtimetableindex = 0;
  int k;
  tinyBufferSize = tinyBufferSize + timetable[oldtimetableindex][index];
  oldtimetableindex = index;
  if ((tinyBufferSize/50) + 5 >= 5000) {
    return;
  }

  // write a little bit in advance
  for(k=0; k<5; k++) {
    tinyBuffer[(tinyBufferStart + (tinyBufferSize/50) + k) % MAX_TINY_BUFFER] = (A & 15)*16;
  }
}

unsigned char SAM::RenderVoicedSample(unsigned short hi, unsigned char off, unsigned char phase1)
{
  do {
    unsigned char sample = sampleTable[hi+off];
    unsigned char bit = 8;
    do {
      if ((sample & 128) != 0) Output(3, 26);
      else Output(4, 6);
      sample <<= 1;
    } while(--bit != 0);
    off++;
  } while (++phase1 != 0);
  return off;
}

void SAM::RenderUnvoicedSample(unsigned short hi, unsigned char off, unsigned char mem53)
{
  do {
    unsigned char bit = 8;
    unsigned char sample = sampleTable[hi+off];
    do {
      if ((sample & 128) != 0) Output(2, 5);
      else Output(1, mem53);
      sample <<= 1;
    } while (--bit != 0);
  } while (++off != 0);
}

// -------------------------------------------------------------------------
//Code48227
// Render a sampled sound from the sampleTable.
//
//   Phoneme   Sample Start   Sample End
//   32: S*    15             255
//   33: SH    257            511
//   34: F*    559            767
//   35: TH    583            767
//   36: /H    903            1023
//   37: /X    1135           1279
//   38: Z*    84             119
//   39: ZH    340            375
//   40: V*    596            639
//   41: DH    596            631
//
//   42: CH
//   43: **    399            511
//
//   44: J*
//   45: **    257            276
//   46: **
//
//   66: P*
//   67: **    743            767
//   68: **
//
//   69: T*
//   70: **    231            255
//   71: **
//
// The SampledPhonemesTable[] holds flags indicating if a phoneme is
// voiced or not. If the upper 5 bits are zero, the sample is voiced.
//
// Samples in the sampleTable are compressed, with bits being converted to
// bytes from high bit to low, as follows:
//
//   unvoiced 0 bit   -> X
//   unvoiced 1 bit   -> 5
//
//   voiced 0 bit     -> 6
//   voiced 1 bit     -> 24
//
// Where X is a value from the table:
//
//   { 0x18, 0x1A, 0x17, 0x17, 0x17 };
//
// The index into this table is determined by masking off the lower
// 3 bits from the SampledPhonemesTable:
//
//        index = (SampledPhonemesTable[i] & 7) - 1;
//
// For voices samples, samples are interleaved between voiced output.


void SAM::RenderSample(unsigned char *mem66, unsigned char consonantFlag, unsigned char mem49)
{
  // mem49 == current phoneme's index

  // mask low three bits and subtract 1 get value to
  // convert 0 bits on unvoiced samples.
  unsigned char hibyte = (consonantFlag & 7)-1;

  // determine which offset to use from table { 0x18, 0x1A, 0x17, 0x17, 0x17 }
  // T, S, Z                0          0x18
  // CH, J, SH, ZH          1          0x1A
  // P, F*, V, TH, DH       2          0x17
  // /H                     3          0x17
  // /X                     4          0x17

  unsigned short hi = hibyte*256;
  // voiced sample?
  unsigned char pitch = consonantFlag & 248;
  if(pitch == 0) {
    // voiced phoneme: Z*, ZH, V*, DH
    pitch = pitches[mem49] >> 4;
    *mem66 = RenderVoicedSample(hi, *mem66, pitch ^ 255);
    return;
  }
  RenderUnvoicedSample(hi, pitch^255, tab48426[hibyte]);
}