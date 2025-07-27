#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>

#include "htproxy.h"

/* NOTE: credit to beejs guide for inspiration */
int main(int argc, char **argv) {
    char *port_number = NULL;
	int cache_enabled = 0;
	int option;
	
    while ((option = getopt(argc, argv, "p:c")) != -1) {
        switch (option) {
            case 'p':
				port_number = optarg;
				break;
			case 'c':
				cache_enabled = 1;
				break;
        }
    }
	
	doTask(port_number, cache_enabled);
}