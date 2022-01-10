/******************************************
Filename    : Time8254.cpp
Author      : Rozano Maniaol
Description : This program provides accurate timestamping using Mode 2 of 8254
			  Most functions are modified versions taken from PCTIM003.TXT by
			  Kris Heidenstrom
			  
Created     : 10/11/06
*******************************************/
#pragma inline;		/* Required for asm pushf, popf, cli, and sti */

#include <conio.h>	/* Needed for bioskey() */
#include <dos.h>
#include <stdio.h>	/* Needed for printf() */
#include <stdlib.h>	/* Needed for exit() */
#include "time8254.h"

extern unsigned _floatconvert;
#pragma extref _floatconvert

TimeStamp8254::TimeStamp8254()
{
	SetMode2();
	GetTimestamp(&ts1);
	dwTotalMicroSec = 0.0;
	dwTotalMilliSec = 0.0;
	dwTotalSec = 0.0;

	nHour = 0;
	nMin = 0;
	fSecond = 0.0;
}

TimeStamp8254::~TimeStamp8254(){}

// Set Channel 0 to Mode 2
void TimeStamp8254::SetMode2()
{
	unsigned int tick_loword;
	tick_loword = * BIOS_TICK_COUNT_P;
	while ((unsigned int) * BIOS_TICK_COUNT_P == tick_loword);
	asm pushf;
	asm cli;
	outportb(0x43, 0x34);		/* Channel 0, mode 2 */
	outportb(0x40, 0x00);		/* Loword of divisor */
	outportb(0x40, 0x00);		/* Hiword of divisor */
	asm popf;
	return;
}

// Get Current Time Stamp
void TimeStamp8254::GetTimestamp( timestamp8254 * tsp )
{
	unsigned long tickcount1, tickcount2;
	unsigned int ctcvalue;
	unsigned char ctclow, ctchigh;
	asm pushf;
	asm cli;
	tickcount1 = * BIOS_TICK_COUNT_P;
	outportb(0x43, 0);			/* Latch value */
	ctclow = inportb(0x40);
	ctchigh = inportb(0x40);	/* Read count in progress */
	asm sti;					/* Force interrupt ENABLE */
	ctcvalue = - ((ctchigh << 8) + ctclow);
	tickcount2 = * BIOS_TICK_COUNT_P;
	asm popf;
	if ((tickcount2 != tickcount1) && (ctcvalue & 0x8000))
		tsp->ticks = tickcount1;
	else
		tsp->ticks = tickcount2;
	tsp->part = ctcvalue;
	return;
}
	
// Calculate Elapsed Time using startts and stopts and save it to diffts
void TimeStamp8254::CalcElapsed(timestamp8254 * startts, timestamp8254 * stopts, timestamp8254 * diffts)
{
	if (startts->ticks <= stopts->ticks)		/* No change of day */
		diffts->ticks = stopts->ticks - startts->ticks;
	else										/* Change of day */
		diffts->ticks = stopts->ticks + 0x001800B0L - startts->ticks;
	diffts->part = stopts->part - startts->part;
	if (stopts->part < startts->part)
		--(diffts->ticks);
	return;
}
	
// Convert Timestamp time.tick and time.part to double
double TimeStamp8254::ConvertTimestamp( timestamp8254 * tsp )
{
	double dwTemp = 0.0;
	dwTemp = (double) (tsp->ticks * 65536.0);
	dwTemp += ( double ) tsp->part;
	return dwTemp;
}


void TimeStamp8254::GetTimeStampVariable()
{
	double dwTimetick = 0.0;
	
	GetTimestamp(&ts1);

	dwTimetick = ConvertTimestamp(&ts1);
	
	dwTotalMicroSec = (dwTimetick * 0.8381);

	dwTotalMilliSec = dwTotalMicroSec / 1000;
	
	dwTotalSec = dwTotalMilliSec / 1000;


	nHour = (int) (dwTotalSec / 3600);

	nMin = (int) ( ( dwTotalSec - (nHour * 3600) ) / 60 );

	fSecond = ( dwTotalSec - ( (nHour * 3600) + (nMin * 60 ) ) );

}


void TimeStamp8254::GetTimeStampVariable(double dwTimetick)
{

	dwTotalMicroSec = (dwTimetick * 0.8381);

	dwTotalMilliSec = dwTotalMicroSec / 1000;
	
	dwTotalSec = dwTotalMilliSec / 1000;


	nHour = (int) (dwTotalSec / 3600);

	nMin = (int) ( ( dwTotalSec - (nHour * 3600) ) / 60 );

	fSecond = ( dwTotalSec - ( (nHour * 3600) + (nMin * 60 ) ) );

}


// Print Time with
// Mode 1    HH:MM:SS.NNNNNN format
//      2    Microsecond
//
//
void TimeStamp8254::PrintTime( double dwTimetick, char bMode )
{
	GetTimeStampVariable(dwTimetick);
	if ( bMode == 2 ) 
		cprintf("%10.0f ", dwTotalMicroSec );
	else
		cprintf("%02d:%02d:%09.6f", nHour, nMin, fSecond);
}


// Print Current Time in HH:MM:SS.NNNNNN format
void TimeStamp8254::PrintCurrentTime()
{
	GetTimestamp(&ts1);
	PrintTime( ConvertTimestamp(&ts1) );
}

// Set Start Time to Current Time
void TimeStamp8254::SetStartTime()
{
	GetTimestamp(&ts1);
}

// Print Start Time in HH:MM:SS.NNNNNN format
void TimeStamp8254::PrintStartTime()
{
	PrintTime( ConvertTimestamp(&ts1) );
}

// Print Start Time in HH:MM:SS.NNNNNN format
void TimeStamp8254::PrintEndTime()
{
	PrintTime( ConvertTimestamp(&ts2) );
}

void TimeStamp8254::PrintElapsedTime(char bMode)
{
	CalcElapsed(&ts1, &ts2, &ts);

	if (bMode == 2) PrintTime( ConvertTimestamp(&ts), 2 );
	else PrintTime( ConvertTimestamp(&ts) );
}

// Print Elapsed Time in HH:MM:SS.NNNNNN format
void TimeStamp8254::PrintCurElapsedTime()
{
	GetTimestamp(&ts2);
	CalcElapsed(&ts1, &ts2, &ts);
	PrintTime( ConvertTimestamp(&ts) );
}

// Print Elapsed Time in Microsecond
void TimeStamp8254::PrintCurElapsedMicroTime()
{
	GetTimestamp(&ts2);
	CalcElapsed(&ts1, &ts2, &ts);
	PrintTime( ConvertTimestamp(&ts), 2 );
}


/*
void main(void) {

	unsigned int ch;
	
	TimeStamp8254 m_TmStamp;
	
	gotoxy(1,1);printf("Press any key to stop\n");

	//m_TmStamp.PrintCurrentTime();
	
	m_TmStamp.SetStartTime();
	gotoxy(1,2); m_TmStamp.PrintStartTime();

	while (!kbhit()) 
	{
		gotoxy(1,3); m_TmStamp.PrintElapsedTime();
	
	}
		m_TmStamp.PrintCurrentTime();
}
*/
