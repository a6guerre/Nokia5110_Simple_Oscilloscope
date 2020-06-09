/*
 * Single linked list API
 * Author: Max Guerrero
 *
 */

#include <stdlib.h>
#include <stdio.h>
//#include "dbg.h"
#include "linked_lists.h"

void *SafeMalloc(size_t size)
{
   void *vp;
   if((vp = malloc(size)) == NULL){
      //fprintf(stderr, "Out of memory\n");
      return NULL;
   }
   return vp;
}

status CreateList(List **list)
{
   List *new_list = (List *)SafeMalloc(sizeof(List));
   if(!new_list)
      return FAIL;
   new_list->head = NULL;
   new_list->tail = NULL;
   new_list->size = 0;
   *list = new_list;
   return OK;
}

bullet_node *insertInBeginning(bullet_node *start, int idx) {
   bullet_node *newNode = SafeMalloc(sizeof(bullet_node));
   newNode->idx = idx;  // Keep track of each node's unique identifier for removal later.
   newNode->posX = 0;
   newNode->posY = 0;
   newNode->next = NULL;
   return newNode;
}

bullet_node *removeNode(List **list, bullet_node **start, int data) {
   // special case: delete first node
   bullet_node *ptr =  (*list)->head;
   bullet_node *temp = NULL;
   //debug("we're alive still");
   if(ptr->idx == data)
   {
      temp = *start;
      *start = temp->next;
      (*list)->head = *start;
      free(temp);
      //debug("new start ptr value %d", start->idx);
      return *start;
   }

   while((ptr->next)->idx != data && ptr->next != NULL)
   {
     //printf("hey\n");
     ptr = ptr->next;
     //debug("current pointer value: %d", ptr->idx);
   }

   if(ptr->next != NULL)
   {
     //debug("node found, deleting");
     temp = ptr->next;
     ptr->next = temp->next;
     free(temp);
     return *start;
   }
   else
   {
      //printf("pointer not found\n");
      return *start;
   }

   return NULL;
}

void insertAtEnd(List *list, bullet_node *crtNode, int idx)
{
   while(crtNode->next != NULL){
      crtNode = crtNode->next;
   }
   bullet_node *newNode = SafeMalloc(sizeof(bullet_node));
   newNode->idx = idx;  // Each new element has an idx 
                        // which is in which position of the list it is.
   newNode->posX = 0;
   newNode->posY = 0;
   newNode->next = NULL;
   crtNode->next = newNode;
   list->tail = newNode;
   if(newNode->next == NULL)
   {
     list->tail = newNode;
   }
}

status appendAtEnd(List *list, int value)
{
   if (list->size == 0)
   {
      ++list->size;
      list->head = insertInBeginning(list->head, list->size);
      list->head->next = NULL;
      list->tail = list->head; // for the special case that we created a new list
      return OK; 
   }
   else
   {
      insertAtEnd(list, list->head,  list->size);
      ++list->size;
      return OK;
   }
}

bullet_node *reverseList(bullet_node *start)
{
   bullet_node *prev, *next, *ptr;
   ptr = start;

   next = ptr->next;
   prev = ptr;
   ptr->next = NULL;
   ptr = next;
  
   while(ptr->next != NULL)
   {
      next = ptr->next;
      ptr->next = prev;
      prev = ptr;
      ptr = next;
   }
   ptr->next = prev;
   start = ptr;

   return start;
}

void printList(bullet_node *crtNode){
   int count = 1;
   while(crtNode->next != NULL) {
      //printf("Value at node %d: %d\n", count, crtNode->idx);
      crtNode = crtNode->next;
      count++;
   }
   //printf("Value at node %d: %d\n", count, crtNode->idx);
}

