///////////////////////////////////////////////////
/* credits to http://www.technoblogy.com/show?L6I*/
/* original library adapted to our needs		 */
/* 08.2020 GRN									 */
///////////////////////////////////////////////////

#ifndef ST7735_H
#define	ST7735_H

#define F_CPU 8000000UL                 // set the CPU clock
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

// function prototypes
void setup(void); 							//Setup
void TestChart (void);						//draw testchart
void PlotText(PGM_P p); 					// Plot text starting at the current plot position
void PlotChar (char c); 					// Plot an ASCII character with bottom left corner at x,y
void FillRect (int w, int h);				//draw filled rectangle
void DrawTo (int x, int y);					// Draw a line to x,y
void PlotPoint (int x, int y); 				// Plot point at x,y
void MoveTo (int x, int y); 					// Move current plot position to x,y
unsigned int Colour (int b, int g, int r);	// define colour
void ClearDisplay (void); 					// clear display
void DisplayOn (void); 						// turn display on
void InitDisplay (void);						// initialise display
void Command4 (uint8_t c, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4); // Send a command followed by four data bytes
void Command (uint8_t c);					// Send a command to the display
void Data (uint8_t d);						// Send a byte to the display
void DrawCircle(uint8_t x0, uint8_t y0, uint8_t r);	// draw circle at position with defined radius
void PlotString(const char str[]);			// Print Sting

#endif
