#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/neutron/newlib_stubs.cpp"

#else

/* Define caddr_t as char* */
#include <sys/types.h>
#include <errno.h>
#include <malloc.h>
/* Define abort() */
#include <stdlib.h>


extern "C" {
    void _exit(int status);
} /* extern "C" */
#define abort() _exit(-1)

extern unsigned long link_constructors_location;
extern unsigned long link_constructors_end;

static void call_constructors(unsigned long *start, unsigned long *end) __attribute__((noinline));

static void call_constructors(unsigned long *start, unsigned long *end)
{
    unsigned long *i;
    void (*funcptr)();
    for (i = start; i < end; i++)
    {
        funcptr=(void (*)())(*i);
        funcptr();
    }
}

extern "C" {
    void CallConstructors(void)
    {
        call_constructors(&link_constructors_location, &link_constructors_end);
    }

    void *__dso_handle = NULL;

    /******************************************************
     * System call reference with suggested stubs:
     * http://sourceware.org/newlib/libc.html#Syscalls
     *****************************************************/
    /*
     * _sbrk() -  allocate incr bytes of memory from the heap.
     *
     *            Return a pointer to the memory, or abort if there
     *            is insufficient memory available on the heap.
     *
     *            The heap is all the RAM that exists between _end and
     *            __Stack_Init, both of which are calculated by the linker.
     *
     *            _end marks the end of all the bss segments, and represents
     *            the highest RAM address used by the linker to locate data
     *            (either initialised or not.)
     *
     *            __Stack_Init marks the bottom of the stack, as reserved
     *            in the linker script (../linker/linker_stm32f10x_md*.ld)
     */

    extern char link_heap_location, link_heap_location_end;
    char* sbrk_heap_top = &link_heap_location;

    caddr_t _sbrk(int incr)
    {
        char* prev_heap;

        if (sbrk_heap_top + incr > &link_heap_location_end)
        {
            volatile struct mallinfo mi = mallinfo();
            errno = ENOMEM;
            (void)mi;
            return (caddr_t) -1;
        }
        prev_heap = sbrk_heap_top;
        sbrk_heap_top += incr;
        return (caddr_t) prev_heap;
    }

    /*
     * Implement C++ new/delete operators using the heap
     */

    void *operator new(size_t size)
    {
        return malloc(size);
    }

    void *operator new[](size_t size)
    {
        return malloc(size);
    }

    void operator delete(void *p)
    {
        free(p);
    }

    void operator delete[](void *p)
    {
        free(p);
    }

    /* Bare metal, no processes, so error */
    int _kill(int pid, int sig)
    {
        return -1;
    }

    /* Bare metal, no processes, so always process id 1 */
    int _getpid(void)
    {
        return 1;
    }

    void _exit(int status) {
        //PANIC(Exit,"Exit Called");
        while (1);
    }

    int _write(int file, char *ptr, int len) { return 0; }
    int _read(int file, char *ptr, int len) { return 0; }
    int _close(int file) { return 0; }
    int _lseek(int file, int ptr, int dir) { return 0; }
    int _fstat(int file, void *sbuf) { return 0; }
    int _isatty(int file) { return 0; }

    /* Default implementation for call made to pure virtual function. */
    void __cxa_pure_virtual() {
        //  PANIC(PureVirtualCall,"Call on pure virtual");
        while (1);
    }

    /* Provide default implemenation for __cxa_guard_acquire() and
     * __cxa_guard_release(). Note: these must be revisited if a multitasking
     * OS is ported to this platform. */
    __extension__ typedef int __guard __attribute__((mode (__DI__)));
    int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
    void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
    void __cxa_guard_abort (__guard *) {};

    /*
       int _write(int file, char *ptr, int len) { return 0; }
       int _read(int file, char *ptr, int len) { return 0; }
       int _close(int file) { return 0; }
       int _lseek(int file, int ptr, int dir) { return 0; }
       int _fstat(int file, void *sbuf) { return 0; }
       int _isatty(int file) { return 0; }
       */

} /* extern "C" */


namespace __gnu_cxx {

void __verbose_terminate_handler()
{
	abort();
}

}

#endif
