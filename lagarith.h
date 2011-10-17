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

#ifndef _MAIN_HEADER
#define _MAIN_HEADER

#include <process.h>
#include <malloc.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>
#include <malloc.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "prediction.h"

#ifndef X64_BUILD
extern bool SSE;
extern bool	SSE2;
#endif
//extern bool SSE3;
extern bool	SSSE3;

unsigned __int64 GetTime();


//#define X64_BUILD // this will be set automaticly if using the release 64 configuration

//#define LOSSLESS_CHECK			// enables decompressing encoded frames and comparing to original

#define LAGARITH_RELEASE		// if this is a version to release, disables all debugging info 

inline void * lag_aligned_malloc( void *ptr, int size, int align, char *str ) {
	if ( ptr ){
		try {
			#ifndef LAGARITH_RELEASE
			char msg[128];
			sprintf_s(msg,128,"Buffer '%s' is not null, attempting to free it...",str);
			MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
			#endif
			_aligned_free(ptr);
		} catch ( ... ){
			#ifndef LAGARITH_RELEASE
			char msg[256];
			sprintf_s(msg,128,"An exception occurred when attempting to free non-null buffer '%s' in lag_aligned_malloc",str);
			MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
			#endif
		}
	}
	return _aligned_malloc(size,align);
}

#ifdef LAGARITH_RELEASE
#define lag_aligned_free(ptr, str) { \
	if ( ptr ){ \
		try {\
			_aligned_free((void*)ptr);\
		} catch ( ... ){ } \
	} \
	ptr=NULL;\
}
#else 
#define lag_aligned_free(ptr, str) { \
	if ( ptr ){ \
		try { _aligned_free(ptr); } catch ( ... ){\
			char err_msg[256];\
			sprintf_s(err_msg,256,"Error when attempting to free pointer %s, value = 0x%X - file %s line %d",str,ptr,__FILE__, __LINE__);\
			MessageBox (HWND_DESKTOP, err_msg, "Error", MB_OK | MB_ICONEXCLAMATION);\
		} \
	} \
	ptr=NULL;\
}
#endif

// y must be 2^n
#define align_round(x,y) ((((unsigned int)(x))+(y-1))&(~(y-1)))

#include "resource.h"
#include "compact.h"

static const DWORD FOURCC_LAGS = mmioFOURCC('L','A','G','S');
static const DWORD FOURCC_YUY2 = mmioFOURCC('Y','U','Y','2');
static const DWORD FOURCC_UYVY = mmioFOURCC('U','Y','V','Y');
static const DWORD FOURCC_YV16 = mmioFOURCC('Y','V','1','6');
static const DWORD FOURCC_YV12 = mmioFOURCC('Y','V','1','2');


// possible frame flags

#define UNCOMPRESSED		1	// Used for debugging
#define UNALIGNED_RGB24		2	// RGB24 keyframe with a width that is not a multiple of 4; this has to be handled 
								// differently due to the fact that previously, the codec did not remove byte padding to 
								// align each scan line. Old versions are decoded as ARITH_RGB24
#define ARITH_YUY2			3	// Standard YUY2 keyframe frame
#define ARITH_RGB24			4	// Standard RGB24 keyframe frame
#define BYTEFRAME			5	// solid greyscale color frame
#define PIXELFRAME			6	// solid non-greyscale color frame
#define OBSOLETE_ARITH		7	// Old RGB keyframe frame, decode support only
#define ARITH_ALPHA			8	// Standard RGBA keyframe frame
#define PIXELFRAME_ALPHA	9	// RGBA pixel frame
#define ARITH_YV12			10	// Standard YV12 frame	
#define REDUCED_RES         11	// Reduced Resolution frame, decode support only


// possible colorspaces
#define RGB24	24
#define RGB32	32
#define YUY2	16
#define YV12	12

struct ThreadData {
	volatile const unsigned char * source;
	volatile unsigned char * dest;
	volatile unsigned char * buffer;
	volatile unsigned int width;
	volatile unsigned int height;
	volatile unsigned int format;
	volatile unsigned int length;	// uncompressed data length
	volatile unsigned int size;		// compressed data length
	volatile HANDLE thread;
	volatile HANDLE StartEvent; 
	volatile HANDLE DoneEvent;
	unsigned int priority;
	CompressClass cObj;
};

class CodecInst {
public:
	int started;			//if the codec has been properly initalized yet
	unsigned char * buffer;
	unsigned char * prev;
	const unsigned char * in;
	unsigned char * out;
	unsigned char * buffer2;

	unsigned int length;
	unsigned int width;
	unsigned int height;
	unsigned int format;	//input format for compressing, output format for decompression. Also the bitdepth.
	
	bool nullframes;
	bool use_alpha;
	int lossy_option;
	bool multithreading;
	CompressClass cObj;
	unsigned int compressed_size;
	ThreadData threads[3];

	Performance performance;

	CodecInst();
	~CodecInst();

	DWORD GetState(LPVOID pv, DWORD dwSize);
	DWORD SetState(LPVOID pv, DWORD dwSize);
	DWORD Configure(HWND hwnd);
	DWORD GetInfo(ICINFO* icinfo, DWORD dwSize);

	DWORD CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD Compress(ICCOMPRESS* icinfo, DWORD dwSize);
	DWORD CompressEnd();

	DWORD DecompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD Decompress(ICDECOMPRESS* icinfo, DWORD dwSize);
	DWORD DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressEnd();

	BOOL QueryConfigure();

	void uncompact_macro( const unsigned char * _in, unsigned char * _out, unsigned int _length, ThreadData * _thread);
	int InitThreads( int encode);
	void EndThreads();

	int CompressRGB24(ICCOMPRESS* icinfo);
	int CompressRGBA(ICCOMPRESS* icinfo);
	int CompressYUV16(ICCOMPRESS* icinfo);
	int CompressYV12(ICCOMPRESS* icinfo);
	int CompressLossy(ICCOMPRESS* icinfo);
	unsigned int HandleTwoCompressionThreads(unsigned int chan_size);
	unsigned int HandleThreeCompressionThreads(unsigned int chan_size);

	void SetSolidFrameRGB24(const unsigned int r, const unsigned int g, const unsigned int b);
	void SetSolidFrameRGB32(const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a);
	void Decode3Channels(unsigned char * dst1, unsigned int len1, unsigned char * dst2, unsigned int len2, unsigned char * dst3, unsigned int len3);
	void ArithRGBDecompress();
	void ArithRGBADecompress();
	void ArithYUY2Decompress();
	void ArithYV12Decompress();
	void UnalignedDecompress();
	void ReduceResDecompress();
};

CodecInst* Open(ICOPEN* icinfo);
DWORD Close(CodecInst* pinst);

#endif