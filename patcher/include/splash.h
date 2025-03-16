#ifndef _SPLASH_H_
#define _SPLASH_H_

typedef enum {
  PAL_640_512_32,
  NTSC_640_448_32
} GSVideoMode;

// Initializes GS and displays FMCB splash screen
void gsClearScreen();
void gsDisplaySplash();

#endif
