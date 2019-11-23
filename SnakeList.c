#include <lpc17xx.h>
#include "stdio.h"
#include "stdlib.h"

//Linked List node used for a linked list representation of the snake
//Node contains x and y positions of each node as well as the color of the node
typedef struct {
	int x;
	int y;
}coord_t;

struct SnakeList_t{
	coord_t pos;
	unsigned short color;
	struct SnakeList_t * next;
};

void addNode(struct SnakeList_t * head,int size) {
	struct SnakeList_t * newNode = NULL;
	newNode = (struct SnakeList_t *)malloc(sizeof(struct SnakeList_t));
	
	newNode->next = head->next;
	head->next = newNode;
	
	size ++;
	
	return;
}

int main(void){
	int listSize = 0;
	
	struct SnakeList_t * head = NULL;
	struct SnakeList_t * tail = NULL;
	
	head = (struct SnakeList_t *)malloc(sizeof(struct SnakeList_t));
	tail = (struct SnakeList_t *)malloc(sizeof(struct SnakeList_t));
	
	head->next = tail;
	head->pos.x = 8;
	head->pos.y = 8;
	tail->pos.x = head->pos.x - 1;
	tail->pos.y = head->pos.y;
	
	addNode(head,listSize);
	addNode(head,listSize);
	addNode(head,listSize);
	
	printf("%d \n",listSize);
}
