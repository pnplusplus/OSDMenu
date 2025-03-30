// Defines paths used by both the patcher and the launcher
#ifndef _DEFAULTS_H_
#define _DEFAULTS_H_

// All files must be placed on the memory card.
// mc? paths are also supported.
#ifndef CONF_PATH
#define CONF_PATH "mc0:/SYS-CONF/OSDMENU.CNF"
#endif

#ifndef LAUNCHER_PATH
#define LAUNCHER_PATH "mc0:/BOOT/launcher.elf"
#endif

#ifndef DKWDRV_PATH
#define DKWDRV_PATH "mc0:/BOOT/DKWDRV.ELF"
#endif

#endif
