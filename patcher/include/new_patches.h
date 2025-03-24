#ifndef _NEW_PATCHES_H_
#define _NEW_PATCHES_H_
// Additional patch patterns for osdmenu-launcher
#include <stdint.h>

// Extends version menu with custom entries
void patchVersionInfo(uint8_t *osd);

#endif
