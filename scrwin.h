#ifndef SCRWIN_H
#define SCRWIN_H

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <bios.h>
#include <ctype.h>
#include "common.h"
#include "guitool.h"

#define MAX_ATTRIB 130
#define MAX_LINE_CHARCNT 75

struct stStrAttr{		
	byte sStrValues[MAX_LINE_CHARCNT];
	byte bAttr;
};


extern "C" void change_txtattr(int nX1, int nY1, int nX2, int nY2, stStrAttr huge *pstTempAttr, int nCtr);

class Scroller{
	public:
		Scroller();
		~Scroller();
	private:
		void DisplayList( word nThreshNo, stStrAttr huge *pstTempStr, byte bFirstX, byte bFirstY, byte bSecondX, byte bSecondY );
		void ScrollList( stStrAttr huge *pstTempStr, word nMaxThreshold, byte bFirstX, byte bFirstY, byte bSecondX, byte bSecondY );
		
	public:
		void show( byte bFirstX, byte bFirstY, byte bSecondX, byte bSecondY, word nFrameFlag, \
			byte bForeGnd, byte bBackGnd, stStrAttr huge *pstTempStr, word nNumOfStr, byte* sTitle, word nType=0);

		
};

#endif

