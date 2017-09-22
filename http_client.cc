/**
 * CS 1652 Project 1 - HTTP Client
 *
 * Zach Scheider - zds14@pitt.edu
 * Henri Frelin - hjf9@pitt.edu
 */

#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <iostream>

using namespace std;

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
    if (minet_connect(socket, &addr) < 0) {
        fprintf(stderr, "Unable to connect to the server.\n");
        free(req);
        exit(-1);
    }


    /* send request message */
    sprintf(req, "GET %s HTTP/1.0\r\n\r\n", server_path);
    if (minet_write(socket, req, strlen(req)) < 0) {
        fprintf(stderr, "Unable to send message to server.\n");
        free(req);
        exit(-1);
    }

    /* wait till socket can be read. */
    /* Hint: use select(), and ignore timeout for now. */

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);

    //if (select(socket + 1, &fds, NULL, NULL, NULL) < 0) {
    if (minet_select(socket + 1, &fds, NULL, NULL, NULL) < 0) {
        fprintf(stderr, "Unable to select a socket.\n");
        free(req);
        minet_close(socket);
        minet_deinit();
        exit(-1);
    }

    /* first read loop -- read headers */
    char buf[BUFSIZE];
    int read = minet_read(socket, buf, BUFSIZE - 1);
    if (read <= 0) {
       fprintf(stderr, "Unable to read from socket.\n");
       free(req);
       close(socket);
       exit(-1);
    }

    string response = "";
    string header = "";
    size_t pos;

    while (read) {
        buf[read] = '\0';
        response += string(buf);
        pos = response.find("\r\n\r\n", 0);

        if (pos != string::npos) {
          header += response.substr(0, pos);
          response = response.substr(pos + 4);
          break;
        } else {
          header += response;
        }

        read = minet_read(socket, buf, BUFSIZE - 1);
    }

    /* examine return code */ 
    pos = header.find(" ");
    int ret_code = stoi(header.substr(pos, header.find(" ", pos)));

    //Skip "HTTP/1.0"
    //remove the '\0'

    // Normal reply has return code 200
    /* print first part of response: header, error code, etc. */
    if (200 == ret_code) {
        ok = true;
        cout << response;
    } else {
        ok = false;
        cerr << header + "\r\n\r\n" + response; // Have to use cerr because of strings
    }

    /* second read loop -- print out the rest of the response: real web content */
    while ((read = minet_read(socket, buf, BUFSIZE - 1))) {
        buf[read] = '\0';
        if(ok) {
            printf("%s", buf);
        }
        else {
            fprintf(stderr, buf);
        }
    }

    /*close socket and deinitialize */
    minet_close(socket);
    minet_deinit();
    free(req);

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}
