#ifndef _GLCD_SCROLL_H
#define _GLCD_SCROLL_H

#define BGC 		Black
#define TXC 		White
#define LCD_WIDTH	20
#define LCD_HEIGTH	10
#define null_ptr 	((void *) 0)
#define FONT_SIZE 	1

#define CACHE_LINE_CAP	25	//How many line can be preserved in the lcd cache.

#define UP 	0x0800000
#define DOWN 	0x2000000
#define LAST 	0x1000000
#define FIRST 	0x4000000

void init_scroll( void );
void print_text( unsigned char *str_ptr, unsigned int str_len );
void print_string( unsigned char *str_ptr );
void append_char( unsigned char _char );
void initJoyStick( void );
void joyStickBusyWaitingMonitor( void );

#endif /* _GLCD_SCROLL_H */
