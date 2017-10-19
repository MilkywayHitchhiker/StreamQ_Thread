// StreamQ_Thread.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "RingBuffer.h"
#include <Windows.h>
#include <process.h>
#include <time.h>
#include <string>
#include <list>


#define ThreadMax 8

#define dfTYPE_ADD_STR		0
#define dfTYPE_DEL_STR		1
#define dfTYPE_PRINT_LIST	2
#define dfTYPE_QUIT		3

using namespace std;


//메세지Q 처리스레드 x3
unsigned int WINAPI WorkerThread (LPVOID lpNum);

HANDLE WakeUpHandle;

list<wstring> g_List;
SRWLOCK ListCS;

CRingbuffer MsgQ;


struct st_MSG_HEAD
{
	short shType;
	short shStrLen;
};


//Message Create Thread
int main()
{
	wprintf (L"===========MainThread Start===========\n");


	HANDLE Thread[ThreadMax];
	for ( int Cnt = 0; Cnt < ThreadMax; Cnt++ )
	{
		Thread[Cnt] =(HANDLE) _beginthreadex (NULL, 0, WorkerThread, 0, 0, NULL);
	}

	WakeUpHandle = CreateEvent (NULL, FALSE, FALSE, NULL);
	InitializeSRWLock (&ListCS);


	st_MSG_HEAD Header;
	WCHAR MainString[12] = L"TEST_STRING";
	WCHAR MsgString[12];
	
	srand ((unsigned int)time(NULL));



	while ( 1 )
	{
		//메시지 타입 선택부
		Header.shType = rand () % 3;
		Header.shStrLen = 0;


		switch ( Header.shType )
		{
		case dfTYPE_ADD_STR:
			Header.shStrLen = rand () % 10 + 1;
			wcsncpy_s (MsgString, MainString, Header.shStrLen);
			break;
		case dfTYPE_DEL_STR:
			break;

		case dfTYPE_PRINT_LIST:
			break;

		default :
			wprintf (L"Header Setting Error");
			break;
		}


		//종료키 입력부
		if ( GetAsyncKeyState ('Q') & 0x8001 )
		{
			//종료메세지 전송
			Header.shType = dfTYPE_QUIT;
			break;
		}

		MsgQ.Lock ();
		if ( MsgQ.GetFreeSize () < sizeof (Header) + Header.shStrLen * 2 )
		{
			int* a = ( int* )1;
			int b = *a;

		}
		else
		{
			MsgQ.Put (( char * )&Header, sizeof (Header)) != sizeof (Header);
			MsgQ.Put (( char * )MsgString, Header.shStrLen * 2) != Header.shStrLen * 2;
		}
		wprintf (L"Size : %d\n", MsgQ.GetUseSize());
		MsgQ.Free ();


		SetEvent (WakeUpHandle);

		Sleep (0);
	}

	
	WaitForMultipleObjects (ThreadMax, Thread, TRUE, 50);
	for ( int Cnt = 0; Cnt < ThreadMax; Cnt++ )
	{
		CloseHandle (Thread[Cnt]);
	}

	wprintf (L"===========MainThread End===========\n");
    return 0;
}

//메세지Q 처리스레드 x3
unsigned int WINAPI WorkerThread (LPVOID lpNum)
{
	wprintf (L"===========WorkerThread Start===========\n");
	int retval;
	bool Flag = true;
	list<wstring>::iterator iter;
	st_MSG_HEAD Header;
	WCHAR str[12];

	while ( Flag )
	{
		retval = WaitForSingleObject (WakeUpHandle, INFINITE);
		if ( retval != WAIT_OBJECT_0 )
		{
			wprintf (L"ERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERRORERROR");
			break;
		}



		while ( 1 )
		{
			MsgQ.Lock ();
			if ( MsgQ.GetUseSize () < sizeof (st_MSG_HEAD) )
			{
				MsgQ.Free ();
				break;
			}

			MsgQ.Get (( char * )&Header, sizeof (Header));
			MsgQ.Get (( char * )str, Header.shStrLen*2);
			MsgQ.Free ();

			if ( Header.shType == dfTYPE_QUIT )
			{
				//종료메세지 전송
				MsgQ.Lock ();
				MsgQ.Put ((char *)&Header,sizeof(Header));
				MsgQ.Free ();

				SetEvent (WakeUpHandle);
				Flag = false;
				break;
			}


			AcquireSRWLockExclusive (&ListCS);
			switch ( Header.shType )
			{
			case dfTYPE_ADD_STR:
				str[Header.shStrLen] = '\0';
				g_List.push_front (str);


				break;
			case dfTYPE_DEL_STR:


				if ( !g_List.empty () )
				{
					g_List.pop_back ();
				}

				break;

			case dfTYPE_PRINT_LIST: 
			{
				wprintf (L"List : ");
				for ( iter = g_List.begin (); iter != g_List.end ();)
				{
					wprintf (L"[%s] ", (*iter).c_str ());
					iter++;
				}
				wprintf (L"\n");
			}
				break;

			default:
				wprintf (L"Header Type Error\n");
				break;
			}
			ReleaseSRWLockExclusive (&ListCS);



		}
	}

	wprintf (L"===========WorkerThread End===========\n");
	return 0;
}