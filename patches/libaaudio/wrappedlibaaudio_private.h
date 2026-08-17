#if !(defined(GO) && defined(GOM) && defined(GO2) && defined(DATA))
#error Meh....
#endif

// Builder lifecycle
GO(AAudio_createStreamBuilder,                     iFp)
GO(AAudioStreamBuilder_delete,                     iFp)
GO(AAudioStreamBuilder_openStream,                 iFpp)

// Builder setters (all: void set(builder*, int32/enum))
GO(AAudioStreamBuilder_setBufferCapacityInFrames,  vFpi)
GO(AAudioStreamBuilder_setChannelCount,            vFpi)
GO(AAudioStreamBuilder_setDirection,               vFpi)
GO(AAudioStreamBuilder_setFormat,                  vFpi)
GO(AAudioStreamBuilder_setPerformanceMode,         vFpi)
GO(AAudioStreamBuilder_setSampleRate,              vFpi)
GO(AAudioStreamBuilder_setSharingMode,             vFpi)
GO(AAudioStreamBuilder_setUsage,                   vFpi)   // API 28+

// Builder setters that take a guest function pointer -> need trampoline
GOM(AAudioStreamBuilder_setDataCallback,           vFEppp)
GOM(AAudioStreamBuilder_setErrorCallback,          vFEppp)

// Stream control + getters
GO(AAudioStream_close,                             iFp)
GO(AAudioStream_getBufferCapacityInFrames,         iFp)
GO(AAudioStream_getBufferSizeInFrames,             iFp)
GO(AAudioStream_getFramesPerBurst,                 iFp)
GO(AAudioStream_getPerformanceMode,                iFp)
GO(AAudioStream_getSharingMode,                    iFp)
GO(AAudioStream_getXRunCount,                      iFp)
GO(AAudioStream_requestStart,                      iFp)
GO(AAudioStream_requestStop,                       iFp)
GO(AAudioStream_setBufferSizeInFrames,             iFpi)
