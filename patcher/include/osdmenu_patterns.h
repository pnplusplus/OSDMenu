#ifndef _OSDMENU_PATTERNS_H_
#define _OSDMENU_PATTERNS_H_
// Additional patch patterns for osdmenu-launcher
#include <stdint.h>

// Pattern for injecting custom entries into the Version submenu
static uint32_t patternVersionInit[] = {
    // Search pattern in the OSD pad handler table (at address 0x208800 for ROMVER 0200E20040614):
    0x00000000, //     nop
                // case 0x16:
    0x0c000000, //     jal versionInfoInit — target function that inits version submenu strings
    0x3c10001f, //     lui s0,0x001F
    0x1000ffad, //     beq zero, zero, <switch>
    0x00000000, //     nop
};
static uint32_t patternVersionInit_mask[] = {0xffffffff, 0xfc000000, 0xffffffff, 0xffffffff, 0xffffffff};

// Pattern for getting the Version submenu string table address from the versionInfoInit function.
// Address will point to the first string location ("Console")
// The table seems to have a fixed location at address 0x1f1238 in ROM versions 1.20-2.50
static uint32_t patternVersionStringTable[] = {
    // Search pattern in the versionInfoInit function that points to the string table address
    0x3c030000, //     lui  v1, <top address bytes>
    0x00000018, //     mult ??,??,??
    0x34630000, //     ori  v1,v1, <bottom address bytes>
};
static uint32_t patternVersionStringTable_mask[] = {0xffff0000, 0x00000018, 0xffff0000};

#endif
