#include "common.h"
#include "init.h"
#include "loader.h"
#include <fcntl.h>
#include <ps2sdkapi.h>
#include <stdio.h>
#include <string.h>

char bdmMountpoint[] = "mass?:";
#define BDM_MAX_DEVICES 10

// Launches ELF from BDM device
int handleBDM(DeviceType device, int argc, char *argv[]) {
  if ((argv[0] == 0) || (strlen(argv[0]) < 5))
    msg("BDM: invalid argument\n");

  // Get relative ELF path from argv[0]
  char *elfPath = argv[0];
  elfPath = strchr(elfPath, ':');
  if (!elfPath)
    msg("BDM: no file path found\n");

  elfPath++;

  // Build ELF path
  char pathbuffer[PATH_MAX];
  strcpy(pathbuffer, bdmMountpoint);
  strncat(pathbuffer, elfPath, PATH_MAX - sizeof(bdmMountpoint));

  // Initialize device modules
  int res = initModules(device);
  if (res)
    return res;

  // Try all BDM devices while decreasing the number of wait
  // attempts for each consecutive device to reduce init times
  int delayAttempts = 20; // Max number of attempts
  for (int i = 0; i < BDM_MAX_DEVICES; i++) {
    // Build mountpoint path
    bdmMountpoint[4] = i + '0';
    pathbuffer[4] = i + '0';
    while (delayAttempts != 0) {
      // Try to open the mountpoint to make sure the device exists
      res = open(bdmMountpoint, O_DIRECTORY | O_RDONLY);
      if (res >= 0) {
        // If mountpoint is available
        close(res);

        // Jump to launch if file exists
        if (!(res = tryFile(pathbuffer)))
          goto found;
        // Else, try next device
        break;
      }
      // If the mountpoitnt is unavailable, delay and decrement the number of wait attempts
      sleep(1);
      delayAttempts--;
    }
    // No more mountpoints available
    if (res < 0)
      break;
  }
  return -ENODEV;

found:
  argv[0] = pathbuffer;
  return LoadELFFromFile(argc, argv);
}
