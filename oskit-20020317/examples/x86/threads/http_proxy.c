/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * HTTP Proxy: This example is intended to demonstrate a multi-threaded
 * program that uses the BSD socket layer. This is a simple HTTP proxy
 * that listens on port 6969. It can handle basic methods like GET, HEAD,
 * CONNECT, and POST, but is by no means a correct, complete, or optimal
 * implementation of the HTTP spec. Its just a demonstration.
 *
 * Big Picture: For each new connection received on port 6969, 3 new threads
 * are launched. The first parses the connection request and gets it sent
 * off to the actual server. The other two threads simply loop, reading from
 * the client and server sides, passing data along to the other side. When
 * either side closes it connection (usually the server when the file has
 * been transferred), all the connections are torn down and the 3 threads
 * are terminated.
 *
 * To test this with Netscape you need to tell it to use a proxy.
 * Under version 3 this is done through the Options -> Network Preferences ->
 * Proxies -> Manual -> Config -> View dialog box.
 * Set HTTP and Security to machine name and port 6969.
 * Under version 4 this is done through the Preferences -> Advanced ->
 * Proxies -> Manual -> View dialog box.
 *
 * Note: the filesytem stuff is only so libstartup can set up things in /etc
 * for us (like /etc/resolv.conf).
 * We could use the bmodfs but we use the netbsd filesystem to test more stuff.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <oskit/threads/pthread.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <assert.h>
#include <sys/stat.h>
#include <oskit/queue.h>
#include <time.h>
#if !defined(OSKIT_UNIX) && defined(OSKIT_X86)
#include <oskit/x86/base_gdt.h>
#endif

int		proxy_socket;
int		death_socket;
int		create_listener_socket(int port);
void		*deathwatch(void *foo);
pthread_mutex_t gethostname_mutex;

int           threads_stack_back_trace(int, int);

#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#ifdef  OSKIT_UNIX
void	start_fs_native_pthreads(char *root);
#endif

/*
 * Each connection has this associated data structure.
 */
typedef struct _connection {
	int		client_sock;	/* Connection from the client */
	int		server_sock;	/* Connection to the server */
	pthread_mutex_t mutex;		/* A lock to protect this structure */
	pthread_cond_t  condvar;	/* Condition for signal */
	int		died;		/* One side or the other died. */
} connection_t;

void	*connection_manager(void *arg);

/*
 * Act as simple HTTP proxy.
 */
int
main()
{
	int		length, sock;
	struct sockaddr client;
	pthread_t	death_tid, tid;
	extern int	threads_debug;

	/*
	 * Init the OS.
	 */
	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();
	threads_debug = 0;
	
#ifndef OSKIT_UNIX
	start_world_pthreads();
#else
#ifdef  REALSTUFF
	start_fs_pthreads("/dev/oskitfs", 0);
	start_network_pthreads();
#else
	start_fs_native_pthreads("/tmp");
	start_network_native_pthreads();
#endif
#endif

	pthread_mutex_init(&gethostname_mutex, 0);
	proxy_socket = create_listener_socket(6969);
	death_socket = create_listener_socket(6666);

	/*
	 * Create a thread so I can talk to the program via telnet.
	 */
	pthread_create(&death_tid, 0, deathwatch, (void *) 0);
	
	/*
	 * The main thread will do nothing except look for new
	 * connections.
	 */
	while (1) {
		sock = accept(proxy_socket, &client, &length);

		if (sock < 0) {
			if (errno == ECONNABORTED) {
				/*
				 * The deathwatch has cut us off at the
				 * knees. Just wait for the end.
				 */
				oskit_pthread_sleep(100000);
			}
			perror("accept");
			exit(1);
		}

		/*
		 * Create a thread to manage this connection.
		 */
		pthread_create(&tid, 0, connection_manager, (void *) sock);
		pthread_detach(tid);
	}
	return 0;
}

/*
 * This program creates a datagram socket, binds a name to it, then reads
 * from the socket.
 */
int
create_listener_socket(int port)
{
	int sock, length, data;
	struct sockaddr_in name;

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening datagram socket");
		exit(1);
	}
	
	data = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &data, sizeof(data))
	    < 0) {
		perror("setsockopt");
		exit(1);
	}

	/* Create name with wildcards. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		perror("binding datagram socket");
		exit(1);
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(sock, (struct sockaddr *) &name, &length)) {
		perror("getting socket name");
		exit(1);
	}
	printf("Socket has port #%d\n", ntohs(name.sin_port));

	if (listen(sock, 20) < 0) {
		perror("listen on socket");
		exit(1);
	}

	return sock;
}

int
connect_to_host(char *host, char *port, int *server_sock)
{
	int			sock, data;
	struct sockaddr_in	name;
	struct hostent		*hp, *gethostbyname();
	struct timeval		timeo;

	/* Create socket on which to send. */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("connect_to_host: opening datagram socket");
		return 1;
	}

	data = 1024 * 32;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &data, sizeof(data)) < 0) {
		perror("setsockopt SO_SNDBUF");
		return 1;
	}
	
	data = 1024 * 32;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &data, sizeof(data)) < 0) {
		perror("setsockopt SO_RCVBUF");
		return 1;
	}
		
	data = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &data, sizeof(data))
	    < 0) {
		perror("setsockopt SO_KEEPALIVE");
		return 1;
	}

	timeo.tv_sec  = 30;
	timeo.tv_usec = 0;
	if (setsockopt(sock,
		       SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo)) < 0) {
		perror("setsockopt SO_RCVTIMEO");
#if 0
		panic("SETSOCKOPT");
#endif
	}

	/*
	 * Construct name, with no wildcards, of the socket to send to.
	 * Gethostbyname() returns a structure including the network address
	 * of the specified host.  The port number is taken from the command
	 * line. 
	 */
	pthread_mutex_lock(&gethostname_mutex);
	hp = gethostbyname(host);
	pthread_mutex_unlock(&gethostname_mutex);

	if (hp == 0) {
		printf("connect_to_host: unknown host: %s\n", host);
		return 1;
	}
	bcopy(hp->h_addr, &name.sin_addr, hp->h_length);
	name.sin_family = AF_INET;
	name.sin_port = htons((port ? atoi(port) : 80));

	if (connect(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		perror("connect_to_host: connecting stream socket");
		return 1;
	}
	
	*server_sock = sock;
	return 0;
}

/*
 * Setup a new connection.
 */
int
setup_connection(int client_sock, connection_t **return_connp)
{
	int		data, c, len, server_sock, tunneling = 0;
	char		buf[BUFSIZ], str[BUFSIZ], *bp;
	char		*last = 0, *method, *url, *rest, *host, *port;
	connection_t	*connp;
	struct timeval  timeo;

	data = 1;
	if (setsockopt(client_sock,
		       SOL_SOCKET, SO_KEEPALIVE, &data, sizeof(data)) < 0) {
		perror("setsockopt SO_KEEPALIVE");
		return 1;
	}

	/*
	 * Set the recv timeout so that reads do not block forever. 
	 */
	timeo.tv_sec  = 10;
	timeo.tv_usec = 0;
	if (setsockopt(client_sock,
		       SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo)) < 0) {
		perror("setsockopt SO_RCVTIMEO");
#if 0
		return 1;
#endif
	}

	/*
	 * Read the first block of data to get the URL info.
	 */
	bzero(buf, BUFSIZ);
	if ((c = recv(client_sock, buf, BUFSIZ, 0)) <= 0) {
		if (c <= 0) {
			if (c == 0) {
				printf("Closing down Client side\n");
				return 1;
			}
			if (errno && errno != EINTR) {
				perror("First read on client side");
				return 1;
			}
		}
	}

	/*
	 * Parse the URL info.
	 */
	bp = buf;
	if ((method = strtok_r(bp, " \t\n\r", &last)) == NULL) {
		printf("Bad Method: %s\n", buf);
		return 1;
	}
	
	if ((url = strtok_r(NULL, " \t\n\r", &last)) == NULL) {
		printf("Bad URL(1): %s\n", buf);
		return 1;
	}
	printf("%d: %s %s\n", (int) pthread_self(), method, url);
	rest = &url[strlen(url)];
	rest++;
	
	/*
	 * Look at the method. Support just GET/POST/HEAD and CONNECT.
	 */
	if (strncasecmp(method, "GET",  3) == 0 ||
	    strncasecmp(method, "POST", 4) == 0 ||
	    strncasecmp(method, "HEAD", 4) == 0) {
		/*
		 * Separate the host name from the actual url.
		 */
		if (strncasecmp(url, "http://", 7) != 0) {
			printf("Bad URL(2): %s\n", url);
			return 1;
		}
		host = url+7;
	}
	else if (strncasecmp(method, "CONNECT", 7) == 0) {
		printf("%d: %s\n", (int) pthread_self(), rest);
		host      = url;
		tunneling = 1;
	}
	else {
		printf("%d: Bad METHOD: %s %s\n",
		       (int) pthread_self(), method, url);
		return 1;
	}

	/*
	 * Extract the port and the url.
	 */
	bp   = host;
	port = (char *) 0;
	while (*bp && !(*bp == '/' || *bp == ':'))
		++bp;

	if (*bp == 0) {
		url  = (char *) 0;
	}
	else {
		if (*bp == ':') {
			*bp++ = '\0';
			port = bp;

			while (*bp && isdigit(*bp))
				++bp;

			if (*bp == 0) {
				url = (char *) 0;
				goto gotit;
			}
		}
		*bp++ = '\0';
		url = bp;
	}
  gotit:
#ifdef  OSKIT_UNIX
	osenv_intr_disable();
#endif
	if (connect_to_host(host, port, &server_sock)) {
		/*
		 * Send back a connection message
		 */
		sprintf(str, "HTTP/1.0 204 Connection Failed\n\n");
		send(client_sock, str, strlen(str), 0);
		return 1;
	}
#ifdef	OSKIT_UNIX
	osenv_intr_enable();
#endif
	if ((connp = (connection_t *) malloc(sizeof(connection_t))) == NULL) {
		printf("setup_connection: Out of memory\n");
		return 1;
	}
	bzero(connp, sizeof(*connp));

	connp->client_sock = client_sock;
	connp->server_sock = server_sock;
	pthread_mutex_init(&connp->mutex, 0);
	pthread_cond_init(&connp->condvar, 0);

	if (tunneling) {
		/*
		 * Send back a connection message
		 */
		sprintf(str, "HTTP/1.0 200 Connection established\n\n");
		len = strlen(str);
		
		if ((c = send(client_sock, str, len, 0)) != len) {
			printf("Unexpected close on client side\n");
			close(server_sock);
			free(connp);
			return 1;
		}
	}
	else {
		/*
		 * Okay, send the initial data block off.
		 */
		sprintf(str, "%s /%s %s", method, url, rest);
		len = strlen(str);

		/* printf("%s\n", str); */

		if ((c = send(server_sock, str, len, 0)) != len) {
			printf("Unexpected close on server side\n");
			close(server_sock);
			free(connp);
			return 1;
		}
	}

	*return_connp = connp;
	return 0;
}

void *
connection_manager(void *arg)
{
	void		*client_side(void *connp);
	void		*server_side(void *connp);
	pthread_t	client_tid, server_tid;
	int		status, client_sock = (int) arg;
	connection_t	*connp;

	printf("Connection accepted. tid=%d\n", (int) pthread_self());

	if (setup_connection(client_sock, &connp)) {
		close(client_sock);
		pthread_exit((void *) 1);
	}

	pthread_mutex_lock(&connp->mutex);

	/*
	 * Create a thread to manage the client side.
	 */
	pthread_create(&client_tid, 0, client_side, (void *) connp);

	/*
	 * Create a thread to manage the server side.
	 */
	pthread_create(&server_tid, 0, server_side, (void *) connp);

	/*
	 * Lets wait for them to change status.
	 */
	while (! connp->died) {
		pthread_cond_wait(&connp->condvar, &connp->mutex);
	}
	pthread_mutex_unlock(&connp->mutex);

	pthread_cancel(client_tid);
	pthread_cancel(server_tid);

	pthread_join(client_tid, (void *) &status);
	pthread_join(server_tid, (void *) &status);

	printf("Manager: tid=%d "
	       "Client(%d) and server(%d) side threads have exited\n",
	       (int) pthread_self(), (int) client_tid, (int) server_tid);

	shutdown(connp->client_sock, 2);
	shutdown(connp->server_sock, 2);
	close(connp->client_sock);
	close(connp->server_sock);

	free(connp);

	return 0;
}

/*
 * Since the POSIX library does not implement cancelation points at
 * all the necessary spots, need to do this ourselves.
 */
void
cleanup_handler(void *arg)
{
	connection_t	*connp = (connection_t *) arg;

	pthread_mutex_lock(&connp->mutex);
	connp->died++;
	pthread_mutex_unlock(&connp->mutex);
	pthread_cond_signal(&connp->condvar);
}

/*
 * Read from the client side of the connection and send to the server
 * side.
 */
void *
client_side(void *arg)
{
	connection_t	*connp = (connection_t *) arg;
	char		buf[4096];
	int		c;

	printf("client_side starting: tid=%d\n", (int) pthread_self());

	pthread_mutex_lock(&connp->mutex);
	pthread_mutex_unlock(&connp->mutex);

	pthread_cleanup_push(cleanup_handler, connp);
	
	while (1) {
		pthread_testcancel();
		if ((c = recv(connp->client_sock, buf, 4096, 0)) <= 0) {
			if (c == 0) {
				printf("Read: Closing Client side\n");
				goto done;
			}
			else if (errno == EAGAIN || errno == EINTR) {
				/*
				 * On client side, a read timeout means no
				 * data available. Thats okay.
				 */
				continue;
			}
			else {
				perror("read on client side");
				goto done;
			}
		}
		/*printf("Client %d\n%s\n", c, buf);*/
		if ((c = send(connp->server_sock, buf, c, 0)) != c) {
			if (c == 0) {
				printf("Write: Closing Client side\n");
				goto done;
			}
			else if (c < 0) {
				perror("write syscall on client side");
				goto done;
			}
			else {
				printf("write data on client side\n");
				goto done;
			}
		}
	}
   done:
	pthread_cleanup_pop(1);
	return 0;
}

/*
 * Read from the server side of the connection and send to the client
 * side.
 */
void *
server_side(void *arg)
{
	connection_t	*connp = (connection_t *) arg;
	char		buf[4096];
	int		c;
	
	printf("server_side starting: tid=%d\n", (int) pthread_self());

	pthread_mutex_lock(&connp->mutex);
	pthread_mutex_unlock(&connp->mutex);

	pthread_cleanup_push(cleanup_handler, connp);
	
	while (1) {
		pthread_testcancel();
		if ((c = recv(connp->server_sock, buf, 4096, 0)) <= 0) {
			if (c == 0) {
				printf("Read: Closing Server side\n");
				goto done;
			}
			else if (errno == EAGAIN) {
				/*
				 * On client side, a read timeout means no
				 * data available. Thats okay.
				 */
				printf("Read: EAGAIN on server side: tid=%d\n",
				       (int) pthread_self());
				goto done;
			}
			else {
				perror("read on server side");
				goto done;
			}
		}
		/*printf("Server %d\n%s\n", c, buf);*/
		if ((c = send(connp->client_sock, buf, c, 0)) != c) {
			if (c == 0) {
				printf("Send: Closing Server side\n");
				goto done;
			}
			else if (c < 0) {
				perror("send syscall on server side");
				goto done;
			}
			else {
				printf("send data on server side\n");
				goto done;
			}
		}
	}
   done:
	pthread_cleanup_pop(1);
	return 0;
}

void *
deathwatch(void *arg)
{
	char		buf[4096];
	int		c, sock;
	struct sockaddr client;

	while (1) {
		sock = accept(death_socket, &client, &c);

		/*
		 * UNIX implementation uses non-blocking I/O.
		 * OSKIT version of accept will call osenv_sleep.
		 */
		if (sock < 0) {
			perror("accept");
			exit(1);
		}

		while (1) {
			if ((c = recv(sock, buf, 4096, 0)) <= 0) {
				if (c == 0) {
					printf("Read: Closing Death Socket\n");
					goto done;
				}
				else {
					perror("read on death side");
					goto done;
				}
			}
			buf[c-1] = 0;
			printf("Death Watch: %d %s\n", c, buf);

			if (strncmp(buf, "reboot", 6) == 0)
				exit(1);
#if !defined(OSKIT_UNIX) && defined(OSKIT_X86)
			if (strncmp(buf, "gdb", 3) == 0) {
				asm("int $3");
				base_gdt_load();
			}
#endif
			if (strncmp(buf, "bta", 3) == 0) {
				int i;
				for (i = 0; i < 100; i++) {
					threads_stack_back_trace(i, 16);
				}
			}
			else if (strncmp(buf, "bt", 2) == 0) {
				buf[4] = 0;
				threads_stack_back_trace(atoi(buf), 24);
			}
		}
           done:
	}
	return 0;
}

/*
 * Case insensitive search of an hlen character array for a substring.
 */
char *
strnstr(char *haystack, int hlen, char *needle)
{
	int nlen = strlen(needle);

	while (hlen >= nlen)
	{
		if (!strncasecmp(haystack, needle, nlen))
			return (char *)haystack;

		haystack++;
		hlen--;
	}
	return 0;
}

/*
 * Search for a particular header, returning a string copy of the value.
 */
char *
getheader(char *buf, int n, char *header)
{
	char	*bp, *ep, *str;
	int	len;
	
	if ((bp = strnstr(buf, n, header)) == NULL)
		return 0;

	/* skip the header */
	while (! isspace(*bp))
		bp++;

	/* and the following spaces */
	while (isspace(*bp))
		bp++;

	/* Skip to the end of the value */
	ep = bp;
	while (! iscntrl(*ep))
		ep++;
	len = ep - bp;

	str = malloc(len + 1);
	if (str) {
		strncpy(str, bp, len);
		str[len] = 0;
	}
	return str;
}

#include <syslog.h>
#include <stdarg.h>

void
my_syslog(int pri, const char *fmt, ...)
{
        va_list args;

        va_start(args, fmt);
        osenv_log(pri, fmt, args);
        va_end(args);
}

oskit_syslogger_t oskit_libc_syslogger = my_syslog;

