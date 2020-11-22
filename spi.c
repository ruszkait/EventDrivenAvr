#include <avr\io.h>
#include "macroes.h"

// ***********************************************************
//
//
void SPI_Init()
{
	// MOSI	-> output
	// SCK	-> output
	// ~SS	-> output
	DDR(B) |= (1 << 2) | (1 << 3) | (1 << 5);

	// Enable SPI					->	SPE = 1
	// Master						->	MSTR = 1
	// MSB first					->	DORD = 0
	// Idle clock H					->	CPOL = 1
	// Sampling at trailing edge	->	CPHA = 1
	// Clk = ClkOsc / 4				->	SPR1 = 0 SPR0 = 0
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL) | (1 << CPHA);

	// Double SPI clock to Clk = ClkOsc / 2
	SPSR = SPI2X;
}

// ***********************************************************
//
//
void SPI_Send(char data)
{
	SPDR = data;

	while (!(SPSR & (1 << SPIF)));
}
