#include <stdlib.h>
#include <stdio.h>
#include "dbg.h"
#include "linked_lists.h"

NODE *createList(NODE *start){
   List *list1;
   CreateList(&list1);
   appendAtEnd(list1, 1);
   printf("HEEEEY %d\n", list1->head->value);

   int i,n,data;
   printf("Enter the number of nodes : ");
   scanf("%d", &n);

   if(n == 0)
	   return start;
   printf("Enter the first node to be entered in list\n");
   scanf("%d", &data);
   start = insertInBeginning(start, data);
   printf("node value of first %d\n", start->value);

   for(i = 2; i <= n; ++i){
      printf("Enter the next element to be inserted : ");
      scanf("%d", &data);
      insertAtEnd(list1, start,data);
   }

   return start;
}

NODE *deleteNodeTest(NODE *start)
{
   int value;
   printf("enter node to be deleted:\n");
   scanf("%d", &value);
   //printf("we're alive still");
   start = removeNode(start, value);
   return start;
}

int main(){
   NODE *start = NULL;
   start = createList(start);
   printList(start);
   start = deleteNodeTest(start);
   printList(start);
   printf("Let's reverse the list\n");
   start = reverseList(start);
   printList(start);
   return 0;
}
