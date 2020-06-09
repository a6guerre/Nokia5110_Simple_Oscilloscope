#ifndef LINKED_LISTS
#define LINKED_LISTS

#include <stdint.h>

typedef struct Node {
   int idx;
   unsigned char posX;
   unsigned char posY;
   struct Node  *next;
   uint8_t visibility;
}bullet_node;

typedef struct {
   bullet_node *head;
   bullet_node *tail;
   int  size;   
} List;


typedef enum
{
  FAIL,
  OK
} status;

void *SafeMalloc(size_t size);
status CreateList(List **list);
status appendAtEnd(List *list, int value);
bullet_node *insertInBeginning(bullet_node *start, int data);
void insertAtEnd(List *list, bullet_node *crtNode, int data);
void printList(bullet_node *crtNode);
bullet_node *removeNode(List **list, bullet_node **start, int data);
bullet_node *reverseList(bullet_node *start);

#endif
