#ifndef _COMMON_H_
#define _COMMON_H_

#include <debug.h>

// Enum for supported devices
typedef enum {
  Device_None = 0,
  Device_Basic = (1 << 0),
  Device_MemoryCard = (1 << 1),
  Device_MMCE = (1 << 2),
  // BDM
  Device_ATA = (1 << 3),
  Device_USB = (1 << 4),
  Device_MX4SIO = (1 << 5),
  Device_iLink = (1 << 6),
  Device_UDPBD = (1 << 7),
  Device_BDM = Device_ATA | Device_USB | Device_MX4SIO | Device_iLink | Device_UDPBD, // For loading common BDM modules
  //
  Device_PFS = (1 << 8),
  Device_CDROM = (1 << 9),
} DeviceType;

typedef enum {
  PAL_640_512_32,
  NTSC_640_448_32
} GSVideoMode;


// Prints a message to the screen and console
void msg(const char *str, ...);

// Prints a message to the screen and console and exits
void fail(const char *str, ...);

// Tests if file exists by opening it
int tryFile(char *filepath);

// Attempts to launch ELF from device and path in argv[0]
int launchPath(int argc, char *argv[]);

// getCNFString is the main CNF parser called for each CNF variable in a CNF file.
// Input and output data is handled via its pointer parameters.
// The return value flags 'false' when no variable is found. (normal at EOF)
int getCNFString(char **cnfPos, char **name, char **value);

#endif
