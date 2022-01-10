#include "time8254.h"
#include "dterase.h"
#include "scrwin.h"


DataEraser gDataEraser;

DataEraser::DataEraser(){}

DataEraser::~DataEraser(){}

// Check if environment is in Windows
word DataEraser::CheckIfWindows()
{
	union REGS regs;
	regs.x.ax = 0x1600;
	int86(0x2f, &regs, &regs);
	return regs.h.al;
		
}

void DataEraser::GetDateTime(SYSTIME *stTimeStamp)
{
	struct time t_time;
	struct date d_date;
	
	getdate(&d_date);
	gettime(&t_time);

	stTimeStamp->mon = d_date.da_mon;
	stTimeStamp->day = d_date.da_day;
	stTimeStamp->yr = d_date.da_year;
	stTimeStamp->hr = t_time.ti_hour;
	stTimeStamp->min = t_time.ti_min;
	stTimeStamp->sec = t_time.ti_sec;

	return;
}

void DataEraser::PrintDateTime(SYSTIME *stStamp)
{
	textattr(SCREEN_TEXT);
	cprintf("%d-%d-%d %02d:%02d:%02d", stStamp->mon, stStamp->day, stStamp->yr, \
				stStamp->hr, stStamp->min, stStamp->sec);
	return;
}

dword DataEraser::ConvertTime(SYSTIME *stTimeStamp)
{
	struct time t_time;
	struct date d_date;

	d_date.da_mon = stTimeStamp->mon;
	d_date.da_day = stTimeStamp->day;
	d_date.da_year = stTimeStamp->yr;

	t_time.ti_hour = stTimeStamp->hr;
	t_time.ti_min  = stTimeStamp->min;
	t_time.ti_sec  = stTimeStamp->sec;
	
	return dostounix(&d_date, &t_time);
}

// Prints the time difference
void DataEraser::PrintTimeDiff(SYSTIME stStart, SYSTIME stStop)
{
	dword dwTimeDiff = (dword) difftime(ConvertTime(&stStop), ConvertTime(&stStart) );
	/*dword dwStartTime = ConvertTime(&stStart);
	dword dwStopTime  = ConvertTime(&stStop);
	dword dwTimeDiff = dwStopTime - dwStartTime;
	*/
	textattr(SCREEN_TEXT);

	word dwHour, dwMin, dwSec;
	dwHour = (word) dwTimeDiff/3600;
	dwMin  = ((dword) dwTimeDiff%3600)/60;
	dwSec  = (word) dwTimeDiff % 60;
	cprintf("%.02d:%.02d:%.02d", dwHour, dwMin, dwSec );
	
}


// Initializes the program
bool DataEraser::InitProgram()
{
	if (CheckIfWindows()) 
	{
		printf("This program does not run under Windows environment.\n");
		return FALSE;
	}	
	

	dword lCurrentFreeMem=coreleft(); 
	lCurrentFreeMem=lCurrentFreeMem-25600;

	word MaxSecXferCnt=(lCurrentFreeMem/2)/SECTOR_SIZE;
    MaxSecXferCnt=MaxSecXferCnt>NUM_OF_BLOCKS?NUM_OF_BLOCKS:MaxSecXferCnt;
  
	cldwReadBufMaxSize = (dword) MaxSecXferCnt*SECTOR_SIZE;
	cldwWriteBufMaxSize = cldwReadBufMaxSize;

	if ( !AllocateBuffer(cldwReadBufMaxSize,cldwWriteBufMaxSize) )
	{ 
		printf("\nAllocation problem"); return FALSE;
	}

	if (ResetDrive()) printf("\nResetting Drive Failed (%Xh)", ResetDrive());
	
	if ( !GetDeviceParam() )		// if drive not found
	{
		printf("\nHard drive not found!");
		return FALSE;
	}

	ChkInt13ExtEnabled();
	return TRUE;
}

///////////////////// START: BASIC SCREENS ////////////////////////

// Initial Config Screen
void DataEraser::InitScreen(void)
{
	byte bCurY=2;
	
	SimpleBox(2, bCurY++, 78, 23, BLUE, WHITE, TRUE, WHITE, RED, "Configuration Settings");

	gotoxy(3,bCurY++);
	cprintf("Identify Device call %s (Int 13/AH=25h)", (IdentifyDevice(ptrbReadBuffer)) ? "passed" : "failed" );  
	
	gotoxy(3,bCurY++);
	if ( clbInt13ExtSupported ) 
	{
		cprintf("Int 13 Extension Supported");
		gotoxy(5,bCurY++); cprintf("Extension Version: %Xh", clbExtVer);
		gotoxy(5,bCurY++); cprintf("Extension Minor Version: %Xh", clbExtMinVer);
		gotoxy(5,bCurY++); cprintf("Enhanced Disk Drive %s", (clbEDDSupported) ? "supported" : "not supported");

		/*if (clbEDDSupported)
		{
			gotoxy(5,bCurY++);
			cprintf("Set Hardware Configuration (Int 13/AH=4Eh) ");
			gotoxy(7,bCurY++);
			cprintf("Enable DMA Max transfer %s (AL=05h)", (setDMA()) ? "passed" : "failed" );  
		}*/

		if ( GetExtDeviceParam() )
		{
			SaveExtDeviceParam();			
		}
	}
	else
		cprintf("Int 13 Extension Not Supported");
	gotoxy(3,bCurY++);
	cprintf("Found %.2fGB drive", cldblCapacity);

	textattr(TIME_TEXT);
	gotoxy(30,20);
	cprintf("Press any key to continue");
	getch();
}

// Drive Information Screen
void DataEraser::DriveInfoScreen(void)
{
	void *ptrbBufferScreen;

	byte bYCoord=4;
	
	if ( (ptrbBufferScreen = malloc(25*80*2)) != NULL)
		gettext(1,1,80,25,ptrbBufferScreen);
	else
		return;

	Textbar(STATUS_TEXT, "ESC - Back", 25);
	textattr(SCREEN_TEXT);
	gotoxy(3,bYCoord++);cprintf("Capacity : %.2fGB",cldblCapacity);
	gotoxy(3,bYCoord++);cprintf("Max Cyl  : %lXh",cldwCyl);
	gotoxy(3,bYCoord++);cprintf("Max Head : %lXh",cldwHead);
	gotoxy(3,bYCoord++);cprintf("Max Sect : %lXh",cldwSect);
	gotoxy(3,bYCoord++);cprintf("NumDrv : %Xh",clbNumDrives);

	if ( clbInt13ExtSupported ) 
	{
		gotoxy(3,bYCoord++);cprintf("Maj Ver: %Xh",clbExtVer);
		gotoxy(3,bYCoord++);cprintf("Min Ver: %Xh",clbExtMinVer);
		gotoxy(3,bYCoord++);cprintf("Flag        : %Xh",clstDiskParam.flag);
		gotoxy(3,bYCoord++);cprintf("Def Cyl     : %lXh",clstDiskParam.def_cyl);
		gotoxy(3,bYCoord++);cprintf("Def Head    : %lXh",clstDiskParam.def_head);
		gotoxy(3,bYCoord++);cprintf("Def Sect    : %lXh",clstDiskParam.def_sect);
		gotoxy(3,bYCoord++);cprintf("Num of sect : %.0f",clstDiskParam.abs_sector);
		gotoxy(3,bYCoord++);cprintf("Byte/sect   : %Xh",clstDiskParam.bytespersect);
		if (clbExtVer >= 0x20)
			{	gotoxy(3,bYCoord++); cprintf("EDD  : %lXh", clstDiskParam.edd ); }
	}

	do {} while (getch()!=ESC);
	
	puttext(1,1,80,25,ptrbBufferScreen); // restore screen
	free(ptrbBufferScreen);
	return;
}

// Screen for View Sector Input
void DataEraser::ViewSectorInputScreen(void)
{
	double dblStartLBA;
	byte *ptrTmp;

	SimpleBox(28, 4, 25, 3, LIGHTGRAY, BLACK, TRUE, BLUE, BLUE, "View Sector");
	gotoxy(30,5);
	cprintf("LBA Start:");
	ptrTmp =  GetInput(41, 5, 10, NUMBER);
	if ( ptrTmp==NULL ) return;
	if ( strcmp(ptrTmp,"")==0 ) dblStartLBA=0;
	else dblStartLBA = atof(ptrTmp);
		
	if (dblStartLBA + 10 > cldblTotalSector)
	{
		DialogBox("Error", "    Invalid Range    ", 1);
		return;
	}
	
	if (! ViewSector(DRIVE, dblStartLBA, 10))
		DialogBox("Error", "Problem on Reading Sector", 1);

	return;
}


// Screen for Read Sector Input
void DataEraser::ReadSectorInputScreen(void)
{
	double dblStartLBA, dblEndLBA;
	byte sTemp[3];
	byte *ptrTmp;
	byte bPattern;
	
	sTemp[2] = '\0';
	
	SimpleBox(28, 4, 26, 4, LIGHTGRAY, BLACK, TRUE, BLUE, BLUE, "Reading Sector");
	gotoxy(30,5);
	cprintf("LBA Start:");
	gotoxy(30,6);
	cprintf("LBA End  :");

	ptrTmp =  GetInput(41, 5, 10, NUMBER);
	if ( ptrTmp==NULL ) return;
	if ( strcmp(ptrTmp,"")==0 ) dblStartLBA=0;
	else dblStartLBA = atof(ptrTmp);
	
	ptrTmp =  GetInput(41, 6, 10, NUMBER);
	if ( ptrTmp==NULL ) return;
	if ( strcmp(ptrTmp,"")==0 ) dblEndLBA=cldblTotalSector;
	else dblEndLBA = atof(ptrTmp);
		
	if ( ( dblStartLBA < 0) || ( dblEndLBA < dblStartLBA ) || 
	( (dblEndLBA - dblStartLBA) < NUM_OF_BLOCKS) || (dblEndLBA > cldblTotalSector) )
	{
		DialogBox("Error", "    Invalid Range    ", 1);
		return;
	}

	if (!MediaScan(DRIVE, dblStartLBA, dblEndLBA, NUM_OF_BLOCKS, READ)) 
		DialogBox("Error", "Problem on Reading Sectors", 1);
	else
		DialogBox("Success", " Finished Reading ", 1);

	return;
}

// Screen for Write Sector Input
void DataEraser::WriteSectorInputScreen(void)
{
	double dblStartLBA, dblEndLBA;
	byte sTemp[3];
	byte *ptrTmp;
	byte bPattern;
	
	sTemp[2] = '\0';
	
	SimpleBox(28, 4, 26, 5, LIGHTGRAY, BLACK, TRUE, BLUE, BLUE, "Write Sector");
	gotoxy(30,5);
	cprintf("LBA Start:");
	gotoxy(30,6);
	cprintf("LBA End  :");
	gotoxy(30,7);
	cprintf("Pattern  : 0x");

	ptrTmp =  GetInput(41, 5, 10, NUMBER);
	if ( ptrTmp==NULL ) return;
	if ( strcmp(ptrTmp,"")==0 ) dblStartLBA=0;
	else dblStartLBA = atof(ptrTmp);

	ptrTmp =  GetInput(41, 6, 10, NUMBER);
	if ( ptrTmp==NULL ) return;
	if ( strcmp(ptrTmp,"")==0 ) dblEndLBA=cldblTotalSector;
	else dblEndLBA = atof(ptrTmp);

	ptrTmp =  GetInput(43, 7, 2, HEX);
	if ( ptrTmp==NULL ) return;
	if ( strcmp(ptrTmp,"")==0 ) FillWriteBuffer();
	else
	{
		strcpy(sTemp, ptrTmp);
		bPattern = ConvFromBCD(sTemp[0],sTemp[1]);
		FillWriteBuffer(bPattern);
	}
	
	if ( ( dblStartLBA < 0) || ( dblEndLBA < dblStartLBA ) || 
	( (dblEndLBA - dblStartLBA) < NUM_OF_BLOCKS) || (dblEndLBA > cldblTotalSector) )
	{
		DialogBox("Error", "    Invalid Range    ", 1);
		return;
	}
	
	if (!MediaScan(DRIVE, dblStartLBA, dblEndLBA, NUM_OF_BLOCKS, WRITE)) 
		DialogBox("Error", "Problem on Writing Sector", 1);
	else
		DialogBox("Success", " Sector overwritten", 1);

	return;
}

///////////////////// END: BASIC SCREENS ////////////////////////


///////////////////// START: SCANS ///////////////////
// Scan drive using Conventional Int 13h
bool DataEraser::ScanConv(byte bDriveNo, double dblLBAStart, double dblLBAEnd, word nSect, byte bCommand, void *buffer )
{
	byte bStat;
	double ctr = dblLBAStart;
	word nCurSec = nSect;
	TimeStamp8254 clTmStamp;

	if ( (dblLBAEnd - ctr ) < nSect) return FALSE;
	if ( CheckDriveReady() ) return FALSE;

	while ( ctr < (dblLBAEnd-nCurSec) )
	{
		if ( StopKeyPressed() ) { bAbortFlag=TRUE; return FALSE;}
		clTmStamp.SetStartTime();
		bStat=Biosdisk(bCommand, bDriveNo, ctr, nCurSec, buffer);
		gotoxy(67,10);
		clTmStamp.PrintCurElapsedMicroTime();
		gotoxy(68,7);
		cprintf("%10.0lf", ctr);
		gotoxy(59,8);
		cprintf("%5.1f%", (ctr/dblLBAEnd)*100);
		gotoxy(65,9);
		if (!bStat)
			cprintf("  Good    ");
		else {
			cprintf("Error(%xh)", bStat);
			return FALSE;
		}
		ctr+=nCurSec;

		if ( ctr >= (dblLBAEnd-nCurSec) )
		{
			nCurSec = dblLBAEnd-ctr;
			Biosdisk(bCommand, bDriveNo, ctr, nCurSec, buffer);
		}
  	}
  	return TRUE;
}

// Scan drive using Int 13h Extensions
bool DataEraser::ScanExtd(byte bDriveNo, double dblLBAStart, double dblLBAEnd, word nSect, byte bCommand, void *buffer)
{
	byte bStat;
	double ctr=dblLBAStart;
	word nCurSec = nSect;
	TimeStamp8254 clTmStamp;

	if ( (dblLBAEnd - ctr ) < nSect) return FALSE;
	if ( CheckDriveReady() ) return FALSE;
	textattr(SCREEN_TEXT);

	while ( ctr < (dblLBAEnd-nCurSec) )
	{
		if ( StopKeyPressed() ) { bAbortFlag=TRUE; return FALSE;}
		clTmStamp.SetStartTime();
		bStat=BiosdiskExt(bCommand, bDriveNo, ctr, nCurSec, buffer);
		gotoxy(67,10);
		clTmStamp.PrintCurElapsedMicroTime();
		gotoxy(68,7);
		cprintf("%10.0lf", ctr);
		gotoxy(59,8);
		cprintf("%5.1f%", (ctr/dblLBAEnd)*100);
		gotoxy(65,9);
		if (!bStat)
			cprintf("  Good    ");
		else {
			cprintf("Error(%xh)", bStat);
			return FALSE;
		}
		
		ctr+=nCurSec;

		if ( ctr >= (double) (dblLBAEnd-nCurSec) )
		{
			nCurSec = dblLBAEnd-ctr;
			BiosdiskExt(bCommand, bDriveNo, ctr, nCurSec, buffer);
		}
  	}
	return TRUE;
}
///////////////////////// END: SCANS ///////////////////


// Choose which scan to use
bool DataEraser::MediaScan(byte bDriveNo, double dblLBAStart, double dblLBAEnd, word nSect, byte bCommand)
{
	void *buffer;
	
	if ( bCommand==WRITE )
		buffer = ptrbWriteBuffer;
	else
		buffer = ptrbReadBuffer;

	textattr(SCREEN_TEXT);
	gotoxy(55, 6);cprintf("Total LBA: %12.0lf",dblLBAEnd);
	gotoxy(55, 7);cprintf("Current LBA: ");
	gotoxy(66, 8);cprintf("complete");
	gotoxy(55, 9);cprintf("Status:");
	gotoxy(55, 10);cprintf("Exec Time: ");
	
	
	if (clbInt13ExtSupported)
	{
		
		return ScanExtd(bDriveNo, dblLBAStart, dblLBAEnd, nSect, (bCommand|0x40), buffer); 
	}
	else
		return ScanConv(bDriveNo, dblLBAStart, dblLBAEnd, nSect, bCommand, buffer);
}

// View Sector Content
bool DataEraser::ViewSector(byte bDriveNo, double dblLBA, word nNumSector)
{
	word nNumofLines = (SECTOR_SIZE*nNumSector)/8;
	word i;
	double dblCurLBA = dblLBA;
	
	stStrAttr *stSectorLine;
	stSectorLine=(stStrAttr*) calloc( nNumofLines, sizeof(stStrAttr));
	
	if (clbInt13ExtSupported)
	{
		BiosdiskExt(EXT_READ, bDriveNo, dblLBA, nNumSector, ptrbReadBuffer);
	}
	else
		Biosdisk(READ, bDriveNo, dblLBA, nNumSector, ptrbReadBuffer);
	

	for ( i=0; i< nNumofLines; i++)
	{		
		(stSectorLine+(i))->bAttr=LIGHTGRAY|BLUE*16;
		if (i % 64 == 0)
		{
		sprintf((stSectorLine+(i))->sStrValues, "%010.0f: %.02X %.02X %.02X %.02X %.02X %.02X %.02X %.02X   %c%c%c%c%c%c%c%c", \

			dblCurLBA, 
			*(ptrbReadBuffer+(i*8)),   *(ptrbReadBuffer+(i*8)+1), *(ptrbReadBuffer+(i*8)+2), *(ptrbReadBuffer+(i*8)+3), \
			*(ptrbReadBuffer+(i*8)+4), *(ptrbReadBuffer+(i*8)+5), *(ptrbReadBuffer+(i*8)+6), *(ptrbReadBuffer+(i*8)+7), \

			PrintChar(*(ptrbReadBuffer+(i*8))), PrintChar(*(ptrbReadBuffer+(i*8)+1)), PrintChar(*(ptrbReadBuffer+(i*8)+2)), PrintChar(*(ptrbReadBuffer+(i*8)+3)), \
			PrintChar(*(ptrbReadBuffer+(i*8)+4)), PrintChar(*(ptrbReadBuffer+(i*8)+5)), PrintChar(*(ptrbReadBuffer+(i*8)+6)), PrintChar(*(ptrbReadBuffer+(i*8)+7)) \
		);
		dblCurLBA++;
		}
		else
		sprintf((stSectorLine+(i))->sStrValues, "            %.02X %.02X %.02X %.02X %.02X %.02X %.02X %.02X   %c%c%c%c%c%c%c%c", \

			*(ptrbReadBuffer+(i*8)),   *(ptrbReadBuffer+(i*8)+1), *(ptrbReadBuffer+(i*8)+2), *(ptrbReadBuffer+(i*8)+3), \
			*(ptrbReadBuffer+(i*8)+4), *(ptrbReadBuffer+(i*8)+5), *(ptrbReadBuffer+(i*8)+6), *(ptrbReadBuffer+(i*8)+7), \

			PrintChar(*(ptrbReadBuffer+(i*8))), PrintChar(*(ptrbReadBuffer+(i*8)+1)), PrintChar(*(ptrbReadBuffer+(i*8)+2)), PrintChar(*(ptrbReadBuffer+(i*8)+3)), \
			PrintChar(*(ptrbReadBuffer+(i*8)+4)), PrintChar(*(ptrbReadBuffer+(i*8)+5)), PrintChar(*(ptrbReadBuffer+(i*8)+6)), PrintChar(*(ptrbReadBuffer+(i*8)+7)) \
		);
		
	}

	Textbar(STATUS_TEXT, "ESC - Back", 25);
	Scroller scrWindow;
	scrWindow.show(13, 8, 66, 20, 1, LIGHTGRAY, BLUE, stSectorLine, i, " Sector Data ", 0);

	return TRUE;
}

///////////////  START: MENU LISTS  ///////////////
// Build the Basic Function List
void DataEraser::BuildFunctionList(stMenuParam* pstList)
{
	strcpy(pstList[0].sMenuItem, "Drive Info");
	pstList[0].bIsEnabled = TRUE;
	pstList[0].bAttr = MENU_TEXT;

	strcpy(pstList[1].sMenuItem, "Write Sector");
	pstList[1].bIsEnabled = TRUE;
	pstList[1].bAttr = MENU_TEXT;

	strcpy(pstList[2].sMenuItem, "Read Sector");
	pstList[2].bIsEnabled = TRUE;
	pstList[2].bAttr = MENU_TEXT;

	strcpy(pstList[3].sMenuItem, "View Sector");
	pstList[3].bIsEnabled = TRUE;
	pstList[3].bAttr = MENU_TEXT;

	strcpy(pstList[4].sMenuItem, "Exit");
	pstList[4].bIsEnabled = TRUE;
	pstList[4].bAttr = MENU_TEXT;
}

// Build the Data Erase List
void DataEraser::BuildEraseList(stMenuParam* pstList)
{
	strcpy(pstList[0].sMenuItem, "DOD 5220.22-M");
	pstList[0].bIsEnabled = TRUE;
	pstList[0].bAttr = MENU_TEXT;

	strcpy(pstList[1].sMenuItem, "Schneier's Algorithm");
	pstList[1].bIsEnabled = TRUE;
	pstList[1].bAttr = MENU_TEXT;

	strcpy(pstList[2].sMenuItem, "British HMG Standard");
	pstList[2].bIsEnabled = TRUE;
	pstList[2].bAttr = MENU_TEXT;

	strcpy(pstList[3].sMenuItem, "Australian ASCI 33");
	pstList[3].bIsEnabled = TRUE;
	pstList[3].bAttr = MENU_TEXT;

	strcpy(pstList[4].sMenuItem, "German VSITR Standard");
	pstList[4].bIsEnabled = TRUE;
	pstList[4].bAttr = MENU_TEXT;

	strcpy(pstList[5].sMenuItem, "Gutmann's Algorithm");
	pstList[5].bIsEnabled = TRUE;
	pstList[5].bAttr = MENU_TEXT;
}
/////////////// END: MENU LISTS ///////////////


//////////////// START: DATA ERASE ALGOS ////////////////
// DOD
bool DataEraser::DODAlgo(void)
{
	bool bStatus=TRUE;
	byte bStat[][2]= {"X", CHECK_MARK};

	Textbar(STATUS_TEXT, "ESC - Cancel", 25);
	textattr(SCREEN_TEXT);
	gotoxy(35,4);
	cprintf("DOD 5220.22-M");
	
	textattr(BLINK_TEXT);
	gotoxy(5,6);
	cprintf("[ ] Pass 1 (0x55)");
	FillWriteBuffer(0x55);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,6);
	cprintf("[%s] Pass 1 (0x55)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,7);
	cprintf("[ ] Pass 2 (0xAA)");
	FillWriteBuffer(0xAA);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,7);
	cprintf("[%s] Pass 2 (0xAA)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,8);
	cprintf("[ ] Pass 3 (0x55)");
	FillWriteBuffer(0x55);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,8);
	cprintf("[%s] Pass 3 (0x55)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,9);
	cprintf("[ ] Pass 4 (0xAA)");
	FillWriteBuffer(0xAA);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,9);
	cprintf("[%s] Pass 4 (0xAA)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,10);
	cprintf("[ ] Pass 5 (0x55)");
	FillWriteBuffer(0x55);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,10);
	cprintf("[%s] Pass 5 (0x55)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,11);
	cprintf("[ ] Pass 6 (0xAA)");
	FillWriteBuffer(0xAA);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,11);
	cprintf("[%s] Pass 6 (0xAA)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,12);
	cprintf("[ ] Pass 7 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,12);
	cprintf("[%s] Pass 7 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	return TRUE;
}

// Schneier Algo
bool DataEraser::SchneierAlgo(void)
{
	bool bStatus=TRUE;
	byte bStat[][2]= {"X", CHECK_MARK};
	
	Textbar(STATUS_TEXT, "ESC - Cancel", 25);
	textattr(SCREEN_TEXT);
	gotoxy(30,4);
	cprintf("Schneier Algorithm");

	textattr(BLINK_TEXT);
	gotoxy(5,6);
	cprintf("[ ] Pass 1 (0xFF)");
	FillWriteBuffer(0xFF);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,6);
	cprintf("[%s] Pass 1 (0xFF)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,7);
	cprintf("[ ] Pass 2 (0x00)");
	FillWriteBuffer(0x00);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,7);
	cprintf("[%s] Pass 2 (0x00)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,8);
	cprintf("[ ] Pass 3 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,8);
	cprintf("[%s] Pass 3 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,9);
	cprintf("[ ] Pass 4 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,9);
	cprintf("[%s] Pass 4 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,10);
	cprintf("[ ] Pass 5 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,10);
	cprintf("[%s] Pass 5 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,11);
	cprintf("[ ] Pass 6 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,11);
	cprintf("[%s] Pass 6 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,12);
	cprintf("[ ] Pass 7 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,12);
	cprintf("[%s] Pass 7 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	return TRUE;

}


bool DataEraser::BritishAlgo(void)
{	
	bool bStatus=TRUE;
	byte bStat[][2]= {"X", CHECK_MARK};

	Textbar(STATUS_TEXT, "ESC - Cancel", 25);
	textattr(SCREEN_TEXT);
	gotoxy(30,4);
	cprintf("British HMG Standard No. 5");

	textattr(BLINK_TEXT);
	gotoxy(5,6);
	cprintf("[ ] Pass 1 (0xFF)");
	FillWriteBuffer(0xFF);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,6);
	cprintf("[%s] Pass 1 (0xFF)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,7);
	cprintf("[ ] Pass 2 (0x00)");
	FillWriteBuffer(0x00);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,7);
	cprintf("[%s] Pass 2 (0x00)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,8);
	cprintf("[ ] Pass 3 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,8);
	cprintf("[%s] Pass 3 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,9);
	cprintf("[ ] Pass 4 Verifying");
	CopyWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, READ);
	textattr(SCREEN_TEXT);
	gotoxy(5,9);
	cprintf("[%s] Pass 4 Verified  ", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	return TRUE;
}


// Australian Algo
bool DataEraser::AustralianAlgo(void)
{
	bool bStatus=TRUE;
	byte bStat[][2]= {"X", CHECK_MARK};

	Textbar(STATUS_TEXT, "ESC - Cancel", 25);
	textattr(SCREEN_TEXT);
	gotoxy(30,4);
	cprintf("Australian ASCI 33");

	textattr(BLINK_TEXT);
	gotoxy(5,6);
	cprintf("[ ] Pass 1 (0xC3)");
	FillWriteBuffer(0xC3);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,6);
	cprintf("[%s] Pass 1 (0xC3)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,7);
	cprintf("[ ] Pass 2 Verifying");
	CopyWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, READ);
	textattr(SCREEN_TEXT);
	gotoxy(5,7);
	cprintf("[%s] Pass 2 Verified  ", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,8);
	cprintf("[ ] Pass 3 (0x3C)");
	FillWriteBuffer(0x3C);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,8);
	cprintf("[%s] Pass 3 (0x3C)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,9);
	cprintf("[ ] Pass 4 Verifying");
	CopyWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, READ);
	textattr(SCREEN_TEXT);
	gotoxy(5,9);
	cprintf("[%s] Pass 4 Verified  ", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,10);
	cprintf("[ ] Pass 5 (0xC3)");
	FillWriteBuffer(0xC3);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,10);
	cprintf("[%s] Pass 5 (0xC3)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,11);
	cprintf("[ ] Pass 6 (0x3C)");
	FillWriteBuffer(0x3C);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,11);
	cprintf("[%s] Pass 6 (0x3C)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,12);
	cprintf("[ ] Pass 7 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,12);
	cprintf("[%s] Pass 7 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	return TRUE;

}


// German Algo
bool DataEraser::GermanAlgo(void)
{
	bool bStatus=TRUE;
	byte bStat[][2]= {"X", CHECK_MARK};

	Textbar(STATUS_TEXT, "ESC - Cancel", 25);
	textattr(SCREEN_TEXT);
	gotoxy(30,4);
	cprintf("German VSITR Standard");

	textattr(BLINK_TEXT);
	gotoxy(5,6);
	cprintf("[ ] Pass 1 (0x00)");
	FillWriteBuffer(0x00);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,6);
	cprintf("[%s] Pass 1 (0x00)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;
	
	textattr(BLINK_TEXT);
	gotoxy(5,7);
	cprintf("[ ] Pass 2 (0xFF)");
	FillWriteBuffer(0xFF);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,7);
	cprintf("[%s] Pass 2 (0xFF)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,8);
	cprintf("[ ] Pass 3 (0x00)");
	FillWriteBuffer(0x00);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,8);
	cprintf("[%s] Pass 3 (0x00)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,9);
	cprintf("[ ] Pass 4 (0xFF)");
	FillWriteBuffer(0xFF);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,9);
	cprintf("[%s] Pass 4 (0xFF)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,10);
	cprintf("[ ] Pass 5 (0x00)");
	FillWriteBuffer(0x00);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,10);
	cprintf("[%s] Pass 5 (0x00)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,11);
	cprintf("[ ] Pass 6 (0xFF)");
	FillWriteBuffer(0xFF);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,11);
	cprintf("[%s] Pass 6 (0xFF)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,12);
	cprintf("[ ] Pass 7 (0xAA)");
	FillWriteBuffer(0xAA);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,12);
	cprintf("[%s] Pass 7 (0xAA)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,13);
	cprintf("[ ] Pass 8 Verifying");
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, READ);
	textattr(SCREEN_TEXT);
	gotoxy(5,13);
	cprintf("[%s] Pass 8 Verified  ", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	return TRUE;
}


bool DataEraser::GutmannAlgo(void)
{
	bool bStatus=TRUE;
	byte bStat[][2]= {"X", CHECK_MARK};

	byte bFirstPattern[] = { 0x92, 0x49, 0x24 };
	byte bSecondPattern[] = { 0x49, 0x24, 0x92 };
	byte bThirdPattern[] = { 0x24, 0x92, 0x49 };
	byte bFourthPattern[] = { 0x6D, 0xB6, 0xDB };
	byte bFifthPattern[] = { 0xB6, 0xDB, 0x6D };
	byte bSixthPattern[] = { 0xDB, 0x6D, 0xB6 };
	
	Textbar(STATUS_TEXT, "ESC - Cancel", 25);
	textattr(SCREEN_TEXT);
	gotoxy(29,4);
	cprintf("Peter Gutmanns' Algorithm");


	textattr(BLINK_TEXT);
	gotoxy(5,6);
	cprintf("[ ] Pass  1 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,6);
	cprintf("[%s] Pass  1 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,7);
	cprintf("[ ] Pass  2 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,7);
	cprintf("[%s] Pass  2 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,8);
	cprintf("[ ] Pass  3 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,8);
	cprintf("[%s] Pass  3 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,9);
	cprintf("[ ] Pass  4 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,9);
	cprintf("[%s] Pass  4 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,10);
	cprintf("[ ] Pass  5 (0x55)");
	FillWriteBuffer(0x55);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,10);
	cprintf("[%s] Pass  5 (0x55)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,11);
	cprintf("[ ] Pass  6 (0xAA)");
	FillWriteBuffer(0xAA);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,11);
	cprintf("[%s] Pass  6 (0xAA)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,12);
	cprintf("[ ] Pass  7 (Pattern)");
	FillWriteBuffer(bFirstPattern, 3);	// 7th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,12);
	cprintf("[%s] Pass  7 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,13);
	cprintf("[ ] Pass  8 (Pattern)");
	FillWriteBuffer(bSecondPattern, 3);	// 8th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,13);
	cprintf("[%s] Pass  8 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,14);
	cprintf("[ ] Pass  9 (Pattern)");
	FillWriteBuffer(bThirdPattern, 3);	// 9th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,14);
	cprintf("[%s] Pass  9 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,15);
	cprintf("[ ] Pass 10 (0x00)");
	FillWriteBuffer(0x00);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,15);
	cprintf("[%s] Pass 10 (0x00)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,16);
	cprintf("[ ] Pass 11 (0x11)");
	FillWriteBuffer(0x11);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,16);
	cprintf("[%s] Pass 11 (0x11)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,17);
	cprintf("[ ] Pass 12 (0x22)");
	FillWriteBuffer(0x22);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,17);
	cprintf("[%s] Pass 12 (0x22)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,18);
	cprintf("[ ] Pass 13 (0x33)");
	FillWriteBuffer(0x33);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,18);
	cprintf("[%s] Pass 13 (0x33)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,19);
	cprintf("[ ] Pass 14 (0x44)");
	FillWriteBuffer(0x44);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,19);
	cprintf("[%s] Pass 14 (0x44)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,20);
	cprintf("[ ] Pass 15 (0x55)");
	FillWriteBuffer(0x55);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,20);
	cprintf("[%s] Pass 15 (0x55)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,21);
	cprintf("[ ] Pass 16 (0x66)");
	FillWriteBuffer(0x66);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,21);
	cprintf("[%s] Pass 16 (0x66)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(5,22);
	cprintf("[ ] Pass 17 (0x77)");
	FillWriteBuffer(0x77);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(5,22);
	cprintf("[%s] Pass 17 (0x77)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,6);
	cprintf("[ ] Pass 18 (0x88)");
	FillWriteBuffer(0x88);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,6);
	cprintf("[%s] Pass 18 (0x88)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,7);
	cprintf("[ ] Pass 19 (0x99)");
	FillWriteBuffer(0x99);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,7);
	cprintf("[%s] Pass 19 (0x99)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,8);
	cprintf("[ ] Pass 20 (0xAA)");
	FillWriteBuffer(0xAA);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,8);
	cprintf("[%s] Pass 20 (0xAA)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,9);
	cprintf("[ ] Pass 21 (0xBB)");
	FillWriteBuffer(0xBB);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,9);
	cprintf("[%s] Pass 21 (0xBB)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,10);
	cprintf("[ ] Pass 22 (0xCC)");
	FillWriteBuffer(0xCC);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,10);
	cprintf("[%s] Pass 22 (0xCC)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,11);
	cprintf("[ ] Pass 23 (0xDD)");
	FillWriteBuffer(0xDD);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,11);
	cprintf("[%s] Pass 23 (0xDD)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,12);
	cprintf("[ ] Pass 24 (0xEE)");
	FillWriteBuffer(0xEE);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,12);
	cprintf("[%s] Pass 24 (0xEE)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,13);
	cprintf("[ ] Pass 25 (0xFF)");
	FillWriteBuffer(0xFF);
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,13);
	cprintf("[%s] Pass 25 (0xFF)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,14);
	cprintf("[ ] Pass 26 (Pattern)");
	FillWriteBuffer(bFirstPattern, 3);	// 26th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,14);
	cprintf("[%s] Pass 26 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,15);
	cprintf("[ ] Pass 27 (Pattern)");
	FillWriteBuffer(bSecondPattern, 3);	// 27th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,15);
	cprintf("[%s] Pass 27 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,16);
	cprintf("[ ] Pass 28 (Pattern)");
	FillWriteBuffer(bThirdPattern, 3);	// 28th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,16);
	cprintf("[%s] Pass 28 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,17);
	cprintf("[ ] Pass 29 (Pattern)");
	FillWriteBuffer(bFourthPattern, 3);	// 29th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,17);
	cprintf("[%s] Pass 29 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,18);
	cprintf("[ ] Pass 30 (Pattern)");
	FillWriteBuffer(bFifthPattern, 3);	// 30th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,18);
	cprintf("[%s] Pass 30 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,19);
	cprintf("[ ] Pass 31 (Pattern)");
	FillWriteBuffer(bSixthPattern, 3);	// 31th pass
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,19);
	cprintf("[%s] Pass 31 (Pattern)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,20);
	cprintf("[ ] Pass 32 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,20);
	cprintf("[%s] Pass 32 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,21);
	cprintf("[ ] Pass 33 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,21);
	cprintf("[%s] Pass 33 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,22);
	cprintf("[ ] Pass 34 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,22);
	cprintf("[%s] Pass 34 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	textattr(BLINK_TEXT);
	gotoxy(28,23);
	cprintf("[ ] Pass 35 (Random)");
	FillWriteBuffer();
	bStatus = MediaScan(DRIVE, 0, cldblTotalSector, NUM_OF_BLOCKS, WRITE);
	textattr(SCREEN_TEXT);
	gotoxy(28,23);
	cprintf("[%s] Pass 35 (Random)", bStat[bStatus]);
	if (bAbortFlag) return FALSE;

	return TRUE;
}
//////////////// END: DATA ERASE ALGOS ////////////////


// Main Program / Screen
void DataEraser::MainProgram()
{
	void *ptrbBufferScreen;
	stMenuParam *ptrBasic, *ptrDataErase;
	bool bFinish=FALSE;
	word  nScanCh;
	byte  bCh, bCh1, bChoice;

	bAbortFlag = FALSE;

	// Generate menu
	if ( (ptrBasic = (stMenuParam*) calloc(5, sizeof(stMenuParam))) == NULL) return;
	BuildFunctionList(ptrBasic);

	if ( (ptrDataErase = (stMenuParam*) calloc(6, sizeof(stMenuParam))) == NULL) return;
	BuildEraseList(ptrDataErase);
	
	SimpleBox(1, 1, 80, 25, CYAN, BLUE, FALSE, NC);

	if ( (ptrbBufferScreen = malloc(25*80*2)) != NULL)
		gettext(1,1,80,25,ptrbBufferScreen);
	else
		return;

	InitScreen();
	
	// restore screen
   	puttext(1,1,80,25,ptrbBufferScreen);
   	
	SimpleBox(2, 3, 78, 22, BLUE, WHITE, TRUE, WHITE);
	Textbar(COMMAND_TEXT, "F3-Basic Function   F4-Data Erase", 2);
	textattr(SCREEN_TEXT);
	gettext(1,1,80,25,ptrbBufferScreen);

	
	// Clear the keyboard buffer
	while(_bios_keybrd(_KEYBRD_READY))	
		_bios_keybrd(_KEYBRD_READ);

	do {
		nScanCh = _bios_keybrd(_KEYBRD_READ);

   		if(!(bCh=nScanCh))
		{
   			bCh1=nScanCh >> 8;
					
			switch(bCh1)
			{
				case F3:
						bChoice = SimpleMenuBox(2, 3, ptrBasic, 5, MENU_TEXT, NC);

						puttext(1,1,80,25,ptrbBufferScreen); // restore screen

						switch(bChoice)
						{
							case 0: DriveInfoScreen();
									break;
							case 1: WriteSectorInputScreen();
									break;
							case 2: ReadSectorInputScreen();
									break;
							case 3: ViewSectorInputScreen();
									break;
							case 4: 
									if ( DialogBox("Exit", "Are you sure you want to exit?", 2) )
									bFinish=TRUE; break;
						}

						puttext(1,1,80,25,ptrbBufferScreen);
						break;

				case F4:
						bChoice = SimpleMenuBox(22, 3, ptrDataErase, 6, MENU_TEXT, NC);

						puttext(1,1,80,25,ptrbBufferScreen); // restore screen

						switch(bChoice)
						{
							case 0 : 
									if ( DialogBox("Warning", "Are you sure you want to erase?", 2) )
									{	
										puttext(1,1,80,25,ptrbBufferScreen);
	
										textattr(TIME_TEXT);
										gotoxy(55, 13);
										cprintf("Start Date/Time:");
										gotoxy(55, 14);
										GetDateTime(&stStartTime);
										PrintDateTime(&stStartTime);
										
										if ( DODAlgo() )
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Success", "Data Erasure Completed!", 1);
										}
										else
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Error", " Data Erasure Failed", 1);
										}
										
									}
									break;

							case 1 : 
									if ( DialogBox("Warning", "Are you sure you want to erase?", 2) )
									{	
										puttext(1,1,80,25,ptrbBufferScreen);
	
										textattr(TIME_TEXT);
										gotoxy(55, 13);
										cprintf("Start Date/Time:");
										gotoxy(55, 14);
										GetDateTime(&stStartTime);
										PrintDateTime(&stStartTime);
										
										if ( SchneierAlgo() )
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Success", "Data Erasure Completed!", 1);
										}
										else
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Error", " Data Erasure Failed", 1);
										}
										
									}
									break;

							case 2 : 
									if ( DialogBox("Warning", "Are you sure you want to erase?", 2) )
									{	
										puttext(1,1,80,25,ptrbBufferScreen);
	
										textattr(TIME_TEXT);
										gotoxy(55, 13);
										cprintf("Start Date/Time:");
										gotoxy(55, 14);
										GetDateTime(&stStartTime);
										PrintDateTime(&stStartTime);
										
										if ( BritishAlgo() )
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Success", "Data Erasure Completed!", 1);
										}
										else
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Error", " Data Erasure Failed", 1);
										}
										
									}
									break;

							case 3 : 
									if ( DialogBox("Warning", "Are you sure you want to erase?", 2) )
									{	
										puttext(1,1,80,25,ptrbBufferScreen);
	
										textattr(TIME_TEXT);
										gotoxy(55, 13);
										cprintf("Start Date/Time:");
										gotoxy(55, 14);
										GetDateTime(&stStartTime);
										PrintDateTime(&stStartTime);
										
										if ( AustralianAlgo() )
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Success", "Data Erasure Completed!", 1);
										}
										else
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Error", " Data Erasure Failed", 1);
										}
										
									}
									break;

							case 4 : 
									if ( DialogBox("Warning", "Are you sure you want to erase?", 2) )
									{	
										puttext(1,1,80,25,ptrbBufferScreen);

										textattr(TIME_TEXT);
										gotoxy(55, 13);
										cprintf("Start Date/Time:");
										gotoxy(55, 14);
										GetDateTime(&stStartTime);
										PrintDateTime(&stStartTime);

										if ( GermanAlgo() )
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Success", "Data Erasure Completed!", 1);
										}
										else
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Error", " Data Erasure Failed", 1);
										}
									}
									break;

							case 5 : 
									if ( DialogBox("Warning", "Are you sure you want to erase?", 2) )
									{	
										puttext(1,1,80,25,ptrbBufferScreen);

										textattr(TIME_TEXT);
										gotoxy(55, 13);
										cprintf("Start Date/Time:");
										gotoxy(55, 14);
										GetDateTime(&stStartTime);
										PrintDateTime(&stStartTime);
										
										if ( GutmannAlgo() )
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Success", "Data Erasure Completed!", 1);
										}
										else
										{
											GetDateTime(&stStopTime);
											gotoxy(55, 16);
											textattr(TIME_TEXT);
											cprintf("Stop Date/Time:");
											gotoxy(55, 17);
											PrintDateTime(&stStopTime);
											gotoxy(55, 19);
											textattr(TIME_TEXT);
											cprintf("Total Time:");
											gotoxy(57, 20);
											PrintTimeDiff(stStartTime, stStopTime);

											DialogBox("Error", " Data Erasure Failed", 1);
										}
									}
									break;

						}
						bAbortFlag=FALSE;
						puttext(1,1,80,25,ptrbBufferScreen);
						break;
			}
			puttext(1,1,80,25,ptrbBufferScreen);
		}
		//else if ( bCh == ESC ) bFinish=TRUE;
		
		
	} while (!bFinish);
	
	free(ptrBasic);
	free(ptrDataErase);
	free(ptrbBufferScreen);
}


int main()
{
	_setcursortype(_NOCURSOR);
	
	clrscr();
	if (!gDataEraser.InitProgram())
	{
		printf("\nProgram terminated");
		exit(1);
	}

	gDataEraser.MainProgram();
	textattr(LIGHTGRAY|BLACK*16);
	clrscr();
	_setcursortype(_NORMALCURSOR);
	return 0;
}
