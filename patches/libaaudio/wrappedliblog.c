#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>

#include "wrappedlibs.h"

#include "debug.h"
#include "wrapper.h"
#include "bridge.h"
#include "librarian/library_private.h"
#include "x64emu.h"
#include "emu/x64emu_private.h"
#include "myalign.h"

#define LIBNAME liblog
const char* liblogName = "liblog.so";

#include "generated/wrappedliblogtypes.h"

#include "wrappercallback.h"

// __android_log_vprint is the va_list-taking sibling of __android_log_print.
// Resolve at init time via the same liblog handle box64 dlopened for us, so
// box64 itself doesn't need -llog on its link line.
typedef int (*android_log_vprint_t)(int prio, const char* tag, const char* fmt, va_list ap);
static android_log_vprint_t p___android_log_vprint = NULL;

#define ADDED_INIT() do { \
    if (my_lib && my_lib->w.lib) { \
        p___android_log_vprint = (android_log_vprint_t)dlsym(my_lib->w.lib, "__android_log_vprint"); \
    } \
} while (0);

EXPORT int my___android_log_print(x64emu_t* emu, int prio, void* tag, void* fmt, void* b)
{
    if (!p___android_log_vprint) return 0;   // liblog not available; drop the log silently
    // pos=3: prio + tag + fmt are the fixed args; varargs start after fmt.
    // Mirrors wrappedlibc's my_printf pattern (pos=1) and my___printf_chk (pos=2).
    myStackAlign(emu, (const char*)fmt, b, emu->scratch, R_EAX, 3);
    PREPARE_VALIST;
    return p___android_log_vprint(prio, (const char*)tag, (const char*)fmt, VARARGS);
}

#include "wrappedlib_init.h"
