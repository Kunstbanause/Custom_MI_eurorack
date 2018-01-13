#include <stdio.h>
#include <stdlib.h>

#include "vocalist.h"
#include "wordlist.h"

void Vocalist::Init() {
    playing = false;
    bank = 0;
    offset = 0;
    word = -1;
    risingEdge = 0;
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
  playing = false;
  if (mode == MODE_NORMAL) {
    sam.LoadTables(&data[wordpos[bank][word]], wordlen[bank][word]);
    sam.InitFrameProcessor();

    validOffset_ = &validOffset[validOffsetPos[bank][word]];
    validOffsetLen_ = validOffsetLen[bank][word];
  }
}

void Vocalist::Render(const uint8_t *sync_buffer, int16_t *output, int bufferLen) {
  unsigned char buffer[6];
  int len = bufferLen >> 2;

  int written = 0;

  while (written < len) {
    int wrote = sam.Drain(0, len - written, &buffer[written]);
    written += wrote;

    if (wrote == 0) {
      if (sam.frameProcessorPosition != validOffset_[offset]) {
        sam.SetFramePosition(validOffset_[offset]);
      }
      sam.ProcessFrame(sam.frameProcessorPosition, sam.framesRemaining);
    }
  }

  for (int i = written; i < len; i++) {
    buffer[i] = 0x80;
  }

  for (int i = 0; i < 6; i+=1) {
    int idx = i << 2;
    int16_t value = (((int16_t) buffer[i])-127) << 8;

    output[idx] = value;
    output[idx+1] = value;
    output[idx+2] = value;
    output[idx+3] = value;
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

void Vocalist::set_pitch(uint16_t braids_pitch) {
  // TODO sam.SetPitch(82 + (64 - (braids_pitch >> 5)));
}

void Vocalist::set_gatestate(bool gs) {
  if (!gatestate && gs) {
    risingEdge = true;
  }
  gatestate = gs;
}


