#include <stdio.h>
#include <stdlib.h>

#include "../resources.h"
#include "vocalist.h"
#include "wordlist.h"

void Vocalist::Init() {
    phase = 0;
    bank = 0;
    offset = 0;
    word = -1;
    mode = MODE_NORMAL;
    
    SetWord(0);
}

void Vocalist::set_shape(int shape) {
  bank = shape;
}

void Vocalist::SetWord(unsigned char w) {
  if (word != w) {
    word = w;
    Load();
  }
}

void Vocalist::Load() {
  scan = false;
  if (mode == MODE_NORMAL) {
    sam.LoadTables(&data[wordpos[bank][word]], wordlen[bank][word]);
    sam.InitFrameProcessor();

    validOffset_ = &validOffset[validOffsetPos[bank][word]];
    validOffsetLen_ = validOffsetLen[bank][word];
  }
}

static const uint16_t kHighestNote = 140 * 128;
static const uint16_t kPitchTableStart = 128 * 128;
static const uint16_t kOctave = 12 * 128;

uint32_t ComputePhaseIncrement(int16_t midi_pitch) {
  if (midi_pitch >= kPitchTableStart) {
    midi_pitch = kPitchTableStart - 1;
  }
  
  int32_t ref_pitch = midi_pitch;
  ref_pitch -= kPitchTableStart;
  
  size_t num_shifts = 0;
  while (ref_pitch < 0) {
    ref_pitch += kOctave;
    ++num_shifts;
  }
  
  uint32_t a = braids::lut_oscillator_increments[ref_pitch >> 4];
  uint32_t b = braids::lut_oscillator_increments[(ref_pitch >> 4) + 1];
  uint32_t phase_increment = a + \
      (static_cast<int32_t>(b - a) * (ref_pitch & 0xf) >> 4);
  phase_increment >>= num_shifts;
  return phase_increment;
}

// in braids, one cycle is 65536 * 65536 in the phase increment.
// and since SAM is outputting 440hz at 22050 sample rate, one SAM cycle is (22050/440) samples
// so every 65536 * 65536 / (22050/440) phase increment, we consume 1 SAM sample.
// but we precompute to get an accurate number without integer overflow or rounding
const uint32_t kPhasePerSample = 85704562;

void Vocalist::Render(const uint8_t *sync_buffer, int16_t *output, int len) {
  int written = 0;
  unsigned char sample;
  int phase_increment = ComputePhaseIncrement(braids_pitch);

  while (written < len) {
    // TODO: try interpolation although I suspect it won't feel right
    output[written++] = samples[0];
    phase += phase_increment;
    while (phase > kPhasePerSample) {
      samples[0] = samples[1];
      phase -= kPhasePerSample;

      int wrote = 0;
      while (wrote == 0) {
        wrote = sam.Drain(0, 1, &sample);
        samples[1] = (((int16_t) sample)-127) << 8;
        
        if (wrote == 0) {
          if (scan) {
            if (sam.framesRemaining == 0) {
              scan = false;
            }
          }
          if (!scan) {
            if (sam.frameProcessorPosition != validOffset_[offset]) {
              // note this resets glottal pulse, and now that we modify frameProcessorPosition below that might
              // be unwanted. If this sounds worse, only modify frameProcessorPosition when scanning.
              sam.SetFramePosition(validOffset_[offset]);
            }
          }
          unsigned char absorbed = sam.ProcessFrame(sam.frameProcessorPosition, sam.framesRemaining);
          sam.frameProcessorPosition += absorbed;
          sam.framesRemaining -= absorbed;
        }
      }
    }
  }
}

void Vocalist::set_parameters(uint16_t parameter1, uint16_t parameter2)
{
  SetWord(parameter1 >> 11);

  // max parameter2 is 32767, divisor must be higher so max offset is sam.totalFrames-1
  if (parameter2 > 32767) {
    parameter2 = 32767;
  }
  offset = validOffsetLen_ * parameter2 / 32768;
}

void Vocalist::set_pitch(uint16_t pitch) {
  braids_pitch = pitch;
}

void Vocalist::Strike() {
  scan = true;
}
