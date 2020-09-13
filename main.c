/* Tiny TFT Graphics Library - see http://www.technoblogy.com/show?L6I

   David Johnson-Davies - www.technoblogy.com - 13th June 2019
   ATtiny85 @ 8 MHz (internal oscillator; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

#define F_CPU 8000000UL                 // set the CPU clock
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "st7735.h"
#include "grn_TWI.h"


#define BACKLIGHT_ON PORTB |= (1<<PB2)
#define BACKLIGHT_OFF PORTB &= ~(1<<PB2)						

#define LED_OFF PORTC &= ~(1<<PC3)
#define LED_ON PORTC |= (1<<PC3)

/* some RGB color definitions                                                 */
#define BLACK        0x0000      
#define RED          0x001F      
#define GREEN        0x07E0      
#define YELLOW       0x07FF      
#define BLUE         0xF800      
#define CYAN         0xFFE0      
#define White        0xFFFF     
#define BLUE_LIGHT   0xFD20      
#define TUERKISE     0xAFE5      
#define VIOLET       0xF81F		
#define WHITE		0xFFFF

#define SEK_POS 10,110

#define RELOAD_ENTPRELL 1 


#define TW_START 0xA4 // send start condition (TWINT,TWSTA,TWEN)
#define TW_READY (TWCR & 0x80) // ready when TWINT returns to logic 1.
#define TW_STATUS (TWSR & 0xF8) // returns value of status register
#define TW_SEND 0x84 // send data (TWINT,TWEN)

#define 	B_BLUE		!(PIND & (1<<PD6))
#define 	B_GREEN		!(PIND & (1<<PD2))
#define	B_YELLOW	!(PIND & (1<<PD5))

#define F_SCL 100000L // I2C clock speed 100 kHz
//#define F_CPU 800000L // I2C clock speed 100 kHz


// Pins already defined in st7735.c
extern int const DC;
extern int const MOSI;
extern int const SCK;
extern int const CS;
// Text scale and plot colours defined in st7735.c
extern int fore; 		// foreground colour
extern int back;      	// background colour
extern int scale;     	// Text size


volatile uint8_t ms10,ms100,sec,min, entprell;
volatile uint16_t speedtest, speedflag;

char stringbuffer[20]; // buffer to store string 
uint8_t addresse, position;//test twi ausgabe

ISR (TIMER1_COMPA_vect);
void twi_Init(void);
void twi_Start(void);
void twi_Stop(void);
void twi_Write(uint8_t u8data);
uint8_t twi_GetStatus(void);

void SPI_MasterTransmit(char cData);

uint8_t ballposx, ballposy, ballvert, ballhoriz, ballspeed,fresh_ball;
uint8_t balkposx, balksize;

void init_ball(void)
{
	#define UP		0
	#define DOWN		1
	#define LEFT 	0
	#define RIGHT	1
	#define BALLSIZE 4
	
	ballposx=50;
	ballposy=50;
	ballvert=UP;
	ballhoriz=LEFT;
	ballspeed=1;
	fresh_ball=1;
}
void init_field(void)
{
	#define FIELD_RIGHT	128
	#define FIELD_LEFT	0
	#define FIELD_TOP	128
	#define FIELD_BOTTOM	0
}

void init_balk(void)
{
	balkposx=0;
	balksize=25;
}

void tft_speedtest(void);

void DrawBall(void)
{
	// 	delete old ball position
	fore=BLACK;
	DrawCircle(ballposx,ballposy,BALLSIZE);
	
	// check field sides
	if(ballposx+BALLSIZE == FIELD_RIGHT)ballhoriz=LEFT;
	if(ballposx-BALLSIZE == FIELD_LEFT)ballhoriz=RIGHT;
	
	// check field top / bottom
	if(ballposy+BALLSIZE== FIELD_TOP)ballvert=DOWN;
	if(ballposy-BALLSIZE == FIELD_LEFT)ballvert=UP;
	
	//	move ball	
	if(ballvert==UP)
	{
		ballposy++;
	}else 
	{
		ballposy--;
	}
	if(ballhoriz==LEFT)
	{
		ballposx--;
	}else 
	{
		ballposx++;
	}
	
	// 	draw new ball position
	fore=WHITE;
	DrawCircle(ballposx,ballposy,BALLSIZE);	

}
int main(void)
{
	DDRB |= (1<<DC) | (1<<CS) | (1<<MOSI) |( 1<<SCK); 	// All outputs
	PORTB = (1<<SCK) | (1<<CS) | (1<<DC);          		// clk, dc, and cs high
	DDRB |= (1<<PB2);									//lcd Backlight output
	PORTB |= (1<<CS) | (1<<PB2);                  		// cs high
	DDRC |= (1<<PC3);									//Reset Output
	DDRD |= (1<<PD7);									//Reset Output
	PORTD |= (1<<PD7);	
									//Reset High
	DDRD &= ~((1<<PD6) | (1<<PD2) | (1<<PD5)); 	//Taster 1-3
	PORTD |= ((1<<PD6) | (1<<PD2) | (1<<PD5)); 	//PUllups für Taster einschalten
	
		//Timer 1 Configuration
	OCR1A = 1249;	//OCR1A = 0x3D08;==1sec
	
    TCCR1B |= (1 << WGM12);
    // Mode 4, CTC on OCR1A

    TIMSK1 |= (1 << OCIE1A);
    //Set interrupt on compare match

    TCCR1B |= (1 << CS11) | (1 << CS10);
    // set prescaler to 64 and start the timer
    
	/* Enable SPI, Master, set clock rate fck/2 */
	SPCR = (1<<SPE)|(1<<MSTR); 
	SPSR = (1<<SPI2X);
	
    sei();
    // enable interrupts
    
    ms10=0;
    ms100=0;
    sec=0;
    min=0;
    entprell=0;
	
	BACKLIGHT_ON;
	LED_ON;

	setup();
	TWIInit();
	init_ball();

	addresse=0;
	position=20;
		
	
	
	fore=WHITE;
	/*MoveTo(10,10);
	sprintf(stringbuffer,"r:%d",234);
	PlotString(stringbuffer);*/
	scale=2;
	  for (;;)
	  {
			if(fresh_ball)//only if position changed
			{
				DrawBall();
				fresh_ball=0;
			}
			
			if(B_BLUE)sec++;
			if(B_GREEN)sec--;
			if(B_YELLOW)sec=0;
			
							
		}//end of for()
}//end of main

ISR (TIMER1_COMPA_vect)
{
	ms10++;
	
	fresh_ball=1;
	if(speedflag==1)speedtest++;
			
	if(ms10==10)	//10ms
	{
		ms10=0;
		ms100++;
		
	}
    if(ms100==10)	//100ms
	{
		ms100=0;
		sec++;
	}
	if(sec==60)	//Minute
	{
		sec=0;
		min++;
	}
}

void SPI_MasterTransmit(char cData)
{
	/* Start transmission */
	SPDR = cData;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)));
}
void tft_speedtest(void)
{
	fore = WHITE; // White
	scale = 1;
	
	speedtest=0;
	speedflag=1;
	
	MoveTo(0,80);
	fore = GREEN;
	FillRect(40,40);
	
	MoveTo(40,80);
	fore = RED;
	FillRect(40,40);
	
	MoveTo(80,80);
	fore = YELLOW;
	FillRect(40,40);
	
	MoveTo(0,40);
	fore = BLUE;
	FillRect(40,40);

	MoveTo(40,40);
	fore = TUERKISE;
	FillRect(40,40);
	
	MoveTo(80,40);
	fore = BLUE_LIGHT;
	FillRect(40,40);
	
	MoveTo(0,0);
	fore = CYAN;
	FillRect(40,40);
	
	MoveTo(40,0);
	fore = VIOLET;
	FillRect(40,40);
	
	MoveTo(80,0);
	fore = WHITE;
	FillRect(40,40);
	
	speedflag=0; // Stop speedcounter
	
	MoveTo(10,0);
	scale=2;
	sprintf(stringbuffer,"sec:%d",speedtest);
	PlotString(stringbuffer);
	
}
