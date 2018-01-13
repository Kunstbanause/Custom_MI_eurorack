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
  int i;
  SetMouthThroat( mouth, throat);

  // bufferpos = 0;
  // // TODO, check for free the memory, 10 seconds of output should be more than enough
  // buffer = (char *) malloc(22050*10);

  for(i=0; i<60; i++) {
    phonemeIndexOutput[i] = 0;
    stressOutput[i] = 0;
    phonemeLengthOutput[i] = 0;
  }
}

bool SAM::LoadNextWord(const unsigned char *phonemeindex, const unsigned char *phonemeLength, const unsigned char *stress, int len) {
  srcpos = 0; // Position in source
  unsigned char destpos = 0; // Position in output

  while(srcpos < len) {
    unsigned char A = phonemeindex[srcpos];
    phonemeIndexOutput[destpos] = A;
    switch(A) {
      case END:
      //Render(&bufferpos, buffer);
      return true;
      case BREAK:
      phonemeIndexOutput[destpos] = END;
      //Render(&bufferpos, buffer);

      return false;
      case 0:
      break;
      default:
      phonemeLengthOutput[destpos] = phonemeLength[srcpos];
      stressOutput[destpos]        = stress[srcpos];
      ++destpos;
    }
    ++srcpos;
  }

  phonemeIndexOutput[destpos] = END;

  return true;
}
