/*Handles TCP communication*/

#define _GNU_SOURCE

#include "tcpHandler.h"
#include "common.h"
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


tcpConnection * TCP_connection_init ()		//TODO consider defining {tcpConnection *} as a standalone type.
{
	tcpConnection * p;
	
	p = malloc (sizeof (tcpConnection));
	if (p == 0)
		return 0;
		
	memset (p, 0, sizeof (tcpConnection));
	
	return p;
}


TCP_CONN_LISTEN_t TCP_connection_listen (tcpConnection * srv)
{
	//open a socket
	srv->fd = socket (AF_INET, SOCK_STREAM/* | SOCK_NONBLOCK*/, 0);
	if (srv->fd == -1)
		return TCP_CONN_LISTEN_ESOCKET;

	//assigning the address & port
	if (bind (srv->fd, (struct sockaddr *) &(srv->socket_data), sizeof (struct sockaddr)) == -1)
		return TCP_CONN_LISTEN_EBIND;

	//listening on the socket
	if (listen (srv->fd, TCP_LISTEN_BACKLOG) == -1)
		return TCP_CONN_LISTEN_ELISTEN;
	
	return TCP_CONN_LISTEN_OK;
}


int TCP_connection_parse_input (tcpConnection * cnt, char * address, unsigned port)
{
	struct sockaddr_in	socket_address_INET;
	struct in_addr		ip;
	
	if (!inet_aton (address, &ip))
		return 1;	//invalid address

	strcpy (cnt->address, inet_ntoa(ip));	//reconvert in string the previously converted address so it's possible to have a "beautyfied" output
	cnt->port = port;
	
	socket_address_INET.sin_family	= AF_INET;
	socket_address_INET.sin_port	= htons (port);	//switch endiannes
	socket_address_INET.sin_addr	= ip;

	memcpy (&(cnt->socket_data), &socket_address_INET, sizeof (struct sockaddr_in));
	return 0;
}


TCP_CONN_CONNECT_t TCP_connection_connect (tcpConnection * cnt)
{
	if (!cnt)
		return TCP_CONN_CONNECT_EGENERIC;

	if (!cnt->fd)	//Check if socket has already be opened in this connection
	{
		//open a socket
		cnt->fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (cnt->fd == -1)
			return TCP_CONN_CONNECT_ESOCKET;
	}

	//connect to host
	if (connect (cnt->fd, (struct sockaddr *) &(cnt->socket_data), sizeof (struct sockaddr)) == -1)
	{
		switch (errno)
		{
		case ECONNREFUSED:
			return TCP_CONN_CONNECT_EREFUSED;
		case ENETUNREACH:
			return TCP_CONN_CONNECT_EUNREACHABLE;
		case EINPROGRESS:
			return TCP_CONN_CONNECT_INPROGRESS;
		default:
			return TCP_CONN_CONNECT_EGENERIC;
		}
	}
	
	return TCP_CONN_CONNECT_OK;
}


TCP_CONN_ACCEPT_t TCP_connection_accept (tcpConnection * srv, tcpConnection ** client)
{
	socklen_t socket_data_len = sizeof (struct sockaddr);
	tcpConnection * client_local;
	
	client_local = TCP_connection_init();
	if (client_local == 0)
		return TCP_CONN_ACCEPT_EINIT;
	
	client_local->fd = accept4 (srv->fd, (struct sockaddr *) &(client_local->socket_data), &socket_data_len, SOCK_NONBLOCK);
	if (client_local->fd == -1)
		return TCP_CONN_ACCEPT_EACCEPT;

	if (socket_data_len != sizeof (client_local->socket_data))
		return TCP_CONN_ACCEPT_EBADADDR;
	strcpy (client_local->address, inet_ntoa (client_local->socket_data.sin_addr));
	client_local->port = ntohs (client_local->socket_data.sin_port);
	
	*client = client_local;	//return

	return TCP_CONN_ACCEPT_OK;
}


TCP_CONN_CLOSE_t TCP_connection_close (tcpConnection * connection)
{
	int dummy_return;

	dummy_return = shutdown (connection->fd, SHUT_RDWR);
	if (dummy_return == -1 && errno != ENOTCONN)	//Ignore if not connected
		return TCP_CONN_CLOSE_ESHUTDOWN;
		
	if (close (connection->fd) == -1)
		return TCP_CONN_CLOSE_ECLOSE;
		
	TCP_connection_destroy (connection);
	
	return TCP_CONN_CLOSE_OK;
}

void TCP_connection_destroy (tcpConnection * conn)
{
	if (conn)
		free (conn);
}

TCP_CONN_RECEIVE_t TCP_connection_receive (tcpConnection * conn)
{
	ssize_t r;

	r = recv (conn->fd, conn->_tcp_recv_buffer, TCP_BUFFER_LENGHT, 0);
	if (r == 0 || (r == -1 && errno == ECONNRESET)) /* Connection reset by peer */
		return TCP_CONN_RECEIVE_DISCONNECTED;
	if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))	//The operation would block
		return TCP_CONN_RECEIVE_BLOCK;
	if (r == -1)
		return TCP_CONN_RECEIVE_ERECV;

	conn->buffer		= conn->_tcp_recv_buffer;
	conn->buffer_len	= r;

	return TCP_CONN_RECEIVE_OK;
}

TCP_CONN_SEND_t TCP_connection_send (tcpConnection * conn)
{
	ssize_t w;

	w = send (conn->fd, conn->buffer, conn->buffer_len, MSG_DONTWAIT);	/* Nonblocking IO.  */
	if (w == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))	//The operation would block
		return TCP_CONN_SEND_BLOCK;
	else if (w == -1)
		return TCP_CONN_SEND_ESEND;

	if (w != conn->buffer_len)
	{
		conn->buffer		+= w;
		conn->buffer_len	-= w;
	}
	else
	{
		conn->buffer		= 0;
		conn->buffer_len	= 0;
	}

	return TCP_CONN_SEND_OK;
}

int TCP_connection_get_socket_error (tcpConnection * conn)
{
	socklen_t socklen;
	int value;

	socklen = sizeof (int);
	if (getsockopt (conn->fd, SOL_SOCKET, SO_ERROR, (void *)&value, &socklen) == -1)
		return -1;
	if (socklen != sizeof (int))
		return -2;

	return value;
}
