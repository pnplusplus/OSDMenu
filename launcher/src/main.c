#include "common.h"
#include "handlers.h"
#include "loader.h"
#include <fcntl.h>
#include <kernel.h>
#include <ps2sdkapi.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

// Reduce binary size by disabling unneeded functionality
void _libcglue_timezone_update() {}
void _libcglue_rtc_update() {}
PS2_DISABLE_AUTOSTART_PTHREAD();

int main(int argc, char *argv[]) {
  if (argc < 2) {
    // Try to quickboot with paths from .CNF located at the current working directory
    fail("Quickboot failed: %d", handleQuickboot(argv[0]));
  }

  // Remove launcher path from arguments
  argc--;
  argv = &argv[1];

#ifdef FMCB
  if (!strncmp("fmcb", argv[0], 4)) {
    fail("Failed to launch %s: %d", argv[0], handleFMCB(argc, argv));
  }
#endif

  fail("Failed to launch %s: %d", argv[0], launchPath(argc, argv));
}
