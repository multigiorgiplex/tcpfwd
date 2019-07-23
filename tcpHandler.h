#ifndef _TCPHANDLER_H
#define _TCPHANDLER_H

#include <netinet/ip.h>		//superset of sys/socket.h and netinet/in.h (see man 7 ip)


#define TCP_LISTEN_BACKLOG	0	/*see man 2 listen*/

typedef struct {
	uint8_t				listen_address[16];
	uint32_t			listen_port;
	int32_t				fd;
	struct sockaddr_in	socket_data;
} tcpServer;

typedef struct {
	uint8_t				address[16];
	uint32_t			port;
	int32_t				fd;
	struct sockaddr_in	socket_data;
} tcpConnection;
	

tcpServer *		TCP_server_init				(void);
int				TCP_server_parse_input		(tcpServer *, char *, unsigned);
int				TCP_server_listen			(tcpServer *);

tcpConnection *	TCP_connection_init			(void);
void			TCP_connection_destroy		(tcpConnection *);
int				TCP_connection_parse_input	(tcpConnection *, char *, unsigned);
int				TCP_connection_connect		(tcpConnection *);
int				TCP_connection_accept		(tcpServer *, tcpConnection **);
int				TCP_connection_close		(tcpConnection *);

#endif
