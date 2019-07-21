/*Handles TCP communication*/

#include "tcpHandler.h"
#include "common.h"
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>



void TCP_server_init (tcpServer * srv)
{
	memset (srv, 0, sizeof (tcpServer));
}

int TCP_server_parse_input (tcpServer * srv, char * address, unsigned port)
{
	struct sockaddr_in	socket_address_INET;
	struct in_addr		ip;

	if (!inet_aton (address, &ip))
		return 10;	//invalid address


	strcpy (srv->listen_address, inet_ntoa(ip));	//reconvert in string the previously converted address
	srv->listen_port = port;

		
	socket_address_INET.sin_family	= AF_INET;
	socket_address_INET.sin_port	= htons (port);	//switch endiannes
	socket_address_INET.sin_addr	= ip;

	memcpy (&(srv->socket_data), &socket_address_INET, sizeof (struct sockaddr));
	return 0;
}

int TCP_server_listen (tcpServer * srv)
{
	//open a socket
	srv->fd = socket (AF_INET, SOCK_STREAM, 0);
	if (srv->fd == -1)
		return 10;

	//assigning the address & port
	if (bind (srv->fd, &(srv->socket_data), sizeof (srv->socket_data)) == -1)
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
