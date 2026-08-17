#if !(defined(GO) && defined(GOM) && defined(GO2) && defined(DATA))
#error Meh....
#endif

// Android liblog — only symbol our winedirectaudio.so pulls is
// __android_log_print (variadic). Wrapped via the same printf pattern box64
// uses in libc: fixed args explicit + trailing V for varargs.
// Signature: iFEippV = int(x64emu_t*, int prio, char* tag, char* fmt, ...)
GOM(__android_log_print, iFEippV)
