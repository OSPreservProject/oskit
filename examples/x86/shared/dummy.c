/*
 * This is to just reference mem_lock so the one from libkern gets
 * pulled in instead of from libc.
 * This is necessary for applications that use the devices (or maybe
 * just Linux ones, which allocate mem in interrupt handlers).
 */
static void dummy() {
        extern void mem_lock(void);
        extern void mem_unlock(void);
        mem_lock();
        mem_unlock();
}
