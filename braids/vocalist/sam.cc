#include <string.h>
#include "sam.h"
#include "RenderTabs.h"

enum {
  pR    = 23,
  pD    = 57,
  pT    = 69,
  BREAK = 254,
  END   = 255
};

// TODO: put these in SAM
char *tinyBuffer;
unsigned short tinyBufferSize; // this is in a weird "actual size * 50" unit because that's what the generated code uses elsewhere for some calculations
unsigned short tinyBufferStart; // this is a direct index into tinyBuffer where the ring buffer begins.

void SAM::Init() {
}

void SAM::LoadTables(const unsigned char *data) {
  unsigned short offset = 0;

  // (char) lengths of the following tables, in order:
  // %w(frequency1 frequency2 frequency3 pitches amplitude1 amplitude2 amplitude3 sampledConsonantFlag)
  const unsigned char *tableLength = &data[0];

  // "table length" table length....... i know
  offset += 8;
  frequency1 = &data[offset];
  offset += tableLength[0];
  frequency2 = &data[offset];
  offset += tableLength[1];
  frequency3 = &data[offset];
  offset += tableLength[2];
  pitches = &data[offset];
  offset += tableLength[3];
  amplitude1 = &data[offset];
  offset += tableLength[4];
  amplitude2 = &data[offset];
  offset += tableLength[5];
  amplitude3 = &data[offset];
  offset += tableLength[6];
  sampledConsonantFlag = &data[offset];
  offset += tableLength[7];

  // measure uncompressed length of frequency1 to calculate total frames in this word
  framesRemaining = 0;
  offset = 0;
  while (offset < tableLength[0]) {
    framesRemaining += frequency1[offset];
    offset += 2;
  }

  frameProcessorPosition = 0;
  totalFrames = framesRemaining;
}

// does not protect against overruns
unsigned char SAM::RLEGet(const unsigned char *rleData, unsigned char idx) {
  while (idx >= rleData[0]) {
    idx -= rleData[0];
    rleData += 2;
  }
  return rleData[1];
}

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
    tinyBuffer[(tinyBufferStart + (tinyBufferSize/50) + k) % MAX_TINY_BUFFER] = A;
  }
}

unsigned char SAM::RenderVoicedSample(unsigned short hi, unsigned char off, unsigned char phase1)
{
  do {
    unsigned char sample = sampleTable[hi+off];
    unsigned char bit = 8;
    do {
      if ((sample & 128) != 0) Output(3, (26 & 0xf) << 4);
      else Output(4, 6<<4);
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
      if ((sample & 128) != 0) Output(2, 5<<4);
      else Output(1, (mem53 & 0xf)<<4);
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
    pitch = RLEGet(pitches, mem49) >> 4;
    *mem66 = RenderVoicedSample(hi, *mem66, pitch ^ 255);
    return;
  }
  RenderUnvoicedSample(hi, pitch^255, tab48426[hibyte]);
}

void SAM::CombineGlottalAndFormants(unsigned char phase1, unsigned char phase2, unsigned char phase3, unsigned char Y)
{
  unsigned int tmp;

  tmp   = multtable[sinus[phase1]     | RLEGet(amplitude1, Y)];
  tmp  += multtable[sinus[phase2]     | RLEGet(amplitude2, Y)];
  tmp  += tmp > 255 ? 1 : 0; // if addition above overflows, we for some reason add one;
  tmp  += multtable[rectangle[phase3] | RLEGet(amplitude3, Y)];
  tmp  += 136;
  // tmp >>= 4; // Scale down to 0..15 range of C64 audio.

  Output(0, tmp);
}

// PROCESS THE FRAMES
//
// In traditional vocal synthesis, the glottal pulse drives filters, which
// are attenuated to the frequencies of the formants.
//
// SAM generates these formants directly with sin and rectangular waves.
// To simulate them being driven by the glottal pulse, the waveforms are
// reset at the beginning of each glottal pulse.
//
void SAM::InitFrameProcessor() {
  frameProcessorPosition = 0;
  glottal_pulse = RLEGet(pitches, 0);
  mem38 = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75
  tinyBufferSize = 0;
  tinyBufferStart = 0;
}

int SAM::Drain(int threshold, int count, uint8_t *buffer)
{
  int available = (tinyBufferSize / 50) - threshold;
  if (available <= 0) {
    return 0;
  }

  // copy out either total available, or remaining count, whichever is lower
  available = (available > count) ? count : available;

  // consume N sound bytes
  for (int k = 0; k < available; k++) {
    buffer[k] = tinyBuffer[(tinyBufferStart+k) % MAX_TINY_BUFFER];
  }

  tinyBufferSize -= (available * 50);
  tinyBufferStart = (tinyBufferStart + available) % MAX_TINY_BUFFER;

  return available;
}

void SAM::SetFramePosition(int pos) {
  frameProcessorPosition = pos;
  framesRemaining = totalFrames - pos;

  //glottal_pulse = pitches[pos];
  //mem38 = glottal_pulse - (glottal_pulse>>2); // mem44 * 0.75
}

unsigned char SAM::ProcessFrame(unsigned char Y, unsigned char mem48)
{
    unsigned char flags = RLEGet(sampledConsonantFlag, Y);
    unsigned char absorbed = 0;

    // unvoiced sampled phoneme?
    if(flags & 248) {
      RenderSample(&mem66, flags, Y);
      // skip ahead two in the phoneme buffer
      speedcounter = speed;
      absorbed = 2;
    } else {
      CombineGlottalAndFormants(phase1, phase2, phase3, Y);

      speedcounter--;
      if (speedcounter == 0) {
        absorbed = 1;

        if(mem48 == 1) {
          return absorbed;
        }
        speedcounter = speed;
      }

      --glottal_pulse;

      if(glottal_pulse != 0) {
        // not finished with a glottal pulse

        --mem38;
        // within the first 75% of the glottal pulse?
        // is the count non-zero and the sampled flag is zero?
        if((mem38 != 0) || (flags == 0)) {
          // reset the phase of the formants to match the pulse
          phase1 += RLEGet(frequency1, Y + absorbed);
          phase2 += RLEGet(frequency2, Y + absorbed);
          phase3 += RLEGet(frequency3, Y + absorbed);
          return absorbed;
        }

        // voiced sampled phonemes interleave the sample with the
        // glottal pulse. The sample flag is non-zero, so render
        // the sample for the phoneme.
        RenderSample(&mem66, flags, Y + absorbed);
      }
    }

    glottal_pulse = RLEGet(pitches, Y + absorbed);
    mem38 = glottal_pulse - (glottal_pulse>>2); // mem44 * 0.75

    // reset the formant wave generators to keep them in
    // sync with the glottal pulse
    phase1 = 0;
    phase2 = 0;
    phase3 = 0;

    return absorbed;
}
