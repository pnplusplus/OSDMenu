
#include "fmcb_patches.h"
#include "osdmenu_patterns.h"
#include "settings.h"
#include <debug.h>
#include <stdint.h>
#include <string.h>

//
// Version info menu patch
//

// Static variables, will be initialized in patchVersionInfo
static char romverValue[] = "\ar0.80VVVVRTYYYYMMDD\ar0.00";
static char mechaconRev[5] = {0};

typedef struct {
  char *name;
  char *value;          // Used for static values
  char *(*valueFunc)(); // Used for dynamic values
} customVersionEntry;

// Table for custom menu entries
// Supports dynamic variables that cam be updated every time the version menu opens
customVersionEntry entries[] = {
    {"OSDMenu Patch", "\ar0.80" GIT_VERSION "\ar0.00", NULL},
    {"ROM", romverValue, NULL},
    {"MechaCon", mechaconRev, NULL},
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

  // Find the first empty entry or an entry that has a name located in the patcher address space (<0x100000)
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
  for (int i = 0; i < sizeof(entries) / sizeof(customVersionEntry); i++) {
    _sw((uint32_t)entries[i].name, ptr);
    if (entries[i].valueFunc)
      _sw((uint32_t)entries[i].valueFunc(), ptr + 4);
    else
      _sw((uint32_t)entries[i].value, ptr + 4);
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
  // Even if it's the same on all ROM versions, this acts as a basic sanity check to make sure
  // the patch is replacing the actual versionInfoInit
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

  // Initialize static values
  // ROM version
  if (settings.romver[0] != '\0') {
    memcpy(&romverValue[6], settings.romver, 14);
  } else {
    romverValue[0] = '-';  // Put placeholer value
    romverValue[1] = '\0'; // Put placeholer value
  }

  // Mechacon revision
  if (settings.romver[0] != '\0')
    strcpy(mechaconRev, settings.mechaconRev);
  else
    mechaconRev[0] = '-';
}
