#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "lru.h"

lru_cache *new_list(){
    lru_cache *list = NULL;
    list = (lru_cache*)malloc(sizeof(lru_cache));
    assert(list!=NULL);
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

char* remove_head(lru_cache *list) {
    list->head = list->head->next;
    char* key = strdup(list->head->prev->key);
    free(list->head->prev);
    list->head->prev = NULL;
    list->size--;
    return key;
}

void insert_tail(lru_cache *list, char key[], char value[], int response_size, int age){
    lru_cache_node *new_node = (lru_cache_node*)malloc(sizeof(lru_cache_node));
    assert(new_node!=NULL);
    strncpy(new_node->key, key, MAX_CACHE_REQ);
    new_node->key[MAX_CACHE_REQ] = '\0';

    memcpy(new_node->value, value, MAX_RESPONSE);
    new_node->value[MAX_RESPONSE] = '\0';

    new_node->response_size = response_size;

    new_node->age = age;

    new_node->prev=NULL;
    new_node->next = NULL;

    if (list->tail == NULL) {
        //first insertion
        list->head = new_node;
        list->tail = new_node;
    } else {
        list->tail->next = new_node;
        new_node->prev = list->tail;
        list->tail = new_node;
    }
    list->size++;
}

//updates an entry that has been used, maintains that the LRU entry be at the head
void update_lru(lru_cache *list, char key[]) {
    lru_cache_node *ptr = list->head;
    while (ptr != NULL && strcmp(ptr->key, key) != 0) {
        ptr = ptr -> next;
    }

    if (ptr->next == NULL || ptr == list->tail) {
        return;
    }

    if (ptr->prev == NULL) {
        list->head = list->head->next;
        list->head->prev = NULL;
        ptr->prev = list->tail;
        ptr->next = NULL;
        list->tail->next = ptr;
        list->tail = ptr;

    } else {
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
        ptr->next = NULL;

        list->tail->next = ptr;
        list->tail = ptr;
    
    }
    
    
}

void print_list(lru_cache *list) {
    if (list->head == NULL) {
        printf("list is empty\n");
        return;
    }
    lru_cache_node *ptr = list->head;
    while (ptr != NULL) {
        printf("list is %s\n", ptr->key);
        ptr = ptr->next;
    }
}

char* get_cached_response(lru_cache *list, char key[], int size) {
    lru_cache_node *ptr = list->head;
    while (ptr != NULL) {
        if (memcmp(ptr->key, key, size) == 0) {
            return ptr->value;
        }
        ptr = ptr->next;
    }
    return NULL;
}

int get_cached_response_size(lru_cache *list, char key[], int size) {
    lru_cache_node *ptr = list->head;
    while (ptr != NULL) {
        if (memcmp(ptr->key, key, size) == 0) {
            return ptr->response_size;
        }
        ptr = ptr->next;
    }
    return 0;
}



void free_list(lru_cache *list){
    lru_cache_node *curr, *previous;
    assert(list!=NULL);
	curr = list->head;
	while (curr) {
		previous = curr;
		curr = curr->next;
		free(previous);
	}
	free(list);
}

void decrement_age(lru_cache *list) {
    lru_cache_node *ptr = list->head;
    while (ptr != NULL) {
        ptr->age--;
        ptr = ptr->next;
    }
}

//gets the age remaining of an entry in the cache
int get_age(lru_cache *list, char key[], int size){
    lru_cache_node *ptr = list->head;
    while (ptr != NULL) {
        if (memcmp(ptr->key, key, size) == 0) {
            return ptr->age;
        }
        ptr = ptr->next;
    }
    return 0;
}

//removes a specific entry from the cache
void remove_entry(lru_cache *list, char key[], int size) {
    if (list == NULL || list->head == NULL) {
        return;
    }

    lru_cache_node *ptr = list->head;
    while (ptr != NULL) {
        if (memcmp(ptr->key, key, size) == 0) {
            if (ptr->prev != NULL) {
                ptr->prev->next = ptr->next;
            } else {
                list->head = ptr->next;
            }
            if (ptr->next != NULL) {
                ptr->next->prev = ptr->prev;
            } else {
                list->tail = ptr->prev;
            }

            free(ptr);
            list->size--;
            return;
        }
        ptr = ptr->next;
    }
}

//used for updating stale etnries with new information
void update_node(lru_cache *list, char key[], int size, char value[], int response_size, int newAge) {
    lru_cache_node *ptr = list->head;
    while (ptr != NULL) {
        if (memcmp(ptr->key, key, size) == 0) {
            memcpy(ptr->value, value, MAX_RESPONSE);
            ptr->value[MAX_RESPONSE] = '\0';
            ptr->response_size = response_size;
            ptr->age = newAge;
            return;
        }
        ptr = ptr->next;
    }
}

