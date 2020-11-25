#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "adts.h"

char *copy(const char *original) {
  char *res = NULL;

  res = malloc ((strlen(original) + 1) * sizeof(char));

  if (!res) {
    // malloc has failed exit
    fprintf(stderr, "The malloc from the copy function has failed\n");
    exit(EXIT_FAILURE);
  }

  strcpy(res, original);

  return res;
}

void clearMap(map *m) {
  mapNode *ptr = m->head;
  while(ptr) {
    mapNode *prev = ptr;
    ptr = ptr->next;
    free(prev->key);
    free(prev);
  }
  m->head = NULL;
  m->size = 0;
}

map constructMap(void) {
  map m = {NULL, 0};
  return m;
}

uint32_t *get(map m, char *key) {
  mapNode *ptr = NULL;

  return lookup(m, key, &ptr) ? &(ptr->value) : NULL;
}

bool isEmptyMap(map m) {
  return !m.size;
}

void put(map *m, char *key, uint32_t value) {
  mapNode *ptr = NULL;
  if(lookup(*m, key, &ptr)) {
    // if found
    ptr->value = value;
  } else {
    // if not found
    mapNode *pNewNode = malloc(sizeof(mapNode));
    pNewNode->next    = NULL;
    pNewNode->key     = copy(key);
    pNewNode->value   = value;
    if (!ptr) {
      // no elemnts in mapping
      m->head = pNewNode;
    } else {
      // make last element point to newNode
      ptr->next = pNewNode;
    }
    m->size++;
  }
}

bool lookup(map m, char *key, mapNode **ptr) {
  *ptr = m.head;
  while (m.head) {
    *ptr = m.head;
    if (!strcmp(m.head->key, key)) {
      // if keys match
      return true;
    }
    m.head = m.head->next;
  }
  return false;
}

void printMap(map m) {
  printf("Map: HEAD: %p, SIZE: %d\n", (void *) m.head, m.size);
  printf("%s\t%s\t%s\n", "NEXT", "KEY", "VALUE");
  while (m.head) {
    printf("%p\t%s\t%d\n", (void *) m.head->next,
                                m.head->key, m.head->value);
    m.head = m.head->next;
  }
  puts("");
}

void clearVector(vector *v) {
  while (!isEmptyVector(*v)) {
    free(getFront(v));
  }
  v->size = 0;
}

vector constructVector(void) {
  vector v = {NULL, NULL, 0};
  return v;
}

void putFront(vector *v, char *value) {
  vectorNode *pNv = malloc(sizeof(vectorNode));
  pNv->previous = NULL;
  pNv->value = copy(value);
  pNv->next = NULL;

  if (isEmptyVector(*v)) {
    v->first = pNv;
    v->last  = pNv;
  } else {
    pNv->next = v->first;
    v->first->previous = pNv;
    v->first = pNv;
  }
  (v->size)++;
}

void putBack(vector *v, char *value) {
  vectorNode *pNv = malloc(sizeof(vectorNode));
  pNv->previous = NULL;
  pNv->value = copy(value);
  pNv->next = NULL;

  if (isEmptyVector(*v)) {
    v->first = pNv;
    v->last  = pNv;
  } else {
    pNv->previous = v->last;
    v->last->next = pNv;
    v->last = pNv;
  }
  (v->size)++;
}

char *peekFront(vector v) {
  if (isEmptyVector(v)) {
    return NULL;
  }

  return v.first->value;
}

char *peekBack(vector v) {
  if (isEmptyVector(v)) {
    return NULL;
  }

  return v.last->value;
}

char *getFront(vector *v) {
  if (isEmptyVector(*v)) {
    return NULL;
  }

  char *ret = peekFront(*v);

  // remove first node and free memory
  vectorNode *removedNode = v->first;
  v->first = removedNode->next;
  if (!isEmptyVector(*v)) {
    v->first->previous = NULL;
  } else {
    v->last = NULL;
  }
  free(removedNode);
  (v->size)--;

  return ret;
}

char *getBack(vector *v) {
  if (isEmptyVector(*v)) {
    return NULL;
  }

  char *ret = peekFront(*v);
  // remove last node and free memory
  vectorNode *removedNode = v->last;
  v->last = removedNode->previous;
  if (!isEmptyVector(*v)) {
    v->last->next = NULL;
  } else {
    v->first = NULL;
  }
  free(removedNode);
  (v->size)--;

  return ret;
}

bool isEmptyVector(vector v) {
  return !v.first || !v.last;
}

void printVector(vector v) {
  printf("Vector: FIRST: %p, LAST: %p, SIZE: %d\n", (void *) v.first,
                  (void *) v.last, v.size);
  printf("%s\t%s\t%s\n", "PREVIOUS", "VALUE", "NEXT");
  while (v.first) {
    printf("%p\t%s\t%p\n", (void *) v.first->previous,
                        v.first->value, (void *) v.first->next);
    v.first = v.first->next;
  }
  puts("");
}

bool contains(vector v, char *value) {
  vectorNode *current = v.first;
  while (current) {
    if (!strcmp(current->value, value)) {
      // vector contains the item
      return true;
    }
    current = current->next;
  }

  return false;
}
