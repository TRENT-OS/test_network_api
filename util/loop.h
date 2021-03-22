#ifndef LOOP_COUNT
#error "LOOP_COUNT not set"
#endif

#if __INCLUDE_LEVEL__ <= LOOP_COUNT
LOOP_ELEMENT
// TODO: check cppcheck warning: #include nested too deeply
// cppcheck-suppress preprocessorErrorDirective
#include "loop.h"
#else
#undef LOOP_COUNT
#undef LOOP_ELEMENT
#endif
