#ifndef _TCPHANDLER_H
#define _TCPHANDLER_H

#include <netinet/ip.h>		//superset of sys/socket.h and netinet/in.h (see man 7 ip)


#define TCP_LISTEN_BACKLOG	0	/*see man 2 listen*/
#define TCP_BUFFER_LENGHT	4096


typedef struct {
	uint8_t				address[16];
	uint32_t			port;
	int32_t				fd;
	struct sockaddr_in	socket_data;
	char *				buffer;
	ssize_t				buffer_len;	//lenght of data inside buffer (read/write)
	char				_tcp_recv_buffer[TCP_BUFFER_LENGHT];	//actual  buffer
} tcpConnection;


tcpConnection *	TCP_connection_init			(void);
int				TCP_connection_listen		(tcpConnection *);
int				TCP_connection_parse_input	(tcpConnection *, char *, unsigned);
int				TCP_connection_connect		(tcpConnection *);
int				TCP_connection_accept		(tcpConnection *, tcpConnection **);
void			TCP_connection_destroy		(tcpConnection *);
int				TCP_connection_close		(tcpConnection *);
int				TCP_connection_receive		(tcpConnection *);
int				TCP_connection_send			(tcpConnection *);

#endif
