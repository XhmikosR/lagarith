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

#include "compact.h"
#include "lagarith.h"
#include "interface.h"
#include "prediction.h"
#include "threading.h"
#include "convert.h"
#include "convert_yv12.h"
#include <float.h>
#ifndef X64_BUILD
#include "huffyuv_a.h"
#include "convert_xvid.cpp"
#endif

// initalize the codec for decompression
DWORD CodecInst::DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut){
	if ( started == 0x1337){
		DecompressEnd();
	}
	started = 0;

	if ( int error = DecompressQuery(lpbiIn,lpbiOut) != ICERR_OK ){
		return error;
	}

	int buffer_size;

	width = lpbiIn->biWidth;
	height = lpbiIn->biHeight;

	format = lpbiOut->biBitCount;
	
	length = width*height*format/8;

	buffer_size = length+2048;
	if ( format >= RGB24 ){
		buffer_size=align_round(width*4,8)*height+2048;
	}

	buffer = (unsigned char *)lag_aligned_malloc( buffer,buffer_size,16,"buffer");
	buffer2 = (unsigned char *)lag_aligned_malloc( prev,buffer_size,16,"prev");

	if ( !buffer || !buffer2 ){
		return ICERR_MEMORY;
	}

	if ( multithreading ){
		int code = InitThreads( false);
		if ( code != ICERR_OK ){
			return code;
		}
	}
	started = 0x1337;
	return ICERR_OK;
}

// release resources when decompression is done
DWORD CodecInst::DecompressEnd(){
	if ( started == 0x1337 ){
		if ( multithreading ){
			EndThreads();
		}

		lag_aligned_free( buffer,"buffer");
		lag_aligned_free( buffer2, "buffer2");
		cObj.FreeCompressBuffers();

	}
	started=0;
	return ICERR_OK;
}

void CodecInst::Decode3Channels(unsigned char * dst1, unsigned int len1, unsigned char * dst2, unsigned int len2, unsigned char * dst3, unsigned int len3){

	const unsigned char * src1 = in + 9;
	const unsigned char * src2 = in + *(UINT32*)(in+1);
	const unsigned char * src3 = in + *(UINT32*)(in+5);

	if ( !multithreading ){
		cObj.Uncompact(src1,dst1,len1);
		cObj.Uncompact(src2,dst2,len2);
		cObj.Uncompact(src3,dst3,len3);
		return;
	} else {
		int size1 = *(UINT32*)(in+1);
		int size2 = *(UINT32*)(in+5);
		int size3 = compressed_size-size2;
		size2 -= size1;
		size1 -= 9;

		// Compressed size should aproximate decoding time.
		// The priority of the largest channel is boosted to improve
		// load balancing when there are fewer cores than channels
		if ( size1 >= size2 && size1 >= size3  ){
			if ( threads[0].priority != THREAD_PRIORITY_BELOW_NORMAL){
				SetThreadPriority(threads[0].thread,THREAD_PRIORITY_BELOW_NORMAL);
				SetThreadPriority(threads[1].thread,THREAD_PRIORITY_BELOW_NORMAL);
				threads[0].priority=THREAD_PRIORITY_BELOW_NORMAL;
				threads[1].priority=THREAD_PRIORITY_BELOW_NORMAL;
			}
		} else if ( size2 >= size3 ){
			if( threads[0].priority != THREAD_PRIORITY_ABOVE_NORMAL ){
				SetThreadPriority(threads[0].thread,THREAD_PRIORITY_ABOVE_NORMAL);
				SetThreadPriority(threads[1].thread,THREAD_PRIORITY_NORMAL);
				threads[0].priority=THREAD_PRIORITY_ABOVE_NORMAL;
				threads[1].priority=THREAD_PRIORITY_NORMAL;
			}
		} else {
			if( threads[1].priority != THREAD_PRIORITY_ABOVE_NORMAL ){
				SetThreadPriority(threads[0].thread,THREAD_PRIORITY_NORMAL);
				SetThreadPriority(threads[1].thread,THREAD_PRIORITY_ABOVE_NORMAL);
				threads[0].priority=THREAD_PRIORITY_NORMAL;
				threads[1].priority=THREAD_PRIORITY_ABOVE_NORMAL;
			}
		}

		threads[0].source=src2;
		threads[0].dest=dst2;
		threads[0].length=len2;
		SetEvent(threads[0].StartEvent);

		threads[1].source=src3;
		threads[1].dest=dst3;
		threads[1].length=len3;
		SetEvent(threads[1].StartEvent);

		cObj.Uncompact(src1,dst1,len1);

		wait_for_threads(2);
	}
}

// decompress a typical RGB24 frame
void CodecInst::ArithRGBDecompress(){

	const unsigned int pixels = width*height;

	unsigned char * bdst = buffer;
	unsigned char * gdst = buffer+pixels;
	unsigned char * rdst = buffer+pixels*2;

	if ( in[0] != OBSOLETE_ARITH ){
		Decode3Channels(bdst, pixels, gdst, pixels, rdst, pixels);
	} else {
		const unsigned char * bsrc = in + 9;
		const unsigned char * gsrc = in + *(UINT32*)(in+1);
		const unsigned char * rsrc = in + *(UINT32*)(in+5);

		ObsoleteUncompact(bsrc,bdst,pixels,buffer2);
		ObsoleteUncompact(gsrc,gdst,pixels,buffer2);
		ObsoleteUncompact(rsrc,rdst,pixels,buffer2);
	}

	if ( (width&3)==0 || in[0] == UNALIGNED_RGB24 ){
		if ( format == RGB24 ){
			Interleave_And_Restore_RGB24(out,rdst,gdst,bdst,width,height,&performance);
		} else {
			Interleave_And_Restore_RGB32(out,rdst,gdst,bdst,width,height,&performance);
		}
	} else {
		Interleave_And_Restore_Old_Unaligned(bdst,gdst,rdst,out,buffer2,(format==RGB24),width,height);
	}
	
}


// decompress a RGB24 pixel frame
void CodecInst::SetSolidFrameRGB24(const unsigned int r, const unsigned int g, const unsigned int b){
	const unsigned int stride = align_round(width*3,4);
	if ( r == g && r == b ){
		memset(out,r,stride*height);
	}
	for ( unsigned int y=0; y<height; y++){
		for ( unsigned int x=0; x<width; x++){
			out[y*stride+x*3+0]=b;
			out[y*stride+x*3+1]=g;
			out[y*stride+x*3+2]=r;
		}
	}
}

void CodecInst::SetSolidFrameRGB32(const unsigned int r, const unsigned int g, const unsigned int b, const unsigned int a){
	if ( r == g && r == b && r==a ){
		memset(out,r,width*height*4);
	}
	unsigned int pixel = b + (g<<8) + (r<<16) + (a<<24);
	for ( unsigned int x=0; x<width*height; x++){
		((unsigned int*)out)[x] = pixel;
	}
}

// decompress a RGBA keyframe
void CodecInst::ArithRGBADecompress(){

	const unsigned int pixels = width*height;
	unsigned char * rdst=buffer;
	unsigned char * gdst=buffer+pixels;
	unsigned char * bdst=buffer+pixels*2;
	unsigned char * adst=buffer+pixels*3;

	const unsigned char * bsrc = in + 13;
	const unsigned char * gsrc = in + *(UINT32*)(in+1);
	const unsigned char * rsrc = in + *(UINT32*)(in+5);
	const unsigned char * asrc = in + *(UINT32*)(in+9);

	if ( !multithreading ){
		cObj.Uncompact(bsrc,bdst,pixels);
		cObj.Uncompact(gsrc,gdst,pixels);
		cObj.Uncompact(rsrc,rdst,pixels);
		if ( format == RGB32 ){
			cObj.Uncompact(asrc,adst,pixels);
		}
	} else {
		int bsize = *(UINT32*)(in+1);
		int gsize = *(UINT32*)(in+5);
		int rsize = *(UINT32*)(in+9);
		int asize =  compressed_size; 
		if ( format == RGB32 ){ // ignore alpha channel if not RGB32
			asize-=rsize;
		}
		rsize -= gsize;
		gsize -= bsize;
		bsize -= 13;

		// Compressed size should aproximate decoding time.
		// This conditional make the smallest channel have lower priority - the idea
		// is to improve load-balancing when there are fewer cores than threads.
		if ( bsize <= rsize && bsize <= gsize && bsize <= asize){
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_BELOW_NORMAL);
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_NORMAL);
			SetThreadPriority(threads[2].thread,THREAD_PRIORITY_NORMAL);
		} else if ( rsize <= gsize && rsize <= asize ){
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_NORMAL);
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_BELOW_NORMAL);
			SetThreadPriority(threads[2].thread,THREAD_PRIORITY_NORMAL);
		} else if ( asize <= gsize ){
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_NORMAL);
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_NORMAL);
			SetThreadPriority(threads[2].thread,THREAD_PRIORITY_BELOW_NORMAL);
		} else {
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_ABOVE_NORMAL);
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_ABOVE_NORMAL);
			SetThreadPriority(threads[2].thread,THREAD_PRIORITY_ABOVE_NORMAL);
		}

		threads[0].source=bsrc;
		threads[0].dest=bdst;
		threads[0].length=pixels;
		SetEvent(threads[0].StartEvent);

		threads[1].source=rsrc;
		threads[1].dest=rdst;
		threads[1].length=pixels;
		SetEvent(threads[1].StartEvent);

		if ( format == RGB32 ){
			threads[2].source=asrc;
			threads[2].dest=adst;
			threads[2].length=pixels;
			SetEvent(threads[2].StartEvent);

			cObj.Uncompact(gsrc,gdst,pixels);
			wait_for_threads(3);
		} else {
			cObj.Uncompact(gsrc,gdst,pixels);
			wait_for_threads(2);
		}
		
	}

	if ( format == RGB24 ){
		Interleave_And_Restore_RGB24(out,rdst,gdst,bdst,width,height,&performance);
	} else {
		Interleave_And_Restore_RGBA(out,rdst,gdst,bdst,adst,width,height,&performance);
	}
}

// Decompress a YUY2 keyframe
void CodecInst::ArithYUY2Decompress(){
	unsigned char * dst = out;
	unsigned char * dst2 = buffer;
	if ( format == YUY2 ){
		dst = buffer;
		dst2= out;
	}

	const unsigned int pixels = width*height;

	unsigned char *ydst = dst;
	unsigned char *udst = ydst+pixels;
	unsigned char *vdst = udst+pixels/2;

	Decode3Channels(ydst, pixels, udst, pixels/2, vdst, pixels/2);

	// special case: RLE detected a solid Y value (compressed size = 2),
	// need to set 2nd Y value for restoration to work right
	if ( *(UINT32*)(in+1) == 11){ 
		dst[1]=dst[0];
	}
	Interleave_And_Restore_YUY2(dst2,ydst,udst,vdst,width,height,&performance);

#ifndef X64_BUILD
	if ( format != YUY2 ){
		if ( format == RGB24 ){
			mmx_YUY2toRGB24(buffer,out,buffer+width*height*2,width*2);
		} else {
			mmx_YUY2toRGB32(buffer,out,buffer+width*height*2,width*2);
		} 
	}
#else
	if ( format != YUY2 ){
		if ( format == RGB24 ){
			mmx_YUY2toRGB24(buffer,out,buffer+width*height*2,width*2,width*2,0);
		} else {
			mmx_YUY2toRGB32(buffer,out,buffer+width*height*2,width*2,width*2,0);
		} 
	}
#endif
 
}

// Decompress a YV12 keyframe
void CodecInst::ArithYV12Decompress(){

	unsigned char * dst = out;
	unsigned char * dst2 = buffer;
	if ( format == YUY2 ){
		dst = buffer;
		dst2 = out;
	}

	const unsigned int pixels = width*height;
	unsigned char * ydst = dst;
	unsigned char * udst = ydst + pixels;
	unsigned char * vdst = udst + pixels/4;

	Decode3Channels(ydst, pixels, udst, pixels/4, vdst, pixels/4);

	Restore_YV12(ydst,udst,vdst,width,height,&performance);

	if ( format == YV12 )
		return;

	//upsample if needed
#ifndef X64_BUILD
	if ( SSE2 ){
		isse_yv12_to_yuy2(dst,dst+width*height+width*height/4,dst+width*height,width,width,width/2,dst2,width*2,height);
	} else {
		mmx_yv12_to_yuy2(dst,dst+width*height+width*height/4,dst+width*height,width,width,width/2,dst2,width*2,height);
	}
#else
	isse_yv12_to_yuy2(dst,dst+width*height+width*height/4,dst+width*height,width,width,width/2,dst2,width*2,height);
#endif

	if ( format == YUY2 )
		return;

	// upsample to RGB
#ifndef X64_BUILD
	if ( format == RGB32 ){
		mmx_YUY2toRGB32(dst2,out,dst2+width*height*2,width*2);
	} else {
		mmx_YUY2toRGB24(dst2,out,dst2+width*height*2,width*2);
	}
#else
	if ( format == RGB32 ){
		mmx_YUY2toRGB32(dst2,out,dst2+width*height*2,width*2,width*2,1);
	} else {
		mmx_YUY2toRGB24(dst2,out,dst2+width*height*2,width*2,width*2,1);
	}
#endif
}

void CodecInst::ReduceResDecompress(){

	width/=2;
	height/=2;

	unsigned char * dest = (format==YV12)?buffer:out;

	const unsigned int pixels = width*height;
	unsigned char * ydst = dest;
	unsigned char * udst = ydst + pixels;
	unsigned char * vdst = udst + pixels/4;

	Decode3Channels(ydst, pixels, udst, pixels/4, vdst, pixels/4);

	Restore_YV12(ydst,udst,vdst,width,height,&performance);

	width*=2;
	height*=2;

	dest=(format==YV12)?out:buffer;

	unsigned char * ysrc = ydst;
	unsigned char * usrc = udst;
	unsigned char * vsrc = vdst;
	ydst = dest;
	udst = ydst+width*height;
	vdst = udst+width*height/4;

	Double_Resolution(ysrc,ydst,buffer2,width/2,height/2);
	Double_Resolution(usrc,udst,buffer2,width/4,height/4);
	Double_Resolution(vsrc,vdst,buffer2,width/4,height/4);

	ysrc = ydst;
	usrc = udst;
	vsrc = vdst;

#ifdef X64_BUILD
	if ( format == RGB24) {
		isse_yv12_to_yuy2(ysrc,vsrc,usrc,width,width,width/2,buffer2,width*2,height);
		mmx_YUY2toRGB24(buffer2,out,buffer2+width*height*2,width*2,width*2,1);
	} else if ( format == RGB32 ) {
		isse_yv12_to_yuy2(ysrc,vsrc,usrc,width,width,width/2,buffer2,width*2,height);
		mmx_YUY2toRGB32(buffer2,out,buffer2+width*height*2,width*2,width*2,1);
	} else if ( format == YUY2 ) {
		isse_yv12_to_yuy2(ysrc,vsrc,usrc,width,width,width/2,out,width*2,height);
	}
#else
	if ( format == RGB24) {
		yv12_to_rgb24_mmx(out,width,ysrc,vsrc,usrc,width,width/2,width,-(int)height);
	} else if ( format == RGB32 ) {
		yv12_to_rgb32_mmx(out,width,ysrc,vsrc,usrc,width,width/2,width,-(int)height);
	} else if ( format == YUY2 ) {
		yv12_to_yuyv_mmx(out,width,ysrc,vsrc,usrc,width,width/2,width,height);
	}
#endif

}

// Called to decompress a frame, the actual decompression will be
// handed off to other functions based on the frame type.
DWORD CodecInst::Decompress(ICDECOMPRESS* icinfo, DWORD dwSize) {
//	try {

	DWORD return_code=ICERR_OK;
	if ( started != 0x1337 ){
		DecompressBegin(icinfo->lpbiInput,icinfo->lpbiOutput);
	}
	out = (unsigned char *)icinfo->lpOutput;
	in  = (unsigned char *)icinfo->lpInput; 
	icinfo->lpbiOutput->biSizeImage = length;

	compressed_size = icinfo->lpbiInput->biSizeImage;

	// according to the avi specs, the calling application is responsible for handling null frames.
	if ( compressed_size == 0 ){
		#ifndef LAGARITH_RELEASE
		MessageBox (HWND_DESKTOP, "Received request to decode a null frame", "Error", MB_OK | MB_ICONEXCLAMATION);
		#endif
		return ICERR_OK;
	}
	if ( compressed_size <= 21 ){
		bool solid = false;
		unsigned int r=0,g=0,b=0,a=255;
		if ( (in[0] == ARITH_RGB24 || in[0] == UNALIGNED_RGB24) && compressed_size == 15){
			// solid frame that wasn't caught by old memcmp logic
			b = in[10];
			g = in[12];
			r = in[14];
			b+=g;
			r+=g;
			solid=true;
		} else if ( in[0] == ARITH_ALPHA && compressed_size == 21){
			// solid frame that wasn't caught by old memcmp logic
			b = in[14];
			g = in[16];
			r = in[18];
			a = in[20];
			b+=g;
			r+=g;
			solid=true;
		} else if ( in[0] == BYTEFRAME ){
			b=g=r=in[1];
			solid=true;
		} else if ( in[0] == PIXELFRAME ){
			b=in[1];
			g=in[2];
			r=in[3];
			solid=true;
		} else if ( in[0] == PIXELFRAME_ALPHA ){
			b=in[1];
			g=in[2];
			r=in[3];
			a=in[4];
			solid=true;
		}
		if ( solid ){
			if ( format == RGB24 ){
				SetSolidFrameRGB24(r,g,b);
			} else {
				SetSolidFrameRGB32(r,g,b,a);
			}
			return ICERR_OK;
		}
	}

	// TMPGEnc (and probably some other programs) like to change the floating point
	// precision. This can occasionally produce rounding errors when scaling the probablity
	// ranges to a power of 2, which in turn produces corrupted video. Here the code checks 
	// the FPU control word and sets the precision correctly if needed.

	unsigned int fpuword = _controlfp(0,0);
	if ( !(fpuword & _PC_53) || (fpuword & _MCW_RC) ){
		_controlfp(_PC_53 | _RC_NEAR,_MCW_PC | _MCW_RC);
#ifndef LAGARITH_RELEASE
		static bool firsttime=true;
		if ( firsttime ){
			firsttime=false;
			MessageBox (HWND_DESKTOP, "Floating point control word is not set correctly, correcting it", "Error", MB_OK | MB_ICONEXCLAMATION);
		}
#endif
	}

	/*interlaced=(in[0]&INTERLACED_FLAG)>0;
	if ( interlaced ){
		//char msg[]="Interlaced frame detected\n";
		//MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
		width*=2;
		height/=2;
	}*/


	switch ( in[0] ){
	case ARITH_RGB24:
		ArithRGBDecompress();
		break;
	case ARITH_ALPHA:
		ArithRGBADecompress();
		break;
	case ARITH_YUY2:
		ArithYUY2Decompress();
		break;
	case OBSOLETE_ARITH:
		ArithRGBDecompress();
		break;
	case ARITH_YV12:
		ArithYV12Decompress();
		break;
	case UNALIGNED_RGB24:
		ArithRGBDecompress();
		break;
	case REDUCED_RES:
		ReduceResDecompress();
		break;

	// redundant check just in case size is incorrect
	case BYTEFRAME:
	case PIXELFRAME:
	case PIXELFRAME_ALPHA:
		{
			unsigned int r=0,g=0,b=0,a=255;
			if ( in[0] == BYTEFRAME ){
				b=g=r=in[1];
			} else if ( in[0] == PIXELFRAME ){
				b=in[1];
				g=in[2];
				r=in[3];
			} else if ( in[0] == PIXELFRAME_ALPHA ){
				b=in[1];
				g=in[2];
				r=in[3];
				a=in[4];
			}
			if ( format == RGB24 ){
				SetSolidFrameRGB24(r,g,b);
			} else {
				SetSolidFrameRGB32(r,g,b,a);
			}
		}
		break;

	case UNCOMPRESSED:
		memcpy(out,in+1,length);
		break;
	default:
		#ifndef LAGARITH_RELEASE
		char emsg[128];
		sprintf_s(emsg,128,"Unrecognized frame type: %d",in[0]);
		MessageBox (HWND_DESKTOP, emsg, "Error", MB_OK | MB_ICONEXCLAMATION);
		#endif
		return_code=ICERR_ERROR;
		break;
	}

	if ( !(fpuword & _PC_53) || (fpuword & _MCW_RC)){
		_controlfp(fpuword,_MCW_PC | _MCW_RC);
	}

	return return_code;
//} catch ( ... ){
//	MessageBox (HWND_DESKTOP, "Exception caught in decompress main", "Error", MB_OK | MB_ICONEXCLAMATION);
//	return ICERR_INTERNAL;
//}
}

//MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);