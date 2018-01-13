#include <string.h>
#include "sam.h"
#include "render.h"
#include "SamTabs.h"

enum {
  pR    = 23,
  pD    = 57,
  pT    = 69,
  BREAK = 254,
  END   = 255
};

void SAM::Init() {
}

void SAM::LoadTables(const unsigned char *data, const unsigned char entryLen) {
  unsigned short offset = 0;
  frequency1 = &data[0];
  offset += entryLen;
  frequency2 = &data[offset];
  offset += entryLen;
  frequency3 = &data[offset];
  offset += entryLen;
  pitches = &data[offset];
  offset += entryLen;
  amplitude1 = &data[offset];
  offset += entryLen;
  amplitude2 = &data[offset];
  offset += entryLen;
  amplitude3 = &data[offset];
  offset += entryLen;
  sampledConsonantFlag = &data[offset];
  offset += entryLen;
  framesRemaining = entryLen;
  totalFrames = entryLen;
}