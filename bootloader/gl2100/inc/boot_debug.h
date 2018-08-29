#ifndef BOOT_DEBUG_H
#define BOOT_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

void log_output(const char* msg);

#if !defined(RELEASE_BUILD) && !defined(DEBUG_BUILD)
#warning  "Defaulting to Release Build"
#define RELEASE_BUILD
#undef  DEBUG_BUILD
#endif

#if defined(DEBUG_BUILD)
#define BOOT_DEBUG        log_output
#else
#define BOOT_DEBUG
#endif


#ifdef __cplusplus
}
#endif


#endif //BOOT_MODE_H
