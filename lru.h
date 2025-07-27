#include <stdio.h>
#include <limits.h>

#define MAX_CACHE_REQ 2000 // max size of request
#define MAX_RESPONSE 102400 // max size of the response

/* structure of a node, contains the key (request) and the value (response)
 and also the age and size of the response. Also points to next and previous node */
typedef struct lru_cache_node {
    struct lru_cache_node *next;
    struct lru_cache_node *prev;
    char key[MAX_CACHE_REQ];
    char value[MAX_RESPONSE];
    int response_size;
    int age;
}lru_cache_node;

// head and tail of the doubly linked list, contains size of the linked list
typedef struct  {
    lru_cache_node *head;
    lru_cache_node *tail;
    int size;
}lru_cache;

// initialises a new list
lru_cache *new_list(); 

// removes and returns the key of the head of the linked list
char* remove_head(lru_cache *list); 

// inserts a node to the tail
void insert_tail(lru_cache *list, char key[], char value[], int response_size, int age); 

// updates the lru list, doesnt remove
void update_lru(lru_cache *list, char key[]); 

// frees the list
void free_list(lru_cache *list); 

// prints the list for testing
void print_list(lru_cache *list);

// gets the response from the cache
char* get_cached_response(lru_cache *list, char key[], int size);

// gets the response size 
int get_cached_response_size(lru_cache *list, char key[], int size);

// decrements all ages in the list
void decrement_age(lru_cache *list);

// gets the age of a node
int get_age(lru_cache *list, char key[], int size);

// removes a specific entry
void remove_entry(lru_cache *list, char key[], int size);

// updates information of an entry
void update_node(lru_cache *list, char key[], int size, char value[], int response_size, int newAge);