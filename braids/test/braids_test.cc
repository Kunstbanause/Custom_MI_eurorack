// Copyright 2012 Olivier Gillet.
//
// Author: Olivier Gillet (pichenettes@mutable-instruments.net)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "braids/macro_oscillator.h"
#include "braids/quantizer.h"
#include "stmlib/test/wav_writer.h"
#include "stmlib/utils/dsp.h"

using namespace braids;
using namespace stmlib;

const uint32_t kSampleRate = 96000;
const uint16_t kAudioBlockSize = 24;

Quantizer quantizer;

void TestAudioRendering() {
  MacroOscillator osc;
  WavWriter wav_writer(1, kSampleRate, 10);
  wav_writer.Open("oscillator.wav");

  quantizer.Init();
  quantizer.enabled_ = false;

  osc.Init();
  osc.set_shape(MACRO_OSC_SHAPE_SAM2);

  for (uint32_t i = 0; i < kSampleRate * 10 / kAudioBlockSize; ++i) {
    // if ((i % 2000) == 0) {
    //   osc.Strike();
    // }
    int16_t buffer[kAudioBlockSize];
    uint8_t sync_buffer[kAudioBlockSize];
    uint16_t tri = (i);
    uint16_t tri2 = (i / 2);
    uint16_t ramp = i * 150;
    tri = tri > 32767 ? 65535 - tri : tri;
    tri2 = tri2 > 32767 ? 65535 - tri2 : tri2;
    osc.set_parameters(tri2<<4, 20000);
    memset(sync_buffer, 0, sizeof(sync_buffer));
    //sync_buffer[0] = (i % 32) == 0 ? 1 : 0;
    osc.set_pitch(((40)<< 7));
    osc.Render(sync_buffer, buffer, kAudioBlockSize);
    wav_writer.WriteFrames(buffer, kAudioBlockSize);
  }
}

void TestQuantizer() {
  Quantizer q;
  q.Init();
  for (int16_t i = 0; i < 16384; ++i) {
    int32_t pitch = i;
    printf("%d quantized to %d\n", i, q.Process(i, 60 << 7));
  }
}

int main(void) {
  // TestQuantizer();
  TestAudioRendering();
}
