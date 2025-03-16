#ifndef _INIT_H_
#define _INIT_H_

#include "common.h"

// Reboots the console
void rebootPS2();

// Shuts down the console. Needs initModules(Device_Basic) to be called first.
void shutdownPS2();

// Initializes IOP modules for given device type
int initModules(DeviceType device);

#endif
