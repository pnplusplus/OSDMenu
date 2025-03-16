#ifndef _INIT_H_
#define _INIT_H_

// Wipes user memory
void wipeUserMem(void);

// Loads IOP modules
int initModules(void);

// Resets IOP before loading OSDSYS
void resetModules();

#endif
