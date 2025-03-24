#ifndef _PATCHES_H_
#define _PATCHES_H_
#include <stdint.h>

// Applies patches and launches OSDSYS
void launchOSDSYS(void);

// Searches for byte pattern in memory
uint8_t *findPatternWithMask(uint8_t *buf, uint32_t bufsize, uint8_t *bytes, uint8_t *mask, uint32_t len);

// Searches for string in memory
char *findString(const char *string, char *buf, uint32_t bufsize);

// Returns the pointer to OSD string
const char *getStringPointer(const char **strings, uint32_t index);

#endif
