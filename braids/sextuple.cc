#include "braids/macro_oscillator.h"

#include <algorithm>

#include "stmlib/utils/dsp.h"

#include "braids/parameter_interpolation.h"
#include "braids/resources.h"
#include "braids/vocalist/wordlist.h"
#include "braids/quantizer.h"

extern braids::Quantizer quantizer;

namespace braids {

#define STACK_SIZE 6

void MacroOscillator::RenderSextuple(
    const uint8_t* sync,
    int16_t* buffer,
    size_t size) {

   AnalogOscillatorShape base_shape;
  // switch (shape_) {
  //   case MACRO_OSC_SHAPE_TRIPLE_SAW:
  //     base_shape = OSC_SHAPE_SAW;
  //     break;
  //   case MACRO_OSC_SHAPE_TRIPLE_TRIANGLE:
  //     base_shape = OSC_SHAPE_TRIANGLE;
  //     break;
  //   case MACRO_OSC_SHAPE_TRIPLE_SQUARE:
  //     base_shape = OSC_SHAPE_SQUARE;
  //     break;
  //   default:
  //     base_shape = OSC_SHAPE_SINE;
  //     break;
  // }
  
  base_shape = OSC_SHAPE_ADDITIVE_SINE;

  analog_oscillator_[0].set_parameter(0);
  analog_oscillator_[1].set_parameter(0);
  analog_oscillator_[2].set_parameter(0);
  analog_oscillator_[3].set_parameter(0);
  analog_oscillator_[4].set_parameter(0);
  analog_oscillator_[5].set_parameter(0);

  uint8_t span = 1 + (parameter_[0] >> 11);

  analog_oscillator_[0].set_pitch(pitch_);
  if (quantizer.enabled_) {
    uint16_t index = quantizer.index;
    for (size_t i = 0; i < STACK_SIZE-1; ++i) {
      index = index + span % 128;
      analog_oscillator_[i+1].set_pitch(quantizer.codebook_[index]);
    }
  } else {
    for (size_t i = 1; i < STACK_SIZE; i++) {
      analog_oscillator_[i].set_pitch(pitch_ + ((i * span) << 7));
    }
  }

  analog_oscillator_[0].set_shape(base_shape);
  analog_oscillator_[1].set_shape(base_shape);
  analog_oscillator_[2].set_shape(base_shape);
  analog_oscillator_[3].set_shape(base_shape);
  analog_oscillator_[4].set_shape(base_shape);
  analog_oscillator_[5].set_shape(base_shape);

  std::fill(&buffer[0], &buffer[size], 0);

  for (size_t i = 0; i < STACK_SIZE; ++i) {
    analog_oscillator_[i].Render(sync, buffer, NULL, size);
  }
}

}