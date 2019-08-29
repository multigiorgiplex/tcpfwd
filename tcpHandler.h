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


/* Common TCP_connection_listen() return values in text form */
typedef enum {
	TCP_CONN_LISTEN_OK,					/* All OK */
	TCP_CONN_LISTEN_ESOCKET,			/* socket() call error, see errno */
	TCP_CONN_LISTEN_EBIND,				/* bind() call error, see errno */
	TCP_CONN_LISTEN_ELISTEN,			/* listen() call error, see errno */
} TCP_CONN_LISTEN_t;

/* Common TCP_connection_accept() return values in text form */
typedef enum {
	TCP_CONN_ACCEPT_OK,					/* All OK */
	TCP_CONN_ACCEPT_EINIT,				/* TCP_connection_init() call error */
	TCP_CONN_ACCEPT_EACCEPT,			/* accept() call error, see errno */
	TCP_CONN_ACCEPT_EBADADDR,			/* accept() call has returned a invalid sockaddr */
} TCP_CONN_ACCEPT_t;

/* Common TCP_connection_connect() return values in text form */
typedef enum {
	TCP_CONN_CONNECT_OK,				/* All OK */
	TCP_CONN_CONNECT_INPROGRESS,		/* Operation now in progress */
	TCP_CONN_CONNECT_ESOCKET,			/* socket() call error, see errno */
	TCP_CONN_CONNECT_EREFUSED,			/* Connection refused */
	TCP_CONN_CONNECT_EUNREACHABLE,		/* Network is unreachable */
	TCP_CONN_CONNECT_EGENERIC,			/* connect() call error, see errno */
} TCP_CONN_CONNECT_t;

/* Common TCP_connection_close() return values in text form */
typedef enum {
	TCP_CONN_CLOSE_OK,					/* All OK */
	TCP_CONN_CLOSE_ESHUTDOWN,			/* shutdown() call error, see errno */
	TCP_CONN_CLOSE_ECLOSE,				/* close() call error, see errno */
} TCP_CONN_CLOSE_t;

/* Common TCP_connection_receive() return values in text form */
typedef enum {
	TCP_CONN_RECEIVE_OK,				/* All OK */
	TCP_CONN_RECEIVE_DISCONNECTED,		/* Other peer has disconnected */
	TCP_CONN_RECEIVE_BLOCK,				/* Read operation is blocking */
	TCP_CONN_RECEIVE_ERECV,				/* recv() call error, see errno */
} TCP_CONN_RECEIVE_t;

/* Common TCP_connection_send() return values in text form */
typedef enum {
	TCP_CONN_SEND_OK,					/* All OK */
	TCP_CONN_SEND_BLOCK,				/* Write operation is blocking */
	TCP_CONN_SEND_ESEND,				/* send() call error, see errno */
} TCP_CONN_SEND_t;


tcpConnection *		TCP_connection_init				(void);
TCP_CONN_LISTEN_t	TCP_connection_listen			(tcpConnection *);
int					TCP_connection_parse_input		(tcpConnection *, char *, unsigned);
TCP_CONN_CONNECT_t	TCP_connection_connect			(tcpConnection *);
TCP_CONN_ACCEPT_t	TCP_connection_accept			(tcpConnection *, tcpConnection **);
void				TCP_connection_destroy			(tcpConnection *);
TCP_CONN_CLOSE_t	TCP_connection_close			(tcpConnection *);
TCP_CONN_RECEIVE_t	TCP_connection_receive			(tcpConnection *);
TCP_CONN_SEND_t		TCP_connection_send				(tcpConnection *);
int					TCP_connection_get_socket_error	(tcpConnection *);

#endif
