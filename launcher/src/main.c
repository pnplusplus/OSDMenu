#include "common.h"
#include "handlers.h"
#include "loader.h"
#include <fcntl.h>
#include <kernel.h>
#include <ps2sdkapi.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fail("Invalid number of arguments\n");
    return -1;
  }

  // Remove launcher path from arguments
  argc--;
  argv = &argv[1];

  if (!strncmp("fmcb", argv[0], 4)) {
    fail("Failed to launch %s: %d", argv[0], handleFMCB(argc, argv));
  } else {
    fail("Failed to launch %s: %d", argv[0], launchPath(argc, argv));
  }

  // Try to launch
  fail("Failed to launch %s: %d", argv[0], launchPath(argc, argv));
}
