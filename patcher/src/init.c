#include <fcntl.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <kernel.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>

// Wipes user memory
void wipeUserMem(void) {
  for (int i = 0x100000; i < 0x02000000; i += 64) {
    asm("\tsq $0, 0(%0) \n"
        "\tsq $0, 16(%0) \n"
        "\tsq $0, 32(%0) \n"
        "\tsq $0, 48(%0) \n" ::"r"(i));
  }
}

// Loads IOP modules
int initModules(void) {
  sceSifInitRpc(0);
  while (!SifIopReset("", 0)) {
  };
  while (!SifIopSync()) {
  };

  sceSifInitRpc(0);

  int ret;
  // Apply patches required to load executables from memory cards
  if ((ret = sbv_patch_enable_lmb()))
    return ret;
  if ((ret = sbv_patch_disable_prefix_check()))
    return ret;
  if ((ret = sbv_patch_fileio()))
    return ret;

  if ((ret = SifLoadModule("rom0:SIO2MAN", 0, NULL)) < 0)
    return ret;
  if ((ret = SifLoadModule("rom0:MCMAN", 0, NULL)) < 0)
    return ret;
  if ((ret = SifLoadModule("rom0:MCSERV", 0, NULL)) < 0)
    return ret;

  fioInit();
  return 0;
}

// Resets IOP before loading OSDSYS
void resetModules(void) {
  while (!SifIopReset("rom0:UDNL rom0:EELOADCNF", 0)) {
  };
  while (!SifIopSync()) {
  };

  SifExitIopHeap();
  SifLoadFileExit();
  sceSifExitRpc();
  sceSifExitCmd();

  FlushCache(0);
  FlushCache(2);
  fioInit();
}
