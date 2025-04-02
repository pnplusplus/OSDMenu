#include "common.h"
#include <ctype.h>
#include <init.h>
#include <ps2sdkapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int handleQuickboot(char *cnfPath) {
  char *ext = strrchr(cnfPath, '.');
  if (!ext || ((strcmp(ext, ".ELF")) && (strcmp(ext, ".elf"))))
    return -ENOENT;

  // Replace .ELF extension with .CNF
  ext[1] = 'C';
  ext[2] = 'N';
  ext[3] = 'F';

  // Open the config file
  FILE *file = fopen(cnfPath, "r");
  if (!file) {
    int res;
    // Init modules only if we have to
    if ((res = initModules(Device_MemoryCard)))
      return res;
    else if (!(file = fopen(cnfPath, "r"))) {
      msg("Quickboot: Failed to open %s\n", cnfPath);
      return -ENOENT;
    }
  }

  // Temporary path and argument lists
  linkedStr *targetPaths = NULL;
  linkedStr *targetArgs = NULL;
  int targetArgc = 1; // argv[0] is the ELF path

  char lineBuffer[PATH_MAX] = {0};
  char *valuePtr = NULL;
  while (fgets(lineBuffer, sizeof(lineBuffer), file)) { // fgets returns NULL if EOF or an error occurs
    // Find the start of the value
    valuePtr = strchr(lineBuffer, '=');
    if (!valuePtr)
      continue;
    *valuePtr = '\0';

    // Trim whitespace and terminate the value
    do {
      valuePtr++;
    } while (isspace((int)*valuePtr));
    valuePtr[strcspn(valuePtr, "\r\n")] = '\0';

    if (!strncmp(lineBuffer, "path", 4)) {
      if ((strlen(valuePtr) > 0))
        targetPaths = addStr(targetPaths, valuePtr);

      continue;
    }
    if (!strncmp(lineBuffer, "arg", 3)) {
      if ((strlen(valuePtr) > 0)) {
        targetArgs = addStr(targetArgs, valuePtr);
        targetArgc++;
      }
      continue;
    }
  }

  // Build argv, freeing targetArgs
  char **targetArgv = malloc(targetArgc * sizeof(char *));
  linkedStr *tlstr;
  if (targetArgs) {
    tlstr = targetArgs;
    for (int i = 1; i < targetArgc; i++) {
      targetArgv[i] = tlstr->str;
      tlstr = tlstr->next;
      free(targetArgs);
      targetArgs = tlstr;
    }
    free(targetArgs);
  }

  // Try every path
  tlstr = targetPaths;
  while (tlstr) {
    targetArgv[0] = tlstr->str;
    // If target path is valid, it'll never return from launchPath
    launchPath(targetArgc, targetArgv);
    free(tlstr->str);
    tlstr = tlstr->next;
    free(targetPaths);
    targetPaths = tlstr;
  }
  free(targetPaths);

  msg("Quickboot: all paths have been tried\n");
  return -ENODEV;
}
