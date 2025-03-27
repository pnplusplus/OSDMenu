#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdint.h>

#define NEWITEMS 100 // the number of max added menu items in osdsys

// Patcher settings struct, contains all configurable patch settings and menu items
typedef struct {
  uint8_t mcSlot;           // Memory card slot contaning currently loaded FREEMCB.CNF
  uint8_t hackedOSDSYS;     // Enable/Disable OSDSYS hacking
  uint8_t skipHDD;          // Enable/Disable HDD Update check
  uint8_t skipDisc;         // Enable/Disable disc boot while inserting them while OSDSYS is loaded
  uint8_t skipLogo;         // Enable/Disable Sony Entertainment logo while loading OSDSYS
  uint8_t goToInnerBrowser; // Enable/Disable inner_browser while loading OSDSYS
  uint8_t scrollMenu;       // Enable/Disable scrolling menu

  // Disc launch parameters
  uint8_t skipPS2LOGO;   // Enable/Disable PS2LOGO
  uint8_t disableGameID; // Enable/Disable visual game ID
  uint8_t useDKWDRV;     // Enable/Disable using DKWDRV for PS1 discs

  char *videoMode;             // OSDSYS Video mode : AUTO, PAL or NTSC
  int menuX;                   // Menu X coordinate (menu center)
  int menuY;                   // Menu Y coordinate (menu center), only for scroll menu
  int enterX;                  // "Enter" button X coordinate (at main OSDSYS menu)
  int enterY;                  // "Enter" button Y coordinate (at main OSDSYS menu)
  int versionX;                // "Version" button X coordinate (at main OSDSYS menu)
  int versionY;                // "Version" button Y coordinate (at main OSDSYS menu)
  int cursorMaxVelocity;       // The cursors movement amplitude, only for scroll menu
  int cursorAcceleration;      // The cursors speed, only for scroll menu
  char *leftCursor;            // The left cursor text, only for scroll menu
  char *rightCursor;           // The right cursor text, only for scroll menu
  char *menuDelimiterTop;      // The top menu delimiter text, only for scroll menu
  char *menuDelimiterBottom;   // The bottom menu delimiter text, only for scroll menu
  uint32_t colorSelected[4];   // The menu items color when selected
  uint32_t colorUnselected[4]; // The menu items color when not selected
  int displayedItems;          // The number of menu items displayed, only for scroll menu

  char *menuItemName[NEWITEMS]; // Menu items text
  int menuItemIdx[NEWITEMS];    // Item index in the config file
  int menuItemCount;            // Total number of valid menu items

  char *launcherPath; // Path to launcher ELF
  char *dkwdrvPath;   // Path to DKWDRV

  // Variables
  char romver[15]; // ROMVER string, initialized before patching
} PatcherSettings;

// Stores patcher settings and OSDSYS menu items
extern PatcherSettings settings;
// Pointer to loaded CNF
extern char *pCNF;

int loadConfig(void);
void initConfig(void);

#endif
