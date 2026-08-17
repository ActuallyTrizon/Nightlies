#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "wrappedlibs.h"

#include "debug.h"
#include "wrapper.h"
#include "bridge.h"
#include "librarian/library_private.h"
#include "x64emu.h"
#include "callback.h"

#define LIBNAME libaaudio
const char* libaaudioName = "libaaudio.so";

#include "generated/wrappedlibaaudiotypes.h"

#include "wrappercallback.h"

// ---- Callback slot pools -----------------------------------------------------
// AAudio invokes the data callback on its own real-time audio thread and the
// error callback on its disconnect thread. Both threads are native arm64 threads
// libaaudio owns; box64 didn't spawn them. RunFunctionFmt handles the guest
// re-entry (same mechanism wrappedlibasound's snd_async_add_handler uses for
// signal-driven callbacks). Four slots per callback type mirrors ALSA's pool.

#define SUPER() \
GO(0)   \
GO(1)   \
GO(2)   \
GO(3)

// AAudioStream_dataCallback:
//   aaudio_data_callback_result_t (*)(AAudioStream*, void* userData, void* audioData, int32_t numFrames)
//   returns int32; args (void*, void*, void*, int32) -> RunFunctionFmt fmt "pppi"
#define GO(A)                                                                      \
static uintptr_t my_aaudio_data_cb_fct_##A = 0;                                    \
static int my_aaudio_data_cb_##A(void* stream, void* userData, void* audioData, int32_t numFrames) \
{                                                                                  \
    return (int)RunFunctionFmt(my_aaudio_data_cb_fct_##A, "pppi",                  \
                               stream, userData, audioData, numFrames);            \
}
SUPER()
#undef GO
static void* findDataCbFct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct)) return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_aaudio_data_cb_fct_##A == (uintptr_t)fct) return my_aaudio_data_cb_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_aaudio_data_cb_fct_##A == 0) { my_aaudio_data_cb_fct_##A = (uintptr_t)fct; return my_aaudio_data_cb_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for AAudio Data callback\n");
    return NULL;
}

// AAudioStream_errorCallback:
//   void (*)(AAudioStream*, void* userData, aaudio_result_t error)
//   returns void; args (void*, void*, int32) -> RunFunctionFmt fmt "ppi"
#define GO(A)                                                                     \
static uintptr_t my_aaudio_err_cb_fct_##A = 0;                                    \
static void my_aaudio_err_cb_##A(void* stream, void* userData, int32_t error)     \
{                                                                                 \
    RunFunctionFmt(my_aaudio_err_cb_fct_##A, "ppi", stream, userData, error);     \
}
SUPER()
#undef GO
static void* findErrorCbFct(void* fct)
{
    if(!fct) return fct;
    if(GetNativeFnc((uintptr_t)fct)) return GetNativeFnc((uintptr_t)fct);
    #define GO(A) if(my_aaudio_err_cb_fct_##A == (uintptr_t)fct) return my_aaudio_err_cb_##A;
    SUPER()
    #undef GO
    #define GO(A) if(my_aaudio_err_cb_fct_##A == 0) { my_aaudio_err_cb_fct_##A = (uintptr_t)fct; return my_aaudio_err_cb_##A; }
    SUPER()
    #undef GO
    printf_log(LOG_NONE, "Warning, no more slot for AAudio Error callback\n");
    return NULL;
}

#undef SUPER

// ---- GOM impls: swap in the trampoline before delegating --------------------

EXPORT void my_AAudioStreamBuilder_setDataCallback(x64emu_t* emu, void* builder, void* callback, void* userData)
{
    (void)emu;
    my->AAudioStreamBuilder_setDataCallback(builder, findDataCbFct(callback), userData);
}

EXPORT void my_AAudioStreamBuilder_setErrorCallback(x64emu_t* emu, void* builder, void* callback, void* userData)
{
    (void)emu;
    my->AAudioStreamBuilder_setErrorCallback(builder, findErrorCbFct(callback), userData);
}

#include "wrappedlib_init.h"
