#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>


#define MAX_BUFFER 102400 // max size of the buffer

// main function to perform the tasks
void doTask(char *portNumber, int cache_enabled);

// creates a listening socket
int create_listening_socket(char* port);

// connect to the proxy as a client and send the request
void sendRequest(int client_fd, char host[], char buffer[], int n, char request_URI[], int stale_flag);

// receive the buffer request and get the response
void get_response(int client_fd, int server_fd, char request_buffer[], char host[], char request_URI[], int stale_flag, int req_size);

// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa);

