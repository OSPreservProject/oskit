#include <sys/linker_set.h>

struct linker_set pcidevice_set;
struct linker_set pnpdevice_set;
struct linker_set eisadriver_set;
#if 0
struct linker_set ifmedia_set;
#endif
struct linker_set sysinit_set;
struct linker_set sysuninit_set;

struct linker_set videodriver_set;
struct linker_set kbddriver_set;
struct linker_set cons_set;

struct linker_set netisr_set;
struct linker_set domain_set;
