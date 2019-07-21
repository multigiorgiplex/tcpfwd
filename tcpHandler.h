#ifndef _TCPHANDLER_H
#define _TCPHANDLER_H

#include <sys/socket.h>

#define TCP_LISTEN_BACKLOG	0	/*see man 2 listen*/

typedef struct {
	char				listen_address[16];
	unsigned int		listen_port;
	int					fd;
	struct sockaddr		socket_data;
} tcpServer;

void	TCP_server_init				(tcpServer *);
int		TCP_server_parse_input		(tcpServer *, char *, unsigned);
int		TCP_server_listen			(tcpServer *);

#endif
