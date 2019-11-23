/*----------------------------------------------------------------------------
* Name:    GLCD_Scroll.c
* Purpose: Showing the a text block on Keil board display.
* Note(s):
*----------------------------------------------------------------------------
* This file is part of the UW-MTE241 course project.
*
* This software is supplied "AS IS" without warranties of any kind.
*
* Copyright (c) 2014. All rights reserved.
*----------------------------------------------------------------------------*/
#include <LPC17xx.h>
#include <stdlib.h>
#include "glcd.h"
#include "glcd_scroll.h"

#define SCREEN_SIZE  			(LCD_WIDTH * LCD_HEIGTH)

uint8_t chache[CACHE_LINE_CAP][LCD_WIDTH + 1];		//The chach size is CACHE_LINE_CAP lines of LCD_WIDTH + 1 length (Each line is an string, and string ends up to '0').

//The starting index of the cache
uint32_t 	cache_start = 0;
//How many lines are filled so far			Therefore, the last line that can be wirtten is 
size_t		cache_size = 0;	
short int last_col_cahche = 0;

uint32_t window_start = 0, window_size = 0;


void init_scroll( void ) {
	GLCD_Init(); 
	
	GLCD_Clear(BGC);
	GLCD_SetBackColor(BGC);
	GLCD_SetTextColor(TXC);
		
	cache_start = 0;
	cache_size = 0;
	last_col_cahche = 0;
	window_start = 0;
	window_size = 0;
	
}

uint32_t last_line( void ) {
	return (cache_start + cache_size) % CACHE_LINE_CAP;
}

uint32_t last_window_line( void ) {
	return (window_start + window_size) % CACHE_LINE_CAP;
}

/*
	Referesh the screen based on the stored characer in the linked list.
*/

void refresh_lcd( void ) {
	size_t	i = 0;
	GLCD_Clear(BGC);
	
	for (i = 0; i <= window_size; ++i ) {
		GLCD_DisplayString  (i, 0, 1, chache[(i + window_start) % CACHE_LINE_CAP ]);
	}
}

/*
	This function prints and records the input character. 
just_record:		If just_record is true, the character won't be printed and just recorded  in the cache.
*/
void append_char( uint8_t _char ) {
	int last_line_to_append;
	//Reached to the end of row
	if ( last_col_cahche >= LCD_WIDTH ) {
		last_col_cahche = 0;
	
		++cache_size;

		if ( cache_size >= CACHE_LINE_CAP ) {
			cache_start = (cache_start + 1) % CACHE_LINE_CAP;
			--cache_size;
		}
			
		if ( window_size >= LCD_HEIGTH - 1 ) {
			window_start = ( window_start + 1 ) % CACHE_LINE_CAP;
			--window_size;
			refresh_lcd();
		}
		++window_size;
		
		
	}
	
	if ( _char == '\n' ) {			
			last_col_cahche = LCD_WIDTH + 1;
	}else{
		last_line_to_append = window_size;
		GLCD_DisplayChar ( last_line_to_append , last_col_cahche, FONT_SIZE, _char);

		chache[last_line()][last_col_cahche] = _char;
		++last_col_cahche;
		chache[last_line()][last_col_cahche] = 0x0;
	}
	
}

void print_text( uint8_t *str_ptr, uint32_t str_len ) {
	size_t i;

	for ( i = 0; i < str_len; ++i ) {
		append_char(str_ptr[i]);
	}
	
}

void print_string( uint8_t *str_ptr ) {
	size_t i;

	for ( i = 0; i < 10000 && str_ptr[i] != 0x0; ++i ) {
		// Find location
	}
	
	print_text(str_ptr, i);
	
}

void moveUp( void ) {
	
	if ( window_start != cache_start ) {
		window_start = (window_start - 1) % CACHE_LINE_CAP;		

		if ( window_size < LCD_HEIGTH - 1 ) {
			++window_size;
		}

		refresh_lcd();
	}
	
}

void moveDown( void ) {

	if ( last_line() != last_window_line() ) {
		window_start = (window_start + 1) % CACHE_LINE_CAP;		

		if ( window_size < LCD_HEIGTH - 1) {
			--window_size;
		}

		refresh_lcd();
	}
	
}

void moveFirst( void ) {
	
	if ( window_start != cache_start ) {
		window_start = cache_start;		

		if ( cache_size >= LCD_HEIGTH ) {
			window_size = LCD_HEIGTH - 1;
		}

		refresh_lcd();
	}
}

void moveLast( void ) {
	int a, b, c;

	if ( last_line() != last_window_line() ) {
		if ( cache_size < LCD_HEIGTH ) {
			window_start = cache_start;
		} else {
			a = last_line();
			b = window_size; 
			c = (a > b ? a - b : a - b  + CACHE_LINE_CAP ) ;
			window_start = c % CACHE_LINE_CAP;
		}

		refresh_lcd();
	}
}


void initJoyStick( void ) {
	//P1.23 Up
	LPC_PINCON->PINSEL3 &= ~(0x03<<14); 				// P1.23 is GPIO
	LPC_GPIO1->FIODIR   &= ~(1<<23); 				// P1.23 is input 

	//P1.25 Down
	LPC_PINCON->PINSEL3 &= ~(0x03<<18); 				// P1.25 is GPIO
	LPC_GPIO1->FIODIR   &= ~(1<<25); 				// P1.25 is input 

	//P1.24 Last
	LPC_PINCON->PINSEL3 &= ~(0x03<<16); 				// P1.24 is GPIO
	LPC_GPIO1->FIODIR   &= ~(1<<24); 				// P1.24 is input 

	//P1.26 First
	LPC_PINCON->PINSEL3 &= ~(0x03<<20); 				// P1.26 is GPIO
	LPC_GPIO1->FIODIR   &= ~(1<<26); 				// P1.26 is input 
}


void joyStickBusyWaitingMonitor( void ) {
	uint32_t i;
	
	while ( 1 ) {
		
		if (        ~(LPC_GPIO1->FIOPIN | ~UP   ) ) {
			moveUp();
		} else if ( ~(LPC_GPIO1->FIOPIN | ~DOWN ) ) {
			moveDown();
		} else if ( ~(LPC_GPIO1->FIOPIN | ~FIRST) ) {
			moveFirst();
		} else if ( ~(LPC_GPIO1->FIOPIN | ~LAST ) ) {
			moveLast();
		}

		for ( i = 0; i < 100000; ++i ) {
			// pause
		}
	}
}
