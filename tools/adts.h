#ifndef ADTS_H
#define ADTS_H

typedef struct map map;
typedef struct mapNode mapNode;
typedef struct vector vector;
typedef struct vectorNode vectorNode;

struct map {
  mapNode *head;
  int     size;
};

struct mapNode {
  mapNode    *next;
  char       *key;
  uint32_t   value;
};

struct vectorNode {
  vectorNode *previous;
  char       *value;
  vectorNode *next;
};

struct vector {
  vectorNode *first;
  vectorNode *last;
  int size;
};

/* returns a copy on the heap of the original string */
char *copy(const char *original);

/* Constructor function that will retrun an empty map */
map constructMap(void);

/* Clears the map and frees all elements*/
void clearMap(map *m);

/* Checks if map is empty */
bool isEmptyMap(map m);

/**
* Retrives the pointer to the value of the key
* Returns NULL if nothing is found
**/
uint32_t *get(map m, char *key);

/**
* If the key is found the value is modified to the value parameter
* If the key is not found the pair (key, value) is added to the table
**/
void put(map *m, char *key, uint32_t value);

/**
* Helper function
* Takes 3 parameters: pointer to table, a key, a pointer which will be
* modified to the element which maches the key or the last element if nothing is
* found or NULL if there are no elements
* Returns false if element is found
* Retruns true if element isn't found
**/
bool lookup(map m, char *key, mapNode **ptr);

/* Prints map */
void printMap(map m);

/* Constructor function that will retrun an empty vector */
vector constructVector(void);

/* Clears the vector and frees all elements*/
void clearVector(vector *v);

/* Checks if vector is empty */
bool isEmptyVector(vector v);

/* Adds a copy of the value parameter to the front */
void putFront(vector *v, char *value);

/* Adds a copy of the value parameter to the back */
void putBack(vector *v, char *value);

/**
* Retruns the front value and removes it
* Returns NULL if vector is empty
**/
char *getFront(vector *v);

/**
* Retruns the back value and removes it
* Returns NULL if vector is empty
**/
char *getBack(vector *v);

/**
* Retruns the front value without removing it
* Returns NULL if vector is empty
**/
char *peekFront(vector v);

/**
* Retruns the back value without removing it
* Returns NULL if vector is empty
**/
char *peekBack(vector v);

/* Prints vector */
void printVector(vector v);

/* Returns true iff the element is in the vector */
bool contains(vector v, char *value);

#endif
