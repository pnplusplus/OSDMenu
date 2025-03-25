
#include "fmcb_patches.h"
#include "osdmenu_patterns.h"
#include "settings.h"
#include <debug.h>
#include <kernel.h>
#include <libgs.h>
#include <stdint.h>
#include <string.h>

//
// Version info menu patch
//

// Static variables, will be initialized in patchVersionInfo
static char romverValue[] = "\ar0.80VVVVRTYYYYMMDD\ar0.00";
static char mechaconRev[13] = {0};
static char eeRevision[5] = {0};
static char gsRevision[5] = {0};

typedef struct {
  char *name;
  char *value;          // Used for static values
  char *(*valueFunc)(); // Used for dynamic values
} customVersionEntry;

// Returns a pointer to GParam array
// GParam values:
// 0 — Interlaced/non-interlaced mode
// 1 — Video mode (PAL/NTSC)
// 2 — Field mode (Field/Frame)
// 3 — GS Revision
// Can't use PS2SDK libgs function because these parameters
// are set by OSDSYS at an unknown address when setting the video mode
static uint16_t *(*sceGsGetGParam)(void) = NULL;

// Formats single-byte number into M.mm string.
// dst is expected to be at least 5 bytes long
void formatRevision(char *dst, uint8_t rev) {
  dst[0] = '0' + (rev >> 4);
  dst[1] = '.';
  dst[2] = '0' + ((rev & 0x0f) / 10);
  dst[3] = '0' + ((rev & 0x0f) % 10);
  dst[4] = '\0';
}

char *getVideoMode() {
  if (!sceGsGetGParam)
    return NULL;

  uint16_t *gParam = sceGsGetGParam();
  switch (gParam[1]) {
  case GS_MODE_PAL:
    return "PAL";
  case GS_MODE_NTSC:
    return "NTSC";
  case GS_MODE_DTV_480P:
    return "480p";
  }

  return "-";
}

char *getGSRevision() {
  if (!sceGsGetGParam)
    return NULL;

  uint16_t *gParam = sceGsGetGParam();
  if (gParam[3]) {
    formatRevision(gsRevision, gParam[3]);
    return gsRevision;
  }

  gsRevision[0] = '-';
  gsRevision[1] = '\0';
  return gsRevision;
}

// Table for custom menu entries
// Supports dynamic variables that will be updated every time the version menu opens
customVersionEntry entries[] = {
    {"Video Mode", NULL, getVideoMode},                       //
    {"OSDMenu Patch", "\ar0.80" GIT_VERSION "\ar0.00", NULL}, //
    {"ROM", romverValue, NULL},                               //
    {"Emotion Engine", eeRevision, NULL},                     //
    {"Graphics Synthesizer", NULL, getGSRevision},            //
    {"MechaCon", mechaconRev, NULL},                          //
};

static void (*versionInfoInit)(void);
uint32_t verinfoStringTableAddr = 0;

// This function will be called every time the version menu opens
void versionInfoInitHandler() {
  // Execute the original init function
  versionInfoInit();

  // Extend the string table used by the version menu drawing function.
  // It picks up the entries automatically and stops once it gets a NULL pointer (0)
  //
  // Each table entry is represented by three words:
  // Word 0 — pointer to entry name string
  // Word 1 — pointer to entry value string
  // Word 2 — indicates whether the entry has a submenu and points to:
  // 1. ROMs <2.00 — a list of newline-separated submenu entries where each menu entry is represented
  //   as a comma-separated list of strings (e.g. 'Disc Speed,Standard,Fast\nTexture Mapping,Standard,Smooth\n').
  //   Used to build the menu, but not used to draw it.
  // 2. ROMs >=2.00 — some unknown value that doesn't seem to be used by the menu functions as modifying it doesn't seem to change anything
  //
  // Can be 0 if entry doesn't have a submenu.

  // Find the first empty entry or an entry with the name located in the patcher address space (<0x100000)
  uint32_t ptr = verinfoStringTableAddr;
  while (1) {
    if ((_lw(ptr) < 0x100000) || (!_lw(ptr) && !_lw(ptr + 4) && !_lw(ptr + 8)))
      break;

    ptr += 12;
  }
  if (ptr == verinfoStringTableAddr) {
    return;
  }

  // Add custom entries
  char *value = NULL;
  for (int i = 0; i < sizeof(entries) / sizeof(customVersionEntry); i++) {
    value = NULL;
    if (entries[i].valueFunc)
      value = entries[i].valueFunc();
    else if (entries[i].value)
      value = entries[i].value;

    if (!value)
      continue;

    _sw((uint32_t)entries[i].name, ptr);
    _sw((uint32_t)value, ptr + 4);
    _sw(0, ptr + 8);
    ptr += 12;
  }
};

// Extends version menu with custom entries by overriding the function called every time the version menu opens
void patchVersionInfo(uint8_t *osd) {
  // Find the function that inits version menu entries
  uint8_t *ptr = findPatternWithMask(osd, 0x00100000, (uint8_t *)patternVersionInit, (uint8_t *)patternVersionInit_mask, sizeof(patternVersionInit));
  if (!ptr)
    return;

  // First pattern word is nop, advance ptr to point to the function call
  ptr += 4;

  // Get the original function call and save the address
  uint32_t tmp = _lw((uint32_t)ptr);
  tmp &= 0x03ffffff;
  tmp <<= 2;
  versionInfoInit = (void *)tmp;

  // Find the string table address in versionInfoInit
  // Even if it's the same in all ROM versions >=1.20, this acts as a basic sanity check
  // to make sure the patch is replacing the actual versionInfoInit
  uint8_t *tableptr = findPatternWithMask((uint8_t *)versionInfoInit, 0x200, (uint8_t *)patternVersionStringTable,
                                          (uint8_t *)patternVersionStringTable_mask, sizeof(patternVersionStringTable));
  if (!tableptr)
    return;

  // Assemble the table address
  tmp = (_lw((uint32_t)tableptr) & 0xFFFF) << 16;
  tmp |= (_lw((uint32_t)tableptr + 8) & 0xFFFF);

  if (tmp > 0x100000 && tmp < 0x2000000)
    // Make sure the address is in the valid address space
    verinfoStringTableAddr = tmp;
  else
    return;

  // Replace versionInfoInit with the custom function
  tmp = 0x0c000000;
  tmp |= ((uint32_t)versionInfoInitHandler >> 2);
  _sw(tmp, (uint32_t)ptr); // jal versionInfoInitHandler

  // Find sceGsGetGParam address
  ptr = findPatternWithMask(osd, 0x00100000, (uint8_t *)patternSCEGetGParam, (uint8_t *)patternSCEGetGParam_mask, sizeof(patternSCEGetGParam));
  if (ptr) {
    tmp = _lw((uint32_t)ptr);
    tmp &= 0x03ffffff;
    tmp <<= 2;
    sceGsGetGParam = (void *)tmp;
  }

  // Initialize static values
  // ROM version
  if (settings.romver[0] != '\0') {
    memcpy(&romverValue[6], settings.romver, 14);
  } else {
    romverValue[0] = '-';  // Put placeholer value
    romverValue[1] = '\0'; // Put placeholer value
  }

  // MechaCon revision
  if (settings.mechaconRev[0] != '\0')
    strcpy(mechaconRev, settings.mechaconRev);
  else
    mechaconRev[0] = '-';

  // EE Revision
  formatRevision(eeRevision, GetCop0(15));
}
