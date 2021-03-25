#ifndef LOOP_COUNT
#error "LOOP_COUNT not set"
#endif

#if __INCLUDE_LEVEL__ <= LOOP_COUNT
LOOP_ELEMENT
// Suppression of "#include nested too deeply" justified because the inclusion
// cylce is terminated by a check of the include level.
// cppcheck-suppress preprocessorErrorDirective
#include "loop.h"
#else
#undef LOOP_COUNT
#undef LOOP_ELEMENT
#endif
