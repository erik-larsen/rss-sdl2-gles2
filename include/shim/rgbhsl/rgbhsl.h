/* lowercase include shim for savers using <rgbhsl/rgbhsl.h> (helios).
 * Lives under include/shim/ so it never case-collides with
 * libs/rsmath/src/Rgbhsl/Rgbhsl.h on case-insensitive filesystems.
 * The real header is included by exact relative path (no include-path
 * search) so clang's -Wnonportable-include-path stays quiet everywhere. */
#ifndef RGBHSL_SHIM_H
#define RGBHSL_SHIM_H
#include "../../../libs/rsmath/src/Rgbhsl/Rgbhsl.h"
#endif
