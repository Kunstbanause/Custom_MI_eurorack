#include "braids/digital_oscillator.h"

#include <algorithm>
#include <cstdio>

#include "stmlib/utils/dsp.h"
#include "stmlib/utils/random.h"

#include "braids/parameter_interpolation.h"
#include "braids/resources.h"
#include "braids/quantizer.h"

extern braids::Quantizer quantizer;

namespace braids {

using namespace stmlib;

const int kStackSize = 6;

const uint8_t amplitudes[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  214,  171,   128,   86,   43,   43,
  42,    85,   128,  170,  213,  255,
  171,   86,     0,   85,  170,  255,
  171,   86,     0,   85,  170,  255,
    0,  255,     0,  255,    0,  255, 
    0,    0,     0,  255,  255,    0, 
    0,  255,   255,  255,    0,    0,
  221,  127,    71,    8,  197,   19,
   98,  137,    55,   70,  122,  153,
   50,  203,    43,  198,
};

void DigitalOscillator::renderChord(
  const uint8_t *sync, 
  int16_t *buffer, 
  size_t size, 
  uint8_t* noteOffset, 
  uint8_t noteCount) {

  uint32_t phase_increment[kStackSize];

  if (noteCount > kStackSize) {
    noteCount = kStackSize;
  }

  uint32_t fm = 0;
  if (quantizer.enabled_) {
    uint16_t index = quantizer.index;
    fm = pitch_ - quantizer.codebook_[quantizer.index];

    for (size_t i = 0; i < noteCount; i++) {
      index = (index + noteOffset[i]) % 128;
      phase_increment[i] = DigitalOscillator::ComputePhaseIncrement(quantizer.codebook_[index] + fm);
    }
  } else {
    for (size_t i = 0; i < noteCount; i++) {
      phase_increment[i] = DigitalOscillator::ComputePhaseIncrement(pitch_ + (noteOffset[i] << 7));
    }
  }

  int32_t target_amplitude[kStackSize];
  int32_t amplitude[kStackSize];
  int16_t pos = parameter_[0] >> 9;
  int16_t fractional = parameter_[0] & 0x1ff;

  for (int i = 0; i < kStackSize; i++) {
    uint32_t x = amplitudes[(pos + i) & 0x3f] << 8;
    uint32_t y = amplitudes[(pos + i + 1) & 0x3f] << 8;
    if (i < noteCount) {
      target_amplitude[i] = ((y * fractional) + (x * (0x1ff - fractional))) >> 9;
    } else {
      target_amplitude[i] = 0;
    }
    amplitude[i] = state_.stack.amplitude[i];
  }
  
  int16_t previous_sample = state_.stack.previous_sample;

  // pulse width for square stack
  const uint32_t pw = 1<<31;

  while (size) {
    int32_t out;
    int32_t tmp;
    uint32_t phase;
    
    out = 0;
    bool reset = *sync++ || *sync++;
    for (int i = 0; i < kStackSize; ++i) {

      // downsampling
      if (i < noteCount) {
        state_.stack.phase[i] += (phase_increment[i] << 1);
      }

      if (reset) {
        state_.stack.phase[i] = 0;
      }

      if (shape_ == OSC_SHAPE_STACK_SAW || shape_ == OSC_SHAPE_CHORD_SAW) {
          //to get 0..2^32 to conform to 32767->-32768
          tmp = (1 << 15) - (state_.stack.phase[i] >> 16);

      } else if (shape_ == OSC_SHAPE_STACK_TRIANGLE || shape_ == OSC_SHAPE_CHORD_TRIANGLE) {
          //to get 0..2^32 to conform to -32768->32768->-32768;
          // range = 0.. 128k, then inverse second 64k
          // so we need top 17 bits of phase
          phase = state_.stack.phase[i] >> 15;

          if (phase > 1<<16) {
            tmp = ((1 << 15) - (phase & 0xffff));
          } else {
            tmp = (phase - (1<<15));
          }
          tmp = (tmp >> 1) + (tmp >> 3);

        } else if (shape_ == OSC_SHAPE_STACK_SQUARE || shape_ == OSC_SHAPE_CHORD_SQUARE) {
          if (state_.stack.phase[i] > pw) {
            // don't use full range as it's more likely to 
            // clip than others
            tmp = 26000;
          } else {
            tmp = -26000;
          }

        } else {           
          tmp = Interpolate824(wav_sine, state_.stack.phase[i]);
          tmp = (tmp >> 1) + (tmp >> 3);
        }

      out += (tmp * amplitude[i]) >> 18;
      amplitude[i] += (target_amplitude[i] - amplitude[i]) >> 4;
    }
    CLIP(out)
    *buffer++ = (out + previous_sample) >> 1;
    *buffer++ = out;

    previous_sample = out;
    size -= 2;
  }
  state_.stack.previous_sample = previous_sample;
  for (size_t i = 0; i < kStackSize; ++i) {
    state_.stack.amplitude[i] = amplitude[i];
  }
}

void DigitalOscillator::RenderStack(
    const uint8_t* sync,
    int16_t* buffer,
    size_t size) {
  
  uint8_t span = 1 + (parameter_[1] >> 11);
  uint8_t offsets[kStackSize];
  uint8_t acc = 0;

  for (uint8_t i = 0; i < kStackSize; i++) {
    offsets[i] = acc;
    acc += span;
  }

  renderChord(sync, buffer, size, offsets, kStackSize);
}

// 7th
// oct
// 9th
// 11th
// add6
// 7th add6
// oct add6
// 9th add6
// add9
// add11
// add13
// octsus4
// 7thsus4
// 7thsus2
// 9thsus4
// 9thsus2

// number of notes, followed by offsets
const uint8_t chords[16][4] = {
  {1, 7, 0, 0}, // octave
  {2, 5, 7, 0}, // octave add6
  {1, 6, 0, 0}, // 7th
  {2, 5, 6, 0}, // 7th add6
  {2, 6, 8, 0}, // 9th
  {3, 6, 8, 10}, // 11th
  {3, 5, 7, 10}, // 11th add6
  {1, 8, 0, 0}, // add9
  {2, 6, 10, 0}, // 7th add11
  {2, 6, 12, 0}, // 7th add13
  {1, 7, 0, 0}, // oct sus4
  {1, 6, 0, 0}, // 7th sus4
  {2, 6, 8, 0}, // 9th sus4
  {3, 6, 8, 10}, // 11th sus4
  {2, 6, 8, 0}, // 9th sus2
};

void DigitalOscillator::RenderDiatonicChord(
    const uint8_t* sync,
    int16_t* buffer,
    size_t size) {
  
  uint8_t extensionIndex = (parameter_[1] >> 12) & 0xf;
  // uint8_t inversion = (parameter_[1] >> 13) & 0x7;
  uint8_t offsets[6];

  if (quantizer.enabled_) {
    offsets[0] = 0;
    offsets[2] = 4;

    if (extensionIndex < 11) {
      offsets[1] = 2;
    } else if (extensionIndex < 15) {
      offsets[1] = 3;
    } else {
      offsets[1] = 1;
    }
  }

  uint8_t len = chords[extensionIndex][0];

  for (size_t i = 0; i < len; i++) {
    offsets[i+3] = chords[extensionIndex][i+1];
  }

  renderChord(sync, buffer, size, offsets, len + 3);
}

}