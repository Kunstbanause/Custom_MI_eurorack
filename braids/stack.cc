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

void DigitalOscillator::RenderStack(
    const uint8_t* sync,
    int16_t* buffer,
    size_t size) {
  
  uint8_t span = 1 + (parameter_[1] >> 11);
  uint32_t phase_increment[kStackSize];

  phase_increment[0] = phase_increment_;
  uint32_t fm = 0;
  if (quantizer.enabled_) {
    uint16_t index = quantizer.index;
    fm = pitch_ - quantizer.codebook_[quantizer.index];

    for (size_t i = 1; i < kStackSize; i++) {
      index = (index + span) % 128;
      phase_increment[i] = DigitalOscillator::ComputePhaseIncrement(quantizer.codebook_[index] + fm);
    }
  } else {
    uint16_t acc = 0;
    for (size_t i = 1; i < kStackSize; i++) {
      acc += span;
      phase_increment[i] = DigitalOscillator::ComputePhaseIncrement(pitch_ + (acc << 7));
    }
  }

  int32_t target_amplitude[kStackSize];
  int32_t amplitude[kStackSize];
  int16_t pos = parameter_[0] >> 9;
  int16_t fractional = parameter_[0] & 0x1ff;

  for (int i = 0; i < kStackSize; i++) {
    uint32_t x = amplitudes[(pos + i) & 0x3f] << 8;
    uint32_t y = amplitudes[(pos + i + 1) & 0x3f] << 8;
    target_amplitude[i] = ((y * fractional) + (x * (0x1ff - fractional))) >> 9;
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
      state_.stack.phase[i] += (phase_increment[i] << 1);

      if (reset) {
        state_.stack.phase[i] = 0;
      }
      switch (shape_) {
        case OSC_SHAPE_STACK_SAW:
          //to get 0..2^32 to conform to 32767->-32768
          tmp = (1 << 15) - (state_.stack.phase[i] >> 16);

          break;
        case OSC_SHAPE_STACK_TRIANGLE:
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

          break;
        case OSC_SHAPE_STACK_SQUARE:
          if (state_.stack.phase[i] > pw) {
            // don't use full range as it's more likely to 
            // clip than others
            tmp = 26000;
          } else {
            tmp = -26000;
          }
          break;
        default:
            tmp = Interpolate824(wav_sine, state_.stack.phase[i]);
            tmp = (tmp >> 1) + (tmp >> 3);
          break;
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

}