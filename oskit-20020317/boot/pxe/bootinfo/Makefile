CFLAGS= -g -DSOLARIS -DHAVE_SOCKADDR_SA_LEN -DUSE_RECVMSG
INCS= -I../../../oskit/boot

all: bootinfo

bootinfo: bootinfo.c
	cc $(CFLAGS) $(INCS) -o bootinfo bootinfo.c $(LFLAGS)

clean: 
	rm bootinfo
