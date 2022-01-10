/******************************************
Filename    : scrwin.cpp
Author      : Rozano Maniaol
Description : This creates a scroller list.  Basically you just have to call the
			  ScrollWindow function
			  
Created     : 6/13/06
*******************************************/

#include "scrwin.h"

#define CUR_ATTR  					0x74 	// red text on lightgray
#define NORM_ATTR  					0x1F 	// white text on Blue


#define MAX_NUM_DISPLAY						6

Scroller::Scroller() {}

Scroller::~Scroller() {}


/******************************************
Method Name	: DisplayList
Description	: This method displays the specified strings

Input		:
			  UINT nThreshNo		- the position/index of the first string
			  stStrAttr *pstTempStr - array pointer to a struct //added ryan cuyos  6/30/06
			  UCHAR bFirstX			- X coord of the upper left corner
			  UCHAR bFirstY			- Y coord of the upper left corner
			  UCHAR bSecondX		- X coord of the lower right corner
			  UCHAR bSecondY		- Y coord of the lower right corner

Output		:	void

*******************************************/

void Scroller::DisplayList( word nThreshNo, stStrAttr huge *pstTempStr, byte bFirstX, byte bFirstY, byte bSecondX, byte bSecondY  )
{
	word nAttrCtr;  	
	word nLeftX=bFirstX+4;
  	word nRightX=bSecondX;
	byte sTempstr[75];
	word nCnt = bSecondX - bFirstX - 4;
	word nMaxDisplay = bSecondY - bFirstY - 1;
			
	memset(sTempstr, '\0', 75);
	memset(sTempstr, ' ', nCnt);

	for (nAttrCtr=0; nAttrCtr < nMaxDisplay; nAttrCtr++)
  	{
		//change_attrib(nLeftX,bFirstY+1+nAttrCtr,nRightX,bFirstY+1+nAttrCtr,pstTempStr[bThreshNo+nAttrCtr].bAttr);
		change_txtattr(nLeftX,bFirstY+1+nAttrCtr,nRightX,bFirstY+1+nAttrCtr,pstTempStr,nThreshNo+nAttrCtr);  //added ryan cuyos 07/04/06
		gotoxy(nLeftX ,bFirstY+1+nAttrCtr);
		cprintf("%s", sTempstr);
		gotoxy(nLeftX ,bFirstY+1+nAttrCtr);
		cprintf("%s ", pstTempStr[nThreshNo + nAttrCtr].sStrValues );
  	}
}


/******************************************
Method Name	: ScrollList
Description	: This method take cares of the keyboard function and call the DisplayList

Input		:
			  stStrAttr *pstTempStr - array pointer to a struct //added ryan cuyos  6/30/06
			  UINT  nMaxThreshold	- total count of the string array
			  UCHAR bFirstX			- X coord of the upper left corner
			  UCHAR bFirstY			- Y coord of the upper left corner
			  UCHAR bSecondX		- X coord of the lower right corner
			  UCHAR bSecondY		- Y coord of the lower right corner


Output		:	void
*******************************************/
void Scroller::ScrollList( stStrAttr huge *pstTempStr, word nMaxThreshold, byte bFirstX, byte bFirstY, byte bSecondX, byte bSecondY )
{ 
	byte bCurrentHrow=bFirstY+1;
  	word nCurrentPtr=0, nCurrentTopPtr=0, nMaxCurTopPtr;
	word nMaxDisplay = bSecondY - bFirstY - 1;
	
	word nLeftX=bFirstX+2, nRightX= bSecondX-2;         //for highlight
	
	byte bQuit=FALSE;
	word nScanCh;
	char ch, ch1;
				
	nMaxCurTopPtr=nMaxThreshold-nMaxDisplay;
	DisplayList(nCurrentPtr, pstTempStr, bFirstX, bFirstY, bSecondX, bSecondY ); 
	change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
	
	do {
    	nScanCh = _bios_keybrd(_KEYBRD_READ);
		if(!(ch=nScanCh))
     	{
			ch1=nScanCh >> 8;
      		switch(ch1)
      		{
				case UPARROW:
							if (bCurrentHrow==bFirstY+1)
		       				{   
					       		if (nCurrentPtr>0)
		         				{
		            				nCurrentPtr--;	
		            				nCurrentTopPtr=nCurrentPtr;
  			    					DisplayList(nCurrentTopPtr, pstTempStr, bFirstX, bFirstY, bSecondX, bSecondY); 
 			    					change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
		         				}
		       				}
		      				else
		       				{
								change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,pstTempStr[nCurrentPtr].bAttr);//edited to return the attribute 07/11/06
					      		bCurrentHrow-=1; 
			  					change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
								nCurrentPtr--;	
		       				}
							break;
				case DOWNARROW: 
							if (bCurrentHrow==bSecondY-1)
		       				{   
								if (nCurrentPtr<nMaxThreshold-1)
		        				{
		        					nCurrentTopPtr++;
									nCurrentPtr++;
									DisplayList(nCurrentTopPtr, pstTempStr, bFirstX, bFirstY, bSecondX, bSecondY); 
									change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
								}  
		       				}
		      				else
		       				{
								if (nCurrentPtr<nMaxThreshold-1)
		       					{
									change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,pstTempStr[nCurrentPtr].bAttr);//edited to return the attribute 07/11/06
		           					bCurrentHrow+=1; 
			    					change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
                           			nCurrentPtr++;
                           		}  
		       				}
							break;
				case PGDOWN:  
							if (nCurrentTopPtr!=nMaxCurTopPtr)
		      				{
								nCurrentTopPtr+=(nMaxDisplay-1);
					      		if (nCurrentTopPtr>nMaxCurTopPtr)
			    					nCurrentTopPtr=nMaxCurTopPtr;
									DisplayList(nCurrentTopPtr, pstTempStr, bFirstX, bFirstY, bSecondX, bSecondY);
									change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,pstTempStr[nCurrentTopPtr].bAttr);
		      				} 
 	              	      	if (nCurrentTopPtr==nMaxCurTopPtr)
		      				{
		          				change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,pstTempStr[nCurrentPtr].bAttr);//edited 07/11/06
			  					bCurrentHrow=!nCurrentTopPtr?(bFirstY+1)+nMaxThreshold-1:(bSecondY-1);
		      				}
	                		change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
		      				nCurrentPtr=nCurrentTopPtr+(bCurrentHrow-(bFirstY+1));
		      				break;
				case PGUP: 	if (nCurrentTopPtr!=0)
		     				{
			 	            	if (nCurrentTopPtr<nMaxDisplay-1) 
 	              	    			nCurrentTopPtr=0;
 	              				else   
 	              	    			nCurrentTopPtr-=(nMaxDisplay-1);
									DisplayList(nCurrentTopPtr, pstTempStr, bFirstX, bFirstY, bSecondX, bSecondY); 
							}	
				      		if (nCurrentTopPtr==0)
		      				{
				          		change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,pstTempStr[nCurrentPtr].bAttr);//edited 07/11/06
			  					bCurrentHrow=(bFirstY+1);
		      				}
		        			change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
		      				nCurrentPtr=nCurrentTopPtr+(bCurrentHrow-(bFirstY+1));
		      				break;
		      	case CTRLHOME:   			
	              			nCurrentTopPtr=0;
					      	nCurrentPtr=0;
			      			DisplayList(nCurrentTopPtr, pstTempStr, bFirstX, bFirstY, bSecondX, bSecondY); 
	              			change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,pstTempStr[nCurrentPtr+(bCurrentHrow-(bFirstY+1))].bAttr);
			 		      	bCurrentHrow=(bFirstY+1);
		      				change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
			   		      	break;
			   	case CTRLEND:   			
	              			nCurrentTopPtr=nMaxCurTopPtr;
		      				nCurrentPtr=nMaxThreshold-1;
			      			DisplayList(nCurrentTopPtr, pstTempStr, bFirstX, bFirstY, bSecondX, bSecondY); 
			      			change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,pstTempStr[nCurrentTopPtr+(bCurrentHrow-(bFirstY+1))].bAttr);
		     				bCurrentHrow=nCurrentPtr>=nMaxDisplay-1?bSecondY-1:(bFirstY+1)+nCurrentPtr;
				            change_attrib(nLeftX-1,bCurrentHrow,nRightX+1,bCurrentHrow,CUR_ATTR);
			 		      	break;
      		}
     	}
     	else if (ch==ESC)
    	{
        	bQuit=TRUE;
        	_setcursortype(_NOCURSOR);  
      	}
    		
    } while (!bQuit);

}


/******************************************
Method Name	: ScrollWindow
Description	: This method initialize the scroller

Input		:
			  UCHAR bFirstX			- X coord of the upper left corner
			  UCHAR bFirstY			- Y coord of the upper left corner
			  UCHAR bSecondX		- X coord of the lower right corner
			  UCHAR bSecondY		- Y coord of the lower right corner
			  UINT  nFrameFlag		- 1 for frame
			  						  0 for box
 			  UCHAR bForeGnd		- foreground color
 			  UCHAR bBackGnd		- background color
			  stStrAttr *pstTempStr - array pointer to a struct //added ryan cuyos  6/30/06
			  UINT  nNumOfStr		- total count of the string array
			  UCHAR* sTitle			-

Output		:	void
*******************************************/
void Scroller::show( byte bFirstX, byte bFirstY, byte bSecondX, byte bSecondY, word nFrameFlag, \
	byte bForeGnd, byte bBackGnd, stStrAttr huge *pstTempStr, word nNumOfStr, byte* sTitle, word nType) 
{
	word nOffset;
	if(nType==0)
	{
		textattr(NORM_ATTR);
		if ( nFrameFlag == 1) 
		{
			//StandardBox(bFirstX, bFirstY, bSecondX-bFirstX+1, bSecondY-bFirstY+1, bBackGnd, DOUBLE,bForeGnd, NO_SHADOW, WHITE, sTitle, TRUE);
			SimpleBox(bFirstX, bFirstY, bSecondX-bFirstX+1, bSecondY-bFirstY+1,  bBackGnd, bForeGnd, TRUE, WHITE, WHITE, sTitle);

		}
		else 
		{
			//StandardBox(bFirstX, bFirstY, bSecondX-bFirstX+1,bSecondY-bFirstY+1, bBackGnd, NO_BORDER, bForeGnd, NO_SHADOW, WHITE, sTitle, TRUE);
			SimpleBox(bFirstX, bFirstY, bSecondX-bFirstX+1, bSecondY-bFirstY+1,  bBackGnd, bForeGnd, FALSE, NC, WHITE, sTitle);

			nOffset =  ((bSecondX - bFirstX)/2) - (strlen(sTitle)/2);
			gotoxy( bFirstX + nOffset, bFirstY);
			cprintf("%s", sTitle);

		}
	ScrollList(pstTempStr, nNumOfStr, bFirstX, bFirstY, bSecondX, bSecondY );
	}
	else if(nType==1)
	ScrollList(pstTempStr,nNumOfStr,bFirstX,bFirstY,bSecondX,bSecondY);
	
}

/***********************************************************
Function Name	:	change_txtattr
Description	 	:	Changes the text attributes of the strings, that will be displayed on the frame
Author			:	Ryan Cuyos 
Date			: 	07/04/06
Input			:
					int nX1					- initial x-coordinate
					int nY1					- initial y-coordinate
					int nX2					- final x-coordinate
					int nY2					- final y-coordinate
					stStrAttr *pstTempAttr	- array pointer to a struct containing the attributes
					int nCtr				- specifies the index of the array
Output			:	void
************************************************************/

void change_txtattr(int nX1, int nY1, int nX2, int nY2, stStrAttr huge *pstTempAttr, int nCtr)
{
	int nXPt, nYPt;

	for(nYPt = nY1; nYPt <= nY2; nYPt++)
	{
		for(nXPt = nX1; nXPt <= nX2; nXPt++)
		{
			gotoxy(nXPt, nYPt);
			textattr(pstTempAttr[nCtr].bAttr);
		}
	}
}

