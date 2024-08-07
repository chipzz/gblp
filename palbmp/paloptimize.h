#ifndef __PALOPTIMIZE_H
#define __PALOPTIMIZE_H

#ifdef __cplusplus
extern "C" {
#endif

// #include <crblib/inc.h>

#include "utypes.h"
#include <stdbool.h>

extern bool palettizePlane24Optimize(ubyte *rgbPlane,int planeLen,ubyte *palette,int palEntries,
		int maxSamples);

	// use maxIterations == 0 or -1 for infinity

#ifdef __cplusplus
}
#endif

#endif
