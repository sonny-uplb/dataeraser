#ifndef DTERASE_H
#define DTERASE_H

#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <mem.h>
#include <alloc.h>
#include <conio.h>
#include <bios.h>
#include <time.h>
#include "hdtool.h"
#include "guitool.h"

typedef struct _SYSTIME
{
	char day;
	char mon;
	int  yr;
	char hr;
	char min;
	char sec;
			
} SYSTIME;

class DataEraser: public BiosCall
{
	public:
	
		DataEraser();
		~DataEraser();

	private:
		bool bAbortFlag;
	public:
		SYSTIME stStartTime, stStopTime;
		
	public:

		word CheckIfWindows(void);
		bool InitProgram(void);
		void InitScreen(void);
		
		void DriveInfoScreen(void);
		void ViewSectorInputScreen(void);
		void ReadSectorInputScreen(void);
		void WriteSectorInputScreen(void);

		void GetDateTime(SYSTIME *stTimeStamp);
		void PrintDateTime(SYSTIME *stTimeStamp);
		dword ConvertTime(SYSTIME *stTimeStamp);
		void PrintTimeDiff(SYSTIME stStart, SYSTIME stStop);

		void BuildFunctionList(stMenuParam* pstList);
		void BuildEraseList(stMenuParam* pstList);

		bool ScanConv(byte bDriveNo, double dblLBAStart, double dblLBAEnd, word nSect, byte bCommand, void *buffer);
		bool ScanExtd(byte bDriveNo, double dblLBAStart, double dblLBAEnd, word nSect, byte bCommand, void *buffer);

		bool MediaScan(byte bDriveNo, double dblLBAStart, double dblLBAEnd, word nSect, byte bCommand);
		bool ViewSector(byte bDriveNo, double dblLBA, word nNumSector);

		bool DODAlgo(void);
		bool SchneierAlgo(void);
		bool BritishAlgo(void);
		bool AustralianAlgo(void);
		bool GermanAlgo(void);
		bool GutmannAlgo(void);

		
		void MainProgram();
};

#endif
