typedef struct listnode {
  char *key;
  char *value;
  struct listnode *next;
} entry_t;

typedef struct map {
  entry_t **table;
} hashmap;

unsigned long hash(unsigned char *str);
hashmap *create_map();
int add_entry(hashmap* map, char *key, char *value);
void *search(hashmap *map, char *key);
int remove_entry(hashmap *map, char *key);
void free_map(hashmap *map);
void print_map(hashmap *map);