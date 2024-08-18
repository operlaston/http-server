#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

#define MAP_SIZE (16)
#define FOUND (1)
#define NOT_FOUND (0)

unsigned long hash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash % MAP_SIZE;
}

hashmap *create_map() {
  hashmap *map = malloc(sizeof(hashmap));
  map->table = malloc(sizeof(entry_t *) * MAP_SIZE);
  for (int i = 0; i < MAP_SIZE; i++) {
    map->table[i] = NULL;
  }
  return map;
}

int add_entry(hashmap *map, char *key, char *value) {
  entry_t *entry = malloc(sizeof(entry_t));
  entry->key = malloc(strlen(key) + 1);
  entry->value = malloc(strlen(value) + 1);
  entry->next = NULL;
  strcpy(entry->key, key);
  strcpy(entry->value, value);
  int pos = hash((unsigned char *)key);
  if (map->table[pos] == NULL) {
    map->table[pos] = entry;
    return NOT_FOUND;
  }
  entry_t *prev = NULL;
  entry_t *curr = map->table[pos];
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      if (prev == NULL) {
        map->table[pos] = entry;
      }
      else {
        prev->next = entry;
      }
      entry->next = curr->next;
      free(curr->key);
      free(curr->value);
      free(curr);
      return FOUND;
    }
    prev = curr;
    curr = curr->next;
  }
  prev->next = entry;
  return NOT_FOUND;
}

void *search(hashmap *map, char *key) {
  int pos = hash((unsigned char *)key);
  entry_t *curr = map->table[pos];
  while(curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      return curr->value;
    }
    curr = curr->next;
  }
  return NULL;
}

int remove_entry(hashmap *map, char *key) {
  int pos = hash((unsigned char *)key);
  entry_t *prev = NULL;
  entry_t *curr = map->table[pos];
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      if (prev == NULL) {
        map->table[pos] = curr->next;
      }
      else {
        prev->next = curr->next;
      }
      free(curr->key);
      free(curr->value);
      free(curr);
      return FOUND;
    }
    prev = curr;
    curr = curr->next;
  }
  return NOT_FOUND;
}

void free_map(hashmap *map) {
  for (int i = 0; i < MAP_SIZE; i++) {
    entry_t *curr = map->table[i];
    while (curr != NULL) {
      entry_t *next = curr->next;
      free(curr->key);
      free(curr->value);
      free(curr);
      curr = next;
    }
  }
  free(map->table);
  free(map);
}

void print_map(hashmap *map) {
  for (int i = 0; i < MAP_SIZE; i++) {
    entry_t *curr = map->table[i];
    while (curr != NULL) {
      printf("{%s, %s} ", curr->key, curr->value);
      curr = curr->next;
    }
  }
  printf("\n");
}