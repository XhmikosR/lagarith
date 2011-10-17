//This file contains functions that perform and undo median predition.

#pragma once

// Used to compare and select between SSE and SSE2 versions of functions
struct Performance{
	int count;
	bool prefere_a;
	unsigned __int64 time_a;
	unsigned __int64 time_b;
};

void Block_Predict( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool rgbmode);
void Block_Predict_YUV16( const unsigned char * __restrict source, unsigned char * __restrict dest, const unsigned int width, const unsigned int length, const bool is_y);

void Decorrilate_And_Split_RGB24(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height, Performance * performance);
void Decorrilate_And_Split_RGB32(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, const unsigned int width, const unsigned int height, Performance * performance);
void Decorrilate_And_Split_RGBA(const unsigned char * __restrict in, unsigned char * __restrict rdst, unsigned char * __restrict gdst, unsigned char * __restrict bdst, unsigned char * __restrict adst, const unsigned int width, const unsigned int height, Performance * performance);
void Split_YUY2(const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height, Performance * performance);
void Split_UYVY(const unsigned char * __restrict src, unsigned char * __restrict ydst, unsigned char * __restrict udst, unsigned char * __restrict vdst, const unsigned int width, const unsigned int height, Performance * performance);

void Interleave_And_Restore_RGB24( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height, Performance * performance);
void Interleave_And_Restore_RGB32( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned int width, const unsigned int height, Performance * performance);
void Interleave_And_Restore_RGBA( unsigned char * __restrict out, const unsigned char * __restrict rsrc, const unsigned char * __restrict gsrc, const unsigned char * __restrict bsrc, const unsigned char * __restrict asrc, const unsigned int width, const unsigned int height, Performance * performance);
void Interleave_And_Restore_YUY2( unsigned char * __restrict out, const unsigned char * __restrict ysrc, const unsigned char * __restrict usrc, const unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height, Performance * performance);
void Restore_YV12( unsigned char * __restrict ysrc, unsigned char * __restrict usrc, unsigned char * __restrict vsrc, const unsigned int width, const unsigned int height, Performance * performance);

void Interleave_And_Restore_Old_Unaligned(unsigned char * bsrc, unsigned char * gsrc, unsigned char * rsrc, unsigned char * dst, unsigned char * buffer, bool rgb24, unsigned int width, unsigned int height);
void Double_Resolution(const unsigned char * src, unsigned char * dst, unsigned char * buffer, const unsigned int width, const unsigned int height);
