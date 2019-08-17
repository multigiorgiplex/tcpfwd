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


int TCP_connection_listen (tcpConnection * srv)
{
	//open a socket
	srv->fd = socket (AF_INET, SOCK_STREAM/* | SOCK_NONBLOCK*/, 0);
	if (srv->fd == -1)
		return 10;

	//assigning the address & port
	if (bind (srv->fd, (struct sockaddr *) &(srv->socket_data), sizeof (struct sockaddr)) == -1)
	{
		close (srv->fd);	//if no error occours, errno is not changed	
		return 11;
	}

	//listening on the socket
	if (listen (srv->fd, TCP_LISTEN_BACKLOG) == -1)
	{
		close (srv->fd);		
		return 12;
	}
	
	return 0;
}


int TCP_connection_parse_input (tcpConnection * cnt, char * address, unsigned port)
{
	struct sockaddr_in	socket_address_INET;
	struct in_addr		ip;
	
	if (!inet_aton (address, &ip))
		return 10;	//invalid address

	strcpy (cnt->address, inet_ntoa(ip));	//reconvert in string the previously converted address so it's possible to have a "beautyfied" output
	cnt->port = port;
	
	socket_address_INET.sin_family	= AF_INET;
	socket_address_INET.sin_port	= htons (port);	//switch endiannes
	socket_address_INET.sin_addr	= ip;

	memcpy (&(cnt->socket_data), &socket_address_INET, sizeof (struct sockaddr_in));
	return 0;
}

int TCP_connection_connect (tcpConnection * cnt)
{
	if (!cnt)
		return 1;

	if (!cnt->fd)	//Check if socket has already be opened in this connection
	{
		//open a socket
		cnt->fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (cnt->fd == -1)
			return 10;
	}

	//connect to host
	if (connect (cnt->fd, (struct sockaddr *) &(cnt->socket_data), sizeof (struct sockaddr)) == -1)
		return 11;
	
	return 0;
}

int TCP_connection_accept (tcpConnection * srv, tcpConnection ** client)
{
	socklen_t socket_data_len = sizeof (struct sockaddr);
	tcpConnection * client_local;
	
	client_local = TCP_connection_init();
	if (client_local == 0) return 1;
	
	client_local->fd = accept4 (srv->fd, (struct sockaddr *) &(client_local->socket_data), &socket_data_len, SOCK_NONBLOCK);
	if (client_local->fd == -1)
		return 10;
	if (socket_data_len != sizeof (client_local->socket_data))
		return 20;
	
	strcpy (client_local->address, inet_ntoa (client_local->socket_data.sin_addr));
	client_local->port = ntohs (client_local->socket_data.sin_port);
	
	*client = client_local;	//return

	return 0;
}

int TCP_connection_close (tcpConnection * connection)
{
	int dummy_return;

	dummy_return = shutdown (connection->fd, SHUT_RDWR);
	if (dummy_return == -1 && errno != ENOTCONN)	//Ignore if not connected
		return 10;
		
	if (close (connection->fd) == -1)
		return 20;
		
	TCP_connection_destroy (connection);
	
	return 0;
}

void TCP_connection_destroy (tcpConnection * conn)
{
	if (conn)
		free (conn);
}

int TCP_connection_receive (tcpConnection * conn)
{
	ssize_t r;

	r = recv (conn->fd, conn->_tcp_recv_buffer, TCP_BUFFER_LENGHT, 0);
	if (r == 0 || (r == -1 && errno == ECONNRESET)) /* Connection reset by peer */
		return -10;
	if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))	//The operation would block
		return -20;
	if (r == -1)
		return 10;

	conn->buffer		= conn->_tcp_recv_buffer;
	conn->buffer_len	= r;

	return 0;
}

int TCP_connection_send (tcpConnection * conn)
{
	ssize_t w;

	w = send (conn->fd, conn->buffer, conn->buffer_len, MSG_DONTWAIT);	/* Nonblocking IO.  */
	if (w == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))	//The operation would block
		return 20;
	else if (w == -1)
		return 10;

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

	return 0;
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
