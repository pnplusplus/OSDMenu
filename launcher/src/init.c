
#include "init.h"
#include "common.h"
#include <ctype.h>
#include <fcntl.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <libpwroff.h>
#include <loadfile.h>
#include <ps2sdkapi.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macros for loading embedded IOP modules
#define IRX_DEFINE(mod)                                                                                                                              \
  extern unsigned char mod##_irx[] __attribute__((aligned(16)));                                                                                     \
  extern uint32_t size_##mod##_irx

// Defines moduleList entry for embedded module
#define INT_MODULE(mod, argFunc, deviceType) {#mod, mod##_irx, &size_##mod##_irx, 0, NULL, deviceType, argFunc}

// Embedded IOP modules
IRX_DEFINE(iomanX);
IRX_DEFINE(fileXio);
IRX_DEFINE(poweroff);
IRX_DEFINE(sio2man);
IRX_DEFINE(mcman);
IRX_DEFINE(mcserv);
IRX_DEFINE(mmceman);
IRX_DEFINE(ps2dev9);
IRX_DEFINE(bdm);
IRX_DEFINE(bdmfs_fatfs);
IRX_DEFINE(ata_bd);
IRX_DEFINE(usbd_mini);
IRX_DEFINE(usbmass_bd_mini);
IRX_DEFINE(mx4sio_bd_mini);
IRX_DEFINE(iLinkman);
IRX_DEFINE(IEEE1394_bd_mini);
IRX_DEFINE(smap_udpbd);
IRX_DEFINE(ps2atad);
IRX_DEFINE(ps2hdd);
IRX_DEFINE(ps2fs);

// Function used to initialize module arguments.
// Must set argLength and return non-null pointer to a argument string if successful.
// Returned pointer must point to dynamically allocated memory
typedef char *(*moduleArgFunc)(uint32_t *argLength);

typedef struct ModuleListEntry {
  char *name;                     // Module name
  unsigned char *irx;             // Pointer to IRX module
  uint32_t *size;                 // IRX size. Uses pointer to avoid compilation issues with internal modules
  uint32_t argLength;             // Total length of argument string
  char *argStr;                   // Module arguments
  DeviceType type;                // Target device
  moduleArgFunc argumentFunction; // Function used to initialize module arguments
} ModuleListEntry;

// Argument functions
char *initSMAPArguments(uint32_t *argLength);
char *initPS2HDDArguments(uint32_t *argLength);
char *initPS2FSArguments(uint32_t *argLength);

// List of modules to load
static ModuleListEntry moduleList[] = {
    INT_MODULE(iomanX, NULL, Device_Basic),
    INT_MODULE(fileXio, NULL, Device_Basic),
    INT_MODULE(sio2man, NULL, Device_MemoryCard | Device_MMCE | Device_UDPBD | Device_CDROM),
    INT_MODULE(mcman, NULL, Device_MemoryCard | Device_UDPBD | Device_CDROM),
    INT_MODULE(mcserv, NULL, Device_MemoryCard | Device_UDPBD | Device_CDROM),
    INT_MODULE(mmceman, NULL, Device_MMCE),
    // DEV9
    INT_MODULE(ps2dev9, NULL, Device_ATA | Device_UDPBD | Device_PFS),
    // BDM with exFAT driver
    INT_MODULE(bdm, NULL, Device_BDM),
    INT_MODULE(bdmfs_fatfs, NULL, Device_BDM),
    // ATA
    INT_MODULE(ata_bd, NULL, Device_ATA),
    // USB
    INT_MODULE(usbd_mini, NULL, Device_USB),
    INT_MODULE(usbmass_bd_mini, NULL, Device_USB),
    // MX4SIO
    INT_MODULE(mx4sio_bd_mini, NULL, Device_MX4SIO),
    // iLink
    INT_MODULE(iLinkman, NULL, Device_iLink),
    INT_MODULE(IEEE1394_bd_mini, NULL, Device_iLink),
    // UDPBD
    INT_MODULE(smap_udpbd, &initSMAPArguments, Device_UDPBD),
    // PFS
    INT_MODULE(ps2atad, NULL, Device_PFS),
    INT_MODULE(ps2hdd, &initPS2HDDArguments, Device_PFS),
    INT_MODULE(ps2fs, &initPS2FSArguments, Device_PFS),
};
#define MODULE_COUNT sizeof(moduleList) / sizeof(ModuleListEntry)

// Loads module, executing argument function if it's present
int loadModule(ModuleListEntry *mod);

// Reboots the console
void rebootPS2() {
  sceSifInitRpc(0);
  while (!SifIopReset("", 0)) {
  };
  while (!SifIopSync()) {
  };
  sceSifExitRpc();
  SifLoadFileInit();
  LoadExecPS2("rom0:OSDSYS", 0, NULL);
}

// Shuts down the console.
// Needs initModules(Device_Basic) to be called first
void shutdownPS2() {
  sceSifInitRpc(0);
  ModuleListEntry poweroff = INT_MODULE(poweroff, NULL, 0);
  loadModule(&poweroff);
  poweroffShutdown();
}

// Initializes IOP modules for given device type
int initModules(DeviceType device) {
  int ret = 0;

  // Initialize the RPC manager
  sceSifInitRpc(0);
  printf("Rebooting IOP\n");
  while (!SifIopReset("", 0)) {
  };
  while (!SifIopSync()) {
  };

  // Initialize the RPC manager
  sceSifInitRpc(0);

  // Apply patches required to load modules from EE RAM
  if ((ret = sbv_patch_enable_lmb()))
    return ret;
  if ((ret = sbv_patch_disable_prefix_check()))
    return ret;

  // Load modules
  for (int i = 0; i < MODULE_COUNT; i++) {
    if (!(device & moduleList[i].type) && (moduleList[i].type != Device_Basic))
      continue;

    if ((ret = loadModule(&moduleList[i]))) {
      msg("ERROR: Failed to initialize module %s: %d\n", moduleList[i].name, ret);
      return ret;
    }

    // Clean up arguments
    if (moduleList[i].argStr != NULL)
      free(moduleList[i].argStr);
  }
  return 0;
}

// Loads module, executing argument function if it's present
int loadModule(ModuleListEntry *mod) {
  int ret, iopret = 0;

  printf("Loading %s\n", mod->name);

  // If module has an arugment function, execute it
  if (mod->argumentFunction != NULL) {
    mod->argStr = mod->argumentFunction(&mod->argLength);
    if (mod->argStr == NULL) {
      return -1;
    }
  }

  ret = SifExecModuleBuffer(mod->irx, *mod->size, mod->argLength, mod->argStr, &iopret);
  if (ret >= 0)
    ret = 0;
  if (iopret == 1)
    ret = iopret;

  return ret;
}

// Argument functions

// Builds IP address argument for SMAP module
// using mc?:SYS-CONF/IPCONFIG.DAT from memory card
char *initSMAPArguments(uint32_t *argLength) {
  // Try to get IP from IPCONFIG.DAT
  // The 'X' in "mcX" will be replaced with memory card number
  static char ipconfigPath[] = "mcX:/SYS-CONF/IPCONFIG.DAT";

  int ipconfigFd, count;
  char ipAddr[16]; // IP address will not be longer than 15 characters
  for (char i = '0'; i < '2'; i++) {
    ipconfigPath[2] = i;
    // Attempt to open IPCONFIG.DAT
    ipconfigFd = open(ipconfigPath, O_RDONLY);
    if (ipconfigFd >= 0) {
      count = read(ipconfigFd, ipAddr, sizeof(ipAddr) - 1);
      close(ipconfigFd);
      break;
    }
  }

  if ((ipconfigFd < 0) || (count < sizeof(ipAddr) - 1)) {
    msg("ERROR: Failed to read IP address from IPCONFIG.DAT\n");
    return NULL;
  }

  count = 0; // Reuse count as line index
  // In case IP address is shorter than 15 chars
  while (!isspace((unsigned char)ipAddr[count])) {
    // Advance index until we read a whitespace character
    count++;
  }

  char ipArg[19]; // 15 bytes for IP string + 3 bytes for 'ip='
  *argLength = 19;
  char *argStr = calloc(sizeof(char), 19);
  snprintf(argStr, sizeof(ipArg), "ip=%s", ipAddr);
  return argStr;
}

// up to 4 descriptors, 20 buffers
static char ps2hddArguments[] = "-o"
                                "\0"
                                "4"
                                "\0"
                                "-n"
                                "\0"
                                "20";
// Sets arguments for PS2HDD modules
char *initPS2HDDArguments(uint32_t *argLength) {
  *argLength = sizeof(ps2hddArguments);

  char *argStr = malloc(sizeof(ps2hddArguments));
  memcpy(argStr, ps2hddArguments, sizeof(ps2hddArguments));
  return argStr;
}

// up to 10 descriptors, 40 buffers
char ps2fsArguments[] = "-o"
                        "\0"
                        "10"
                        "\0"
                        "-n"
                        "\0"
                        "40";
// Sets arguments for PS2HDD modules
char *initPS2FSArguments(uint32_t *argLength) {
  *argLength = sizeof(ps2fsArguments);

  char *argStr = malloc(sizeof(ps2fsArguments));
  memcpy(argStr, ps2fsArguments, sizeof(ps2fsArguments));
  return argStr;
}
