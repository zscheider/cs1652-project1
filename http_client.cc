/**
 * CS 1652 Project 1 - HTTP Client
 *
 * Zach Scheider - zds14@pitt.edu
 * Henri Frelin -
 */

#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>

#define BUFSIZE 1024

int main(int argc, char * argv[]) {

    char * server_name = NULL;
    int server_port    = -1;
    char * server_path = NULL;
    char * req         = NULL;
    bool ok            = false;

    /*parse args */
    if (argc != 5) {
	fprintf(stderr, "usage: http_client k|u server port path\n");
	exit(-1);
    }

    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];

    req = (char *)malloc(strlen("GET  HTTP/1.0\r\n\r\n") 
			 + strlen(server_path) + 1);  

    /* initialize */
    if (toupper(*(argv[1])) == 'K') { 
	minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') { 
	minet_init(MINET_USER);
    } else {
	fprintf(stderr, "First argument must be k or u\n");
    free(req);
	exit(-1);
    }

    /* make socket */
    int socket = minet_socket(SOCK_STREAM);
    if (-1 == socket) {
        fprintf(stderr, "Socket was not created. Exiting program.\n");
        free(req);
        exit(-1);
    }

    /* get host IP address  */
    /* Hint: use gethostbyname() */
    struct hostent *host = gethostbyname(server_name);
    if (host == NULL) {
        fprintf(stderr, "Could not retrieve IP from the server name entered.\n");
        free(req);
        close(socket);
        exit(-1);
    }

    /* set address */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    memcpy(&addr.sin_addr.s_addr, host->h_addr, host->h_length);


    /* connect to the server socket */


    /* send request message */
    sprintf(req, "GET %s HTTP/1.0\r\n\r\n", server_path);

    /* wait till socket can be read. */
    /* Hint: use select(), and ignore timeout for now. */

    /* first read loop -- read headers */

    /* examine return code */   

    //Skip "HTTP/1.0"
    //remove the '\0'

    // Normal reply has return code 200

    /* print first part of response: header, error code, etc. */

    /* second read loop -- print out the rest of the response: real web content */

    /*close socket and deinitialize */

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}
