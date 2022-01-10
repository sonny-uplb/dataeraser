#ifndef time8254_H
#define time8254_H


typedef struct {
	unsigned long ticks;
	unsigned int part;
} timestamp8254;

#define BIOS_TICK_COUNT_P ((volatile unsigned long far *) 0x0040006CL)

class TimeStamp8254 {
	public:
		TimeStamp8254();
		~TimeStamp8254();

		double dwTotalMilliSec;

	private:
		timestamp8254 ts, ts1, ts2;

	protected:
		double dwTotalMicroSec;
		double dwTotalSec;

		unsigned int nHour;
		unsigned int nMin;
		float fSecond;
		
	private:
		void SetMode2();
		double ConvertTimestamp(timestamp8254 * tsp );
		void GetTimestamp(timestamp8254 * tsp);
		void CalcElapsed(timestamp8254 * startts, timestamp8254 * stopts, timestamp8254 * diffts);
		void PrintTime( double dwTimetick, char bMode=1 );
		void GetTimeStampVariable(double timetick);

	public:
		void GetTimeStampVariable();
		void SetStartTime();
		void PrintEndTime();
		void PrintCurrentTime();
		void PrintStartTime();
		void PrintElapsedTime(char bMode=1);
		void PrintCurElapsedTime();
		void PrintCurElapsedMicroTime();
			
};
#endif

