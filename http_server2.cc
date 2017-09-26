/**
 * CS 1652 Project 1 - HTTP Client
 *
 * Zach Scheider - zds14@pitt.edu
 * Henri Frelin - hjf9@pitt.edu
 */

#include "minet_socket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <unistd.h>

using namespace std;

#define BUFSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int sock);
int parse(const char * request, char * filename, int len);
int parse (const char * source, char * name, int length) {

  int sourceLength = strlen(source);
  char*temp0 = new char[sourceLength+1];
  char*temp1 = new char[sourceLength+1];
  strcpy(temp0, source);
  char*temp2 = strstr(temp0, " HTTP/1.");

  if (!temp2 || (strncmp(temp0, "GET ", 4) != 0)) {
    delete [] temp0;
    delete [] temp1;
    return -1;
  }
  *temp2 = '\0';
  if (temp0 [4] == '/') {
    	strcpy(temp1, &temp0[5]);
  } else {
    	strcpy(temp1, &temp0[4]);
  }
  if ((int)strlen(temp1) + 1 > length) {
    	delete [] temp0;
    	delete [] temp1;
    	return -1;
  }
  strcpy(name, temp1);
  	delete[]temp0;
  	delete[]temp1;
  	return 0;
}

int main(int argc, char * argv[]) {
    int server_port = -1;
    int rc          =  0;
    int sock        = -1;

    /* parse command line args */
    if (argc != 3) {
	fprintf(stderr, "usage: http_server1 k|u port\n");
	exit(-1);
    }

    server_port = atoi(argv[2]);

    if (server_port < 1500) {
	fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", server_port);
	exit(-1);
    }

    /* initialize and make socket */
    if (toupper(*(argv[1])) == 'K') {
        minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') {
        minet_init(MINET_USER);
    } else {
        fprintf(stderr, "First argument must be k or u\n");
        exit(-1);
    }

    sock = minet_socket(SOCK_STREAM);

    if (-1 == sock) {
        fprintf(stderr, "Socket was not created. Exiting program.\n");
        exit(-1);
    }

    /* set server address*/
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server_port);

    /* bind listening socket */
    int bind = minet_bind(sock, &addr);
    if(bind < 0){
        printf("Error with binding socket\n");
        exit(-1);
    }
    /* start listening */
    int listen = minet_listen(sock, SOMAXCONN);
    if(listen < 0){
        printf("Error with listener socket\n");
        exit(-1);
    }

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    fd_set master;
    fd_set connections;
    FD_ZERO(&master);
    FD_ZERO(&connections);
    FD_SET(sock, &master);

    int maxsocks = sock;

    /* connection handling loop: wait to accept connection */
    while (1) {

        /* create read list */
        connections = master;

	   /* do a select */
        int selected = minet_select(maxsocks + 1, &connections, 0, 0, 0);

        if (selected) {
            /* process sockets that are ready */
            for (int i = 0; i <= maxsocks; i++) {
                if(FD_ISSET(i, &connections)) {
                    /* for the accept socket, add accepted connection to connections */
                    if(i == sock) {
                        int client = minet_accept(sock, 0);

                        if(client) {
                            if(client > maxsocks) {
                                maxsocks = client;
                            }

                            FD_SET(client, &master);
                        }
                        else {
                            // failed
                            continue;
                        }
                    } else {

                        /* for a connection socket, handle the connection */
                        rc = handle_connection(i);
                        close(i);
                        FD_CLR(i, &master);
                    }

                }
            }

        }
    }
}

int handle_connection(int sock) {
    bool ok = true;

    const char * ok_response_f = "HTTP/1.0 200 OK\r\n"	\
	"Content-type: text/plain\r\n"			\
	"Content-length: %d \r\n\r\n";
    char ok_response[strlen(ok_response_f)];


    const char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"	\
	"Content-type: text/html\r\n\r\n"			\
	"<html><body bgColor=black text=white>\n"		\
	"<h2>404 FILE NOT FOUND</h2>\n"				\
	"</body></html>\n";

    /* first read loop -- get request and headers*/
    char getBuf[BUFSIZE];
    int read = minet_read(sock, getBuf, BUFSIZE -1);  //RETEST HERE
    if(read <= 0){
        printf("No bytes\n");
        exit(-1);
    }
    getBuf[read] = '\0';

    string temp;
    size_t temp1;
    while(read > 0){
        temp = temp + string(getBuf);
        temp1 = temp.find("\r\n\r\n");

        if(temp1 != string::npos){
            temp = temp.substr(0, temp1);
            break; // break when we hit /r/n/r/n
        }

        read = minet_read(sock, getBuf, BUFSIZE - 1);
        getBuf[read] = '\0';
    }

    /* parse request to get file name */
    /* Assumption: this is a GET request and filename contains no spaces*/
    char name[FILENAMESIZE];
    int getName = parse(temp.c_str(), name, FILENAMESIZE);

    if(getName < 0){
        printf("Error with file name parse!");
        ok = false;
    }

    if(!strcmp(name, "")){ // RETEST THIS
        //strcpy(name, "index.html");
    }

    /* try opening the file */
    char buffer[BUFSIZE];
    string fileToString;
    int sending;
    FILE * file = fopen(name, "rb");

    if(file == NULL){
        ok = false;
        printf("NULL FILE ERROR");
    }

    if (ok) {
        while ((read = fread(&buffer, 1, BUFSIZE - 1, file))) {
          if (read <= 0) {
            break;
          }
          buffer[read] = '\0';
          fileToString += string(buffer);
        }

        /* send response */
        /* send headers */
        // Create response with the content length.
        sprintf(ok_response, ok_response_f, fileToString.size());
        // Add the response and content into a string
        fileToString = string(ok_response) + fileToString;

        /* send file */
        sending = minet_write(sock, const_cast<char*>(fileToString.c_str()), fileToString.size());

        if (sending <= 0) {
            fprintf(stderr, "Unable to send file");
            ok = false;
        }
    } else {
        // send error response
        sending = minet_write(sock, const_cast<char*>(notok_response), strlen(notok_response));
        if (sending <= 0) {
          fprintf(stderr, "Error sending the not OK response.\n");
          ok = false;
        }
    }

    /* close socket and free space */

    if (ok) {
	   return 0;
    } else {
	   return -1;
    }
}
