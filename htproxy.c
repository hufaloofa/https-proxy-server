#define _POSIX_C_SOURCE 200112L
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>

#include "htproxy.h"
#include "lru.h"
#include "helper.h"

/* NOTE: credit to beej's guide for inspiration */

int cache_enabled = 0; // flag to see whether caching is enabled or not
lru_cache *lru_list; // lru list for caching

void doTask(char *port_number, int cache_enabled_check) {
    cache_enabled = cache_enabled_check;
    int sockfd, newsockfd;
	char buffer[MAX_BUFFER];
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;
	char s[INET6_ADDRSTRLEN];
	int total_read = 0; // total bytes read from the request
	
	lru_list = new_list();

    // create the listening socket
    sockfd = create_listening_socket(port_number);
    

    // Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
    // backlog == 10
	if (listen(sockfd, 10) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

    // Accept a connection - blocks until a connection is ready to be accepted
	// Get back a new file descriptor to communicate on
	client_addr_size = sizeof client_addr;
	int n;
	while (1) {
        // decrement all ages of the nodes in the list
		if(lru_list->head != NULL){
			decrement_age(lru_list);
		}

        // accept the first instance
		newsockfd =
			accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
		if (newsockfd < 0) {
			perror("accept");
			continue;
		}
		inet_ntop(client_addr.sin_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
		printf("Accepted\n");
		fflush(stdout);

		total_read = 0;

        // receive the request and find out where the end of the request is
		while((n = recv(newsockfd, buffer + total_read, MAX_BUFFER - total_read - 1, 0)) > 0){
			total_read += n;
			buffer[total_read] = '\0';
			char* header_ending = strstr(buffer, "\r\n\r\n");
			if (header_ending != NULL){
				break;
			}
			if (total_read >= MAX_BUFFER -1){
				printf("bufferFull\n");
				break;
			}
		}
		if (n < 0 ){
			perror("recv");
			exit(EXIT_FAILURE);
		}

        // dupe the request to perform string operations on one and store the other
		char dupe_buffer[MAX_BUFFER];
		strncpy(dupe_buffer, buffer, MAX_BUFFER);

		char *tail = NULL;
		char *line = strtok(buffer, "\r\n");
		char host[MAX_BUFFER] = {0};
		char request_line[MAX_BUFFER] = {0};

        // search the request for the values we need
		while (line != NULL) {
			search_string(line, "Host: ", 6, host, sizeof(host) - 1);
			search_string(line, "GET ", 4, request_line, sizeof(request_line) - 1);
			tail = line;
			line = strtok(NULL,  "\r\n");
		}

		printf("Request tail %s\n", tail);
		int stale_flag = 0;
		char *request_URI = strtok(request_line, " "); // split the uri from https version
        int response_size;
		if (cache_enabled) {
			response_size = get_cached_response_size(lru_list, dupe_buffer, total_read);
		}
		if (cache_enabled && response_size
			&& get_age(lru_list, dupe_buffer, total_read) <= 0){ //check if entry is stale
			stale_flag = 1;
		}

		// if statement to see if request key is in cache, and whether cached entry is stale or not
		if (cache_enabled && response_size && total_read < 2000 && stale_flag == 0) { // if cache is enabled and the request is in the lru list do the caching
			update_lru(lru_list, dupe_buffer); // update the lru list
			printf("Serving %s %s from cache\n", host, request_URI);
			char* cached_response = get_cached_response(lru_list, dupe_buffer, total_read);
            
            // send the response from the cache
        	int sent = 0;
			while (sent < response_size) {
				int n = send(newsockfd, cached_response + sent, response_size - sent, 0);
				if (n <= 0) {
					perror("send from cache");
					break;
				}
				sent += n;
			}
		}else { // else do the standard procedure
			if (cache_enabled && lru_list->size == 10 && stale_flag == 0) { // if lru reached max size, evict regardless of caching, unless working with stale entry

                // search for the removed host and uri from the cache and print it
				char* removed = remove_head(lru_list); 
				char *line = strtok(removed, "\r\n");
				char removed_host[MAX_BUFFER] = {0};
				char removed_request_line[MAX_BUFFER] = {0};

				while (line != NULL) {
					search_string(line, "Host: ", 6, removed_host, sizeof(removed_host) - 1);
			        search_string(line, "GET ", 4, removed_request_line, sizeof(removed_request_line) - 1);
					line = strtok(NULL,  "\r\n");
				}
				char *removed_Request_URI = strtok(removed_request_line, " ");
				printf("Evicting %s %s from cache\n", removed_host, removed_Request_URI);
			}
			if (stale_flag){
				printf("Stale entry for %s %s\n", host, request_URI);
			}
			printf("GETting %s %s\n", host, request_URI);
			sendRequest(newsockfd, host, dupe_buffer, total_read, request_URI, stale_flag); // go to the next step, which is to connect to the proxy as a client
		}
		close(newsockfd);
		fflush(stdout);
	}
	free_list(lru_list);
    close(sockfd);
}

int create_listening_socket(char* port) {
    int s, sockfd;
	struct addrinfo hints, *res, *p;
    // Create address we're going to listen on (with given port number)
	memset(&hints, 0, sizeof hints);    // make sure the struct is empty
	hints.ai_family = AF_INET6;       // Set to IPV6
	hints.ai_socktype = SOCK_STREAM; // Connection-mode byte streams, TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept, fills in IP
	// node (NULL means any interface), service (port), hints, res
	s = getaddrinfo(NULL, port, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

    // loop through all the results and bind to the first we can
    for (p = res; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            perror("server: socket");
            continue;
        }

        int enable = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            perror("setsockopt");
            exit(1);
        }
		if (p->ai_family == AF_INET6) {
    		int no = 0;
    		if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) < 0) {
        		perror("setsockopt IPV6_V6ONLY");
        		exit(1);
    		}
		}

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }

        break;
    }
	freeaddrinfo(res); // all done with this

	return sockfd;
}

void sendRequest(int client_fd, char host[], char buffer[], int n, char request_URI[], int stale_flag){
	struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
	char s[INET6_ADDRSTRLEN];
	int server_fd;
    if (getaddrinfo(host, "80", &hints, &res) != 0) return;

    // loop through all the results and connect to the first we can
	for(p = res; p != NULL; p = p->ai_next) {
        if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_fd);
            perror("client: connect");
            continue;
        }

        break;
    }
	
	if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return;
    }
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	freeaddrinfo(res); // all done

	send(server_fd, buffer, n, 0);  // send request to origin
	get_response(client_fd, server_fd, buffer, host, request_URI, stale_flag, n);
}

void get_response(int client_fd, int server_fd, char request_buffer[], char host[], char request_URI[], int stale_flag, int req_size){ 
	char buffer[MAX_BUFFER];
	int n; // number of bytes read per packet recieved
	int total_read = 0; // total number of bytes we have read
	int header_len = 0; // length of the response header
    int valid_cache = 1; // flag to see if caching is valid

    // receive the response and keep track of where the header ends, so we know where the body begins
	while((n = recv(server_fd, buffer + total_read, MAX_BUFFER - total_read - 1, 0)) > 0){
		total_read += n;
		
		// search for "\r\n\r\n" 
		char* header_ending = NULL;
		for (int i = 0; i <= total_read - 4; i++) {
			if (buffer[i] == '\r' && buffer[i+1] == '\n' && 
			    buffer[i+2] == '\r' && buffer[i+3] == '\n') {
				header_ending = buffer + i;
				break;
			}
		}
		
		if (header_ending != NULL){
			header_len = header_ending - buffer + 4;
			break;			
		}
		if (total_read >= MAX_BUFFER -1){
			printf("bufferFull\n");
			break;
		}
	}

	if (n < 0) {
		perror("recv headers");
		exit(EXIT_FAILURE);
	}
	
    // dupe the header so we can perform search on it
	char header_buffer_dupe[MAX_BUFFER];
    memcpy(header_buffer_dupe, buffer, header_len);
	header_buffer_dupe[header_len] = '\0';

    // get the values we want from the dupe
	int content_length = -1;
	char *line = strtok(header_buffer_dupe, "\r\n");
	char cache_control_string[MAX_BUFFER] = {0};
	while (line != NULL) {
		search_string(line, "Cache-Control: ", 15, cache_control_string, sizeof(cache_control_string) - 1);
		if (strncasecmp(line, "Content-Length:", 15) == 0) {
			content_length = atoi(line + 15);
		}
		line = strtok(NULL, "\r\n");
	}
    printf("Response body length %d\n", content_length);

    // sets the cache control string to lower case to combat case sensitivity
	to_lower(cache_control_string);

	int max_age_val = INT_MAX; // set max age to a very big value
	char *max_age_str = strstr(cache_control_string, "max-age=");
	if(max_age_str != NULL){ //max-age is insdie the cache control headers
		max_age_val = atoi(max_age_str + 8);
	}

    // check if valid cache or not
	char *private = strstr(cache_control_string, "private");
	char *no_store = strstr(cache_control_string, "no-store");
	char *no_cache = strstr(cache_control_string, "no-cache");
	char *must_reval = strstr(cache_control_string, "must-revalidate");
	char *proxy_reval = strstr(cache_control_string, "proxy-revalidate");

	if (private != NULL || no_store != NULL || no_cache != NULL
	|| max_age_val == 0 || must_reval != NULL || proxy_reval != NULL) {
		valid_cache = 0;
	}

	int already_read = total_read - header_len; // how much we have already read
	int remaining = content_length - already_read; // how much is still remaining to read
    int total_response_size = content_length + header_len; // total response size

	// read rest of the body until no more is remaining or until we have read the max length allowed for caching
	while (remaining > 0 && total_read <= MAX_BUFFER) {
		n = recv(server_fd, buffer + total_read, MAX_BUFFER, 0);
		if (n <= 0) {
			perror("recv body");
			return;
		}
		
		total_read += n;
		remaining -= n;
	}

    // send what we have currently received
	int sent = 0;
	while (sent < total_read) {
		n = send(client_fd, buffer + sent, total_read - sent, 0);
		if (n <= 0) {
			perror("send response");
			return;
		}
		sent += n;
	}

    // cache if response size is less than max allowed size
	if (total_response_size <= MAX_BUFFER && cache_enabled ) {
		if (valid_cache) {
			char buffer_dupe[MAX_BUFFER];
			memcpy(buffer_dupe, buffer, total_read);
			buffer_dupe[total_read] = '\0'; 

			if(stale_flag == 1 ){ // stale entry, so we need to update the entry and subsequently the lru cache
				update_node(lru_list, request_buffer, req_size, buffer_dupe, total_read, max_age_val); 
				update_lru(lru_list, request_buffer);
			} else { // cache a "fresh" copy
				insert_tail(lru_list, request_buffer, buffer_dupe, total_read, max_age_val);
			}
		} else if (stale_flag == 1){ // not caching (due to cache headers) and is a stale entry so must evict
			// evict stale entry
			remove_entry(lru_list, request_buffer, req_size);
			printf("Evicting %s %s from cache\n", host, request_URI);
		} else {
			printf("Not caching %s %s\n", host, request_URI);
		}
	} 
	if((total_response_size > MAX_BUFFER) && cache_enabled && stale_flag == 1){ // wasnt cached due to size not because cache-headers
		remove_entry(lru_list, request_buffer, req_size);
		printf("Evicting %s %s from cache\n", host, request_URI);
	}

    // if there are still any bytes remaining due to a response longer than allowed for caching, ie no max size of response, receive and send it
	while (remaining > 0) {
        n = recv(server_fd, buffer, MAX_BUFFER, 0);
        if (n <= 0) {
            perror("recv body");
            return;
        }

        if (send(client_fd, buffer, n, 0) != n) {
            perror("send body");
            return;
        }

        remaining -= n;
    }
}

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}