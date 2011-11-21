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

#include "interface.h"
#include "lagarith.h"
#include "prediction.h"
#include "threading.h"
#include <float.h>
#include "convert_lagarith.h"

#ifndef X64_BUILD
#include "convert_yuy2.cpp"
#include "convert_yv12.cpp"
#else
#include "convert.h"
#endif

// initalize the codec for compression
DWORD CodecInst::CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut){
	if ( started  == 0x1337 ){
		CompressEnd();
	}
	started = 0;

	if ( int error = CompressQuery(lpbiIn,lpbiOut) != ICERR_OK ){
		return error;
	}

	width = lpbiIn->biWidth;
	height = lpbiIn->biHeight;

	format = lpbiIn->biBitCount;
	length = width*height*format/8;

	unsigned int buffer_size;
	if ( format < RGB24 ){
		buffer_size = align_round(width,32)*height*format/8+1024;
	} else {
		buffer_size = align_round(width,16)*height*4+1024;
	}
	
	buffer = (unsigned char *)lag_aligned_malloc(buffer, buffer_size,16,"buffer");
	buffer2 = (unsigned char *)lag_aligned_malloc(buffer2, buffer_size,16,"buffer2");
	prev = (unsigned char *)lag_aligned_malloc(prev, buffer_size,16,"prev");

	if ( !buffer || !buffer2 || !prev ){
		lag_aligned_free(buffer,"buffer");
		lag_aligned_free(buffer2,"buffer2");
		lag_aligned_free(prev,"prev");
		return ICERR_MEMORY;
	}

	if ( !cObj.InitCompressBuffers( width*height ) ){
		return ICERR_MEMORY;
	}

	if ( multithreading ){
		int code = InitThreads( true );
		if ( code != ICERR_OK ){
			return code;
		}
	}

	started = 0x1337;

	return ICERR_OK;
}

// get the maximum size a compressed frame will take;
// 105% of image size + 1KB should be plenty even for random static

DWORD CodecInst::CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut){
	return (DWORD)( align_round(lpbiIn->biWidth,16)*lpbiIn->biHeight*lpbiIn->biBitCount/8*1.05 + 1024);
}

// release resources when compression is done

DWORD CodecInst::CompressEnd(){
	if ( started  == 0x1337 ){
		if ( multithreading ){
			EndThreads();
		}

		lag_aligned_free( buffer,"buffer");
		lag_aligned_free( buffer2,"buffer2");
		lag_aligned_free( prev,"prev");
		cObj.FreeCompressBuffers();
	}
	started = 0;
	return ICERR_OK;
}

unsigned int CodecInst::HandleTwoCompressionThreads(unsigned int chan_size){
	
	unsigned int channel_sizes[3];
	channel_sizes[0]=chan_size;

	unsigned int current_size = chan_size+9;
	
	HANDLE events[2];
	events[0]=threads[0].DoneEvent;
	events[1]=threads[1].DoneEvent;

	int pos = WaitForMultipleObjects(2,&events[0],false,INFINITE);
	pos -= WAIT_OBJECT_0;

	ThreadData * finished = &threads[pos];
	ThreadData * running = &threads[!pos];

	((DWORD*)(out+1))[pos] = current_size;
	channel_sizes[pos+1] = finished->size;
	memcpy(out+current_size, (unsigned char*)finished->dest, channel_sizes[pos+1]);
	current_size += finished->size;

	pos = !pos;
	((DWORD*)(out+1))[pos] = current_size;

	WaitForSingleObject(running->DoneEvent,INFINITE);

	finished = running;
	channel_sizes[pos+1] = finished->size;

	memcpy(out+current_size, (unsigned char*)finished->dest, channel_sizes[pos+1]);
	current_size += finished->size;
	
	if ( channel_sizes[0] >= channel_sizes[1] && channel_sizes[0] >= channel_sizes[2]) {
		if ( threads[0].priority != THREAD_PRIORITY_BELOW_NORMAL){
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_BELOW_NORMAL);
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_BELOW_NORMAL);
			threads[0].priority=THREAD_PRIORITY_BELOW_NORMAL;
			threads[1].priority=THREAD_PRIORITY_BELOW_NORMAL;
		}
	} else if ( channel_sizes[1] >= channel_sizes[2] ){
		if ( threads[0].priority != THREAD_PRIORITY_ABOVE_NORMAL ){
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_ABOVE_NORMAL);
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_NORMAL);
			threads[0].priority=THREAD_PRIORITY_ABOVE_NORMAL;
			threads[0].priority=THREAD_PRIORITY_NORMAL;
		}
	} else {
		if ( threads[1].priority != THREAD_PRIORITY_ABOVE_NORMAL ){
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_NORMAL);
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_ABOVE_NORMAL);
			threads[0].priority=THREAD_PRIORITY_NORMAL;
			threads[0].priority=THREAD_PRIORITY_ABOVE_NORMAL;
		}
	}
	return current_size;
}

unsigned int CodecInst::HandleThreeCompressionThreads(unsigned int chan_size){
	
	unsigned int channel_sizes[4];
	channel_sizes[0]=chan_size;

	unsigned int current_size = chan_size+13;
	
	HANDLE events[3];
	events[0]=threads[0].DoneEvent;
	events[1]=threads[1].DoneEvent;
	events[2]=threads[2].DoneEvent;

	int positions[] = {0,1,2};

	for ( int a=3;a>0;a--){
		int pos = WaitForMultipleObjects(a,&events[0],false,INFINITE);
		pos -= WAIT_OBJECT_0;
	
		int thread_num = positions[pos];
		positions[pos]=positions[a-1];
		events[pos] = events[a-1];

		ThreadData * finished = &threads[thread_num];
		
		unsigned int fsize = finished->size;
		((DWORD*)(out+1))[thread_num] = current_size;
		channel_sizes[thread_num+1] = fsize;

		memcpy(out+current_size, (unsigned char*)finished->dest, fsize);
		current_size += fsize;
	}

	if ( channel_sizes[3] >= channel_sizes[1] && channel_sizes[3] >= channel_sizes[2] && channel_sizes[3] >= channel_sizes[0]) {
		if ( threads[2].priority != THREAD_PRIORITY_ABOVE_NORMAL){
			if ( threads[0].priority != THREAD_PRIORITY_NORMAL){
				SetThreadPriority(threads[0].thread,THREAD_PRIORITY_NORMAL);
				threads[0].priority = THREAD_PRIORITY_NORMAL;
			}
			if ( threads[1].priority != THREAD_PRIORITY_NORMAL){
				SetThreadPriority(threads[1].thread,THREAD_PRIORITY_NORMAL);
				threads[1].priority = THREAD_PRIORITY_NORMAL;
			}
			SetThreadPriority(threads[2].thread,THREAD_PRIORITY_ABOVE_NORMAL);
			threads[2].priority = THREAD_PRIORITY_ABOVE_NORMAL;
		}
	} else if ( channel_sizes[0] >= channel_sizes[1] && channel_sizes[0] >= channel_sizes[2]) {
		if ( threads[0].priority != THREAD_PRIORITY_BELOW_NORMAL){
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_BELOW_NORMAL);
			threads[0].priority = THREAD_PRIORITY_BELOW_NORMAL;
		}
		if ( threads[1].priority != THREAD_PRIORITY_BELOW_NORMAL){
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_BELOW_NORMAL);
			threads[1].priority = THREAD_PRIORITY_BELOW_NORMAL;
		}
		if ( threads[2].priority != THREAD_PRIORITY_BELOW_NORMAL){
			SetThreadPriority(threads[2].thread,THREAD_PRIORITY_BELOW_NORMAL);
			threads[2].priority = THREAD_PRIORITY_BELOW_NORMAL;
		}
	} else if ( channel_sizes[1] >= channel_sizes[2] ){
		if ( threads[0].priority != THREAD_PRIORITY_ABOVE_NORMAL){
			if ( threads[2].priority != THREAD_PRIORITY_NORMAL){
				SetThreadPriority(threads[2].thread,THREAD_PRIORITY_NORMAL);
				threads[2].priority = THREAD_PRIORITY_NORMAL;
			}
			if ( threads[1].priority != THREAD_PRIORITY_NORMAL){
				SetThreadPriority(threads[1].thread,THREAD_PRIORITY_NORMAL);
				threads[1].priority = THREAD_PRIORITY_NORMAL;
			}
			SetThreadPriority(threads[0].thread,THREAD_PRIORITY_ABOVE_NORMAL);
			threads[0].priority = THREAD_PRIORITY_ABOVE_NORMAL;
		}
	} else {
		if ( threads[1].priority != THREAD_PRIORITY_ABOVE_NORMAL){
			if ( threads[0].priority != THREAD_PRIORITY_NORMAL){
				SetThreadPriority(threads[0].thread,THREAD_PRIORITY_NORMAL);
				threads[0].priority = THREAD_PRIORITY_NORMAL;
			}
			if ( threads[2].priority != THREAD_PRIORITY_NORMAL){
				SetThreadPriority(threads[2].thread,THREAD_PRIORITY_NORMAL);
				threads[2].priority = THREAD_PRIORITY_NORMAL;
			}
			SetThreadPriority(threads[1].thread,THREAD_PRIORITY_ABOVE_NORMAL);
			threads[1].priority = THREAD_PRIORITY_ABOVE_NORMAL;
		}
	}

	return current_size;
}

// Encode a typical RGB24 frame, input can be RGB24 or RGB32
int CodecInst::CompressRGB24(ICCOMPRESS* icinfo){

	const unsigned char * src=in;
	const unsigned int pixels = width*height;
	const unsigned int block_len=align_round(pixels+32,16);
	unsigned char * bplane=buffer;
	unsigned char * gplane=buffer+block_len;
	unsigned char * rplane=buffer+block_len*2;


	// convert the interleaved channels to planer, aligning if needed
	if ( format == RGB24 ){
		Decorrilate_And_Split_RGB24(in,rplane,gplane,bplane,width,height,&performance);
	} else {
		Decorrilate_And_Split_RGB32(in,rplane,gplane,bplane,width,height,&performance);
	}

	int size=0;
	if ( !multithreading ) { // perform prediction

		unsigned char * bpred = buffer2;
		unsigned char * gpred = buffer2+block_len;
		unsigned char * rpred = buffer2+block_len*2;
		
		Block_Predict(bplane,bpred,width,pixels,true);
		Block_Predict(gplane,gpred,width,pixels,true);
		Block_Predict(rplane,rpred,width,pixels,true);

		size = cObj.Compact(bpred,out+9,pixels); 
		size +=9;

		*(UINT32*)(out+1)=size;
		size += cObj.Compact(gpred,out+size, pixels);

		*(UINT32*)(out+5)=size;
		size += cObj.Compact(rpred,out+size, pixels);

	} else { 
		
		unsigned char * rcomp = prev;
		unsigned char * gcomp = prev+align_round(length/2+128,16);

		threads[0].source=gplane;
		threads[0].dest=gcomp;
		threads[0].length=pixels;
		SetEvent(threads[0].StartEvent);
		threads[1].source=rplane;
		threads[1].dest=rcomp;
		threads[1].length=pixels;
		SetEvent(threads[1].StartEvent);
	
		unsigned char * bpred = buffer2;
		Block_Predict(bplane,bpred,width,block_len,true);

		unsigned int blue_size = cObj.Compact(bpred,out+9,pixels);
		
		size = HandleTwoCompressionThreads(blue_size);

	}

	if ( (width%4) == 0 ){
		out[0]=ARITH_RGB24;
	} else {
		// old versions didn't handle the padding correctly,
		// this tag lets the codec handle the versions differently 
		out[0]=UNALIGNED_RGB24; 
	}

	// minimum size possible, indicates the frame was one solid color
	if ( size == 15 ){
		if ( in[0]==in[1] && in[0] == in[2] ){
			out[0]=BYTEFRAME;
			out[1]=in[0];
			size = 2;
		} else {
			out[0]=PIXELFRAME;
			out[1]=in[0];
			out[2]=in[1];
			out[3]=in[2];
			size = 4;
		}
	}
	
	icinfo->lpbiOutput->biSizeImage = size;
	return ICERR_OK;
}

// Compress an RGBA frame (alpha compression is enabled and the input is RGB32)
int CodecInst::CompressRGBA(ICCOMPRESS* icinfo){

	const unsigned char * src=in;
	const unsigned int pixels = width*height;
	const unsigned int block_len=align_round(pixels+32,16);
	unsigned char * bplane=buffer;
	unsigned char * gplane=buffer+block_len;
	unsigned char * rplane=buffer+block_len*2;
	unsigned char * aplane=buffer+block_len*3;

	// convert the interleaved channels to planer
	Decorrilate_And_Split_RGBA(in,rplane,gplane,bplane,aplane,width,height,&performance);
	
	int size;
	out[0]=ARITH_ALPHA;
	if ( !multithreading ){

		unsigned char * bpred = buffer2;
		unsigned char * gpred = buffer2+block_len;
		unsigned char * rpred = buffer2+block_len*2;
		unsigned char * apred = buffer2+block_len*3;

		Block_Predict(bplane,bpred,width,pixels,true);
		Block_Predict(gplane,gpred,width,pixels,true);
		Block_Predict(rplane,rpred,width,pixels,true);
		Block_Predict(aplane,apred,width,pixels,true);

		size = 13;
		size += cObj.Compact(bpred,out+size,pixels);
		*(int*)(out+1)=size;
		size += cObj.Compact(gpred,out+size, pixels);
		*(int*)(out+5)=size;
		size += cObj.Compact(rpred,out+size, pixels);
		*(int*)(out+9)=size;
		size += cObj.Compact(apred,out+size, pixels);
	} else {

		unsigned char * rcomp = prev;
		unsigned char * bpred = buffer2;
		unsigned char * gcomp = buffer2+align_round(pixels+64,16);
		unsigned char * acomp = prev+align_round(pixels*2+128,16);

		threads[0].source=gplane;
		threads[0].dest=gcomp;
		threads[0].length=pixels;
		SetEvent(threads[0].StartEvent);

		threads[1].source=rplane;
		threads[1].dest=rcomp;
		threads[1].length=pixels;
		SetEvent(threads[1].StartEvent);

		threads[2].source=aplane;
		threads[2].dest=acomp;
		threads[2].length=pixels;
		SetEvent(threads[2].StartEvent);

		Block_Predict(bplane,bpred,width,block_len,true);
		unsigned int size_blue =cObj.Compact(bpred,out+13,pixels);

		size = HandleThreeCompressionThreads(size_blue);
	}

	if ( size == 21 ){
		out[0]=PIXELFRAME_ALPHA;
		out[1]=in[0];
		out[2]=in[1];
		out[3]=in[2];
		out[4]=in[3];
		size = 5;
	}

	icinfo->lpbiOutput->biSizeImage = size;
	return ICERR_OK;
}

// Compress a 4:2:2 YUV frame, input can be YUY2, UYVY, or YV16
int CodecInst::CompressYUV16(ICCOMPRESS* icinfo){

	const unsigned int pixels	= width*height;
	unsigned char * ysrc		= prev;
	unsigned char * usrc		= ysrc+align_round(pixels+16,16);
	unsigned char * vsrc		= usrc+align_round(pixels/2+32,16);
	unsigned char * ydest		= buffer2;
	unsigned char * udest		= ydest+align_round(pixels+16,16);
	unsigned char * vdest		= udest+align_round(pixels/2+32,16);

	const unsigned char * y;
	const unsigned char * u;
	const unsigned char * v;

	if ( icinfo->lpbiInput->biCompression == FOURCC_YV16 ){
		y = in;
		v = in+pixels;
		u = in+pixels+pixels/2;

		ydest += ((int)y)&15;
		udest += ((int)u)&15;
		vdest += ((int)v)&15;
	} else if ( lossy_option ){
		y = in;
		u = in + align_round(width*height+16,16);
		v = in + align_round(width*height*3/2+32,16);
	} else {
		if ( icinfo->lpbiInput->biCompression == FOURCC_YUY2  ){
			Split_YUY2(in,ysrc,usrc,vsrc,width,height,&performance);
		} else {
			Split_UYVY(in,ysrc,usrc,vsrc,width,height,&performance);
		}
		y = ysrc;
		u = usrc;
		v = vsrc;
	}

	int size;
	if ( !multithreading ){

		Block_Predict_YUV16(y,ydest,width,pixels,true);
		Block_Predict_YUV16(u,udest,width/2,pixels/2,false);
		Block_Predict_YUV16(v,vdest,width/2,pixels/2,false);

		size = 9;
		size += cObj.Compact(ydest,out+size,pixels);
		*(UINT32*)(out+1)=size;
		size += cObj.Compact(udest,out+size,pixels/2);
		*(UINT32*)(out+5)=size;
		size += cObj.Compact(vdest,out+size,pixels/2);
	} else {

		unsigned char * ucomp = buffer;
		unsigned char * vcomp = buffer+align_round(pixels+64,16);

		threads[0].source=u;
		threads[0].dest=ucomp;
		threads[0].length=pixels/2;
		SetEvent(threads[0].StartEvent);
		threads[1].source=v;
		threads[1].dest=vcomp;
		threads[1].length=pixels/2;
		SetEvent(threads[1].StartEvent);

		Block_Predict_YUV16(y,ydest,width,pixels,true);

		unsigned int y_size = cObj.Compact(ydest,out+9,pixels);
		size = HandleTwoCompressionThreads(y_size);
	}
	out[0] = ARITH_YUY2;
	icinfo->lpbiOutput->biSizeImage = size;
	//assert( *(__int64*)(out+9) != 0 );
	assert( *(__int64*)(out+*(UINT32*)(out+1)) != 0 );
	assert( *(__int64*)(out+*(UINT32*)(out+5)) != 0 );
	return ICERR_OK;
}

int CodecInst::CompressYV12(ICCOMPRESS* icinfo){

	const unsigned int pixels	= width*height;

	const unsigned char * ysrc = in;
	const unsigned char * vsrc = in+pixels;
	const unsigned char * usrc = in+pixels+pixels/4;
	if ( lossy_option ){
		usrc = ysrc + align_round(width*height+16,16);
		vsrc = ysrc + align_round(width*height*3/2+32,16);
	}


	int size;
	if ( !multithreading ){

		unsigned char * ydest = buffer;
		unsigned char * vdest = buffer + align_round(pixels+32,16);
		unsigned char * udest = buffer + align_round(pixels+pixels/4+64,16);
		ydest += (unsigned int)ysrc&15;
		vdest += (unsigned int)vsrc&15;
		udest += (unsigned int)usrc&15;

		// perform prediction
		Block_Predict(ysrc,ydest,width,pixels,false);
		Block_Predict(vsrc,vdest,width/2,pixels/4,false);
		Block_Predict(usrc,udest,width/2,pixels/4,false);

		size = 9;
		size+=cObj.Compact(ydest,out+size,pixels);
		*(UINT32*)(out+1)=size;
		size+=cObj.Compact(vdest,out+size,pixels/4);
		*(UINT32*)(out+5)=size;
		size+=cObj.Compact(udest,out+size,pixels/4);
	} else {

		unsigned char * ucomp = buffer2;
		unsigned char * vcomp = buffer2+align_round(pixels/2+64,16);

		threads[0].source=vsrc;
		threads[0].dest=vcomp;
		threads[0].length=pixels/4;
		SetEvent(threads[0].StartEvent);
		threads[1].source=usrc;
		threads[1].dest=ucomp;
		threads[1].length=pixels/4;
		SetEvent(threads[1].StartEvent);

		unsigned char * ydest = buffer;
		ydest += (unsigned int)ysrc&15;

		Block_Predict(ysrc,ydest,width,pixels,false);

		unsigned int y_size = cObj.Compact(ydest,out+9,pixels);
		size = HandleTwoCompressionThreads(y_size);
	}
	out[0] = ARITH_YV12;
	icinfo->lpbiOutput->biSizeImage = size;
	return ICERR_OK;
}

// This downsamples the colorspace if the set encoding colorspace is lower than the
// colorspace of the input video
int CodecInst::CompressLossy(ICCOMPRESS * icinfo ){

	unsigned char * lossy_buffer=prev;

	const unsigned char * const stored_in=in;

	// separate channels slightly so packed writes cannot overwright following channel
	unsigned int upos = align_round(width*height+16,16);
	unsigned int vpos = align_round(width*height*3/2+32,16);

#ifndef X64_BUILD
	if ( SSE2 ){
#endif
		if ( lossy_option == 2 ){
			if ( format == RGB24 ){
				ConvertRGB24toYV12_SSE2(in,lossy_buffer,lossy_buffer+upos,lossy_buffer+vpos,width,height);
			} else if ( format == RGB32 ){
				ConvertRGB32toYV12_SSE2(in,lossy_buffer,lossy_buffer+upos,lossy_buffer+vpos,width,height);
			} else {
				isse_yuy2_to_yv12(in,width*2,width*2,lossy_buffer,lossy_buffer+upos,lossy_buffer+vpos,width,width/2,height);
			}
		} else {
			if ( format == RGB24 ){
				ConvertRGB24toYV16_SSE2(in,lossy_buffer,lossy_buffer+upos,lossy_buffer+vpos,width,height);
			} else {
				ConvertRGB32toYV16_SSE2(in,lossy_buffer,lossy_buffer+upos,lossy_buffer+vpos,width,height);
			}
		}
#ifndef X64_BUILD
	} else {
		if ( lossy_option == 2 ){
			if ( format == RGB24 ){
				mmx_ConvertRGB24toYUY2(in,buffer,width*3,width*2,width,height);
			} else if ( format == RGB32 ){
				mmx_ConvertRGB32toYUY2(in,buffer,width*4,width*2,width,height);
			}
			const unsigned char * src = (format>=RGB24)?buffer:in;
			if ( SSE ){
				isse_yuy2_to_yv12(src,width*2,width*2,lossy_buffer,lossy_buffer+upos,lossy_buffer+vpos,width,width/2,height);
			} else {
				mmx_yuy2_to_yv12(src,width*2,width*2,lossy_buffer,lossy_buffer+upos,lossy_buffer+vpos,width,width/2,height);
			}
		} else {
			if ( format == RGB24 ){
				mmx_ConvertRGB24toYUY2(in,buffer,width*3,width*2,width,height);
			} else if ( format == RGB32 ){
				mmx_ConvertRGB32toYUY2(in,buffer,width*4,width*2,width,height);
			}
			Split_YUY2(buffer,lossy_buffer,lossy_buffer+upos,lossy_buffer+vpos,width,height,&performance);
		}
	}
#endif
	in=lossy_buffer;

	DWORD ret_val;
	if ( lossy_option == 2 ){
		ret_val = CompressYV12(icinfo);
	} else {
		ret_val = CompressYUV16(icinfo);
	}
	in=stored_in;
	return ret_val;
}

// called to compress a frame; the actual compression will be
// handed off to other functions depending on the color space and settings

DWORD CodecInst::Compress(ICCOMPRESS* icinfo, DWORD dwSize) {

	out = (unsigned char *)icinfo->lpOutput;
	in  = (unsigned char *)icinfo->lpInput;

	if ( icinfo->lFrameNum == 0 ){
		if ( started != 0x1337 ){
			if ( int error = CompressBegin(icinfo->lpbiInput,icinfo->lpbiOutput) != ICERR_OK )
				return error;
		}
	} else if ( nullframes ){
		// compare in two parts, video is probably more likely to change in middle than at bottom
		unsigned int pos = length/2+15;
		pos&=(~15);
		if (!memcmp(in+pos,prev+pos,length-pos) && !memcmp(in,prev,pos) ){
			icinfo->lpbiOutput->biSizeImage =0;
			*icinfo->lpdwFlags = 0;
			return ICERR_OK;
		}
	}
	if ( icinfo->lpckid ){
		*icinfo->lpckid = 'cd';
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
	
	int ret_val;

	if ( lossy_option ){
		ret_val = CompressLossy(icinfo);
	} else if ( format == RGB24 || ( format == RGB32 && !use_alpha) ){
		ret_val = CompressRGB24(icinfo);

	/*
	{
		unsigned char * compare = (unsigned char *)malloc(width*height*4+1024);
		const unsigned char * const holdin = in;
		unsigned char * holdout = out;
		in = out;
		out = compare;
		ArithDecompress();
		for ( int a=0;a<width*height*format/8;a++){
			if ( holdin[a] != compare[a] ){
				__asm int 3;
			}
		}

		free(compare);
		in = holdin;
		out = holdout;
	}*/

	} else if ( format == YUY2 ){
		ret_val = CompressYUV16(icinfo);
	} else if ( format == YV12 ){
		ret_val = CompressYV12(icinfo);
	} else {
		ret_val = CompressRGBA(icinfo);
	}

	if ( nullframes ){
		memcpy( prev,in, length);
	}

	if (ret_val == ICERR_OK ){
		*icinfo->lpdwFlags = AVIIF_KEYFRAME;
	}

	if ( !(fpuword & _PC_53) || (fpuword & _MCW_RC)){
		_controlfp(fpuword,_MCW_PC | _MCW_RC);
	}

	return (DWORD)ret_val;
}

//MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);