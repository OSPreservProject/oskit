/*
 * This glue file is needed because netbsd's <sys/tty.h>
 * uses `c_cc' as an element name after including <sys/termios.h>.
 * This loses since our termios.h #define's c_cc.
 */

#include <oskit/c/termios.h>
#undef c_cc
