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

//This file contains functions that perform and undo median predition that are used by both x86 and x64
#define WIN32_LEAN_AND_MEAN
#include "prediction.h"
#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <windows.h>
#include <stdio.h>

bool SSSE3;

// Round x up to the next multiple of y if it is not a multiple. Y must be a power of 2.
#define align_round(x,y) ((((unsigned int)(x))+(y-1))&(~(y-1)))

// this effectively performs a bubble sort to select the median:
// min(max(min(x,y),z),max(x,y))
// this assumes 32 bit ints, and that x and y are >=0 and <= 0x80000000
inline int median(int x,int y,int z ) {
	int delta = x-y;
	delta &= delta>>31;
	y+=delta; // min
	x-=delta; // max
	delta = y-z;
	delta &= delta>>31;
	y -=delta;	// max
	delta = y-x;
	delta &= delta>>31;
	return  x+delta;	// min
}

#ifndef X64_BUILD
unsigned __int64 GetTime(){
	unsigned __int64 tsc=0;
	__asm{
		lea		ebx,tsc
		rdtsc
		mov		[ebx],eax
		mov		[ebx+4],edx

	}
	return tsc;
}

#include "prediction_x86.cpp"
#endif

void Block_Predict_SSE2( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool rgbmode){

	//unsigned __int64 t1,t2;
	//t1 = GetTime();
	// 4.9

	unsigned int align_shift = (16 - ((unsigned int)source&15))&15;

	// predict the bottom row
	unsigned int a;
	__m128i t0 = _mm_setzero_si128();
	if ( align_shift ){
		dest[0]=source[0];
		for ( a=1;a<align_shift;a++){
			dest[a] = source[a]-source[a-1];
		}
		t0 = _mm_cvtsi32_si128(source[align_shift-1]);
	}
	for ( a=align_shift;a<width;a+=16){
		__m128i x = *(__m128i*)&source[a];
		t0 = _mm_or_si128(t0,_mm_slli_si128(x,1));
		*(__m128i*)&dest[a]=_mm_sub_epi8(x,t0);
		t0 = _mm_srli_si128(x,15);
	}

	if ( width>=length )
		return;

	__m128i z;
	__m128i x;

	if ( rgbmode ){
		x = _mm_setzero_si128();
		z = _mm_setzero_si128();
	} else {
		x = _mm_cvtsi32_si128(source[width-1]);
		z = _mm_cvtsi32_si128(source[0]);
	}

	a = width;
	{
		// this block makes sure that source[a] is aligned to 16
		__m128i srcs = _mm_loadu_si128((__m128i *)&source[a]);
		__m128i y = _mm_loadu_si128((__m128i *)&source[a-width]);

		x = _mm_or_si128(x,_mm_slli_si128(srcs,1));
		z = _mm_or_si128(z,_mm_slli_si128(y,1));

		__m128i tx = _mm_unpackhi_epi8(x,_mm_setzero_si128());
		__m128i ty = _mm_unpackhi_epi8(y,_mm_setzero_si128());
		__m128i tz = _mm_unpackhi_epi8(z,_mm_setzero_si128());

		tz = _mm_add_epi16(_mm_sub_epi16(tx,tz),ty);

		tx = _mm_unpacklo_epi8(x,_mm_setzero_si128());
		ty = _mm_unpacklo_epi8(y,_mm_setzero_si128());
		z = _mm_unpacklo_epi8(z,_mm_setzero_si128());
		z = _mm_add_epi16(_mm_sub_epi16(tx,z),ty);
		z = _mm_packus_epi16(z,tz);

		__m128i i = _mm_min_epu8(x,y);
		x = _mm_max_epu8(x,y);
		i = _mm_max_epu8(i,z);
		x = _mm_min_epu8(x,i);

		srcs = _mm_sub_epi8(srcs,x);
		_mm_storeu_si128((__m128i*)&dest[a],srcs);
	}
	
	if ( align_shift ){
		a = align_round(a,16);
		a += align_shift;
		if ( a > width+16 ){
			a -= 16;
		}
	} else {
		a+=16;
		a&=(~15);
	}
	// source[a] is now aligned
	x = _mm_cvtsi32_si128(source[a-1]);
	z = _mm_cvtsi32_si128(source[a-width-1]);

	const unsigned int end = (length>=15)?length-15:0;

	// if width is a multiple of 16, use faster aligned reads
	// inside the prediction loop
	if ( width%16 == 0 ){
		for ( ;a<end;a+=16){

			__m128i srcs = *(__m128i *)&source[a];
			__m128i y = *(__m128i *)&source[a-width];

			x = _mm_or_si128(x,_mm_slli_si128(srcs,1));
			z = _mm_or_si128(z,_mm_slli_si128(y,1));

			__m128i tx = _mm_unpackhi_epi8(x,_mm_setzero_si128());
			__m128i ty = _mm_unpackhi_epi8(y,_mm_setzero_si128());
			__m128i tz = _mm_unpackhi_epi8(z,_mm_setzero_si128());

			tz = _mm_add_epi16(_mm_sub_epi16(tx,tz),ty);

			tx = _mm_unpacklo_epi8(x,_mm_setzero_si128());
			ty = _mm_unpacklo_epi8(y,_mm_setzero_si128());
			z = _mm_unpacklo_epi8(z,_mm_setzero_si128());
			z = _mm_add_epi16(_mm_sub_epi16(tx,z),ty);
			z = _mm_packus_epi16(z,tz);

			__m128i i = _mm_min_epu8(x,y);
			x = _mm_max_epu8(x,y);
			i = _mm_max_epu8(i,z);
			x = _mm_min_epu8(x,i);

			i = _mm_srli_si128(srcs,15);

			srcs = _mm_sub_epi8(srcs,x);

			z = _mm_srli_si128(y,15);
			x = i;

			*(__m128i*)&dest[a] = srcs;
		}
	} else {
		// main prediction loop, source[a-width] is unaligned
		for ( ;a<end;a+=16){

			__m128i srcs = *(__m128i *)&source[a];
			__m128i y = _mm_loadu_si128((__m128i *)&source[a-width]); // only change from aligned version

			x = _mm_or_si128(x,_mm_slli_si128(srcs,1));
			z = _mm_or_si128(z,_mm_slli_si128(y,1));

			__m128i tx = _mm_unpackhi_epi8(x,_mm_setzero_si128());
			__m128i ty = _mm_unpackhi_epi8(y,_mm_setzero_si128());
			__m128i tz = _mm_unpackhi_epi8(z,_mm_setzero_si128());

			tz = _mm_add_epi16(_mm_sub_epi16(tx,tz),ty);

			tx = _mm_unpacklo_epi8(x,_mm_setzero_si128());
			ty = _mm_unpacklo_epi8(y,_mm_setzero_si128());
			z = _mm_unpacklo_epi8(z,_mm_setzero_si128());
			z = _mm_add_epi16(_mm_sub_epi16(tx,z),ty);
			z = _mm_packus_epi16(z,tz);

			__m128i i = _mm_min_epu8(x,y);
			x = _mm_max_epu8(x,y);
			i = _mm_max_epu8(i,z);
			x = _mm_min_epu8(x,i);

			i = _mm_srli_si128(srcs,15);

			srcs = _mm_sub_epi8(srcs,x);

			z = _mm_srli_si128(y,15);
			x = i;

			*(__m128i*)&dest[a] = srcs;
		}
	}
	for ( ; a<length; a++ ){
		int x = source[a-1];
		int y = source[a-width];
		int z = x+y-source[a-width-1];
		dest[a] = source[a]-median(x,y,z);
	}

	/*t2 = GetTime();
	t2 -= t1;
	static HANDLE file = NULL;
	char msg[128];
	if ( !file ){
		file =  CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		sprintf(msg,"%f,=median(A1:A1000)\n",t2/100000.0);
	} else {
		sprintf(msg,"%f\n",t2/100000.0);
	}
	unsigned long bytes;
	WriteFile(file,msg,strlen(msg),&bytes,NULL);*/
}

void Block_Predict_YUV16_SSE2( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool is_y){

	unsigned int align_shift = (16 - ((int)source&15))&15;

	// predict the bottom row
	unsigned int a;
	__m128i t0 = _mm_setzero_si128();
	if ( align_shift ){
		dest[0]=source[0];
		for ( a=1;a<align_shift;a++){
			dest[a] = source[a]-source[a-1];
		}
		t0 = _mm_cvtsi32_si128(source[align_shift-1]);
	}
	for ( a=align_shift;a<width;a+=16){
		__m128i x = *(__m128i*)&source[a];
		t0 = _mm_or_si128(t0,_mm_slli_si128(x,1));
		*(__m128i*)&dest[a]=_mm_sub_epi8(x,t0);
		t0 = _mm_srli_si128(x,15);
	}
	if ( width>=length )
		return;

	__m128i z;
	__m128i x;

	const __m128i zero = _mm_setzero_si128();
	a = width;
	{
		// this block makes sure that source[a] is aligned to 16
		__m128i srcs = _mm_loadu_si128((__m128i *)&source[a]);
		__m128i y = _mm_loadu_si128((__m128i *)&source[a-width]);

		x = _mm_slli_si128(srcs,1);
		z = _mm_slli_si128(y,1);
		z = _mm_add_epi8(_mm_sub_epi8(x,z),y);

		__m128i i = _mm_min_epu8(x,y);
		x = _mm_max_epu8(x,y);
		i = _mm_max_epu8(i,z);
		x = _mm_min_epu8(x,i);

		srcs = _mm_sub_epi8(srcs,x);
		_mm_storeu_si128((__m128i*)&dest[a],srcs);
	}

	if ( align_shift ){
		a = align_round(a,16);
		a += align_shift;
		if ( a > width+16 ){
			a -= 16;
		}
	} else {
		a+=16;
		a&=(~15);
	}
	// source[a] is now aligned
	x = _mm_cvtsi32_si128(source[a-1]);
	z = _mm_cvtsi32_si128(source[a-width-1]);


	const unsigned int end = length&(~15);
	// if width is a multiple of 16, use faster aligned reads
	// inside the prediction loop
	if ( width%16 == 0 ){
		for ( ;a<end;a+=16){

			__m128i srcs = *(__m128i *)&source[a];
			__m128i y = *(__m128i *)&source[a-width];

			x = _mm_or_si128(x,_mm_slli_si128(srcs,1));
			z = _mm_or_si128(z,_mm_slli_si128(y,1));
			z = _mm_add_epi8(_mm_sub_epi8(x,z),y);

			__m128i i = _mm_min_epu8(x,y);
			x = _mm_max_epu8(x,y);
			i = _mm_max_epu8(i,z);
			x = _mm_min_epu8(x,i);

			i = _mm_srli_si128(srcs,15);

			srcs = _mm_sub_epi8(srcs,x);

			z = _mm_srli_si128(y,15);
			x = i;

			*(__m128i*)&dest[a] = srcs;
		}
	} else {
		// main prediction loop, source[a-width] is unaligned
		for ( ;a<end;a+=16){

			__m128i srcs = *(__m128i *)&source[a];
			__m128i y = _mm_loadu_si128((__m128i *)&source[a-width]); // only change from aligned version

			x = _mm_or_si128(x,_mm_slli_si128(srcs,1));
			z = _mm_or_si128(z,_mm_slli_si128(y,1));
			z = _mm_add_epi8(_mm_sub_epi8(x,z),y);

			__m128i i = _mm_min_epu8(x,y);
			x = _mm_max_epu8(x,y);
			i = _mm_max_epu8(i,z);
			x = _mm_min_epu8(x,i);

			i = _mm_srli_si128(srcs,15);

			srcs = _mm_sub_epi8(srcs,x);

			z = _mm_srli_si128(y,15);
			x = i;

			*(__m128i*)&dest[a] = srcs;
		}
	}
	for ( ; a<length; a++ ){
		int x = source[a-1];
		int y = source[a-width];
		int z = (x+y-source[a-width-1])&255;
		dest[a] = source[a]-median(x,y,z);
	}

	if ( is_y ){
		dest[1]=source[1];
	}
	dest[width] = source[width]-source[width-1];
	dest[width+1] = source[width+1]-source[width];

	if ( is_y ){
		dest[width+2] = source[width+2]-source[width+1];
		dest[width+3] = source[width+3]-source[width+2];
	}
}

void Decorrilate_And_Split_RGB24_SSE2(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height){
	

	//unsigned __int64 t1,t2;
	//static HANDLE file = NULL;
	//t1 = GetTime();

	const unsigned int stride = align_round(width*3,4);
	if ( SSSE3 ){
		// 10
		const unsigned int vsteps = (width&7)?height:1;
		const unsigned int wsteps = (width&7)?(width&(~7)):width*height;

		const __m128i shuffle = _mm_setr_epi8(0,3,6,9, 2,5,8,11, 1,4,7,10, -1,-1,-1,-1); // bbbb rrrr gggg 0000
		//__asm int 3;
		for ( unsigned int y=0; y<vsteps; y++){
			unsigned int x;
			for ( x=0; x<wsteps; x+=8){
				__m128i s0 = _mm_lddqu_si128((__m128i*)&in[y*stride+x*3+0]);
				__m128i s1 = _mm_loadl_epi64((__m128i*)&in[y*stride+x*3+16]);
				s1 = _mm_alignr_epi8(s1,s0,12);
				s0 = _mm_shuffle_epi8(s0,shuffle);
				s1 = _mm_shuffle_epi8(s1,shuffle);
				
				__m128i g = _mm_unpackhi_epi32(s0,s1);
				__m128i br = _mm_unpacklo_epi32(s0,s1);
				g = _mm_unpacklo_epi64(g,g);
				br = _mm_sub_epi8(br,g);


				_mm_storel_epi64((__m128i*)&bdst[y*width+x],br);
				_mm_storel_epi64((__m128i*)&gdst[y*width+x],g);
				_mm_storel_epi64((__m128i*)&rdst[y*width+x],_mm_srli_si128(br,8));
			}
			for ( ;x < width; x++ ){
				bdst[y*width+x] = in[y*stride+x*3+0]-in[y*stride+x*3+1];
				gdst[y*width+x] = in[y*stride+x*3+1];
				rdst[y*width+x] = in[y*stride+x*3+2]-in[y*stride+x*3+1];
			}
		}
	} else {
		// 25
		// 23
		const unsigned int vsteps = (width&3)?height:1;
		const unsigned int wsteps = (width&3)?(width&(~3)):width*height;

		__m128i mask = _mm_set1_epi16(255);
		for ( unsigned int y=0; y<vsteps; y++){
			unsigned int x;
			for ( x=0; x<wsteps; x+=4){
				__m128i x0 = _mm_loadl_epi64((__m128i*)&in[y*stride+x*3+0]);
				__m128i x1 = _mm_srli_si128(x0,3);
				__m128i x3 = _mm_cvtsi32_si128(*(int*)&in[y*stride+x*3+8]);
				__m128i x2 = _mm_unpacklo_epi64(x0,x3);
				x3 = _mm_srli_si128(x3,1);
				x2 = _mm_srli_si128(x2,6);

				x0 = _mm_unpacklo_epi32(x0,x1);
				x2 = _mm_unpacklo_epi32(x2,x3);
				x0 = _mm_unpacklo_epi64(x0,x2);

				x0 = _mm_shufflelo_epi16(x0,(0<<0)+(2<<2)+(1<<4)+(3<<6));
				x0 = _mm_shufflehi_epi16(x0,(0<<0)+(2<<2)+(1<<4)+(3<<6));
				x0 = _mm_shuffle_epi32(x0,(0<<0)+(2<<2)+(1<<4)+(3<<6));

				__m128i g = _mm_srli_epi16(x0,8);
				x0 = _mm_and_si128(x0,mask);
				x0 = _mm_packus_epi16(x0,g);
				g = _mm_shuffle_epi32(x0,(2<<0)+(2<<2)+(2<<4)+(2<<6));
				x0 = _mm_sub_epi8(x0,g);


				*(int*)&bdst[y*width+x] = _mm_cvtsi128_si32(x0);
				*(int*)&gdst[y*width+x] = _mm_cvtsi128_si32(g);
				*(int*)&rdst[y*width+x] = _mm_cvtsi128_si32(_mm_srli_si128(x0,4));
			}
			for ( ;x < width; x++ ){
				bdst[y*width+x] = in[y*stride+x*3+0]-in[y*stride+x*3+1];
				gdst[y*width+x] = in[y*stride+x*3+1];
				rdst[y*width+x] = in[y*stride+x*3+2]-in[y*stride+x*3+1];
			}
		}
	}

	/*t2 = GetTime();
	t2 -= t1;
	char msg[128];
	if ( !file ){
		file =  CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		sprintf(msg,"%f,=median(A1:A1000)\n",t2/100000.0);
	} else {
		sprintf(msg,"%f\n",t2/100000.0);
	}
	unsigned long bytes;
	WriteFile(file,msg,strlen(msg),&bytes,NULL);*/
}

void Decorrilate_And_Split_RGB32_SSE2(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height){

	//unsigned __int64 t1,t2;
	//t1 = GetTime();

	// 13
	// 12

	unsigned int a=0;
	unsigned int align = (unsigned int)in;
	align &= 15;
	align /=4;

	for ( a=0;a < align; a++ ){
		bdst[a] = in[a*4+0]-in[a*4+1];
		gdst[a] = in[a*4+1];
		rdst[a] = in[a*4+2]-in[a*4+1];
	}

	const unsigned int end = (width*height-align)&(~7);
	const __m128i mask = _mm_set1_epi16(255);
	for ( ; a<end; a+=8){
		
		__m128i x0 = *(__m128i*)&in[a*4+0];
		__m128i x2 = *(__m128i*)&in[a*4+16];
		__m128i x1 = x0;
		__m128i x3 = x2;
		x0 = _mm_srli_epi16(x0,8); // ga
		x1 = _mm_and_si128(x1,mask); // br
		x2 = _mm_srli_epi16(x2,8);
		x3 = _mm_and_si128(x3,mask);

		x0 = _mm_packus_epi16(x0,x2);
		x1 = _mm_packus_epi16(x1,x3);
		x2 = x0;
		x3 = x1;

		__m128i g = _mm_and_si128(x2,mask);
		__m128i r = _mm_srli_epi16(x1,8);
		__m128i b = _mm_and_si128(x3,mask);
		
		b = _mm_packus_epi16(b,r);
		g = _mm_packus_epi16(g,g);
		b = _mm_sub_epi8(b,g);


		_mm_storel_epi64((__m128i*)&bdst[a],b);
		_mm_storel_epi64((__m128i*)&gdst[a],g);
		_mm_storel_epi64((__m128i*)&rdst[a],_mm_srli_si128(b,8));
		
	}
	for ( ;a < width*height; a++ ){
		bdst[a] = in[a*4+0]-in[a*4+1];
		gdst[a] = in[a*4+1];
		rdst[a] = in[a*4+2]-in[a*4+1];
	}

	/*t2 = GetTime();
	t2 -= t1;
	char msg[128];
	static HANDLE file = NULL;
	if ( !file ){
		file =  CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		sprintf(msg,"%f,=median(A1:A1000)\n",t2/100000.0);
	} else {
		sprintf(msg,"%f\n",t2/100000.0);
	}
	unsigned long bytes;
	WriteFile(file,msg,strlen(msg),&bytes,NULL);*/
}

void Decorrilate_And_Split_RGBA_SSE2(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, unsigned char * __restrict adst, const unsigned int width, const unsigned int height){
	
	//unsigned __int64 t1,t2;
	//t1 = GetTime();

	// 15

	const __m128i mask = _mm_set1_epi16(255);
	
	unsigned int a=0;
	unsigned int align = (unsigned int)in;
	align &= 15;
	align /= 4;

	for ( a=0;a < align; a++ ){
		bdst[a] = in[a*4+0]-in[a*4+1];
		gdst[a] = in[a*4+1];
		rdst[a] = in[a*4+2]-in[a*4+1];
		adst[a] = in[a*4+3];
	}

	const unsigned int end = (width*height-align)&(~7);
	for ( ; a<end; a+=8){
		
		__m128i x0 = *(__m128i*)&in[a*4+0];
		__m128i x2 = *(__m128i*)&in[a*4+16];
		__m128i x1 = x0;
		__m128i x3 = x2;
		x0 = _mm_srli_epi16(x0,8); // ga
		x1 = _mm_and_si128(x1,mask); // br
		x2 = _mm_srli_epi16(x2,8);
		x3 = _mm_and_si128(x3,mask);

		x0 = _mm_packus_epi16(x0,x2);
		x1 = _mm_packus_epi16(x1,x3);
		x2 = x0;
		x3 = x1;

		__m128i g = _mm_and_si128(x2,mask);
		__m128i alpha = _mm_srli_epi16(x0,8);
		__m128i r = _mm_srli_epi16(x1,8);
		__m128i b = _mm_and_si128(x3,mask);

		b = _mm_sub_epi8(b,g);
		r = _mm_sub_epi8(r,g);
		
		b = _mm_packus_epi16(b,r);
		g = _mm_packus_epi16(g,alpha);


		_mm_storel_epi64((__m128i*)&bdst[a],b);
		_mm_storel_epi64((__m128i*)&gdst[a],g);
		_mm_storel_epi64((__m128i*)&rdst[a],_mm_srli_si128(b,8));
		_mm_storel_epi64((__m128i*)&adst[a],_mm_srli_si128(g,8));
		
	}

	for ( ;a < width*height; a++ ){
		bdst[a] = in[a*4+0]-in[a*4+1];
		gdst[a] = in[a*4+1];
		rdst[a] = in[a*4+2]-in[a*4+1];
		adst[a] = in[a*4+3];
	}

	/*t2 = GetTime();
	t2 -= t1;
	char msg[128];
	static HANDLE file = NULL;
	if ( !file ){
		file =  CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		sprintf(msg,"%f,=median(A1:A1000)\n",t2/100000.0);
	} else {
		sprintf(msg,"%f\n",t2/100000.0);
	}
	unsigned long bytes;
	WriteFile(file,msg,strlen(msg),&bytes,NULL);*/
}

void Split_YUY2_SSE2(const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height){
	//unsigned __int64 t1,t2;
	//t1 = GetTime();
	const __m128i ymask = _mm_set1_epi32(0x00FF00FF);
	const __m128i umask = _mm_set1_epi32(0x0000FF00);

	// 5.8

	unsigned int a=0;
	unsigned int align = (unsigned int)src;
	align &= 15;
	align /= 4;

	for ( ;a<align;a+=2){
		ydst[a+0] = src[a*2+0];
		vdst[a/2] = src[a*2+1];
		ydst[a+1] = src[a*2+2];
		udst[a/2] = src[a*2+3];
	}

	const unsigned int end = (width*height-align)&(~15);
	for ( ;a<end;a+=16){
		__m128i y0 = *(__m128i*)&src[a*2+0];
		__m128i y1 = *(__m128i*)&src[a*2+16];

		__m128i u0 = _mm_srli_epi32(_mm_and_si128(y0,umask),8);
		__m128i v0 = _mm_srli_epi32(y0,24);
		y0 = _mm_and_si128(y0,ymask);

		__m128i u1 = _mm_srli_epi32(_mm_and_si128(y1,umask),8);
		__m128i v1 = _mm_srli_epi32(y1,24);
		y1 = _mm_and_si128(y1,ymask);

		y0 = _mm_packus_epi16(y0,y1);
		v0 = _mm_packus_epi16(v0,v1);
		u0 = _mm_packus_epi16(u0,u1);
		u0 = _mm_packus_epi16(u0,v0);

		*(__m128i*)&ydst[a] = y0;
		_mm_storel_epi64((__m128i*)&udst[a/2],u0);
		_mm_storel_epi64((__m128i*)&vdst[a/2],_mm_srli_si128(u0,8));
	}

	for ( ;a<height*width;a+=2){
		ydst[a+0] = src[a*2+0];
		vdst[a/2] = src[a*2+1];
		ydst[a+1] = src[a*2+2];
		udst[a/2] = src[a*2+3];
	}

	/*t2 = GetTime();
	t2 -= t1;
	char msg[128];
	static HANDLE file = NULL;
	if ( !file ){
		file =  CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		sprintf(msg,"%f,=median(A1:A1000)\n",t2/100000.0);
	} else {
		sprintf(msg,"%f\n",t2/100000.0);
	}
	unsigned long bytes;
	WriteFile(file,msg,strlen(msg),&bytes,NULL);*/
}

void Split_UYVY_SSE2(const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height){
	

	unsigned int a=0;
	unsigned int align = (unsigned int)src;
	align &= 15;
	align /= 4;

	for ( ;a<align;a+=2){
		vdst[a/2] = src[a*2+0];
		ydst[a+0] = src[a*2+1];
		udst[a/2] = src[a*2+2];
		ydst[a+1] = src[a*2+3];
	}

	const __m128i cmask = _mm_set_epi32(0x000000FF,0x000000FF,0x000000FF,0x000000FF);
	const unsigned int end = (width*height-align)&(~15);
	for ( ;a<end;a+=16){
		__m128i u0 = *(__m128i*)&src[a*2+0];
		__m128i u1 = *(__m128i*)&src[a*2+16];

		__m128i y0 = _mm_srli_epi16(u0,8);
		__m128i v0 = _mm_and_si128(_mm_srli_si128(u0,2),cmask);
		u0 = _mm_and_si128(u0,cmask);

		__m128i y1 = _mm_srli_epi16(u1,8);
		__m128i v1 = _mm_and_si128(_mm_srli_si128(u1,2),cmask);
		u1 = _mm_and_si128(u1,cmask);

		y0 = _mm_packus_epi16(y0,y1);
		v0 = _mm_packus_epi16(v0,v1);
		u0 = _mm_packus_epi16(u0,u1);
		u0 = _mm_packus_epi16(u0,v0);

		*(__m128i*)&ydst[a] = y0;
		_mm_storel_epi64((__m128i*)&udst[a/2],u0);
		_mm_storel_epi64((__m128i*)&vdst[a/2],_mm_srli_si128(u0,8));
	}
	
	for ( ;a<height*width;a+=2){
		vdst[a/2] = src[a*2+0];
		ydst[a+0] = src[a*2+1];
		udst[a/2] = src[a*2+2];
		ydst[a+1] = src[a*2+3];
	}
}


void Interleave_And_Restore_YUY2_SSE2( unsigned char * __restrict output, const unsigned char * __restrict ysrc, const unsigned char * __restrict usrc, const unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height){


	// restore the bottom row of pixels + 2 pixels
	{
		int y,u,v;
		output[0]=ysrc[0];
		output[1]=u=usrc[0];
		output[2]=y=ysrc[1];
		output[3]=v=vsrc[0];

		for ( unsigned int a=1;a<width/2+2;a++){
			output[a*4+0]=y+=ysrc[a*2+0];
			output[a*4+1]=u+=usrc[a];
			output[a*4+2]=y+=ysrc[a*2+1];
			output[a*4+3]=v+=vsrc[a];
		}
	}

	if ( height<=1)
		return;

	// 40
	// 38
	// 37
	// 30
	// 28

	//unsigned __int64 t1,t2;
	//static HANDLE file = CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
	//t1 = GetTime();

	const unsigned int stride = width*2;
	unsigned int a=width/2+2;

	// make sure output[a*4-stride] is aligned
	for( ;(a*4-stride)&15;a++ ){
		int x = output[a*4-2];
		int y = output[a*4-stride];
		int z = (x+y-output[a*4-stride-2])&255;
		output[a*4+0]=ysrc[a*2+0]+median(x,y,z);

		x = output[a*4-3];
		y = output[a*4-stride+1];
		z = (x+y-output[a*4-stride-3])&255;
		output[a*4+1]=usrc[a]+median(x,y,z);

		x = output[a*4];
		y = output[a*4-stride+2];
		z = (x+y-output[a*4-stride])&255;
		output[a*4+2]=ysrc[a*2+1]+median(x,y,z);

		x = output[a*4-1];
		y = output[a*4-stride+3];
		z = (x+y-output[a*4-stride-1])&255;
		output[a*4+3]=vsrc[a]+median(x,y,z);
	}

	{
		__m128i x = _mm_setr_epi8(output[a*4-2],output[a*4-3],0,output[a*4-1],0,0,0,0,0,0,0,0,0,0,0,0);
		__m128i z = _mm_setr_epi8(output[a*4-3-stride],output[a*4-2-stride],output[a*4-1-stride],0,0,0,0,0,0,0,0,0,0,0,0,0);
		const int ymask=255;
		const int cmask=~255;
		unsigned int ending = ((height*width)/2-3);

		// restore the majority of the pixles using SSE2 median prediction
		for ( ; a<ending; a+=4){
			__m128i srcs = _mm_loadl_epi64((__m128i *)&ysrc[a*2]);
			__m128i temp0 = _mm_cvtsi32_si128( *(int*)&usrc[a]);
			__m128i temp1 = _mm_cvtsi32_si128( *(int*)&vsrc[a]);
			srcs = _mm_unpacklo_epi8(srcs,_mm_unpacklo_epi8(temp0,temp1));

			__m128i y = _mm_load_si128((__m128i *)&output[a*4-stride]);

			z = _mm_or_si128(z,_mm_slli_si128(y,3)); // z=uyvyuyvy
			z = _mm_or_si128(_mm_srli_epi16(z,8),_mm_slli_epi16(z,8));// z = yuyvyuyv
			z = _mm_sub_epi8(_mm_add_epi8(x,y),z);

			__m128i i = _mm_min_epu8(x,y);//min
			__m128i j = _mm_max_epu8(x,y);//max
			i = _mm_max_epu8(i,z);//max
			j = _mm_min_epu8(i,j);//min
			j = _mm_add_epi8(j,srcs); // yu-v------------
			// mask and shift j (yuv)
			j = _mm_shufflelo_epi16(j,(0<<0)+(0<<2)+(0<<4)+(1<<6));
			j = _mm_and_si128(j,_mm_setr_epi16(0,ymask,cmask,cmask,0,0,0,0));
			x = _mm_or_si128(x,j);
			z = _mm_add_epi8(z,j);

			i = _mm_min_epu8(x,y);//min
			j = _mm_max_epu8(x,y);//max
			i = _mm_max_epu8(i,z);//max
			j = _mm_min_epu8(i,j);//min
			j = _mm_add_epi8(j,srcs); // yuyv-u-v--------

			// mask and shift j (yuv)
			j = _mm_slli_si128(j,4);
			j = _mm_shufflelo_epi16(j,(3<<4));
			j = _mm_and_si128(j,_mm_setr_epi16(0,0,ymask,0,cmask,cmask,0,0));

			x = _mm_or_si128(x,j);
			z = _mm_add_epi8(z,j);

			i = _mm_min_epu8(x,y);//min
			j = _mm_max_epu8(x,y);//max
			i = _mm_max_epu8(i,z);//max
			j = _mm_min_epu8(i,j);//min
			j = _mm_add_epi8(j,srcs); // yuyvyu-v-u-v----
			// mask and shift j (y only)
			j = _mm_and_si128(j,_mm_setr_epi16(0,0,ymask,0,0,0,0,0));
			j = _mm_slli_epi32(j,16);
			x = _mm_or_si128(x,j);
			z = _mm_add_epi8(z,j);

			i = _mm_min_epu8(x,y);//min
			j = _mm_max_epu8(x,y);//max
			i = _mm_max_epu8(i,z);//max
			j = _mm_min_epu8(i,j);//min
			j = _mm_add_epi8(j,srcs); // yuyvyuyv-u-v----
			// mask and shift j (yuv )
			j = _mm_slli_si128(j,4);
			j = _mm_shufflehi_epi16(j,(1<<0)+(1<<2)+(2<<4)+(3<<6));
			j = _mm_and_si128(j,_mm_setr_epi16(0,0,0,0,ymask,0,cmask,cmask));
			x = _mm_or_si128(x,j);
			z = _mm_add_epi8(z,j);

			i = _mm_min_epu8(x,y);//min
			j = _mm_max_epu8(x,y);//max
			i = _mm_max_epu8(i,z);//max
			j = _mm_min_epu8(i,j);//min
			j = _mm_add_epi8(j,srcs); // yuyvyuyvyu-v-u-v
			// mask and shift j (y only )
			j = _mm_slli_si128(j,2);
			j = _mm_and_si128(j,_mm_setr_epi16(0,0,0,0,0,ymask,0,0));
			x = _mm_or_si128(x,j);
			z = _mm_add_epi8(z,j);

			i = _mm_min_epu8(x,y);//min
			j = _mm_max_epu8(x,y);//max
			i = _mm_max_epu8(i,z);//max
			j = _mm_min_epu8(i,j);//min
			j = _mm_add_epi8(j,srcs); // yuyvyuyvyuyv-u-v
			// mask and shift j (y only )
			j = _mm_and_si128(j,_mm_setr_epi16(0,0,0,0,0,ymask,0,0));
			j = _mm_slli_si128(j,2);
			x = _mm_or_si128(x,j);
			z = _mm_add_epi8(z,j);

			i = _mm_min_epu8(x,y);//min
			j = _mm_max_epu8(x,y);//max
			i = _mm_max_epu8(i,z);//max
			j = _mm_min_epu8(i,j);//min
			j = _mm_add_epi8(j,srcs); // yuyvyuyvyuyvyu-v
			// mask and shift j (y only )
			j = _mm_slli_si128(j,2);
			j = _mm_and_si128(j,_mm_setr_epi16(0,0,0,0,0,0,0,ymask));
			x = _mm_or_si128(x,j);
			z = _mm_add_epi8(z,j);

			i = _mm_min_epu8(x,y);//min
			x = _mm_max_epu8(x,y);//max
			i = _mm_max_epu8(i,z);//max
			x = _mm_min_epu8(i,x);//min
			x = _mm_add_epi8(x,srcs); // yuyvyuyvyuyvyuyv
			_mm_storeu_si128((__m128i*)&output[a*4],x); // will be aligned if width%8 is 0; but the speed change is negligable in my tests
			x = _mm_srli_epi64(x,40);
			x = _mm_unpackhi_epi8(x,_mm_setzero_si128());
			x = _mm_shufflelo_epi16(x,(1<<0)+(0<<2)+(3<<4)+(2<<6));
			x = _mm_packus_epi16(x,_mm_setzero_si128());

			z = _mm_srli_si128(y,13);

		}
	}

	// finish off any remaining pixels
	for( ;a < width*height/2;a++ ){
		int x = output[a*4-2];
		int y = output[a*4-stride];
		int z = (x+y-output[a*4-stride-2])&255;
		output[a*4+0]=ysrc[a*2+0]+median(x,y,z);

		x = output[a*4-3];
		y = output[a*4-stride+1];
		z = (x+y-output[a*4-stride-3])&255;
		output[a*4+1]=usrc[a]+median(x,y,z);

		x = output[a*4];
		y = output[a*4-stride+2];
		z = (x+y-output[a*4-stride])&255;
		output[a*4+2]=ysrc[a*2+1]+median(x,y,z);

		x = output[a*4-1];
		y = output[a*4-stride+3];
		z = (x+y-output[a*4-stride-1])&255;
		output[a*4+3]=vsrc[a]+median(x,y,z);
	}

	//t2 = GetTime();
	//t2 -= t1;
	//char msg[128];
	//sprintf(msg,"%f\n",t2/100000.0);
	//unsigned long bytes;
	//WriteFile(file,msg,strlen(msg),&bytes,NULL);
}

void Interleave_And_Restore_RGB24_SSE2( unsigned char * __restrict output, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height){

	const unsigned int stride = align_round(width*3,4);

	// restore the bottom row
	{
		int r=0;
		int g=0;
		int b=0;
		for ( unsigned int a=0;a<width;a++){
			output[a*3]=b+=bsrc[a];
			output[a*3+1]=g+=gsrc[a];
			output[a*3+2]=r+=rsrc[a];
		}
	}

	if ( !height )
		return;

	//unsigned __int64 t1,t2;
	//t1 = GetTime();

	// 81
	// 70
	// 55
	// 52
	__m128i z = _mm_setzero_si128();
	__m128i x = _mm_setzero_si128();

	unsigned int w=0;
	unsigned int h=0;

	const __m128i mask = _mm_setr_epi8(0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,0,0,0);

	// if there is no need for padding, treat it as one long row
	const unsigned int vsteps = (width&3)?height:2;
	const unsigned int wsteps = (width&3)?(width&(~3)):(width*height-width-4);

	for ( h=1;h<vsteps;h++){
		w=0;
		for ( ;w<wsteps;w+=4){
			unsigned int a = stride*h+w*3;
			__m128i b = _mm_cvtsi32_si128( *(unsigned int *)&bsrc[width*h+w] );
			__m128i g = _mm_cvtsi32_si128( *(unsigned int *)&gsrc[width*h+w] );
			__m128i r = _mm_cvtsi32_si128( *(unsigned int *)&rsrc[width*h+w] );
			b = _mm_unpacklo_epi8(b,g);
			r = _mm_unpacklo_epi8(r,r);

			__m128i src1 = _mm_unpacklo_epi16(b,r);
			__m128i src2 = _mm_unpackhi_epi8(src1,_mm_setzero_si128());
			src1 = _mm_unpacklo_epi8(src1,_mm_setzero_si128());

			__m128i y = _mm_loadl_epi64((__m128i *)&output[a-stride]);
			__m128i ys = _mm_cvtsi32_si128(*(int*)&output[a-stride+8]);
			y = _mm_unpacklo_epi64(y,ys);


			__m128i recorrilate = mask;
			recorrilate = _mm_and_si128(recorrilate,y);
			recorrilate = _mm_or_si128(_mm_srli_si128(recorrilate,1),_mm_slli_si128(recorrilate,1));
			recorrilate = _mm_add_epi8(recorrilate,y);
			_mm_storel_epi64((__m128i *)&output[a-stride],recorrilate);
			*(int*)&output[a-stride+8] = _mm_cvtsi128_si32(_mm_srli_si128(recorrilate,8));

			ys = y;
			y = _mm_slli_si128(y,1);
			y = _mm_unpacklo_epi8(y,_mm_setzero_si128());
			y = _mm_shufflelo_epi16(y,(1<<0)+(2<<2)+(3<<4)+(0<<6));
			ys = _mm_srli_si128(ys,5);
			ys = _mm_unpacklo_epi8(ys,_mm_setzero_si128());
			ys = _mm_shufflelo_epi16(ys,(1<<0)+(2<<2)+(3<<4)+(0<<6));
			z = _mm_unpacklo_epi64(z,y);

			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			__m128i i = _mm_min_epi16(x,y);//min
			__m128i j = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			j = _mm_min_epi16(i,j);//min
			j = _mm_add_epi8(j,src1);
			j = _mm_slli_si128(j,8);
			x = _mm_or_si128(x,j);
			z = _mm_add_epi16(z,j);

			/*******/

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);

			__m128i temp = x;
			x = _mm_srli_si128(x,8);
			z = _mm_srli_si128(y,8);
			z = _mm_unpacklo_epi64(z,ys);
			z = _mm_sub_epi16(_mm_add_epi16(x,ys),z);
			y = ys;
			

			/*******/

			i = _mm_min_epi16(x,y);//min
			j = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			j = _mm_min_epi16(i,j);//min
			j = _mm_add_epi8(j,src2);
			j = _mm_slli_si128(j,8);
			x = _mm_or_si128(x,j);
			z = _mm_add_epi16(z,j);
			
			/*******/

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);

			temp = _mm_shufflelo_epi16(temp,(3<<0)+(0<<2)+(1<<4)+(2<<6));
			temp = _mm_slli_si128(temp,2);

			x = _mm_shufflelo_epi16(x,(3<<0)+(0<<2)+(1<<4)+(2<<6));
			temp = _mm_packus_epi16(temp,_mm_srli_si128(x,2));

			_mm_storel_epi64((__m128i *)&output[a],_mm_srli_si128(temp,2));
			*(int*)&output[a+8] = _mm_cvtsi128_si32(_mm_srli_si128(temp,10));

			x = _mm_srli_si128(x,8);
			z = _mm_srli_si128(y,8);
		}
		if ( h < vsteps-1){
			// if the width is not mod 4, take care of the remaining pixels in the row
			for ( ;w<width;w++){
			
				unsigned int a = stride*h+w*3;

				__m128i src = _mm_cvtsi32_si128(bsrc[width*h+w]+(gsrc[width*h+w]<<8)+(rsrc[width*h+w]<<16));
				src = _mm_unpacklo_epi8(src,_mm_setzero_si128());
			
				// row has padding, no overrun risk
				__m128i y = _mm_cvtsi32_si128(*(int *)&output[a-stride]);
				y = _mm_unpacklo_epi8(y,_mm_setzero_si128());

				output[a+0-stride]+=output[a+1-stride];
				output[a+2-stride]+=output[a+1-stride];
				z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

				__m128i i = _mm_min_epi16(x,y);//min
				x = _mm_max_epi16(x,y);//max
				i = _mm_max_epi16(i,z);//max
				x = _mm_min_epi16(i,x);//min
				x = _mm_add_epi8(x,src);

				// row has padding, no overrun risk
				*(int*)&output[a] = _mm_cvtsi128_si32(_mm_packus_epi16(x,x));

				z = y;
			}
		}
	}

	h = height-1;
	w %= width;
	// take care of any remaining pixels
	for ( ;w<width;w++){	
		unsigned int a = stride*h+w*3;

		__m128i src = _mm_cvtsi32_si128(bsrc[width*h+w]+(gsrc[width*h+w]<<8)+(rsrc[width*h+w]<<16));
		src = _mm_unpacklo_epi8(src,_mm_setzero_si128());

		// row has padding, no overrun risk
		__m128i y = _mm_cvtsi32_si128(*(int *)&output[a-stride]);
		y = _mm_unpacklo_epi8(y,_mm_setzero_si128());

		output[a+0-stride]+=output[a+1-stride];
		output[a+2-stride]+=output[a+1-stride];
		z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

		__m128i i = _mm_min_epi16(x,y);//min
		x = _mm_max_epi16(x,y);//max
		i = _mm_max_epi16(i,z);//max
		x = _mm_min_epi16(i,x);//min
		x = _mm_add_epi8(x,src);

		unsigned int temp = _mm_cvtsi128_si32(_mm_packus_epi16(x,x));
		output[a+0] = temp;
		output[a+1] = temp>>8;
		output[a+2] = temp>>16;

		z = y;
	}

	for ( unsigned int a=stride*(height-1);a<stride*height;a+=3){
	//for ( unsigned int a=0;a<stride*height;a+=3){
		output[a]+=output[a+1];
		output[a+2]+=output[a+1];
	}

	/*t2 = GetTime();
	t2 -= t1;
	static HANDLE file = NULL;
	char msg[128];
	if ( !file ){
		file =  CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		sprintf(msg,"%f,=median(A1:A1000)\n",t2/100000.0);
	} else {
		sprintf(msg,"%f\n",t2/100000.0);
	}
	unsigned long bytes;
	WriteFile(file,msg,strlen(msg),&bytes,NULL);*/
}

void Interleave_And_Restore_RGB32_SSE2( unsigned char * __restrict output, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height){

	const unsigned int stride = width*4;
	{
		int r=0;
		int g=0;
		int b=0;
		for ( unsigned int a=0;a<width;a++){
			output[a*4+0]=b+=bsrc[a];
			output[a*4+1]=g+=gsrc[a];
			output[a*4+2]=r+=rsrc[a];
			output[a*4+3]=255;
		}
	}

	if ( height==1)
		return;

	//unsigned __int64 t1,t2;
	//t1 = GetTime();

	__m128i z = _mm_setzero_si128();
	__m128i x = _mm_setzero_si128();

	const unsigned int end = ((width*(height-1))&(~3))+width;
	unsigned int a=width;

	if ( (stride&15)==0){
		
		// 85
		// 77
		// 65
		// 46

		for ( ;a<end;a+=4){
			__m128i b = _mm_cvtsi32_si128( *(unsigned int *)&bsrc[a] );
			__m128i g = _mm_cvtsi32_si128( *(unsigned int *)&gsrc[a] );
			__m128i r = _mm_cvtsi32_si128( *(unsigned int *)&rsrc[a] );
			b = _mm_unpacklo_epi8(b,g);
			r = _mm_unpacklo_epi8(r,_mm_setzero_si128());

			__m128i src1 = _mm_unpacklo_epi16(b,r);
			__m128i src2 = _mm_unpackhi_epi8(src1,_mm_setzero_si128());
			src1 = _mm_unpacklo_epi8(src1,_mm_setzero_si128());

			__m128i temp2 = _mm_load_si128((__m128i *)&output[a*4-stride]); // must be %16
			__m128i y = _mm_unpacklo_epi8(temp2,_mm_setzero_si128());
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			__m128i i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);
			src1 = _mm_srli_si128(src1,8);

			z = y;
			y = _mm_srli_si128(y,8);
			__m128i temp1 = x;

			/*******/
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);

			temp1 = _mm_unpacklo_epi64(temp1,x);

			z = y;

			/*******/
			i = temp2;
			y = _mm_unpackhi_epi8(temp2,_mm_setzero_si128());
			temp2 = _mm_srli_epi16(temp2,8);
			temp2 = _mm_shufflelo_epi16(temp2,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			temp2 = _mm_shufflehi_epi16(temp2,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			temp2 = _mm_add_epi8(temp2,i);
			_mm_store_si128((__m128i *)&output[a*4-stride],temp2); // must be %16

			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);
			src2 = _mm_srli_si128(src2,8);

			z = y;
			y = _mm_srli_si128(y,8);
			temp2 = x;

			/*******/
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);

			temp2 = _mm_unpacklo_epi64(temp2,x);
			temp1 = _mm_packus_epi16(temp1,temp2);

			_mm_store_si128((__m128i *)&output[a*4],temp1);

			z = y;
		}
	} else { // width is not mod 4, change pixel store to unaligned move

		// 52
		for ( ;a<end;a+=4){
			__m128i b = _mm_cvtsi32_si128( *(unsigned int *)&bsrc[a] );
			__m128i g = _mm_cvtsi32_si128( *(unsigned int *)&gsrc[a] );
			__m128i r = _mm_cvtsi32_si128( *(unsigned int *)&rsrc[a] );
			b = _mm_unpacklo_epi8(b,g);
			r = _mm_unpacklo_epi8(r,_mm_setzero_si128());

			__m128i src1 = _mm_unpacklo_epi16(b,r);
			__m128i src2 = _mm_unpackhi_epi8(src1,_mm_setzero_si128());
			src1 = _mm_unpacklo_epi8(src1,_mm_setzero_si128());

			__m128i temp2 = _mm_load_si128((__m128i *)&output[a*4-stride]); // must be %16
			__m128i y = _mm_unpacklo_epi8(temp2,_mm_setzero_si128());
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			__m128i i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);
			src1 = _mm_srli_si128(src1,8);

			z = y;
			y = _mm_srli_si128(y,8);
			__m128i temp1 = x;

			/*******/
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);

			temp1 = _mm_unpacklo_epi64(temp1,x);

			z = y;

			/*******/
			i = temp2;
			y = _mm_unpackhi_epi8(temp2,_mm_setzero_si128());
			temp2 = _mm_srli_epi16(temp2,8);
			temp2 = _mm_shufflelo_epi16(temp2,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			temp2 = _mm_shufflehi_epi16(temp2,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			temp2 = _mm_add_epi8(temp2,i);
			_mm_store_si128((__m128i *)&output[a*4-stride],temp2); // must be %16

			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);
			src2 = _mm_srli_si128(src2,8);

			z = y;
			y = _mm_srli_si128(y,8);
			temp2 = x;

			/*******/
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);

			temp2 = _mm_unpacklo_epi64(temp2,x);
			temp1 = _mm_packus_epi16(temp1,temp2);

			_mm_storeu_si128((__m128i *)&output[a*4],temp1); // only change from aligned version

			z = y;
		}
	}

	// finish off any remaining pixels (0-3 pixels)
	for ( ;a < width*height;a++ ){
		__m128i src = _mm_cvtsi32_si128(bsrc[a]+(gsrc[a]<<8)+(rsrc[a]<<16));
		src = _mm_unpacklo_epi8(src,_mm_setzero_si128());

		__m128i y = _mm_cvtsi32_si128(*(int *)&output[a*4-stride]);
		output[a*4-stride+0] += output[a*4-stride+1];
		output[a*4-stride+2] += output[a*4-stride+1];
		y = _mm_unpacklo_epi8(y,_mm_setzero_si128());
		z = _mm_subs_epu16(_mm_add_epi16(x,y),z);

		__m128i i = _mm_min_epi16(x,y);//min
		x = _mm_max_epi16(x,y);//max
		i = _mm_max_epi16(i,z);//max
		x = _mm_min_epi16(x,i);//min
		x = _mm_add_epi8(x,src);

		*(int *)&output[a*4] = _mm_cvtsi128_si32(_mm_packus_epi16(x,x));

		z = y;
	}

	// finish recorrilating top row
	for ( a=stride*(height-1);a<stride*height;a+=4){
		output[a]+=output[a+1];
		output[a+2]+=output[a+1];
	}

	/*t2 = GetTime();
	t2 -= t1;
	static HANDLE file = NULL;
	char msg[128];
	if ( !file ){
		file =  CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		sprintf(msg,"%f,=median(A1:A1000)\n",t2/100000.0);
	} else {
		sprintf(msg,"%f\n",t2/100000.0);
	}
	unsigned long bytes;
	WriteFile(file,msg,strlen(msg),&bytes,NULL);*/
}

void Interleave_And_Restore_RGBA_SSE2( unsigned char * __restrict output, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned char * __restrict asrc, const unsigned int width, const unsigned int height){
	
	const unsigned int stride = width*4;
	{
		int r=0;
		int g=0;
		int b=0;
		int alpha=0;
		for ( unsigned int a=0;a<width;a++){
			output[a*4+0]=b+=bsrc[a];
			output[a*4+1]=g+=gsrc[a];
			output[a*4+2]=r+=rsrc[a];
			output[a*4+3]=alpha+=asrc[a];
		}
	}

	if ( height==1)
		return;

	__m128i z = _mm_setzero_si128();
	__m128i x = _mm_setzero_si128();

	const unsigned int end = ((width*(height-1))&(~3))+width;
	unsigned int a=width;

	if ( (stride&15)==0){
		
		// 85
		// 77
		// 65
		// 46

		for ( ;a<end;a+=4){
			__m128i b = _mm_cvtsi32_si128( *(unsigned int *)&bsrc[a] );
			__m128i g = _mm_cvtsi32_si128( *(unsigned int *)&gsrc[a] );
			__m128i r = _mm_cvtsi32_si128( *(unsigned int *)&rsrc[a] );
			__m128i alpha = _mm_cvtsi32_si128( *(unsigned int *)&asrc[a]);
			b = _mm_unpacklo_epi8(b,g);
			r = _mm_unpacklo_epi8(r,alpha);

			__m128i src1 = _mm_unpacklo_epi16(b,r);
			__m128i src2 = _mm_unpackhi_epi8(src1,_mm_setzero_si128());
			src1 = _mm_unpacklo_epi8(src1,_mm_setzero_si128());

			__m128i temp2 = _mm_load_si128((__m128i *)&output[a*4-stride]); // must be %16
			__m128i y = _mm_unpacklo_epi8(temp2,_mm_setzero_si128());
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			__m128i i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);
			src1 = _mm_srli_si128(src1,8);

			z = y;
			y = _mm_srli_si128(y,8);
			__m128i temp1 = x;

			/*******/
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);

			temp1 = _mm_unpacklo_epi64(temp1,x);

			z = y;

			/*******/
			i = temp2;
			y = _mm_unpackhi_epi8(temp2,_mm_setzero_si128());
			temp2 = _mm_srli_epi16(temp2,8);
			temp2 = _mm_shufflelo_epi16(temp2,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			temp2 = _mm_shufflehi_epi16(temp2,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			temp2 = _mm_add_epi8(temp2,i);
			_mm_store_si128((__m128i *)&output[a*4-stride],temp2); // must be %16

			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);
			src2 = _mm_srli_si128(src2,8);

			z = y;
			y = _mm_srli_si128(y,8);
			temp2 = x;

			/*******/
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);

			temp2 = _mm_unpacklo_epi64(temp2,x);
			temp1 = _mm_packus_epi16(temp1,temp2);

			_mm_store_si128((__m128i *)&output[a*4],temp1);

			z = y;
		}
	} else { // width is not mod 4, change pixel store to unaligned move

		// 52
		for ( ;a<end;a+=4){
			__m128i b = _mm_cvtsi32_si128( *(unsigned int *)&bsrc[a] );
			__m128i g = _mm_cvtsi32_si128( *(unsigned int *)&gsrc[a] );
			__m128i r = _mm_cvtsi32_si128( *(unsigned int *)&rsrc[a] );
			__m128i alpha = _mm_cvtsi32_si128( *(unsigned int *)&asrc[a]);
			b = _mm_unpacklo_epi8(b,g);
			r = _mm_unpacklo_epi8(r,alpha);

			__m128i src1 = _mm_unpacklo_epi16(b,r);
			__m128i src2 = _mm_unpackhi_epi8(src1,_mm_setzero_si128());
			src1 = _mm_unpacklo_epi8(src1,_mm_setzero_si128());

			__m128i temp2 = _mm_load_si128((__m128i *)&output[a*4-stride]); // must be %16
			__m128i y = _mm_unpacklo_epi8(temp2,_mm_setzero_si128());
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			__m128i i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);
			src1 = _mm_srli_si128(src1,8);

			z = y;
			y = _mm_srli_si128(y,8);
			__m128i temp1 = x;

			/*******/
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src1);

			temp1 = _mm_unpacklo_epi64(temp1,x);

			z = y;

			/*******/
			i = temp2;
			y = _mm_unpackhi_epi8(temp2,_mm_setzero_si128());
			temp2 = _mm_srli_epi16(temp2,8);
			temp2 = _mm_shufflelo_epi16(temp2,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			temp2 = _mm_shufflehi_epi16(temp2,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			temp2 = _mm_add_epi8(temp2,i);
			_mm_store_si128((__m128i *)&output[a*4-stride],temp2); // must be %16

			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);
			src2 = _mm_srli_si128(src2,8);

			z = y;
			y = _mm_srli_si128(y,8);
			temp2 = x;

			/*******/
			z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

			i = _mm_min_epi16(x,y);//min
			x = _mm_max_epi16(x,y);//max
			i = _mm_max_epi16(i,z);//max
			x = _mm_min_epi16(i,x);//min
			x = _mm_add_epi8(x,src2);

			temp2 = _mm_unpacklo_epi64(temp2,x);
			temp1 = _mm_packus_epi16(temp1,temp2);

			_mm_storeu_si128((__m128i *)&output[a*4],temp1); // only change for unaligned version

			z = y;
		}
	}

	// finish off any remaining pixels (0-3 pixels)
	for ( ;a < width*height;a++ ){
		__m128i src = _mm_cvtsi32_si128(bsrc[a]+(gsrc[a]<<8)+(rsrc[a]<<16)+(asrc[a]<<24));
		src = _mm_unpacklo_epi8(src,_mm_setzero_si128());

		__m128i y = _mm_cvtsi32_si128(*(int *)&output[a*4-stride]);
		output[a*4-stride+0] += output[a*4-stride+1];
		output[a*4-stride+2] += output[a*4-stride+1];
		y = _mm_unpacklo_epi8(y,_mm_setzero_si128());
		z = _mm_subs_epu16(_mm_add_epi16(x,y),z);

		__m128i i = _mm_min_epi16(x,y);//min
		x = _mm_max_epi16(x,y);//max
		i = _mm_max_epi16(i,z);//max
		x = _mm_min_epi16(x,i);//min
		x = _mm_add_epi8(x,src);

		*(int *)&output[a*4] = _mm_cvtsi128_si32(_mm_packus_epi16(x,x));

		z = y;
	}

	// finish recorrilating top row
	for ( unsigned int a=stride*(height-1);a<stride*height;a+=4){
		output[a]+=output[a+1];
		output[a+2]+=output[a+1];
	}
}

void Restore_YV12_SSE2(unsigned char * __restrict ysrc, unsigned char * __restrict usrc, unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height){
	unsigned int a;
	{
		int y = 0;
		int u = 0;
		int v = 0;
		for ( a=0;a<width/2;a++){
			usrc[a]=u+=usrc[a];
			vsrc[a]=v+=vsrc[a];
		}
		for ( a=0;a<width;a++){
			ysrc[a]=y+=ysrc[a];
		}
	}

	//unsigned __int64 t1,t2;
	//t1 = GetTime();

	// 72
	// 41
	// 35
	const __m128i mask = _mm_setr_epi32(0x00ff00ff,0x00ff00ff,0,0);

	__m128i x = _mm_setr_epi16(ysrc[width-1],usrc[width/2-1],vsrc[width/2-1],0,0,0,0,0);
	__m128i z = _mm_setr_epi16(ysrc[0],usrc[0],vsrc[0],0,0,0,0,0);

	for ( a=width; a<((width*height/4 + width/2)&(~3)); a+=4){

		__m128i y = _mm_cvtsi32_si128(*(int*)&ysrc[a-width]);
		__m128i y1 = _mm_cvtsi32_si128(*(int*)&usrc[a-width]);
		__m128i y2 = _mm_cvtsi32_si128(*(int*)&vsrc[a-width]);
		y = _mm_unpacklo_epi8(y,y1);
		y2 = _mm_unpacklo_epi8(y2,_mm_setzero_si128());
		y = _mm_unpacklo_epi16(y,y2);

		__m128i srcs =_mm_cvtsi32_si128(*(int*)&ysrc[a]);
		__m128i s1 = _mm_cvtsi32_si128(*(int*)&usrc[a-width/2]);
		__m128i s2 = _mm_cvtsi32_si128(*(int*)&vsrc[a-width/2]);

		srcs = _mm_unpacklo_epi8(srcs,s1);
		s2 = _mm_unpacklo_epi8(s2,_mm_setzero_si128());
		srcs = _mm_unpacklo_epi16(srcs,s2);

		__m128i temp = _mm_unpackhi_epi64(y,srcs); // store for later
		y = _mm_unpacklo_epi8(y,_mm_setzero_si128());
		srcs = _mm_unpacklo_epi8(srcs,_mm_setzero_si128());

		z = _mm_unpacklo_epi64(z,y);
		z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

		__m128i i = _mm_min_epi16(x,y);
		__m128i j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,srcs);
		j = _mm_slli_si128(j,8);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		i = _mm_min_epi16(x,y);
		x = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		x = _mm_min_epi16(x,i);
		x = _mm_add_epi8(x,srcs);

		z = _mm_srli_si128(y,8);
		srcs = _mm_unpackhi_epi8(temp,_mm_setzero_si128());
		y = _mm_unpacklo_epi8(temp,_mm_setzero_si128());
		z = _mm_unpacklo_epi64(z,y);

		temp = x;
		x = _mm_srli_si128(x,8);

		z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

		i = _mm_min_epi16(x,y);
		j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,srcs);
		j = _mm_slli_si128(j,8);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		temp = _mm_or_si128(temp,_mm_srli_si128(temp,7));

		i = _mm_min_epi16(x,y);
		x = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		x = _mm_min_epi16(x,i);
		x = _mm_add_epi8(x,srcs);
		i = x;
		x = _mm_srli_si128(x,7);
		i = _mm_or_si128(i,x);
		temp = _mm_unpacklo_epi16(temp,i);

		*(int*)&ysrc[a]			= _mm_cvtsi128_si32(temp);
		*(int*)&usrc[a-width/2] = _mm_cvtsi128_si32(_mm_srli_si128(temp,4));
		*(int*)&vsrc[a-width/2] = _mm_cvtsi128_si32(_mm_srli_si128(temp,8));

		x = _mm_srli_epi16(x,8);
		z = _mm_srli_si128(y,8);
	}

	for ( ;a<width*height/4 + width/2;a++){
		int i = ysrc[a-1];
		int j = ysrc[a-width];
		int k = i+j-ysrc[a-width-1];
		ysrc[a]+=median(i,j,k);

		i = usrc[a-width/2-1];
		j = usrc[a-width/2-width/2];
		k = i+j-usrc[a-width/2-width/2-1];
		usrc[a-width/2]+=median(i,j,k);
		
		i = vsrc[a-width/2-1];
		j = vsrc[a-width/2-width/2];
		k = i+j-vsrc[a-width/2-width/2-1];
		vsrc[a-width/2]+=median(i,j,k);
	}

	for ( ;a&7;a++){
		int x = ysrc[a-1];
		int y = ysrc[a-width];
		int z = x+y-ysrc[a-width-1];
		x = median(x,y,z);
		ysrc[a]+=x;
	}

	x = _mm_cvtsi32_si128(ysrc[a-1]);
	z = _mm_cvtsi32_si128(ysrc[a-width-1]);

	__m128i shift_mask = _mm_cvtsi32_si128(255);

	for ( ;a<((width*height)&(~7));a+=8){
		__m128i src = _mm_loadl_epi64((__m128i*)&ysrc[a]);
		__m128i y = _mm_loadl_epi64((__m128i*)&ysrc[a-width]);
		y = _mm_unpacklo_epi8(y,_mm_setzero_si128());
		src = _mm_unpacklo_epi8(src,_mm_setzero_si128());

		z = _mm_or_si128(z,_mm_slli_si128(y,2));

		z = _mm_sub_epi16(_mm_add_epi16(x,y),z);

		__m128i i = _mm_min_epi16(x,y);
		__m128i j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,src); // 1 correct
		// mask and shift j
		j = _mm_and_si128(j,shift_mask);
		j = _mm_slli_si128(j,2);
		shift_mask = _mm_slli_si128(shift_mask,2);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		i = _mm_min_epi16(x,y);
		j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,src); // 2 correct
		// mask and shift j
		j = _mm_and_si128(j,shift_mask);
		j = _mm_slli_si128(j,2);
		shift_mask = _mm_slli_si128(shift_mask,2);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		i = _mm_min_epi16(x,y);
		j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,src); // 3 correct
		// mask and shift j
		j = _mm_and_si128(j,shift_mask);
		j = _mm_slli_si128(j,2);
		shift_mask = _mm_slli_si128(shift_mask,2);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		i = _mm_min_epi16(x,y);
		j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,src); // 4 correct
		j = _mm_and_si128(j,shift_mask);
		j = _mm_slli_si128(j,2);
		shift_mask = _mm_slli_si128(shift_mask,2);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		i = _mm_min_epi16(x,y);
		j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,src); // 5 correct
		j = _mm_and_si128(j,shift_mask);
		j = _mm_slli_si128(j,2);
		shift_mask = _mm_slli_si128(shift_mask,2);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		i = _mm_min_epi16(x,y);
		j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,src); // 6 correct
		j = _mm_and_si128(j,shift_mask);
		j = _mm_slli_si128(j,2);
		shift_mask = _mm_slli_si128(shift_mask,2);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		i = _mm_min_epi16(x,y);
		j = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		j = _mm_min_epi16(j,i);
		j = _mm_add_epi8(j,src); // 7 correct
		j = _mm_and_si128(j,shift_mask);
		j = _mm_slli_si128(j,2);
		shift_mask = _mm_srli_si128(shift_mask,12);
		x = _mm_or_si128(x,j);
		z = _mm_add_epi16(z,j);

		i = _mm_min_epi16(x,y);
		x = _mm_max_epi16(x,y);
		i = _mm_max_epi16(i,z);
		x = _mm_min_epi16(x,i);
		x = _mm_add_epi8(x,src); // 8 correct

		_mm_storel_epi64((__m128i*)&ysrc[a], _mm_packus_epi16(x,x));

		x = _mm_srli_si128(x,14);
		z = _mm_srli_si128(y,14);
	}

	for ( ;a<width*height;a++){
		int x = ysrc[a-1];
		int y = ysrc[a-width];
		int z = x+y-ysrc[a-width-1];
		x = median(x,y,z);
		ysrc[a]+=x;
	}

	
	/*t2 = GetTime();
	t2 -= t1;
	static HANDLE file = NULL;
	char msg[128];
	if ( !file ){
		file =  CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.csv",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		sprintf(msg,"%f,=median(A1:A1000)\n",t2/100000.0);
	} else {
		sprintf(msg,"%f\n",t2/100000.0);
	}
	unsigned long bytes;
	WriteFile(file,msg,strlen(msg),&bytes,NULL);*/
}


void Double_Resolution(const unsigned char * src, unsigned char * dst, unsigned char * buffer, unsigned int width, unsigned int height ){

	for ( unsigned int y=0;y<height;y++){
		unsigned int x=0;
		for ( ;x<width-1;x++){
			dst[y*2*width*2+x*2]=src[y*width+x];
			dst[y*2*width*2+x*2+1]=(src[y*width+x]+src[y*width+x+1]+1)/2;
		}
		dst[y*2*width*2+x*2]=src[y*width+x];
		dst[y*2*width*2+x*2+1]=src[y*width+x];
	}
	for ( unsigned int y=0;y<height*2-2;y+=2){
		for ( unsigned int x=0;x<width*2;x++){
			dst[y*width*2+x+width*2]=(dst[y*width*2+x]+dst[y*width*2+x+width*4]+1)/2;
		}
	}
	memcpy(dst+height*2*width*2-width*2,dst+height*2*width*2-width*4,width*2);
}

void Interleave_And_Restore_Old_Unaligned(unsigned char * bsrc, unsigned char * gsrc, unsigned char * rsrc, unsigned char * dst, unsigned char * buffer, bool rgb24, unsigned int width, unsigned int height){
	const unsigned int stride = align_round(width*3,4);
	unsigned char * output = (rgb24)?dst:buffer;

	output[0]=bsrc[0];
	output[1]=gsrc[0];
	output[2]=rsrc[0];

	for ( unsigned int a=1;a<width;a++){
		output[a*3+0]=bsrc[a]+output[a*3-3];
		output[a*3+1]=gsrc[a]+output[a*3-2];
		output[a*3+2]=rsrc[a]+output[a*3-1];
	}

	output[width*3+0]=bsrc[width]+output[0];
	output[width*3+1]=gsrc[width]+output[1];
	output[width*3+2]=rsrc[width]+output[2];

	for ( unsigned int a=width+1;a<width*height;a++){
		int x = output[a*3-3];
		int y = output[a*3-width*3];
		int z = x+y-output[a*3-width*3-3];
		output[a*3+0]=bsrc[a]+median(x,y,z);

		x = output[a*3-2];
		y = output[a*3-width*3+1];
		z = x+y-output[a*3-width*3-2];
		output[a*3+1]=gsrc[a]+median(x,y,z);

		x = output[a*3-1];
		y = output[a*3-width*3+2];
		z = x+y-output[a*3-width*3-1];
		output[a*3+2]=rsrc[a]+median(x,y,z);
	}

	for ( unsigned int a=0;a<width*height*3;a+=3){
		output[a+0]+=output[a+1];
		output[a+2]+=output[a+1];
	}

	memcpy(output+width*height*3,output+width*height*3-stride,height*stride-width*height*3);
	if ( !rgb24 ){
		for ( unsigned int y=0;y<height;y++){
			for ( unsigned int x=0;x<width;x++){
				dst[y*width*4+x*4+0]=buffer[y*stride+x*3+0];
				dst[y*width*4+x*4+1]=buffer[y*stride+x*3+1];
				dst[y*width*4+x*4+2]=buffer[y*stride+x*3+2];
				dst[y*width*4+x*4+3]=255;
			}
		}
	}
}

#ifndef X64_BUILD

// Some processors have better performance with MMX/SSE than with SSE2.
// This macro selects the faster of the SSE/SSE2 versions of functions.
// The first 10 frames of a video are used as timing runs (the data of the
// frames won't affect the speed), with 5 frames run under each version of
// the function. The total number of CPU cycles used for each run is counted,
// and the minimum of the times are compared to judge the relative
// performance. The minimum is used since it should best filter out noise
// (from task switches, paging, etc.) in such a small sample. 
#define Select_Fastest(function_a,function_b){\
	int count = performance->count;\
	if ( count >= 10 ){\
		if ( performance->prefere_a ){\
			function_a;\
		} else {\
			function_b;\
		}\
	} else {\
		if ( count&1 ){\
			unsigned __int64 t1 = GetTime();\
			function_a;\
			t1 = GetTime()-t1;\
			if ( performance->time_a > t1){\
				performance->time_a = t1;\
			}\
		} else {\
			unsigned __int64 t1 = GetTime();\
			function_b;\
			t1 = GetTime()-t1;\
			if ( performance->time_b > t1){\
				performance->time_b = t1;\
			}\
		}\
		count++;\
		performance->count=count;\
		if ( count==10){\
			performance->prefere_a = performance->time_a<=performance->time_b;\
			/*char msg[128];\
			//sprintf_s(msg,sizeof(msg),"Prefere %s (ratio = %f)",performance->prefere_a?"SSE2":"MMX",((double)performance->time_a)/performance->time_b);\
			//MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);*/ \
		}\
	}\
}

void Block_Predict( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool rgbmode){
	if ( SSE2 ){
		Block_Predict_SSE2(source,dest,width,length,rgbmode);
	} else {
		Block_Predict_MMX(source,dest,width,length,rgbmode);
	}
}

void Block_Predict_YUV16( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool is_y){
	if ( SSE2 ){	
		Block_Predict_YUV16_SSE2(source,dest,width,length,is_y);
	} else {
		Block_Predict_YUV16_MMX(source,dest,width,length,is_y);
	}
}

void Decorrilate_And_Split_RGB24( const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Decorrilate_And_Split_RGB24_SSE2(in,rdst,gdst,bdst,width,height),
			Decorrilate_And_Split_RGB24_MMX(in,rdst,gdst,bdst,width,height)
		);
	} else {
		Decorrilate_And_Split_RGB24_MMX(in,rdst,gdst,bdst,width,height);
	}
}

void Decorrilate_And_Split_RGB32( const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Decorrilate_And_Split_RGB32_SSE2(in,rdst,gdst,bdst,width,height),
			Decorrilate_And_Split_RGB32_MMX(in,rdst,gdst,bdst,width,height)
		);
	} else {
		Decorrilate_And_Split_RGB32_MMX(in,rdst,gdst,bdst,width,height);
	}
}


void Decorrilate_And_Split_RGBA( const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, unsigned char * __restrict adst, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Decorrilate_And_Split_RGBA_SSE2(in,rdst,gdst,bdst,adst,width,height),
			Decorrilate_And_Split_RGBA_MMX(in,rdst,gdst,bdst,adst,width,height)
		);
	} else {
		Decorrilate_And_Split_RGBA_MMX(in,rdst,gdst,bdst,adst,width,height);
	}
}

void Split_YUY2( const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Split_YUY2_SSE2(src,ydst,udst,vdst,width,height),
			Split_YUY2_MMX(src,ydst,udst,vdst,width,height)
		);
	} else {
		Split_YUY2_MMX(src,ydst,udst,vdst,width,height);
	}
}

void Split_UYVY( const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Split_UYVY_SSE2(src,ydst,udst,vdst,width,height),
			Split_UYVY_MMX(src,ydst,udst,vdst,width,height)
		);
	} else {
		Split_UYVY_MMX(src,ydst,udst,vdst,width,height);
	}
}

void Interleave_And_Restore_RGB24( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Interleave_And_Restore_RGB24_SSE2(out,rsrc,gsrc,bsrc,width,height),
			Interleave_And_Restore_RGB24_MMX(out,rsrc,gsrc,bsrc,width,height)
		);
	} else {
		Interleave_And_Restore_RGB24_MMX(out,rsrc,gsrc,bsrc,width,height);
	}
}

void Interleave_And_Restore_RGB32( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Interleave_And_Restore_RGB32_SSE2(out,rsrc,gsrc,bsrc,width,height),
			Interleave_And_Restore_RGB32_MMX(out,rsrc,gsrc,bsrc,width,height)
		);
	} else {
		Interleave_And_Restore_RGB32_MMX(out,rsrc,gsrc,bsrc,width,height);
	}
}

void Interleave_And_Restore_RGBA( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned char * __restrict asrc, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Interleave_And_Restore_RGBA_SSE2(out,rsrc,gsrc,bsrc,asrc,width,height),
			Interleave_And_Restore_RGBA_MMX(out,rsrc,gsrc,bsrc,asrc,width,height)
		);
	} else {
		Interleave_And_Restore_RGBA_MMX(out,rsrc,gsrc,bsrc,asrc,width,height);
	}
}

void Interleave_And_Restore_YUY2( unsigned char * __restrict out, const unsigned char * __restrict ysrc, const unsigned char * __restrict usrc, const unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Interleave_And_Restore_YUY2_SSE2(out,ysrc,usrc,vsrc,width,height),
			Interleave_And_Restore_YUY2_MMX(out,ysrc,usrc,vsrc,width,height)
		);
	} else {
		Interleave_And_Restore_YUY2_MMX(out,ysrc,usrc,vsrc,width,height);
	}
}

void Restore_YV12( unsigned char * __restrict ysrc, unsigned char * __restrict usrc, unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height, Performance * performance){
	if ( SSE2 ){
		Select_Fastest(
			Restore_YV12_SSE2(ysrc,usrc,vsrc,width,height),
			Restore_YV12_MMX(ysrc,usrc,vsrc,width,height)
		);
	} else {
		Restore_YV12_MMX(ysrc,usrc,vsrc,width,height);
	}
}

#else // x64 build

void Block_Predict( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool rgbmode){
	Block_Predict_SSE2(source,dest,width,length,rgbmode);
}

void Block_Predict_YUV16( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool is_y){
	Block_Predict_YUV16_SSE2(source,dest,width,length,is_y);
}

void Decorrilate_And_Split_RGB24(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height, Performance * performance){
	Decorrilate_And_Split_RGB24_SSE2(in,rdst,gdst,bdst,width,height);
}

void Decorrilate_And_Split_RGB32(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height, Performance * performance){
	Decorrilate_And_Split_RGB32_SSE2(in,rdst,gdst,bdst,width,height);
}

void Decorrilate_And_Split_RGBA(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, unsigned char * __restrict adst, const unsigned int width, const unsigned int height, Performance * performance){
	Decorrilate_And_Split_RGBA_SSE2(in,rdst,gdst,bdst,adst,width,height);
}

void Split_YUY2(const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height, Performance * performance){
	Split_YUY2_SSE2(src,ydst,udst,vdst,width,height);
}

void Split_UYVY(const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height, Performance * performance){
	Split_UYVY_SSE2(src,ydst,udst,vdst,width,height);
}

void Interleave_And_Restore_RGB24( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height, Performance * performance){
	Interleave_And_Restore_RGB24_SSE2(out,rsrc,gsrc,bsrc,width,height);
}

void Interleave_And_Restore_RGB32( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height, Performance * performance){
	Interleave_And_Restore_RGB32_SSE2(out,rsrc,gsrc,bsrc,width,height);
}

void Interleave_And_Restore_RGBA( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned char * __restrict asrc, const unsigned int width, const unsigned int height, Performance * performance){
	Interleave_And_Restore_RGBA_SSE2(out,rsrc,gsrc,bsrc,asrc,width,height);
}

void Interleave_And_Restore_YUY2( unsigned char * __restrict out, const unsigned char * __restrict ysrc, const unsigned char * __restrict usrc, const unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height, Performance * performance){
	Interleave_And_Restore_YUY2_SSE2(out,ysrc,usrc,vsrc,width,height);
}

void Restore_YV12( unsigned char * __restrict ysrc, unsigned char * __restrict usrc, unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height, Performance * performance){
	Restore_YV12_SSE2(ysrc,usrc,vsrc,width,height);
}
#endif

//MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);