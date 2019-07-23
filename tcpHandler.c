/*Handles TCP communication*/

#include "tcpHandler.h"
#include "common.h"
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>



tcpServer * TCP_server_init ()
{
	void * p;
	
	p = malloc (sizeof (tcpServer));
	if (p == 0)
		return 0;
		
	memset (p, 0, sizeof (tcpServer));
	
	return p;
}

tcpConnection * TCP_connection_init ()		//TODO consider defining {tcpConnection *} as a standalone type.
{
	void * p;
	
	p = malloc (sizeof (tcpConnection));
	if (p == 0)
		return 0;
		
	memset (p, 0, sizeof (tcpConnection));
	
	return p;
}

void TCP_connection_destroy (tcpConnection * conn)
{
	free (conn);
}

int TCP_server_parse_input (tcpServer * srv, char * address, unsigned port)
{
	struct sockaddr_in	socket_address_INET;
	struct in_addr		ip;

	if (!inet_aton (address, &ip))
		return 10;	//invalid address

	strcpy (srv->listen_address, inet_ntoa(ip));	//reconvert in string the previously converted address so it's possible to have a "beautyfied" output
	srv->listen_port = port;
		
	socket_address_INET.sin_family	= AF_INET;
	socket_address_INET.sin_port	= htons (port);	//switch endiannes
	socket_address_INET.sin_addr	= ip;

	memcpy (&(srv->socket_data), &socket_address_INET, sizeof (struct sockaddr_in));
	return 0;
}

int TCP_server_listen (tcpServer * srv)
{
	//open a socket
	srv->fd = socket (AF_INET, SOCK_STREAM, 0);
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
	//open a socket
	cnt->fd = socket (AF_INET, SOCK_STREAM, 0);
	if (cnt->fd == -1)
		return 10;
		
	//connect to host
	if (connect (cnt->fd, (struct sockaddr *) &(cnt->socket_data), sizeof (struct sockaddr)) == -1)
	{
		close (cnt->fd);
		return 11;
	}
	
	return 0;
}

int TCP_connection_accept (tcpServer * srv, tcpConnection ** client)
{
	socklen_t socket_data_len = sizeof (struct sockaddr);
	tcpConnection * client_local;
	
	client_local = TCP_connection_init();
	if (client_local == 0) return 1;
	
	client_local->fd = accept (srv->fd, (struct sockaddr *) &(client_local->socket_data), &socket_data_len);
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
	if (shutdown (connection->fd, SHUT_RDWR) == -1)
		return 1;
		
	TCP_connection_destroy (connection);
	
	return 0;
}