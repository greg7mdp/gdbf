// ------------- Configuration -------------
#define GDBF_PROFILING_BUFFER_BYTES (64 * 1024 * 1024)
#define GDBF_PROFILING_CLOCK CLOCK_MONOTONIC
// #define GDBF_PROFILING_CLOCK CLOCK_THREAD_CPUTIME_ID
// -----------------------------------------

/*
------------------------------------------------------------------------------
This file is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2021 nakst
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#ifdef __cplusplus
   #define GDBF_PROFILING_EXTERN extern "C"
#else
   #define GDBF_PROFILING_EXTERN
#endif

typedef struct GdbfProfilingEntry {
   void*    thisFunction;
   uint64_t timeStamp;
} GdbfProfilingEntry;

static __thread bool gdbfProfilingEnabledOnThisThread;
static bool          gdbfProfilingEnabled;
static size_t        gdbfProfilingBufferSize;
GdbfProfilingEntry*  gdbfProfilingBuffer;
uintptr_t            gdbfProfilingBufferPosition;
uint64_t             gdbfProfilingTicksPerMs;

#define GDBF_PROFILING_FUNCTION(_exiting)                                                                   \
   (void)callSite;                                                                                          \
                                                                                                            \
   if (gdbfProfilingBufferPosition < gdbfProfilingBufferSize && gdbfProfilingEnabledOnThisThread) {         \
      GdbfProfilingEntry* entry = (GdbfProfilingEntry*)&gdbfProfilingBuffer[gdbfProfilingBufferPosition++]; \
      entry->thisFunction       = thisFunction;                                                             \
      struct timespec time;                                                                                 \
      clock_gettime(GDBF_PROFILING_CLOCK, &time);                                                           \
      entry->timeStamp = ((uint64_t)time.tv_sec * 1000000000 + time.tv_nsec) | ((uint64_t)_exiting << 63);  \
   }

GDBF_PROFILING_EXTERN __attribute__((no_instrument_function)) void __cyg_profile_func_enter(void* thisFunction,
                                                                                            void* callSite) {
   GDBF_PROFILING_FUNCTION(0);
}

GDBF_PROFILING_EXTERN __attribute__((no_instrument_function)) void __cyg_profile_func_exit(void* thisFunction,
                                                                                           void* callSite) {
   GDBF_PROFILING_FUNCTION(1);
}

GDBF_PROFILING_EXTERN __attribute__((no_instrument_function)) void GdbfProfilingStart() {
   assert(!gdbfProfilingEnabled);
   assert(!gdbfProfilingEnabledOnThisThread);
   assert(gdbfProfilingBufferSize);
   gdbfProfilingEnabled             = true;
   gdbfProfilingEnabledOnThisThread = true;
   gdbfProfilingBufferPosition      = 0;
}

GDBF_PROFILING_EXTERN __attribute__((no_instrument_function)) void GdbfProfilingStop() {
   assert(gdbfProfilingEnabled);
   assert(gdbfProfilingEnabledOnThisThread);
   gdbfProfilingEnabled             = false;
   gdbfProfilingEnabledOnThisThread = false;
}

__attribute__((constructor)) __attribute__((no_instrument_function)) void GdbfProfilingInitialise() {
   gdbfProfilingBufferSize = GDBF_PROFILING_BUFFER_BYTES / sizeof(GdbfProfilingEntry);
   gdbfProfilingBuffer     = (GdbfProfilingEntry*)malloc(GDBF_PROFILING_BUFFER_BYTES);
   gdbfProfilingTicksPerMs = 1000000;
   assert(gdbfProfilingBufferSize && gdbfProfilingBuffer);
}
