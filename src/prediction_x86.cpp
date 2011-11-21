#ifndef X64_BUILD

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

//This file contains functions that perform and undo median predition for x86

bool SSE;
bool SSE2;

void Decorrilate_And_Split_RGB24_MMX(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height){
	const unsigned int stride = align_round(width*3,4);
	//34
	//28
	
	const unsigned int vsteps = (width&3)?height:1;
	const unsigned int wsteps = (width&3)?(width&(~3)):width*height;
	__m64 mask = _mm_set_pi32(255,255);
	for ( unsigned int y=0; y<vsteps; y++){
		unsigned int x;
		for ( x=0; x<wsteps; x+=4){
			__m64 x0 = *(__m64*)&in[y*stride+x*3+0];
			__m64 x1 = _mm_srli_si64(x0,24);
			__m64 x3 = _mm_cvtsi32_si64(*(int*)&in[y*stride+x*3+8]);
			__m64 x2 = _mm_or_si64(_mm_srli_si64(x0,6*8),_mm_slli_si64(x3,16));
			x3 = _mm_srli_pi32(x3,8);

			x0 = _mm_unpacklo_pi32(x0,x1);
			x2 = _mm_unpacklo_pi32(x2,x3);

			__m64 r = _mm_srli_si64(x0,16);
			__m64 g = _mm_srli_si64(x0,8);
			__m64 b = x0;
			r = _mm_and_si64(r,mask);
			g = _mm_and_si64(g,mask);
			b = _mm_and_si64(b,mask);

			x0 = _mm_srli_si64(x2,16);
			x1 = _mm_srli_si64(x2,8);
			//x2 = x2

			x0 = _mm_and_si64(x0,mask);
			x1 = _mm_and_si64(x1,mask);
			x2 = _mm_and_si64(x2,mask);

			r = _mm_packs_pi32(r,x0);
			g = _mm_packs_pi32(g,x1);
			b = _mm_packs_pi32(b,x2);

			r = _mm_packs_pu16(r,r);
			g = _mm_packs_pu16(g,g);
			b = _mm_packs_pu16(b,b);

			r = _mm_sub_pi8(r,g);
			b = _mm_sub_pi8(b,g);

			*(int*)&bdst[y*width+x] = _mm_cvtsi64_si32(b);
			*(int*)&gdst[y*width+x] = _mm_cvtsi64_si32(g);
			*(int*)&rdst[y*width+x] = _mm_cvtsi64_si32(r);
		}
		for ( ;x < width; x++ ){
			bdst[y*width+x] = in[y*stride+x*3+0]-in[y*stride+x*3+1];
			gdst[y*width+x] = in[y*stride+x*3+1];
			rdst[y*width+x] = in[y*stride+x*3+2]-in[y*stride+x*3+1];
		}
	}
	
	_mm_empty();
}

//7.6
void Decorrilate_And_Split_RGB32_MMX(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height){

	__m64 mask = _mm_set_pi32(255,255);
	unsigned int a=0;
	const unsigned int end = ((width*height)&(~7));
	for ( a=0; a<end; a+=8){
		
		__m64 x0 = *(__m64*)&in[a*4+0];
		__m64 x2 = *(__m64*)&in[a*4+8];

		__m64 r = _mm_srli_si64(x0,16);
		__m64 g = _mm_srli_si64(x0,8);
		__m64 b = x0;
		r = _mm_and_si64(r,mask);
		g = _mm_and_si64(g,mask);
		b = _mm_and_si64(b,mask);

		x0 = _mm_srli_si64(x2,16);
		__m64 x1 = _mm_srli_si64(x2,8);
		//x2 = x2

		x0 = _mm_and_si64(x0,mask);
		x1 = _mm_and_si64(x1,mask);
		x2 = _mm_and_si64(x2,mask);

		r = _mm_packs_pi32(r,x0);
		g = _mm_packs_pi32(g,x1);
		b = _mm_packs_pi32(b,x2);


		x0 = *(__m64*)&in[a*4+16];
		x2 = *(__m64*)&in[a*4+24];

		__m64 r2 = _mm_srli_si64(x0,16);
		__m64 g2 = _mm_srli_si64(x0,8);
		__m64 b2 = x0;
		r2 = _mm_and_si64(r2,mask);
		g2 = _mm_and_si64(g2,mask);
		b2 = _mm_and_si64(b2,mask);

		x0 = _mm_srli_si64(x2,16);
		x1 = _mm_srli_si64(x2,8);
		//x2 = x2

		x0 = _mm_and_si64(x0,mask);
		x1 = _mm_and_si64(x1,mask);
		x2 = _mm_and_si64(x2,mask);

		r2 = _mm_packs_pi32(r2,x0);
		g2 = _mm_packs_pi32(g2,x1);
		b2 = _mm_packs_pi32(b2,x2);

		r = _mm_packs_pu16(r,r2);
		g = _mm_packs_pu16(g,g2);
		b = _mm_packs_pu16(b,b2);

		r = _mm_sub_pi8(r,g);
		b = _mm_sub_pi8(b,g);

		*(__m64*)&bdst[a] = b;
		*(__m64*)&gdst[a] = g;
		*(__m64*)&rdst[a] = r;
	}
	for ( ;a < width*height; a++ ){
			bdst[a] = in[a*4+0]-in[a*4+1];
			gdst[a] = in[a*4+1];
			rdst[a] = in[a*4+2]-in[a*4+1];
		}
	_mm_empty();
}

void Decorrilate_And_Split_RGBA_MMX(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, unsigned char * __restrict adst, const unsigned int width, const unsigned int height){

	__m64 mask = _mm_set_pi32(255,255);
	unsigned int a;
	const unsigned int end = (width*height)&(~3);
	// the increased register pressure from alpha makes it slightly faster to only do 4 pixels/loop
	for ( a=0; a<end; a+=4){

		__m64 x0 = *(__m64*)&in[a*4+0];
		__m64 x2 = *(__m64*)&in[a*4+8];

		__m64 alpha = _mm_srli_pi32(x0,24);
		__m64 r = _mm_srli_si64(x0,16);
		__m64 g = _mm_srli_si64(x0,8);
		__m64 b = x0;
		r = _mm_and_si64(r,mask);
		g = _mm_and_si64(g,mask);
		b = _mm_and_si64(b,mask);

		x0 = _mm_srli_si64(x2,16);
		__m64 x1 = _mm_srli_si64(x2,8);
		__m64 x3 = _mm_srli_pi32(x2,24);

		x0 = _mm_and_si64(x0,mask);
		x1 = _mm_and_si64(x1,mask);
		x2 = _mm_and_si64(x2,mask);

		r = _mm_packs_pi32(r,x0);
		g = _mm_packs_pi32(g,x1);
		b = _mm_packs_pi32(b,x2);
		alpha = _mm_packs_pi32(alpha,x3);

		r = _mm_packs_pu16(r,r);
		g = _mm_packs_pu16(g,g);
		b = _mm_packs_pu16(b,b);
		alpha = _mm_packs_pu16(alpha,alpha);

		r = _mm_sub_pi8(r,g);
		b = _mm_sub_pi8(b,g);

		*(int*)&bdst[a] = _mm_cvtsi64_si32(b);
		*(int*)&gdst[a] = _mm_cvtsi64_si32(g);
		*(int*)&rdst[a] = _mm_cvtsi64_si32(r);
		*(int*)&adst[a] = _mm_cvtsi64_si32(alpha);
	}
	for ( ;a < width*height; a++ ){
			bdst[a] = in[a*4+0]-in[a*4+1];
			gdst[a] = in[a*4+1];
			rdst[a] = in[a*4+2]-in[a*4+1];
			adst[a] = in[a*4+3];
		}
	_mm_empty();
}

void Split_YUY2_MMX(const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height){
	const __m64 ymask = _mm_set_pi32(0x00FF00FF,0x00FF00FF);

	unsigned int a;
	const unsigned int end = (width*height)&(~7);
	for ( a=0;a<end;a+=8){
		__m64 y0 = *(__m64*)&src[a*2+0];
		__m64 y1 = *(__m64*)&src[a*2+8];

		__m64 u0 = _mm_srli_pi32(_mm_slli_pi32(y0,16),24);
		__m64 v0 = _mm_srli_pi32(y0,24);
		y0 = _mm_and_si64(y0,ymask);

		__m64 u1 = _mm_srli_pi32(_mm_slli_pi32(y1,16),24);
		__m64 v1 = _mm_srli_pi32(y1,24);
		y1 = _mm_and_si64(y1,ymask);

		y0 = _mm_packs_pu16(y0,y1);
		v0 = _mm_packs_pu16(v0,v1);
		u0 = _mm_packs_pu16(u0,u1);
		v0 = _mm_packs_pu16(v0,v0);
		u0 = _mm_packs_pu16(u0,u0);

		*(__m64*)&ydst[a] = y0;
		*(int*)&vdst[a/2] = _mm_cvtsi64_si32(v0);
		*(int*)&udst[a/2] = _mm_cvtsi64_si32(u0);
	}

	for ( ;a<height*width;a+=2){
		ydst[a+0] = src[a*2+0];
		udst[a/2] = src[a*2+1];
		ydst[a+1] = src[a*2+2];
		vdst[a/2] = src[a*2+3];
	}

	_mm_empty();
}

void Split_UYVY_MMX(const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height){
	const __m64 cmask = _mm_set_pi32(0x000000FF,0x000000FF);

	unsigned int a;
	const unsigned int end = (width*height)&(~7);
	for ( a=0;a<end;a+=8){
		__m64 u0 = *(__m64*)&src[a*2+0];
		__m64 u1 = *(__m64*)&src[a*2+8];

		__m64 y0 = _mm_srli_pi16(u0,8);
		__m64 v0 = _mm_and_si64(_mm_srli_si64(u0,16),cmask);
		u0 = _mm_and_si64(u0,cmask);

		__m64 y1 = _mm_srli_pi16(u1,8);
		__m64 v1 = _mm_and_si64(_mm_srli_si64(u1,16),cmask);
		u1 = _mm_and_si64(u1,cmask);

		y0 = _mm_packs_pu16(y0,y1);
		v0 = _mm_packs_pu16(v0,v1);
		u0 = _mm_packs_pu16(u0,u1);
		v0 = _mm_packs_pu16(v0,v0);
		u0 = _mm_packs_pu16(u0,u0);

		*(__m64*)&ydst[a] = y0;
		*(int*)&vdst[a/2] = _mm_cvtsi64_si32(v0);
		*(int*)&udst[a/2] = _mm_cvtsi64_si32(u0);
	}

	for ( ;a<height*width;a+=2){
		udst[a/2] = src[a*2+0];
		ydst[a+0] = src[a*2+1];
		vdst[a/2] = src[a*2+2];
		ydst[a+1] = src[a*2+3];
	}

	_mm_empty();
}

void Block_Predict_MMX( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool rgbmode){

	unsigned int align_shift = (8 - ((unsigned int)source&7))&7;

	// predict the bottom row
	unsigned int a;
	__m64 t0 = _mm_setzero_si64();
	if ( align_shift ){
		dest[0]=source[0];
		for ( a=1;a<align_shift;a++){
			dest[a] = source[a]-source[a-1];
		}
		t0 = _mm_cvtsi32_si64(source[align_shift-1]);// load 1 byte
	}
	for ( a=align_shift;a<width;a+=8){
		__m64 x = *(__m64*)&source[a];
		t0 = _mm_or_si64(t0,_mm_slli_si64(x,8));
		*(__m64*)&dest[a]=_mm_sub_pi8(x,t0);
		t0 = _mm_srli_si64(x,7*8);
	}
	if ( width>=length ){
		_mm_empty();
		return;
	}

	__m64 z;
	__m64 x;

	if ( rgbmode ){
		x = _mm_setzero_si64();
		z = _mm_setzero_si64();
	} else {
		x = _mm_cvtsi32_si64(source[width-1]);
		z = _mm_cvtsi32_si64(source[0]);
	}

	// make sure that source[a] is a multiple of 8 so that only source[a-width]
	// will be unaligned if width is not a multiple of 8
	a = width;
	{
		__m64 srcs = *(__m64 *)&source[a];
		__m64 y = *(__m64 *)&source[a-width];

		x = _mm_or_si64(x,_mm_slli_si64(srcs,8));
		z = _mm_or_si64(z,_mm_slli_si64(y,8));

		__m64 tx = _mm_unpackhi_pi8(x,_mm_setzero_si64());
		__m64 ty = _mm_unpackhi_pi8(y,_mm_setzero_si64());
		__m64 tz = _mm_unpackhi_pi8(z,_mm_setzero_si64());

		tz = _mm_add_pi16(_mm_sub_pi16(tx,tz),ty);

		tx = _mm_unpacklo_pi8(x,_mm_setzero_si64());
		ty = _mm_unpacklo_pi8(y,_mm_setzero_si64());
		z = _mm_unpacklo_pi8(z,_mm_setzero_si64());
		z = _mm_add_pi16(_mm_sub_pi16(tx,z),ty);
		z = _mm_packs_pu16(z,tz);

		__m64 i = _mm_sub_pi8(x,_mm_subs_pu8(x,y));//min
		x = _mm_add_pi8(y,_mm_subs_pu8(x,y));//max
		i = _mm_add_pi8(z,_mm_subs_pu8(i,z));//max
		x = _mm_sub_pi8(i,_mm_subs_pu8(i,x));//min

		i = _mm_srli_si64(srcs,7*8);

		srcs = _mm_sub_pi8(srcs,x);

		z = _mm_srli_si64(y,7*8);
		x = i;

		*(__m64*)&dest[a]=srcs;
	}

	if ( align_shift ){
		a = align_round(a,8);
		a += align_shift;
		if ( a > width+8 ){
			a -= 8;
		}
	} else {
		a+=8;
		a&=(~7);
	}

	x = _mm_cvtsi32_si64(source[a-1]);
	z = _mm_cvtsi32_si64(source[a-width-1]);

	const unsigned int end = (length)&(~7);
	// main prediction loop, source[a] will be a multiple of 8
	if ( SSE ){
		for ( ;a<end;a+=8){

			__m64 srcs = *(__m64 *)&source[a];
			__m64 y = *(__m64 *)&source[a-width];

			x = _mm_or_si64(x,_mm_slli_si64(srcs,8));
			z = _mm_or_si64(z,_mm_slli_si64(y,8));

			__m64 tx = _mm_unpackhi_pi8(x,_mm_setzero_si64());
			__m64 ty = _mm_unpackhi_pi8(y,_mm_setzero_si64());
			__m64 tz = _mm_unpackhi_pi8(z,_mm_setzero_si64());

			tz = _mm_add_pi16(_mm_sub_pi16(tx,tz),ty);

			tx = _mm_unpacklo_pi8(x,_mm_setzero_si64());
			ty = _mm_unpacklo_pi8(y,_mm_setzero_si64());
			z = _mm_unpacklo_pi8(z,_mm_setzero_si64());
			z = _mm_add_pi16(_mm_sub_pi16(tx,z),ty);
			z = _mm_packs_pu16(z,tz);

			__m64 i = _mm_min_pu8(x,y);
			x = _mm_max_pu8(x,y);
			i = _mm_max_pu8(i,z);
			x = _mm_min_pu8(x,i);

			i = _mm_srli_si64(srcs,7*8);

			srcs = _mm_sub_pi8(srcs,x);

			z = _mm_srli_si64(y,7*8);
			x = i;

			*(__m64*)&dest[a]=srcs;
		}
	} else {
		for ( ;a<end;a+=8){

			__m64 srcs = *(__m64 *)&source[a];
			__m64 y = *(__m64 *)&source[a-width];

			x = _mm_or_si64(x,_mm_slli_si64(srcs,8));
			z = _mm_or_si64(z,_mm_slli_si64(y,8));

			__m64 tx = _mm_unpackhi_pi8(x,_mm_setzero_si64());
			__m64 ty = _mm_unpackhi_pi8(y,_mm_setzero_si64());
			__m64 tz = _mm_unpackhi_pi8(z,_mm_setzero_si64());

			tz = _mm_add_pi16(_mm_sub_pi16(tx,tz),ty);

			tx = _mm_unpacklo_pi8(x,_mm_setzero_si64());
			ty = _mm_unpacklo_pi8(y,_mm_setzero_si64());
			z = _mm_unpacklo_pi8(z,_mm_setzero_si64());
			z = _mm_add_pi16(_mm_sub_pi16(tx,z),ty);
			z = _mm_packs_pu16(z,tz);

			__m64 i = _mm_sub_pi8(x,_mm_subs_pu8(x,y));//min
			x = _mm_add_pi8(y,_mm_subs_pu8(x,y));//max
			i = _mm_add_pi8(z,_mm_subs_pu8(i,z));//max
			x = _mm_sub_pi8(i,_mm_subs_pu8(i,x));//min

			i = _mm_srli_si64(srcs,7*8);

			srcs = _mm_sub_pi8(srcs,x);

			z = _mm_srli_si64(y,7*8);
			x = i;

			*(__m64*)&dest[a]=srcs;
		}
	}
	_mm_empty();

	for ( ; a<length; a++ ){
		int x = source[a-1];
		int y = source[a-width];
		int z = x+y-source[a-width-1];
		dest[a] = source[a]-median(x,y,z);
	}
	
}

void Block_Predict_YUV16_MMX( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool is_y){

	unsigned int align_shift = (8 - ((unsigned int)source&7))&7;

	// predict the bottom row
	unsigned int a;
	__m64 t0 = _mm_setzero_si64();
	if ( align_shift ){
		dest[0]=source[0];
		for ( a=1;a<align_shift;a++){
			dest[a] = source[a]-source[a-1];
		}
		t0 = _mm_cvtsi32_si64(source[align_shift-1]);
	}
	for ( a=align_shift;a<width;a+=8){
		__m64 x = *(__m64*)&source[a];
		t0 = _mm_or_si64(t0,_mm_slli_si64(x,8));
		*(__m64*)&dest[a]=_mm_sub_pi8(x,t0);
		t0 = _mm_srli_si64(x,7*8);
	}

	if ( width>=length ){
		_mm_empty();
		return;
	}

	__m64 z = _mm_setzero_si64();
	__m64 x = _mm_setzero_si64();
	
	// make sure that source[a] is a multiple of 8 so that only source[a-width]
	// will be unaligned if width is not a multiple of 8
	a = width;
	{
		__m64 srcs = *(__m64 *)&source[a];
		__m64 y = *(__m64 *)&source[a-width];

		x = _mm_slli_si64(srcs,8);
		z = _mm_slli_si64(y,8);
		z = _mm_add_pi8(_mm_sub_pi8(x,z),y);

		__m64 i = _mm_sub_pi8(x,_mm_subs_pu8(x,y));//min
		x = _mm_add_pi8(y,_mm_subs_pu8(x,y));//max
		i = _mm_add_pi8(z,_mm_subs_pu8(i,z));//max
		x = _mm_sub_pi8(i,_mm_subs_pu8(i,x));//min

		srcs = _mm_sub_pi8(srcs,x);
		*(__m64*)&dest[a]=srcs;
	}

	if ( align_shift ){
		a = align_round(a,8);
		a += align_shift;
		if ( a > width+8 ){
			a -= 8;
		}
	} else {
		a+=8;
		a&=(~7);
	}

	x = _mm_cvtsi32_si64(source[a-1]);
	z = _mm_cvtsi32_si64(source[a-width-1]);

	const unsigned int end = (length)&(~7);

	// main prediction loop, source[a] will be a multiple of 8
	if ( SSE ){
		for ( ;a<end;a+=8){

			__m64 srcs = *(__m64 *)&source[a];
			__m64 y = *(__m64 *)&source[a-width];

			x = _mm_or_si64(x,_mm_slli_si64(srcs,8));
			z = _mm_or_si64(z,_mm_slli_si64(y,8));
			z = _mm_add_pi8(_mm_sub_pi8(x,z),y);

			__m64 i = _mm_min_pu8(x,y);
			x = _mm_max_pu8(x,y);
			i = _mm_max_pu8(i,z);
			x = _mm_min_pu8(x,i);

			i = _mm_srli_si64(srcs,7*8);

			srcs = _mm_sub_pi8(srcs,x);

			z = _mm_srli_si64(y,7*8);
			x = i;

			*(__m64*)&dest[a]=srcs;
		}
	} else {
		for ( ;a<end;a+=8){

			__m64 srcs = *(__m64 *)&source[a];
			__m64 y = *(__m64 *)&source[a-width];

			x = _mm_or_si64(x,_mm_slli_si64(srcs,8));
			z = _mm_or_si64(z,_mm_slli_si64(y,8));
			z = _mm_add_pi8(_mm_sub_pi8(x,z),y);

			__m64 i = _mm_sub_pi8(x,_mm_subs_pu8(x,y));//min
			x = _mm_add_pi8(y,_mm_subs_pu8(x,y));//max
			i = _mm_add_pi8(z,_mm_subs_pu8(i,z));//max
			x = _mm_sub_pi8(i,_mm_subs_pu8(i,x));//min

			i = _mm_srli_si64(srcs,7*8);

			srcs = _mm_sub_pi8(srcs,x);

			z = _mm_srli_si64(y,7*8);
			x = i;

			*(__m64*)&dest[a]=srcs;
		}
	}

	_mm_empty();

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

void Interleave_And_Restore_YUY2_MMX( unsigned char * __restrict output, const unsigned char * __restrict ysrc, const unsigned char * __restrict usrc, const unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height){

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

	if ( height==1)
		return;

	const unsigned int stride = width*2;
	unsigned int a=width/2+2;
	__m64 x = _mm_setr_pi8(output[a*4-2],output[a*4-3],0,output[a*4-1],0,0,0,0);
	__m64 z = _mm_setr_pi8(output[a*4-3-stride],output[a*4-2-stride],output[a*4-1-stride],0,0,0,0,0);
	const __m64 mask = _mm_setr_pi8(0,0,0,0,-1,0,0,0);

	// restore all the remaining pixels using median prediction
	if ( SSE ){
		// 54
		// 50
		// 49
		const __m64 mask2 = _mm_setr_pi8(0,0,-1,0,0,-1,0,-1);
		for ( ; a<(height*width)/2; a+=2){
			__m64 srcs = _mm_cvtsi32_si64( *(unsigned int *)&ysrc[a*2]);
			__m64 temp0 = _mm_insert_pi16(_mm_setzero_si64(),*(int*)&usrc[a],0);
			__m64 temp1 = _mm_insert_pi16(_mm_setzero_si64(),*(int*)&vsrc[a],0);
			srcs = _mm_unpacklo_pi8(srcs,_mm_unpacklo_pi8(temp0,temp1));

			__m64 y = *(__m64 *)&output[a*4-stride];

			z = _mm_or_si64(z,_mm_slli_si64(y,24)); // z=uyvyuyvy
			z = _mm_or_si64(_mm_srli_pi16(z,8),_mm_slli_pi16(z,8));// z = yuyvyuyv
			z = _mm_sub_pi8(_mm_add_pi8(x,y),z);

			__m64 i = _mm_min_pu8(x,y);//min
			__m64 j = _mm_max_pu8(x,y);//max
			i = _mm_max_pu8(i,z);//max
			j = _mm_min_pu8(i,j);//min
			j = _mm_add_pi8(j,srcs); // yu-v----
			// mask and shift j (yuv)
			j = _mm_shuffle_pi16(j,(0<<0)+(0<<2)+(0<<4)+(1<<6));
			j = _mm_and_si64(j,mask2);
			x = _mm_or_si64(x,j);
			z = _mm_add_pi8(z,j);

			i = _mm_min_pu8(x,y);//min
			j = _mm_max_pu8(x,y);//max
			i = _mm_max_pu8(i,z);//max
			j = _mm_min_pu8(i,j);//min
			j = _mm_add_pi8(j,srcs); // yuyv----

			// mask and shift j (y only)
			j = _mm_slli_si64(j,5*8);
			j = _mm_srli_pi32(j,3*8);

			x = _mm_or_si64(x,j);
			z = _mm_add_pi8(z,j);

			i = _mm_min_pu8(x,y);//min
			j = _mm_max_pu8(x,y);//max
			i = _mm_max_pu8(i,z);//max
			j = _mm_min_pu8(i,j);//min
			j = _mm_add_pi8(j,srcs); // yuyvyu-v
			// mask and shift j (y only)
			j = _mm_and_si64(j,mask);
			j = _mm_slli_si64(j,2*8);
			x = _mm_or_si64(x,j);
			z = _mm_add_pi8(z,j);

			i = _mm_min_pu8(x,y);//min
			x = _mm_max_pu8(x,y);//max
			i = _mm_max_pu8(i,z);//max
			x = _mm_min_pu8(i,x);//min
			x = _mm_add_pi8(x,srcs);
			*(__m64*)&output[a*4] = x;
			j = x;
			x = _mm_slli_si64(x,1*8);
			x = _mm_srli_si64(x,7*8);
			j = _mm_srli_pi16(j,1*8);
			j = _mm_srli_si64(j,3*8);
			x = _mm_or_si64(x,j);

			z = _mm_srli_si64(y,5*8);
		}
	} else {
		// 88
		// 64
		__declspec(align(8)) unsigned char temp[8]={0,0,0,0,0,0,0,0};
		for ( ; a<(height*width)/2; a+=2){
			__m64 srcs = _mm_cvtsi32_si64( *(unsigned int *)&ysrc[a*2]);
			temp[0]=usrc[a];
			temp[1]=vsrc[a];
			temp[2]=usrc[a+1];
			temp[3]=vsrc[a+1];
			srcs = _mm_unpacklo_pi8(srcs,_mm_cvtsi32_si64(*(int*)&temp[0]));

			__m64 y = *(__m64 *)&output[a*4-stride];

			z = _mm_or_si64(z,_mm_slli_si64(y,24)); // z=uyvyuyvy
			z = _mm_or_si64(_mm_srli_pi16(z,8),_mm_slli_pi16(z,8));// z = yuyvyuyv
			z = _mm_sub_pi8(_mm_add_pi8(x,y),z);

			__m64 i = _mm_sub_pi8(x,_mm_subs_pu8(x,y));//min
			__m64 j = _mm_add_pi8(y,_mm_subs_pu8(x,y));//max
			i = _mm_add_pi8(z,_mm_subs_pu8(i,z));//max
			j = _mm_sub_pi8(i,_mm_subs_pu8(i,j));//min
			j = _mm_add_pi8(j,srcs); // yu-v----
			// mask and shift j (y only)
			j = _mm_slli_si64(j,7*8);
			j = _mm_srli_si64(j,5*8);
			x = _mm_or_si64(x,j);
			z = _mm_add_pi8(z,j);

			i = _mm_sub_pi8(x,_mm_subs_pu8(x,y));//min
			j = _mm_add_pi8(y,_mm_subs_pu8(x,y));//max
			i = _mm_add_pi8(z,_mm_subs_pu8(i,z));//max
			j = _mm_sub_pi8(i,_mm_subs_pu8(i,j));//min
			j = _mm_add_pi8(j,srcs); // yuyv----

			// mask and shift j (yuv)
			i = _mm_slli_si64(j,3*8);
			i = _mm_slli_pi16(i,1*8);
			j = _mm_slli_si64(j,5*8);
			j = _mm_srli_pi32(j,3*8);
			j = _mm_or_si64(i,j);

			x = _mm_or_si64(x,j);
			z = _mm_add_pi8(z,j);

			i = _mm_sub_pi8(x,_mm_subs_pu8(x,y));//min
			j = _mm_add_pi8(y,_mm_subs_pu8(x,y));//max
			i = _mm_add_pi8(z,_mm_subs_pu8(i,z));//max
			j = _mm_sub_pi8(i,_mm_subs_pu8(i,j));//min
			j = _mm_add_pi8(j,srcs); // yuyvyu-v
			// mask and shift j (y only)
			j = _mm_and_si64(j,mask);
			j = _mm_slli_si64(j,2*8);
			x = _mm_or_si64(x,j);
			z = _mm_add_pi8(z,j);

			i = _mm_sub_pi8(x,_mm_subs_pu8(x,y));//min
			x = _mm_add_pi8(y,_mm_subs_pu8(x,y));//max
			i = _mm_add_pi8(z,_mm_subs_pu8(i,z));//max
			x = _mm_sub_pi8(i,_mm_subs_pu8(i,x));//min
			x = _mm_add_pi8(x,srcs);
			*(__m64*)&output[a*4] = x;
			j = x;
			x = _mm_slli_si64(x,1*8);
			x = _mm_srli_si64(x,7*8);
			j = _mm_srli_pi16(j,1*8);
			j = _mm_srli_si64(j,3*8);
			x = _mm_or_si64(x,j);

			z = _mm_srli_si64(y,5*8);
		}
	}

	_mm_empty();

}

void Interleave_And_Restore_RGB24_MMX( unsigned char * __restrict output, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height){

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
	//static HANDLE file = CreateFile("C:\\Users\\Lags\\Desktop\\lag_log.cvs",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
	//t1 = GetTime();

	__m64 x = _mm_setzero_si64();
	__m64 z = _mm_setzero_si64();

	const __m64 mask = _mm_setr_pi8(0,-1,0,0,-1,0,0,0);

	unsigned int w=0;
	unsigned int h=0;

	// if there is no need for padding, treat it as one long row
	const unsigned int vsteps = (width&3)?height:2;
	const unsigned int wsteps = (width&3)?(width&(~3)):(width*height-width-4);

	if ( SSE ){
		// 85	279
		//		218
		for ( h=1;h<vsteps;h++){
			w=0;
			for ( ;w<wsteps;w+=4){
				unsigned int a = stride*h+w*3;
				__m64 b = _mm_cvtsi32_si64( *(unsigned int *)&bsrc[width*h+w] );
				__m64 g = _mm_cvtsi32_si64( *(unsigned int *)&gsrc[width*h+w] );
				__m64 r = _mm_cvtsi32_si64( *(unsigned int *)&rsrc[width*h+w] );
				b = _mm_unpacklo_pi8(b,g);
				r = _mm_unpacklo_pi8(r,r);

				__m64 src2 = _mm_unpackhi_pi16(b,r);
				__m64 src1 = _mm_unpacklo_pi16(b,r);

				__m64 temp = *(__m64 *)&output[a-stride];
				b = _mm_and_si64(temp,mask);
				b = _mm_or_si64(_mm_srli_si64(b,8),_mm_slli_si64(b,8));
				b = _mm_add_pi8(b,temp);
				*(__m64 *)&output[a-stride] = b;
				__m64 y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
				temp = _mm_srli_si64(temp,24);
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				__m64 i = _mm_min_pi16(x,y);//min
				x = _mm_max_pi16(x,y);//max
				i = _mm_max_pi16(i,z);//max
				x = _mm_min_pi16(i,x);//min
				x = _mm_add_pi8(x,_mm_unpacklo_pi8(src1,_mm_setzero_si64()));
				src1 = _mm_unpackhi_pi8(src1,_mm_setzero_si64());

				z = y;
				y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
				temp = _mm_slli_si64(x,16);

				/*******/
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				i = _mm_min_pi16(x,y);//min
				x = _mm_max_pi16(x,y);//max
				i = _mm_max_pi16(i,z);//max
				x = _mm_min_pi16(i,x);//min
				x = _mm_add_pi8(x,src1);

				*(__m64 *)&output[a] = _mm_srli_si64(_mm_packs_pu16(temp,x),8);

				z = y;

				/*******/
				temp = *(__m64 *)&output[a+6-stride];
				y = _mm_and_si64(temp,mask);
				y = _mm_or_si64(_mm_srli_si64(y,8),_mm_slli_si64(y,8));
				y = _mm_add_pi8(y,temp);
				*(__m64 *)&output[a-stride+6] = y;
				y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
				temp = _mm_srli_si64(temp,24);
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				i = _mm_min_pi16(x,y);//min
				x = _mm_max_pi16(x,y);//max
				i = _mm_max_pi16(i,z);//max
				x = _mm_min_pi16(i,x);//min
				x = _mm_add_pi8(x,_mm_unpacklo_pi8(src2,_mm_setzero_si64()));
				src2 = _mm_unpackhi_pi8(src2,_mm_setzero_si64());

				z = y;
				y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
				temp = _mm_slli_si64(x,16);

				/*******/
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				i = _mm_min_pi16(x,y);//min
				x = _mm_max_pi16(x,y);//max
				i = _mm_max_pi16(i,z);//max
				x = _mm_min_pi16(i,x);//min
				x = _mm_add_pi8(x,src2);

				*(__m64 *)&output[a+6] = _mm_srli_si64(_mm_packs_pu16(temp,x),8);

				z = y;
			}
			
			for ( ;w<width;w++){
				// padding, no risk of overrunning when reading/writing 4 bytes

				unsigned int a = stride*h+w*3;

				__m64 src = _mm_cvtsi32_si64(bsrc[width*h+w]+(gsrc[width*h+w]<<8)+(rsrc[width*h+w]<<16));
				src = _mm_unpacklo_pi8(src,_mm_setzero_si64());
					
				__m64 y = _mm_cvtsi32_si64(*(int*)&output[a-stride]);
				y = _mm_unpacklo_pi8(y,_mm_setzero_si64());

				output[a+0-stride]+=output[a+1-stride];
				output[a+2-stride]+=output[a+1-stride];
				z = _mm_sub_pi16(_mm_add_pi16(x,y),z);

				__m64 i = _mm_min_pi16(x,y);//min
				x = _mm_max_pi16(x,y);//max
				i = _mm_max_pi16(i,z);//max
				x = _mm_min_pi16(i,x);//min
				x = _mm_add_pi8(x,src);

				*(int*)&output[a] = _mm_cvtsi64_si32(_mm_packs_pu16(x,x));

				z = y;
			}
		}
	} else {
		// 131	360
		// 97
		// 92
		for ( h=1;h<vsteps;h++){
			w=0;
			for ( ;w<wsteps;w+=4){
				unsigned int a = stride*h+w*3;
				__m64 b = _mm_cvtsi32_si64( *(unsigned int *)&bsrc[width*h+w] );
				__m64 g = _mm_cvtsi32_si64( *(unsigned int *)&gsrc[width*h+w] );
				__m64 r = _mm_cvtsi32_si64( *(unsigned int *)&rsrc[width*h+w] );
				b = _mm_unpacklo_pi8(b,g);
				r = _mm_unpacklo_pi8(r,r);

				__m64 src2 = _mm_unpackhi_pi16(b,r);
				__m64 src1 = _mm_unpacklo_pi16(b,r);

				__m64 temp = *(__m64 *)&output[a-stride];
				b = _mm_and_si64(temp,mask);
				b = _mm_or_si64(_mm_srli_si64(b,8),_mm_slli_si64(b,8));
				b = _mm_add_pi8(b,temp);
				*(__m64 *)&output[a-stride] = b;
				__m64 y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
				temp = _mm_srli_si64(temp,24);
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
				x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
				i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
				x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
				x = _mm_add_pi8(x,_mm_unpacklo_pi8(src1,_mm_setzero_si64()));
				src1 = _mm_unpackhi_pi8(src1,_mm_setzero_si64());

				z = y;
				y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
				temp = _mm_slli_si64(x,16);

				/*******/
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
				x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
				i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
				x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
				x = _mm_add_pi8(x,src1);

				*(__m64 *)&output[a] = _mm_srli_si64(_mm_packs_pu16(temp,x),8);

				z = y;

				/*******/
				temp = *(__m64 *)&output[a+6-stride];
				y = _mm_and_si64(temp,mask);
				y = _mm_or_si64(_mm_srli_si64(y,8),_mm_slli_si64(y,8));
				y = _mm_add_pi8(y,temp);
				*(__m64 *)&output[a-stride+6] = y;
				y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
				temp = _mm_srli_si64(temp,24);
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
				x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
				i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
				x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
				x = _mm_add_pi8(x,_mm_unpacklo_pi8(src2,_mm_setzero_si64()));
				src2 = _mm_unpackhi_pi8(src2,_mm_setzero_si64());

				z = y;
				y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
				temp = _mm_slli_si64(x,16);

				/*******/
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
				x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
				i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
				x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
				x = _mm_add_pi8(x,src2);

				*(__m64 *)&output[a+6] = _mm_srli_si64(_mm_packs_pu16(temp,x),8);

				z = y;
			}
			
			for ( ;w<width;w++){
				// padding, no risk of overrunning when reading/writing 4 bytes
				
				unsigned int a = stride*h+w*3;

				__m64 src = _mm_cvtsi32_si64(bsrc[width*h+w]+(gsrc[width*h+w]<<8)+(rsrc[width*h+w]<<16));
				src = _mm_unpacklo_pi8(src,_mm_setzero_si64());

				__m64 y = _mm_cvtsi32_si64(*(int*)&output[a-stride]);
				y = _mm_unpacklo_pi8(y,_mm_setzero_si64());

				output[a+0-stride]+=output[a+1-stride];
				output[a+2-stride]+=output[a+1-stride];
				z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

				__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
				x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
				i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
				x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
				x = _mm_add_pi8(x,src);


				*(int*)&output[a] = _mm_cvtsi64_si32(_mm_packs_pu16(x,x));

				z = y;
			}
		}
	}

	
	// handle very last pixels of a frame without padding seperate to avoid buffer overruns
	if ( (width&3)==0){
		h=height-1;
		w=width-4;
		unsigned int a = stride*h+w*3;
		__m64 b = _mm_cvtsi32_si64( *(unsigned int *)&bsrc[width*h+w] );
		__m64 g = _mm_cvtsi32_si64( *(unsigned int *)&gsrc[width*h+w] );
		__m64 r = _mm_cvtsi32_si64( *(unsigned int *)&rsrc[width*h+w] );
		b = _mm_unpacklo_pi8(b,g);
		r = _mm_unpacklo_pi8(r,r);

		__m64 src2 = _mm_unpackhi_pi16(b,r);
		__m64 src1 = _mm_unpacklo_pi16(b,r);

		__m64 temp = *(__m64 *)&output[a-stride];
		b = _mm_and_si64(temp,mask);
		b = _mm_or_si64(_mm_srli_si64(b,8),_mm_slli_si64(b,8));
		b = _mm_add_pi8(b,temp);
		*(__m64 *)&output[a-stride] = b;
		__m64 y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
		temp = _mm_srli_si64(temp,24);
		z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

		__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
		x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
		i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
		x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
		x = _mm_add_pi8(x,_mm_unpacklo_pi8(src1,_mm_setzero_si64()));
		src1 = _mm_unpackhi_pi8(src1,_mm_setzero_si64());

		z = y;
		y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
		temp = _mm_slli_si64(x,16);

		/*******/
		z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

		i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
		x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
		i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
		x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
		x = _mm_add_pi8(x,src1);

		*(__m64 *)&output[a] = _mm_srli_si64(_mm_packs_pu16(temp,x),8);

		z = y;

		/*******/
		temp = *(__m64 *)&output[a+6-stride];
		y = _mm_and_si64(temp,mask);
		y = _mm_or_si64(_mm_srli_si64(y,8),_mm_slli_si64(y,8));
		y = _mm_add_pi8(y,temp);
		*(__m64 *)&output[a-stride+6] = y;
		y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
		temp = _mm_srli_si64(temp,24);
		z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

		i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
		x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
		i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
		x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
		x = _mm_add_pi8(x,_mm_unpacklo_pi8(src2,_mm_setzero_si64()));
		src2 = _mm_unpackhi_pi8(src2,_mm_setzero_si64());

		z = y;
		y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
		temp = _mm_slli_si64(x,16);

		/*******/
		z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

		i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
		x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
		i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
		x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
		x = _mm_add_pi8(x,src2);
		x = _mm_srli_si64(_mm_packs_pu16(temp,x),8);

		*(int *)&output[a+6] = _mm_cvtsi64_si32(x);
		*(int *)&output[a+8] = _mm_cvtsi64_si32(_mm_srli_si64(x,16));
	}

	_mm_empty();

	// recorrilate the last row
	for ( unsigned int a=stride*(height-1);a<stride*height;a+=3){
		output[a]+=output[a+1];
		output[a+2]+=output[a+1];
	}
	
	//t2 = GetTime();
	//t2 -= t1;
	//char msg[128];
	//sprintf(msg,"%f\n",t2/100000.0);
	//unsigned long bytes;
	//WriteFile(file,msg,strlen(msg),&bytes,NULL);

}

void Interleave_And_Restore_RGB32_MMX( unsigned char * __restrict output, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height){

	const unsigned int stride = width*4;
	unsigned int a;
	{
		int r=0;
		int g=0;
		int b=0;
		for ( a=0;a<width;a++){
			output[a*4+0]=b+=bsrc[a];
			output[a*4+1]=g+=gsrc[a];
			output[a*4+2]=r+=rsrc[a];
			output[a*4+3]=255;
		}
	}

	if ( height==1)
		return;

	__m64 x = _mm_setzero_si64();
	__m64 z = _mm_setzero_si64();

	const unsigned int end = ((width*(height-1))&(~3))+width;


	if (  SSE ){
		// 95	260
		// 75	208
		for ( a=width;a<end;a+=4){
			__m64 b = _mm_cvtsi32_si64( *(unsigned int *)&bsrc[a] );
			__m64 g = _mm_cvtsi32_si64( *(unsigned int *)&gsrc[a] );
			__m64 r = _mm_cvtsi32_si64( *(unsigned int *)&rsrc[a] );
			b = _mm_unpacklo_pi8(b,g);
			r = _mm_unpacklo_pi8(r,_mm_setzero_si64());

			__m64 src2 = _mm_unpackhi_pi16(b,r);
			__m64 src1 = _mm_unpacklo_pi16(b,r);

			__m64 temp = *(__m64 *)&output[a*4-stride];
			b = _mm_shuffle_pi16(temp,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			b = _mm_srli_pi16(b,8);
			b = _mm_add_pi8(b,temp);
			*(__m64 *)&output[a*4-stride] = b;
			__m64 y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			__m64 i = _mm_min_pi16(x,y);//min
			x = _mm_max_pi16(x,y);//max
			i = _mm_max_pi16(i,z);//max
			x = _mm_min_pi16(i,x);//min
			x = _mm_add_pi8(x,_mm_unpacklo_pi8(src1,_mm_setzero_si64()));
			src1 = _mm_unpackhi_pi8(src1,_mm_setzero_si64());

			z = y;
			y = _mm_unpackhi_pi8(temp,_mm_setzero_si64());
			temp = x;

			/*******/
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_min_pi16(x,y);//min
			x = _mm_max_pi16(x,y);//max
			i = _mm_max_pi16(i,z);//max
			x = _mm_min_pi16(i,x);//min
			x = _mm_add_pi8(x,src1);

			*(__m64 *)&output[a*4] = _mm_packs_pu16(temp,x);

			z = y;

			/*******/
			temp = *(__m64 *)&output[a*4+8-stride];
			y = _mm_shuffle_pi16(temp,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			y = _mm_srli_pi16(y,8);
			y = _mm_add_pi8(y,temp);
			*(__m64 *)&output[a*4+8-stride] = y;
			y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_min_pi16(x,y);//min
			x = _mm_max_pi16(x,y);//max
			i = _mm_max_pi16(i,z);//max
			x = _mm_min_pi16(i,x);//min
			x = _mm_add_pi8(x,_mm_unpacklo_pi8(src2,_mm_setzero_si64()));
			src2 = _mm_unpackhi_pi8(src2,_mm_setzero_si64());

			z = y;
			y = _mm_unpackhi_pi8(temp,_mm_setzero_si64());
			temp = x;

			/*******/
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_min_pi16(x,y);//min
			x = _mm_max_pi16(x,y);//max
			i = _mm_max_pi16(i,z);//max
			x = _mm_min_pi16(i,x);//min
			x = _mm_add_pi8(x,src2);

			*(__m64 *)&output[a*4+8] = _mm_packs_pu16(temp,x);

			z = y;

		}
	} else {
		//135	340
		// 92

		const __m64 mask = _mm_setr_pi32(0x0000ff00,0x0000ff00);
		for ( a=width;a<end;a+=4){
			__m64 b = _mm_cvtsi32_si64( *(unsigned int *)&bsrc[a] );
			__m64 g = _mm_cvtsi32_si64( *(unsigned int *)&gsrc[a] );
			__m64 r = _mm_cvtsi32_si64( *(unsigned int *)&rsrc[a] );
			b = _mm_unpacklo_pi8(b,g);
			r = _mm_unpacklo_pi8(r,_mm_setzero_si64());

			__m64 src2 = _mm_unpackhi_pi16(b,r);
			__m64 src1 = _mm_unpacklo_pi16(b,r);

			__m64 temp = *(__m64 *)&output[a*4-stride]; // load y, then recorrilate B and R
			b = _mm_and_si64(temp,mask);
			b = _mm_or_si64(_mm_srli_pi32(b,8),_mm_slli_pi32(b,8));
			b = _mm_add_pi8(b,temp);
			*(__m64 *)&output[a*4-stride] = b;
			__m64 y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,_mm_unpacklo_pi8(src1,_mm_setzero_si64()));
			src1 = _mm_unpackhi_pi8(src1,_mm_setzero_si64());

			z = y;
			y = _mm_unpackhi_pi8(temp,_mm_setzero_si64());
			temp = x;

			/*******/
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,src1);

			*(__m64 *)&output[a*4] = _mm_packs_pu16(temp,x);

			z = y;

			/*******/
			temp = *(__m64 *)&output[a*4+8-stride];
			y = _mm_and_si64(temp,mask);
			y = _mm_or_si64(_mm_srli_pi32(y,8),_mm_slli_pi32(y,8));
			y = _mm_add_pi8(y,temp);
			*(__m64 *)&output[a*4+8-stride] = y;
			y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,_mm_unpacklo_pi8(src2,_mm_setzero_si64()));
			src2 = _mm_unpackhi_pi8(src2,_mm_setzero_si64());

			z = y;
			y = _mm_unpackhi_pi8(temp,_mm_setzero_si64());
			temp = x;

			/*******/
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,src2);

			*(__m64 *)&output[a*4+8] = _mm_packs_pu16(temp,x);

			z = y;
		}
	}

	for ( ;a < width*height;a++ ){
		__m64 src = _mm_cvtsi32_si64(bsrc[a]+(gsrc[a]<<8)+(rsrc[a]<<16));
		src = _mm_unpacklo_pi8(src,_mm_setzero_si64());

		__m64 y = _mm_cvtsi32_si64(*(int *)&output[a*4-stride]);
		output[a*4-stride+0] += output[a*4-stride+1];
		output[a*4-stride+2] += output[a*4-stride+1];
		y = _mm_unpacklo_pi8(y,_mm_setzero_si64());
		z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

		__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
		x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
		i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
		x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
		x = _mm_add_pi8(x,src);

		*(int *)&output[a*4] = _mm_cvtsi64_si32(_mm_packs_pu16(x,x));

		z = y;
	}

	_mm_empty();

	for ( a=stride*(height-1);a<stride*height;a+=4){
		output[a+0]+=output[a+1];
		output[a+2]+=output[a+1];
	}

}

void Interleave_And_Restore_RGBA_MMX( unsigned char * __restrict output, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned char * __restrict asrc, const unsigned int width, const unsigned int height){

	const unsigned int stride = width*4;
	unsigned int a=0;
	{
		int r=0;
		int g=0;
		int b=0;
		int alpha=0;
		for ( ;a<width;a++){
			output[a*4+0]=b+=bsrc[a];
			output[a*4+1]=g+=gsrc[a];
			output[a*4+2]=r+=rsrc[a];
			output[a*4+3]=alpha+=asrc[a];
		}
	}

	if ( height==1)
		return;

	__m64 x = _mm_setzero_si64();
	__m64 z = _mm_setzero_si64();

	const unsigned int end = ((width*(height-1))&(~3))+width;
	
	if ( SSE ){
		// 337
		// 218
		for ( a=width;a<end;a+=4){
			__m64 b = _mm_cvtsi32_si64( *(unsigned int *)&bsrc[a] );
			__m64 g = _mm_cvtsi32_si64( *(unsigned int *)&gsrc[a] );
			__m64 r = _mm_cvtsi32_si64( *(unsigned int *)&rsrc[a] );
			__m64 alpha = _mm_cvtsi32_si64( *(unsigned int *)&asrc[a] );
			b = _mm_unpacklo_pi8(b,g);
			r = _mm_unpacklo_pi8(r,alpha);

			__m64 src2 = _mm_unpackhi_pi16(b,r);
			__m64 src1 = _mm_unpacklo_pi16(b,r);

			__m64 temp = *(__m64 *)&output[a*4-stride];
			b = _mm_shuffle_pi16(temp,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			b = _mm_srli_pi16(b,8);
			b = _mm_add_pi8(b,temp);
			*(__m64 *)&output[a*4-stride] = b;

			__m64 y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			__m64 i = _mm_min_pi16(x,y);//min
			x = _mm_max_pi16(x,y);//max
			i = _mm_max_pi16(i,z);//max
			x = _mm_min_pi16(i,x);//min
			x = _mm_add_pi8(x,_mm_unpacklo_pi8(src1,_mm_setzero_si64()));
			src1 = _mm_unpackhi_pi8(src1,_mm_setzero_si64());

			z = y;
			y = _mm_unpackhi_pi8(temp,_mm_setzero_si64());
			
			temp = x;
			/*******/
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_min_pi16(x,y);//min
			x = _mm_max_pi16(x,y);//max
			i = _mm_max_pi16(i,z);//max
			x = _mm_min_pi16(i,x);//min
			x = _mm_add_pi8(x,src1);

			*(__m64 *)&output[a*4] = _mm_packs_pu16(temp,x);

			z = y;

			/*******/
			temp = *(__m64 *)&output[a*4+8-stride];
			y = _mm_shuffle_pi16(temp,(0<<0)+(0<<2)+(2<<4)+(2<<6));
			y = _mm_srli_pi16(y,8);
			y = _mm_add_pi8(y,temp);
			*(__m64 *)&output[a*4+8-stride] = y;

			y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_min_pi16(x,y);//min
			x = _mm_max_pi16(x,y);//max
			i = _mm_max_pi16(i,z);//max
			x = _mm_min_pi16(i,x);//min
			x = _mm_add_pi8(x,_mm_unpacklo_pi8(src2,_mm_setzero_si64()));
			src2 = _mm_unpackhi_pi8(src2,_mm_setzero_si64());

			z = y;
			y = _mm_unpackhi_pi8(temp,_mm_setzero_si64());
			temp = x;

			/*******/
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_min_pi16(x,y);//min
			x = _mm_max_pi16(x,y);//max
			i = _mm_max_pi16(i,z);//max
			x = _mm_min_pi16(i,x);//min
			x = _mm_add_pi8(x,src2);

			*(__m64 *)&output[a*4+8] = _mm_packs_pu16(temp,x);

			z = y;

		}
	} else {
		//130	376
		//124	319
		//104
		// 97
		const __m64 mask = _mm_setr_pi32(0x0000ff00,0x0000ff00);
		for ( a=width;a<end;a+=4){
			__m64 b = _mm_cvtsi32_si64( *(unsigned int *)&bsrc[a] );
			__m64 g = _mm_cvtsi32_si64( *(unsigned int *)&gsrc[a] );
			__m64 r = _mm_cvtsi32_si64( *(unsigned int *)&rsrc[a] );
			__m64 alpha = _mm_cvtsi32_si64( *(unsigned int *)&asrc[a] );
			b = _mm_unpacklo_pi8(b,g);
			r = _mm_unpacklo_pi8(r,alpha);

			__m64 src2 = _mm_unpackhi_pi16(b,r);
			__m64 src1 = _mm_unpacklo_pi16(b,r);

			__m64 temp = *(__m64 *)&output[a*4-stride];
			b = _mm_and_si64(temp,mask);
			b = _mm_or_si64(_mm_srli_pi32(b,8),_mm_slli_pi32(b,8));
			b = _mm_add_pi8(b,temp);
			*(__m64 *)&output[a*4-stride] = b;
			__m64 y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,_mm_unpacklo_pi8(src1,_mm_setzero_si64()));
			src1 = _mm_unpackhi_pi8(src1,_mm_setzero_si64());

			z = y;
			y = _mm_unpackhi_pi8(temp,_mm_setzero_si64());
			temp = x;

			/*******/
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,src1);

			*(__m64 *)&output[a*4] = _mm_packs_pu16(temp,x);

			z = y;

			/*******/
			temp = *(__m64 *)&output[a*4+8-stride];
			y = _mm_and_si64(temp,mask);
			y = _mm_or_si64(_mm_srli_pi32(y,8),_mm_slli_pi32(y,8));
			y = _mm_add_pi8(y,temp);
			*(__m64 *)&output[a*4+8-stride] = y;
			y = _mm_unpacklo_pi8(temp,_mm_setzero_si64());
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,_mm_unpacklo_pi8(src2,_mm_setzero_si64()));
			src2 = _mm_unpackhi_pi8(src2,_mm_setzero_si64());

			z = y;
			y = _mm_unpackhi_pi8(temp,_mm_setzero_si64());
			temp = x;

			/*******/
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,src2);

			*(__m64 *)&output[a*4+8] = _mm_packs_pu16(temp,x);

			z = y;

		}
	}
	for ( ;a < width*height;a++ ){
		__m64 src = _mm_cvtsi32_si64(bsrc[a]+(gsrc[a]<<8)+(rsrc[a]<<16)+(asrc[a]<<24));
		src = _mm_unpacklo_pi8(src,_mm_setzero_si64());

		__m64 y = _mm_cvtsi32_si64(*(int *)&output[a*4-stride]);
		output[a*4-stride+0] += output[a*4-stride+1];
		output[a*4-stride+2] += output[a*4-stride+1];
		y = _mm_unpacklo_pi8(y,_mm_setzero_si64());
		z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

		__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
		x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
		i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
		x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
		x = _mm_add_pi8(x,src);

		*(int *)&output[a*4] = _mm_cvtsi64_si32(_mm_packs_pu16(x,x));

		z = y;
	}

	_mm_empty();

	for ( a=stride*(height-1);a<stride*height;a+=4){
		output[a]+=output[a+1];
		output[a+2]+=output[a+1];
	}
}

void Restore_YV12_MMX(unsigned char * __restrict ysrc, unsigned char * __restrict usrc, unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height){
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
	//static HANDLE file = CreateFile("C:\\Documents and Settings\\Administrator\\Desktop\\lag_log.cvs",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
	//t1 = GetTime();

	if ( SSE ){
		// 71
		// 51
		const __m64 mask = _mm_set_pi32(0x00ff00ff,0x00ff00ff);

		__m64 x = _mm_setr_pi16(ysrc[width-1],usrc[width/2-1],vsrc[width/2-1],0);
		__m64 z = _mm_setr_pi16(ysrc[width-1]-ysrc[0],usrc[width/2-1]-usrc[0],vsrc[width/2-1]-vsrc[0],0);

		for ( a=width; a<width*height/4 + width/2; a+=2){

			__m64 ys = _mm_setzero_si64();
			ys = _mm_insert_pi16(ys,*(int*)&ysrc[a-width],0);
			ys = _mm_insert_pi16(ys,*(int*)&usrc[a-width/2-width/2],1);
			ys = _mm_insert_pi16(ys,*(int*)&vsrc[a-width/2-width/2],2);

			__m64 s = _mm_setzero_si64();
			s = _mm_insert_pi16(s,*(int*)&ysrc[a],0);
			s = _mm_insert_pi16(s,*(int*)&usrc[a-width/2],1);
			s = _mm_insert_pi16(s,*(int*)&vsrc[a-width/2],2);

			__m64 y = _mm_and_si64(ys,mask);
			ys = _mm_srli_pi16(ys,8);
			z = _mm_add_pi16(z,y);

			__m64 srcs = _mm_and_si64(s,mask);
			s = _mm_srli_pi16(s,8);

			__m64 i = _mm_min_pi16(x,y);
			x = _mm_max_pi16(x,y);
			i = _mm_max_pi16(i,z);
			x = _mm_min_pi16(x,i);
			x = _mm_add_pi8(x,srcs);
			__m64 r = x;

			srcs = s;
			z = _mm_sub_pi16(x,y);
			y = ys;
			z = _mm_add_pi16(z,y);

			i = _mm_min_pi16(x,y);
			x = _mm_max_pi16(x,y);
			i = _mm_max_pi16(i,z);
			x = _mm_min_pi16(x,i);
			x = _mm_add_pi8(x,srcs);
			r = _mm_or_si64(r,_mm_slli_pi16(x,8));

			*(unsigned short *)&ysrc[a] = _mm_extract_pi16(r,0);
			*(unsigned short *)&usrc[a-width/2] = _mm_extract_pi16(r,1);
			*(unsigned short *)&vsrc[a-width/2] = _mm_extract_pi16(r,2);

			z = _mm_sub_pi16(x,y);
		}

		unsigned int align4 = (a+3)&(~3);
		for ( ;a<align4;a++){
			int x = ysrc[a-1];
			int y = ysrc[a-width];
			int z = x+y-ysrc[a-width-1];
			x = median(x,y,z);
			ysrc[a]+=x;
		}

		x = _mm_cvtsi32_si64(ysrc[a-1]);
		z = _mm_cvtsi32_si64(ysrc[a-width-1]);
		for ( ;a<((width*height)&(~3));a+=4){
			__m64 src = _mm_cvtsi32_si64(*(int*)&ysrc[a]);
			__m64 y = _mm_cvtsi32_si64(*(int*)&ysrc[a-width]);
			y = _mm_unpacklo_pi8(y,_mm_setzero_si64());
			src = _mm_unpacklo_pi8(src,_mm_setzero_si64());

			z = _mm_or_si64(z,_mm_slli_si64(y,2*8));

			z = _mm_sub_pi16(_mm_add_pi16(x,y),z);

			
			__m64 i = _mm_min_pi16(x,y);
			__m64 j = _mm_max_pi16(x,y);
			i = _mm_max_pi16(i,z);
			j = _mm_min_pi16(j,i);
			j = _mm_add_pi8(j,src); // 1 correct
			// mask and shift j
			j = _mm_slli_si64(j,6*8);
			j = _mm_srli_si64(j,4*8);
			x = _mm_or_si64(x,j);
			z = _mm_add_pi16(z,j);

			i = _mm_min_pi16(x,y);
			j = _mm_max_pi16(x,y);
			i = _mm_max_pi16(i,z);
			j = _mm_min_pi16(j,i);
			j = _mm_add_pi8(j,src); // 2 correct
			// mask and shift j
			j = _mm_slli_si64(j,4*8);
			j = _mm_srli_pi32(j,2*8);
			x = _mm_or_si64(x,j);
			z = _mm_add_pi16(z,j);

			i = _mm_min_pi16(x,y);
			j = _mm_max_pi16(x,y);
			i = _mm_max_pi16(i,z);
			j = _mm_min_pi16(j,i);
			j = _mm_add_pi8(j,src); // 3 correct
			// mask and shift j
			j = _mm_srli_si64(j,4*8);
			j = _mm_slli_si64(j,6*8);
			x = _mm_or_si64(x,j);
			z = _mm_add_pi16(z,j);

			i = _mm_min_pi16(x,y);
			x = _mm_max_pi16(x,y);
			i = _mm_max_pi16(i,z);
			x = _mm_min_pi16(x,i);
			x = _mm_add_pi8(x,src); // 4 correct

			*(int*)&ysrc[a] = _mm_cvtsi64_si32(_mm_packs_pu16(x,x));

			x = _mm_srli_si64(x,6*8);
			z = _mm_srli_si64(y,6*8);
		}
	} else {
		// 80
		// 69
		const __m64 mask = _mm_set_pi32(0x00ff00ff,0x00ff00ff);

		__m64 x = _mm_setr_pi16(ysrc[width-1],usrc[width/2-1],vsrc[width/2-1],0);
		__m64 z = _mm_setr_pi16(ysrc[0],usrc[0],vsrc[0],0);
		__declspec(align(8)) unsigned char temp[24];

		for ( a=width; a<width*height/4 + width/2; a+=2){
			temp[0]=ysrc[a];
			temp[1]=ysrc[a+1];
			temp[2]=usrc[a-width/2];
			temp[3]=usrc[a-width/2+1];
			temp[4]=vsrc[a-width/2];
			temp[5]=vsrc[a-width/2+1];

			__m64 s = *(__m64*)&temp[0];

			__m64 srcs = _mm_and_si64(s,mask);
			s = _mm_srli_pi16(s,8);

			temp[8+0]=ysrc[a-width];
			temp[8+1]=ysrc[a-width+1];
			temp[8+2]=usrc[a-width/2-width/2];
			temp[8+3]=usrc[a-width/2-width/2+1];
			temp[8+4]=vsrc[a-width/2-width/2];
			temp[8+5]=vsrc[a-width/2-width/2+1];

			__m64 ys = *(__m64*)&temp[8];

			__m64 y = _mm_and_si64(ys,mask);
			ys = _mm_srli_pi16(ys,8);
			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));
			x = _mm_add_pi8(x,srcs);
			//x = _mm_and_si64(x,mask);
			__m64 r = x;

			srcs = s;
			z = _mm_subs_pu16(_mm_add_pi16(x,ys),y);
			y = ys;

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,srcs);
			r = _mm_or_si64(r,_mm_slli_pi16(x,8));
			//x = _mm_and_si64(x,mask);

			*(__m64*)&temp[16] = r;

			z = y;

			ysrc[a+0] = temp[16+0];
			ysrc[a+1] = temp[16+1];
			usrc[a-width/2+0] = temp[16+2];
			usrc[a-width/2+1] = temp[16+3];
			vsrc[a-width/2+0] = temp[16+4];
			vsrc[a-width/2+1] = temp[16+5];
		}
		unsigned int align4 = (a+3)&(~3);
		for ( ;a<align4;a++){
			int x = ysrc[a-1];
			int y = ysrc[a-width];
			int z = x+y-ysrc[a-width-1];
			ysrc[a]+=median(x,y,z);
		}

		x = _mm_cvtsi32_si64(ysrc[a-1]); // load 1 byte
		z = _mm_cvtsi32_si64(ysrc[a-width-1]); // load 1 byte
		const unsigned int end = (width*height)&(~3);
		for ( ;a<end;a+=4){
			__m64 src = _mm_cvtsi32_si64(*(int*)&ysrc[a]);
			__m64 y = _mm_cvtsi32_si64(*(int*)&ysrc[a-width]);
			y = _mm_unpacklo_pi8(y,_mm_setzero_si64());
			src = _mm_unpacklo_pi8(src,_mm_setzero_si64());

			__m64 zy = _mm_or_si64(z,_mm_slli_si64(y,2*8));

			z = _mm_subs_pu16(_mm_add_pi16(x,y),z);

			__m64 i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			__m64 j = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			j = _mm_sub_pi16(i,_mm_subs_pu16(i,j));//min
			j = _mm_add_pi8(j,src); // 1 correct
			// mask and shift j
			j = _mm_slli_si64(j,6*8);
			j = _mm_srli_si64(j,4*8);
			x = _mm_or_si64(x,j);
			z = _mm_subs_pu16(_mm_add_pi16(x,y),zy);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			j = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			j = _mm_sub_pi16(i,_mm_subs_pu16(i,j));//min
			j = _mm_add_pi8(j,src); // 2 correct
			// mask and shift j
			j = _mm_slli_si64(j,4*8);
			j = _mm_srli_pi32(j,2*8);
			x = _mm_or_si64(x,j);
			z = _mm_subs_pu16(_mm_add_pi16(x,y),zy);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			j = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			j = _mm_sub_pi16(i,_mm_subs_pu16(i,j));//min
			j = _mm_add_pi8(j,src); // 3 correct
			// mask and shift j
			j = _mm_srli_si64(j,4*8);
			j = _mm_slli_si64(j,6*8);
			x = _mm_or_si64(x,j);
			z = _mm_subs_pu16(_mm_add_pi16(x,y),zy);

			i = _mm_sub_pi16(x,_mm_subs_pu16(x,y));//min
			x = _mm_add_pi16(y,_mm_subs_pu16(x,y));//max
			i = _mm_add_pi16(z,_mm_subs_pu16(i,z));//max
			x = _mm_sub_pi16(i,_mm_subs_pu16(i,x));//min
			x = _mm_add_pi8(x,src); // 4 correct

			*(int*)&ysrc[a] = _mm_cvtsi64_si32(_mm_packs_pu16(x,x));

			x = _mm_srli_si64(x,6*8);
			z = _mm_srli_si64(y,6*8);
		}
	}
	_mm_empty();

	for ( ;a<width*height;a++){
		int x = ysrc[a-1];
		int y = ysrc[a-width];
		int z = x+y-ysrc[a-width-1];
		ysrc[a]+=median(x,y,z);
	}

	//t2 = GetTime();
	//t2 -= t1;
	//char msg[128];
	//sprintf(msg,"%f\n",t2/100000.0);
	//unsigned long bytes;
	//WriteFile(file,msg,strlen(msg),&bytes,NULL);
}
#endif