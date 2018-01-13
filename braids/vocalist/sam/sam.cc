#include <string.h>
#include "debug.h"
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

extern int debug;

void SAM::SetSpeed(unsigned char _speed) {speed = _speed;};
void SAM::SetPitch(unsigned char _pitch) {pitch = _pitch;};
void SAM::SetMouth(unsigned char _mouth) {mouth = _mouth;};
void SAM::SetThroat(unsigned char _throat) {throat = _throat;};
void SAM::EnableSingmode() {singmode = 1;};

void SAM::Init() {
  SetMouthThroat( mouth, throat);

  // bufferpos = 0;
  // // TODO, check for free the memory, 10 seconds of output should be more than enough
  // buffer = (char *) malloc(22050*10);
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