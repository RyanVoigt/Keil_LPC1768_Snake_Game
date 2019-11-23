#include <lpc17xx.h>
#include "stdio.h"
#include "stdlib.h"
#include "uart.h"
#include "GLCD.h"
#include <cmsis_os2.h>
#include "stdbool.h"

/*
NOTE:
The following code reflects the more optimized version of the game implementation as 
opposed to the original design document. A switch was made from 6 threads to 4 threads, 
by combining the food generation, collision and graphics rendering threads into a single
task. This was done to prevent processor time from being spent on checking for food regeneration
and collision if the snake had not yet moved. The following implementation causes this 
logic to only be evaluated when the snakes motion is altered and its position is refreshed on the
screen.
*/


/* ---------------------------Global Variables ---------------------------*/

//Screen Global Dimensions and Display Parameters
#define WIDTH 320              
#define HEIGHT 240
#define GRID_WIDTH 15             
#define GRID_HEIGHT 15
#define SNAKE_SIZE 30
unsigned short SNAKE_COLOR = Purple;
unsigned short GRASS_COLOR = Green;
unsigned short HEAD_COLOR = White;
unsigned short FOOD_COLOR = Red;

//Data protection/initilization globals
void INT0Init(void);
osMutexId_t mutex;

//Logical Globals used for the gameplay flow
int score = 0;
int gameOver = 0;
uint32_t dir = 1;
double speed; 

//Data structure for the storing the individual body segments of the snake
typedef struct node{
	int xpos;
	int ypos;
	unsigned short color;
	struct node * next;
}node_t;

//Data structure for storing the overall snake. Includes the length of the snake, 
// as well as points to the start and end of the snake. Note in this implementation, 
// the tail node describes a node resembling grass behind the snake
typedef struct{
	int length;
	struct node * head;
	struct node * tail;
}snakeList_t;

//Define the snake global snake list
snakeList_t snake;

// Data structure used for storing the newly generated food points that are consumed by 
// the snake
typedef struct{
	int x;
	int y;
}Food_t;

//initialize the first food location
Food_t foodLocation = {5, 7};


//direction string as an integer
enum joy_stick_dir{
			none, right, down, left, up
};


/* ---------------------------Global Methods ---------------------------*/

//The fillCoord funtion is used to color the individual segments of the screen
//as well as the various body segments of the snake. 
void fillCoord(uint32_t x, uint32_t y, unsigned short color){
		for(int i=0; i<GRID_WIDTH;i++){
			uint32_t pixel_x = i + GRID_WIDTH*x;
			for(int j=0;j<GRID_HEIGHT;j++){
				GLCD_SetTextColor(color);
				uint32_t pixel_y = j + GRID_HEIGHT*y;				
				GLCD_PutPixel(pixel_x,pixel_y);
			}
		}
 }
 
 
/* ---------------------------Snake Data Structure Methods ---------------------------*/
//Linked List node used for a linked list representation of the snake
//Node contains x and y positions of each node as well as the color of the node

// addBodySeg increases the length of snake by inserting a node at the back of the 
// Linked List
void addBodySeg(snakeList_t * snake) {
	//create a new node to be inserted
	node_t * newNode = (node_t *)malloc(sizeof(node_t));
	newNode->next = NULL;
	
	//configure the node to the settings used for the tail node
	newNode->xpos = snake->tail->xpos;
	newNode->ypos = snake->tail->ypos;
	newNode->color = GRASS_COLOR;
	
	//cause the current tail to become a body segment
	snake->tail->color= SNAKE_COLOR;
	snake->tail->next = newNode;
	snake->tail = newNode; 
	
	//increment the length of the snake
	snake->length++;
}

// snakeMove triggers an update of the nodes of the global snake which rewrites the
// position of the snake nodes
void snakeMove (snakeList_t * snake){
		//initialize the necessary update values
		int delX=0, delY = 0;
		int xCurr=0,yCurr=0,xNew=0,yNew=0;
	
		//acquire the mutex and read the current value of the snake direction variable
		osMutexAcquire(mutex,osWaitForever);
		if (dir == up)delY = -1;
		else if (dir==down)delY = 1;
		else if (dir==right)delX=1;
		else if (dir==left)delX=-1;
		osMutexRelease(mutex);
	
		node_t * nodeCurr = snake->head;
	
		xNew = nodeCurr->xpos;
		yNew = nodeCurr->ypos;
		
		nodeCurr->xpos = xNew + delX;
		nodeCurr->ypos = yNew + delY;
	
		nodeCurr = nodeCurr->next;
	
		//propogate the previous positions throughout the snake 
		for(int i =0; i<snake->length-1;i++){
			xCurr = nodeCurr->xpos;
			yCurr = nodeCurr->ypos;
	
			nodeCurr->xpos = xNew;
			nodeCurr->ypos = yNew;
			
			xNew = xCurr;
			yNew = yCurr;
			
			nodeCurr = nodeCurr->next;
		}
}

// printSnake is used to render the snake on the GLCD screen
void printSnake(void){
		node_t * curr = snake.head;
		while(curr != NULL){
			fillCoord(curr->xpos,curr->ypos,curr->color);
			curr = curr->next;
		}
	}


/* ---------------------------Food Data Structure Methods --------------------------------*/
//date structure used to produce food tokens on the screen by randomly generating positions
// for the food to appear
 void foodGeneration(void){
	foodLocation.x = (rand() % (19));
	foodLocation.y = (rand() % (14));
	fillCoord(foodLocation.x,foodLocation.y,FOOD_COLOR);
}
 
/* ---------------------------Snake Collision Method -------------------------------------*/
int collision(void){
		//border collison, acquire the mutex to read the position of the snake before rewriting it
		osMutexAcquire(mutex,osWaitForever);
		if(snake.head->xpos > 20 || snake.head->ypos < 0 || snake.head->ypos > 15  || snake.head->xpos < 0){
			gameOver = 1;
		}
		
		//Food Collision and update score on LED
		if(snake.head->ypos == foodLocation.y && snake.head->xpos == foodLocation.x){
			foodGeneration();
			addBodySeg(&snake);
			score++;
		}
		node_t * nodeCurr = snake.head->next;
		
		//Checking for body collisions
		for(int i =0; i<snake.length-2;i++){
			if(snake.head->ypos ==  nodeCurr->ypos && snake.head->xpos == nodeCurr->xpos){
				gameOver = 1;
			}
			nodeCurr = nodeCurr->next;
		}
		osMutexRelease(mutex);
		return 1;
}



/* ---------------------------Thread 1: Graphics Rendering -------------------------------------*/
 void GraphicsDisplay(void *args){
	 GLCD_Init();
	 GLCD_Clear(Black);
	 GLCD_SetBackColor(Black);
	 
	 //create the introduction screen. will remain until int0 is pressed.
	 unsigned char s[18]="SNAKE ON THE LOOSE";
	 unsigned char control1[30]="USE JOYSTICK TO CONTROL SNAKE";
	 unsigned char control2[35]="USE POTENTIOMETER INCREASE SPEED";
	 unsigned char b[18]="PRESS INT0 TO PLAY";
	 GLCD_SetTextColor(Red);
	 GLCD_DisplayString(14, 1,1,s);
	 GLCD_DisplayString(16, 10,0,control1);
	 GLCD_DisplayString(18, 9,0,control2);
	 GLCD_DisplayString(20, 16,0,b);
	 
	 //monitor for button press to start the game
	 int enterGame = 1;
	 while(enterGame){
		 if((LPC_GPIO2->FIOPIN & (1 << 10))==0){
			 enterGame = 0;
		 }
	 }
	 
	//creating the grid
	GLCD_Clear(GRASS_COLOR);
	foodGeneration();
	 
	//main game functionality and rendering
	while(!gameOver){
		snakeMove(&snake);
		collision();
		printSnake();
		osDelay(speed);
		osThreadYield();
	}
	
	//game over screen rendering
		GLCD_Clear(Black);
		GLCD_SetBackColor(Black);
		unsigned char gameov[18]="GAME OVER";
		unsigned char restboard[18]="PLEASE RESET BOARD";
		unsigned char scorechar[20];
		sprintf(scorechar, "SCORE: %-7d", score);
		GLCD_SetTextColor(Red);
		GLCD_DisplayString(13, 5,1,gameov);
		GLCD_DisplayString(15, 5,1,scorechar);
		GLCD_DisplayString(20, 16,0,restboard);
	}
	
	
	/* ---------------------------Thread 2: Joystick Control Polling -------------------------------------*/
	// function that polls the joystick and returns the current state
  int get_dir(void){
	 int value = none;
	 if( !(LPC_GPIO1->FIOPIN & (1 << 24)) && dir != left) value = right;
	 else if( !(LPC_GPIO1->FIOPIN & (1 << 23)) && dir != down) value = up;
	 else if( !(LPC_GPIO1->FIOPIN & (1 << 25)) && dir != up) value = down; 
	 else if( !(LPC_GPIO1->FIOPIN & (1 << 26)) && dir != right) value = left;
	 return value;
 } 

 // thread to update the current direction of the snake's motion
 void joyStick(void *args){
	 bool no_dir = true;
	 int action;
	 
	while(1){
		action = get_dir();
	
		if(no_dir && (action != none)){
			no_dir = false;
			osMutexAcquire(mutex,osWaitForever);
			dir = action;
			osMutexRelease(mutex);
		}
		else if (action == none){
			no_dir = true;
		}
		osThreadYield();
	}
}
 
/* ---------------------------Thread 3: Speed Control -------------------------------------*/
 void potentiometer(void *args){
	 while(1){
	 	//taking input from potentiometer 
		LPC_ADC->ADCR |= (1 << 24);
		osMutexAcquire(mutex,osWaitForever);
		speed = ((LPC_ADC->ADGDR & 0xfff0) >> 4);
		// converts the 27 -> 4075 range to between 50 and 450
		speed = (speed/4075)*400+50;
		osMutexRelease(mutex);
		osThreadYield();
	}
 }


/* ---------------------------Thread 4: LED Display -------------------------------------*/
 //thread to update the LED's with the current score of the game 
 void scoreupdater(void *args){
	 while(1){
			LPC_GPIO1->FIOCLR |= 0xB0000000;
			LPC_GPIO2->FIOCLR |= 0x0000007C;
			if( (score & 1) == 1)
				LPC_GPIO2->FIOSET |= 0x00000040;
			if( (score & 2) == 2)
				LPC_GPIO2->FIOSET |= 0x00000020;
			if( (score & 4) == 4)
				LPC_GPIO2->FIOSET |= 0x00000010;
			if( (score & 8) == 8)
				LPC_GPIO2->FIOSET |= 0x00000008;
			if( (score & 16) == 16)
				LPC_GPIO2->FIOSET |= 0x00000004;
			if( (score & 32) == 32)
				LPC_GPIO1->FIOSET |= 0x80000000;
			if( (score & 64) == 64)
				LPC_GPIO1->FIOSET |= 0x20000000;
			if( (score & 128) == 128)
				LPC_GPIO1->FIOSET |= 0x10000000;
			osThreadYield();
		}
 }

 
 
int main(void){
		printf("starting project \n");
		
		// create the head and tail nodes of the snake
		node_t head;
		node_t tail;
		
		//initialize the snake properties for when the game begins
		head.xpos = 8; head.ypos=8;
		head.color = HEAD_COLOR;
		head.next = &tail; 
		
		tail.xpos = 9; tail.ypos=8;
		tail.color = GRASS_COLOR;	
		tail.next = NULL;
		
		snake.head = &head;
		snake.tail = &tail;
		snake.length = 2;
	
		//turning on I/O devices and AC/DC converter
		LPC_GPIO1->FIODIR |= 0xB0000000;
		LPC_GPIO2->FIODIR |= 0x0000007C;
		LPC_GPIO1->FIOCLR |= 0xB0000000;
		LPC_GPIO2->FIOCLR |= 0x0000007C;
		LPC_PINCON->PINSEL1 &= ~(0x03<<18);
		LPC_PINCON->PINSEL1 |= (0x01<<18);
		LPC_SC->PCONP |= (1 << 12); 
		LPC_ADC->ADCR = (1 << 2) | (4 << 8) | (1 << 21);
		
		//generate the seed for the food generation call
		srand(1);
		
		//initialize the threads and mutex being used
		osKernelInitialize();
		mutex = osMutexNew(NULL);
		osThreadNew(GraphicsDisplay, NULL, NULL);
		osThreadNew(joyStick, NULL, NULL);
		osThreadNew(potentiometer, NULL, NULL);
		osThreadNew(scoreupdater, NULL, NULL);
		osKernelStart();
		
		while(1);
}
