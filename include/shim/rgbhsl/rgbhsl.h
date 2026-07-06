/* lowercase include shim for savers using <rgbhsl/rgbhsl.h> (helios).
 * Lives under include/shim/ so it never case-collides with
 * libs/rsmath/src/Rgbhsl/Rgbhsl.h on case-insensitive filesystems. */
#ifndef RGBHSL_SHIM_H
#define RGBHSL_SHIM_H
#include <Rgbhsl/Rgbhsl.h>
#endif
