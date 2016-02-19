/*
 * This is to just reference mem_lock so the one from libkern gets
 * pulled in.
 */

static void
dummy()
{
        extern void mem_lock(void);
        extern void mem_unlock(void);
        mem_lock();
        mem_unlock();
}
