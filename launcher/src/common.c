#include "handlers.h"
#include "init.h"
#include <debug.h>
#include <fcntl.h>
#include <kernel.h>
#include <ps2sdkapi.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int isScreenInited = 0;

void initScreen() {
  if (isScreenInited)
    return;

  init_scr();
  scr_setCursor(0);
  isScreenInited = 1;
}

// Prints a message to the screen and console
void msg(const char *str, ...) {
  va_list args;
  va_start(args, str);

  initScreen();

  scr_vprintf(str, args);
  vprintf(str, args);

  va_end(args);
}

// Prints a message to the screen and console and exits
void fail(const char *str, ...) {
  va_list args;
  va_start(args, str);

  initScreen();

  scr_vprintf(str, args);
  vprintf(str, args);

  va_end(args);

  sleep(10);
  rebootPS2();
}

// Tests if file exists by opening it
int tryFile(char *filepath) {
  int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    return fd;
  }
  close(fd);
  return 0;
}

// Attempts to launch ELF from device and path in argv[0]
int launchPath(int argc, char *argv[]) {
  int ret = 0;
  if (!strncmp("mmce", argv[0], 4)) {
    ret = handleMMCE(argc, argv);
  } else if (!strncmp("mc", argv[0], 2)) {
    ret = handleMC(argc, argv);
  } else if (!strncmp("mass", argv[0], 4) || !strncmp("usb", argv[0], 3)) {
    ret = handleBDM(Device_USB, argc, argv);
  } else if (!strncmp("ata", argv[0], 3)) {
    ret = handleBDM(Device_ATA, argc, argv);
  } else if (!strncmp("mx4sio", argv[0], 6)) {
    ret = handleBDM(Device_MX4SIO, argc, argv);
  } else if (!strncmp("ilink", argv[0], 5)) {
    ret = handleBDM(Device_iLink, argc, argv);
  } else if (!strncmp("udpbd", argv[0], 5)) {
    ret = handleBDM(Device_UDPBD, argc, argv);
  } else if (!strncmp("hdd", argv[0], 3)) {
    ret = handlePFS(argc, argv);
  } else if (!strncmp("cdrom", argv[0], 5)) {
    ret = handleCDROM(argc, argv);
  }

  return ret;
}

// Based on LaunchELF code
// getCNFString is the main CNF parser called for each CNF variable in a CNF file.
// Input and output data is handled via its pointer parameters.
// The return value flags 'false' when no variable is found. (normal at EOF)
int getCNFString(char **cnfPos, char **name, char **value) {
  char *pName, *pValue, *pToken = *cnfPos;

nextLine:
  while ((*pToken <= ' ') && (*pToken > '\0'))
    pToken += 1; // Skip leading whitespace, if any
  if (*pToken == '\0')
    return 0; // Exit at EOF

  pName = pToken;      // Current pos is potential name
  if (*pToken < 'A') { // If line is a comment line
    while ((*pToken != '\r') && (*pToken != '\n') && (*pToken > '\0'))
      pToken += 1; // Seek line end
    goto nextLine; // Go back to try next line
  }

  while ((*pToken >= 'A') || ((*pToken >= '0') && (*pToken <= '9')))
    pToken += 1; // Seek name end
  if (*pToken == '\0')
    return 0; // Exit at EOF

  while ((*pToken <= ' ') && (*pToken > '\0'))
    *pToken++ = '\0'; // Zero and skip post-name whitespace
  if (*pToken != '=')
    return 0;       // Exit (syntax error) if '=' missing
  *pToken++ = '\0'; // Zero '=' (possibly terminating name)

  while ((*pToken <= ' ') && (*pToken > '\0')      // Skip pre-value whitespace, if any
         && (*pToken != '\r') && (*pToken != '\n') // but do not pass the end of the line
         && (*pToken != '\7')                      // allow ctrl-G (BEL) in value
  )
    pToken += 1;
  if (*pToken == '\0')
    return 0;      // Exit at EOF
  pValue = pToken; // Current pos is potential value

  while ((*pToken != '\r') && (*pToken != '\n') && (*pToken != '\0'))
    pToken += 1; // Seek line end
  if (*pToken != '\0')
    *pToken++ = '\0'; // Terminate value (passing if not EOF)
  while ((*pToken <= ' ') && (*pToken > '\0'))
    pToken += 1; // Skip following whitespace, if any

  *cnfPos = pToken; // Set new CNF file position
  *name = pName;    // Set found variable name
  *value = pValue;  // Set found variable value
  return 1;
}
