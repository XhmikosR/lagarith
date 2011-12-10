//	Lagarith v1.3.25, copyright 2011 by Ben Greenwood.
//	http://lags.leetcode.net/codec.html
//
//	This program is free software; you can redistribute it and/or
//	modify it under the terms of the GNU General Public License
//	as published by the Free Software Foundation; either version 2
//	of the License, or (at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "lagarith.h"
#include "prediction.h"
#include <float.h>

DWORD WINAPI encode_worker_thread( LPVOID i ){

	ThreadData * threaddata = (ThreadData *)i;

	_controlfp(_PC_53 | _RC_NEAR, _MCW_PC | _MCW_RC);

	WaitForSingleObject(threaddata->StartEvent,INFINITE);

	const unsigned int width = threaddata->width;
	const unsigned int height = threaddata->height;

	const unsigned int format=threaddata->format;

	unsigned char * const buffer =( unsigned char *)threaddata->buffer;
	assert(buffer!=NULL);

	while ( true ){

		unsigned int length = threaddata->length;
		if ( length == 0xFFFFFFFF ){
			break;
		} else if ( length > 0 ){

			unsigned char * src = (unsigned char *)threaddata->source;
			unsigned char * dest = (unsigned char *)threaddata->dest;
			unsigned char * dst = buffer + ((unsigned int)src&15);

			if ( format == YUY2 ){
				Block_Predict_YUV16(src,dst,width,length,false);
			} else {
				Block_Predict(src,dst,width,length,(format!=YV12));
			}

			threaddata->size=threaddata->cObj.Compact(dst,dest,length);
			assert( *(__int64*)dest != 0 );
			threaddata->length=0;
		} else {
			assert(false);
		}

		SignalObjectAndWait(threaddata->DoneEvent,threaddata->StartEvent,INFINITE,false);
		
	}
	threaddata->cObj.FreeCompressBuffers();
	lag_aligned_free(threaddata->buffer,"Thread Buffer");
	threaddata->length=0;
	return 0;
}

DWORD WINAPI decode_worker_thread( LPVOID i ){
	ThreadData * threaddata = (ThreadData *)i;
	
	_controlfp(_PC_53 | _RC_NEAR, _MCW_PC | _MCW_RC);

	WaitForSingleObject(threaddata->StartEvent,INFINITE);
	while ( true ){

		unsigned int length = threaddata->length;

		if ( length == 0xFFFFFFFF ){
			break;
		} else if ( length > 0 ){
			unsigned char * src=(unsigned char *)threaddata->source;
			unsigned char * dest=(unsigned char *)threaddata->dest;
			threaddata->cObj.Uncompact(src,dest,length);
			threaddata->length=0;
		} else {
			assert(false);
		}
		SignalObjectAndWait(threaddata->DoneEvent,threaddata->StartEvent,INFINITE,false);
	}
	threaddata->cObj.FreeCompressBuffers();
	threaddata->length=0;
	return 0;
}

int CodecInst::InitThreads( int encode ){

	DWORD temp;
	threads[0].length=0;
	threads[1].length=0;
	threads[2].length=0;

	threads[0].StartEvent=CreateEvent(NULL,false,false,NULL);
	threads[0].DoneEvent=CreateEvent(NULL,false,false,NULL);
	threads[1].StartEvent=CreateEvent(NULL,false,false,NULL);
	threads[1].DoneEvent=CreateEvent(NULL,false,false,NULL);
	threads[2].StartEvent=CreateEvent(NULL,false,false,NULL);
	threads[2].DoneEvent=CreateEvent(NULL,false,false,NULL);

	unsigned int use_format=format;
	if ( lossy_option ){
		if ( lossy_option==1 ){
			use_format=YUY2;
		} else {
			use_format=YV12;
		}
	}

	if ( use_format > YUY2 ){
		threads[0].width=width;
		threads[1].width=width;
	} else {
		threads[0].width=width/2;
		threads[1].width=width/2;
	}
	threads[2].width=width;

	
	if ( use_format != YV12 ){
		threads[0].height=height;
		threads[1].height=height;
	} else {
		threads[0].height=height/2;
		threads[1].height=height/2;
	}
	threads[2].height=height;

	threads[0].format=use_format;
	threads[1].format=use_format;
	threads[2].format=use_format;

	threads[0].thread=NULL;
	threads[1].thread=NULL;
	threads[2].thread=NULL;
	threads[0].buffer=NULL;
	threads[1].buffer=NULL;
	threads[2].buffer=NULL;

	int buffer_size = align_round(threads[0].width,16)*threads[0].height+256;

	bool memerror = false;
	bool interror = false;

	if ( encode ) {
		if ( !threads[0].cObj.InitCompressBuffers( buffer_size ) || !threads[1].cObj.InitCompressBuffers( buffer_size )) {
			memerror = true;
		}
	}
	if ( !memerror ){
		if ( lossy_option==0 && format == RGB32 ){
			if ( encode ){
				if ( !threads[2].cObj.InitCompressBuffers( buffer_size )) {
					memerror = true;
				}
			}
			if ( !memerror ){
				if ( encode ){
					threads[2].thread=CreateThread(NULL,0, encode_worker_thread, &threads[2],CREATE_SUSPENDED,&temp);
				} else {
					threads[2].thread=CreateThread(NULL,0, decode_worker_thread, &threads[2],CREATE_SUSPENDED,&temp);
				}
				if ( threads[2].thread ){
					SetThreadPriority(threads[2].thread,THREAD_PRIORITY_BELOW_NORMAL);
				} else {
					interror = true;
				}
			}
		}
		if ( !interror && !memerror ){
			if ( encode ){
				threads[0].thread=CreateThread(NULL,0, encode_worker_thread, &threads[0],CREATE_SUSPENDED,&temp);
				threads[1].thread=CreateThread(NULL,0, encode_worker_thread, &threads[1],CREATE_SUSPENDED,&temp);
			} else {
				threads[0].thread=CreateThread(NULL,0, decode_worker_thread, &threads[0],CREATE_SUSPENDED,&temp);
				threads[1].thread=CreateThread(NULL,0, decode_worker_thread, &threads[1],CREATE_SUSPENDED,&temp);
			}
			if ( !threads[0].thread || !threads[1].thread || (use_format==RGB32 && !threads[2].thread )){
				interror = true;
			} else {
				SetThreadPriority(threads[0].thread,THREAD_PRIORITY_BELOW_NORMAL);
				SetThreadPriority(threads[1].thread,THREAD_PRIORITY_BELOW_NORMAL);
			}
		}
	}
	
	if ( !memerror && !interror && encode ){
		threads[0].buffer = (unsigned char *)lag_aligned_malloc((void *)threads[0].buffer,buffer_size,16,"threads[0].buffer");
		threads[1].buffer = (unsigned char *)lag_aligned_malloc((void *)threads[1].buffer,buffer_size,16,"threads[1].buffer");
		if ( format == RGB32 ){
			threads[2].buffer = (unsigned char *)lag_aligned_malloc((void *)threads[2].buffer,buffer_size,16,"threads[2].buffer");
			if ( threads[2].buffer == NULL )
				memerror = true;
		}
		if (threads[0].buffer == NULL || threads[1].buffer == NULL ){
			memerror = true;
		}
	}

	if ( memerror || interror ){

		if ( encode ){
			lag_aligned_free(threads[0].buffer,"threads[0].buffer");
			lag_aligned_free(threads[1].buffer,"threads[1].buffer");
			threads[0].cObj.FreeCompressBuffers();
			threads[1].cObj.FreeCompressBuffers();
			threads[2].cObj.FreeCompressBuffers();
			lag_aligned_free(threads[2].buffer,"threads[2].buffer");
		}

		threads[0].thread=NULL;
		threads[1].thread=NULL;
		threads[2].thread=NULL;
		if ( memerror ){
			return ICERR_MEMORY;
		} else {
			return ICERR_INTERNAL;
		}
	} else {
		ResumeThread(threads[0].thread);
		ResumeThread(threads[1].thread);
		if ( threads[2].thread ){
			ResumeThread(threads[2].thread);
		}
		return ICERR_OK;
	}
}

void CodecInst::EndThreads(){
	threads[0].length=0xFFFFFFFF;
	threads[1].length=0xFFFFFFFF;
	threads[2].length=0xFFFFFFFF;

	HANDLE events[3];
	events[0]=threads[0].thread;
	events[1]=threads[1].thread;
	events[2]=threads[2].thread;

	if ( threads[0].thread ){
		SetEvent(threads[0].StartEvent);
	}
	if ( threads[1].thread ){
		SetEvent(threads[1].StartEvent);
	}
	if ( threads[2].thread ){
		SetEvent(threads[2].StartEvent);
		WaitForMultipleObjectsEx(3,&events[0],true,10000,true);
	} else {
		WaitForMultipleObjectsEx(2,&events[0],true,10000,true);
	}

	if ( !CloseHandle(threads[0].thread) ){
		/*LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			0, // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		MessageBox( NULL, (char *)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
		LocalFree( lpMsgBuf );*/
		//MessageBox (HWND_DESKTOP, "CloseHandle failed for thread A", "Error", MB_OK | MB_ICONEXCLAMATION);
	}
	if ( !CloseHandle(threads[1].thread) ){
	//	MessageBox (HWND_DESKTOP, "CloseHandle failed for thread B", "Error", MB_OK | MB_ICONEXCLAMATION);
	}
	if ( threads[2].thread && format == RGB32 && !CloseHandle(threads[2].thread) ){
		//MessageBox (HWND_DESKTOP,"CloseHandle failed for thread C", "Error", MB_OK | MB_ICONEXCLAMATION);
	}
	threads[0].thread=NULL;
	threads[1].thread=NULL;
	threads[2].thread=NULL;
	threads[0].length=0;
	threads[1].length=0;
	threads[2].length=0;
}

//MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);