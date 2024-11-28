#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>

// Struct definitions
typedef struct {
    char *key;
    char *value;
} hash_item;

typedef struct {
    int size;
    int count;
    int base_size;
    hash_item **items;
} hash_table;

// Function prototypes
hash_table *new_sized(const int base_size);
hash_table* new(void);
void del_hashtable(hash_table *ht);
void hash_insert(hash_table* ht, const char* key, const char* value);
char* hash_search(hash_table* ht, const char* key);
void hash_delete(hash_table* ht, const char* key);

// Breakpoint handling (using hash table for breakpoint storage)
void add_breakpoint(hash_table *ht, intptr_t address, const char *breakpoint_info);
const char* get_breakpoint_info(hash_table *ht, intptr_t address);
void remove_breakpoint(hash_table *ht, intptr_t address);

#endif // HASHTABLE_H
