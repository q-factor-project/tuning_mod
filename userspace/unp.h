/* include unph */
/* Our own header.  Tabs are set for 4 spaces, not 8 */

#ifndef	__unp_h
#define	__unp_h

/* If anything changes in the following list of #includes, must change
   acsite.m4 also, for configure's tests. */

#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#if TIME_WITH_SYS_TIME
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#else
#if HAVE_SYS_TIME_H
#include	<sys/time.h>	/* includes <time.h> unsafely */
#else
#include	<time.h>		/* old system? */
#endif
#endif
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		/* for Unix domain sockets */

# include	<pthread.h>

#if 0 //keep for reference
#ifndef INET6_ADDRSTRLEN
/* $$.Ic INET6_ADDRSTRLEN$$ */
#define	INET6_ADDRSTRLEN	46	/* max size of IPv6 address string:
				   "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx" or
				   "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:ddd.ddd.ddd.ddd\0"
				    1234567890123456789012345678901234567890123456 */
#endif
#endif

/* Following could be derived from SOMAXCONN in <sys/socket.h>, but many
   kernels still #define it as 5, while actually supporting many more */
#define	LISTENQ		1024	/* 2nd argument to listen() */

/* Miscellaneous constants */
#define	MAXLINE		4096	/* max text line length */

/* Following shortens all the typecasts of pointer arguments: */
#define	SA	struct sockaddr

//Added for Q-Factor
struct args {
	unsigned int len;
	char msg[80];
};
/*****************/
void str_cli(int sockfd, struct args *this_test);
void     process_request(int);
void     read_sock(int);
//void     str_cli(FILE *, int);
const char              *Inet_ntop(int, const void *, char *, size_t);
void                     Inet_pton(int, const char *, void *);
			/* prototypes for our Unix wrapper functions: see {Sec errors} */
void	 Close(int);
pid_t	 Fork(void);

/* prototypes for our socket wrapper functions: see {Sec errors} */
int	 Accept(int, SA *, socklen_t *);
void	 Bind(int, const SA *, socklen_t);
int	 Connect(int, const SA *, socklen_t);
void	 Listen(int, int);
int	 Socket(int, int, int);
void	 Writen(int, void *, size_t);

int	 err_sys(const char *, ...);
#endif	/* __unp_h */
