#include "handlers.h"
#include "init.h"
#include <debug.h>
#include <fcntl.h>
#include <kernel.h>
#include <ps2sdkapi.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int isScreenInited = 0;

void initScreen() {
  if (isScreenInited)
    return;

  init_scr();
  scr_setCursor(0);
  scr_printf(".\n\n\n\n"); // To avoid messages being hidden by overscan
  isScreenInited = 1;
}

// Prints a message to the screen and console
void msg(const char *str, ...) {
  va_list args;
  va_start(args, str);

  initScreen();

  scr_vprintf(str, args);

  va_end(args);
}

// Prints a message to the screen and console and exits
void fail(const char *str, ...) {
  va_list args;
  va_start(args, str);

  initScreen();

  scr_vprintf(str, args);

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
  if (!strncmp("mc", argv[0], 2)) {
    ret = handleMC(argc, argv);
#ifdef MMCE
  } else if (!strncmp("mmce", argv[0], 4)) {
    ret = handleMMCE(argc, argv);
#endif
#ifdef USB
  } else if (!strncmp("mass", argv[0], 4) || !strncmp("usb", argv[0], 3)) {
    ret = handleBDM(Device_USB, argc, argv);
#endif
#ifdef ATA
  } else if (!strncmp("ata", argv[0], 3)) {
    ret = handleBDM(Device_ATA, argc, argv);
#endif
#ifdef MX4SIO
  } else if (!strncmp("mx4sio", argv[0], 6)) {
    ret = handleBDM(Device_MX4SIO, argc, argv);
#endif
#ifdef ILINK
  } else if (!strncmp("ilink", argv[0], 5)) {
    ret = handleBDM(Device_iLink, argc, argv);
#endif
#ifdef UDPBD
  } else if (!strncmp("udpbd", argv[0], 5)) {
    ret = handleBDM(Device_UDPBD, argc, argv);
#endif
#ifdef APA
  } else if (!strncmp("hdd", argv[0], 3)) {
    ret = handlePFS(argc, argv);
#endif
#ifdef CDROM
  } else if (!strncmp("cdrom", argv[0], 5)) {
    ret = handleCDROM(argc, argv);
#endif
  } else {
    return -ENODEV;
  }

  return ret;
}

// Adds a new string to linkedStr and returns
linkedStr *addStr(linkedStr *lstr, char *str) {
  linkedStr *newLstr = malloc(sizeof(linkedStr));
  newLstr->str = strdup(str);
  newLstr->next = NULL;

  if (lstr) {
    linkedStr *tLstr = lstr;
    // If lstr is not null, go to the last element and
    // link the new element
    while (tLstr->next)
      tLstr = tLstr->next;

    tLstr->next = newLstr;
    return lstr;
  }

  // Else, return the new element as the first one
  return newLstr;
}

// Frees all elements of linkedStr
void freeLinkedStr(linkedStr *lstr) {
  if (!lstr)
    return;

  linkedStr *tPtr = lstr;
  while (lstr->next) {
    free(lstr->str);
    tPtr = lstr->next;
    free(lstr);
    lstr = tPtr;
  }
  free(lstr->str);
  free(lstr);
}
