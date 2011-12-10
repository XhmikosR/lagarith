//	Lagarith v1.3.27, copyright 2011 by Ben Greenwood.
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

#ifndef _COMPACT_CPP
#define _COMPACT_CPP

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <math.h>
#include "lagarith.h"
#include "fibonacci.h"
#include "zerorle.h"

// scale the byte probablities so the cumulative
// probabilty is equal to a power of 2
void CompressClass::Scaleprob(unsigned int length){
	assert(length > 0);
	assert(length < 0x80000000);

	unsigned int temp = 1;
	while ( temp < length )
		temp<<=1;

	int a;

	if ( temp != length ){
		double factor = ((int)temp)/((double)(int)length);
		double temp_array[256];

		
		for ( a = 0; a < 256; a++ ){
			temp_array[a] = ((int)prob_ranges[a+1])*factor;
		}
		for ( a = 0; a < 256; a++ ){
			prob_ranges[a+1]=(int)temp_array[a];
		}
		unsigned int newlen=0;
		for ( a = 1; a < 257; a++ ){
			newlen+=prob_ranges[a];
		}

		newlen = temp-newlen;
		assert(newlen < 0x80000000);

		if ( newlen & 0x80000000 ){ // should never happen
			prob_ranges[1]+=newlen;
			newlen=0;
		}

		a =0;
		unsigned int b=0;
		while ( newlen  ){
			if ( prob_ranges[b+1] ){
				prob_ranges[b+1]++;
				newlen--;
			}
			b++;
			b&=0x7f;
		}
	}

	for ( a =0; temp; a++)
		temp>>=1;
	scale =a-1;
	for ( a = 1; a < 257; a++ ){
		prob_ranges[a]+=prob_ranges[a-1];
	}
}

// read the byte frequency header
unsigned int CompressClass::Readprob(const unsigned char * in){

	unsigned int length=0;
	unsigned int skip;

	//prob_ranges[0]=0;
	memset(prob_ranges,0,sizeof(prob_ranges));

	skip = FibonacciDecode(in,&prob_ranges[1]);
	if ( !skip )
		return 0;

	for (unsigned int a =1;a< 257; a++){
		length+=prob_ranges[a];
	}

	if ( !length )
		return 0;

	Scaleprob(length);
	return skip;
}

// Determine the frequency of each byte in a byte stream; the frequencies are then scaled
// so the total is a power of 2. This allows binary shifts to be used instead of some
// multiply and divides in the compression/decompression routines.
// If out is set, the freqencies are also written and the output size returned.
unsigned int CompressClass::Calcprob(const unsigned char * const in, unsigned int length, unsigned char * out){

	unsigned int table2[256];
	memset(prob_ranges,0,257*sizeof(unsigned int));
	memset(table2,0,sizeof(table2));
	unsigned int a=0;
	for (a=0; a < (length&(~1)); a+=2 ){
		prob_ranges[in[a]+1]++;
		table2[in[a+1]]++;
	}
	if ( a < length ){
		prob_ranges[in[a]+1]++;
	}
	for ( a=0;a<256;a++){
		prob_ranges[a+1]+=table2[a];
	}
 
	// Clamp prob_ranges total to 1<<19 to ensure the range coder has enough precision to work with;
	// Larger totals reduce compression due to the reduced precision of the range variable, and
	// totals larger than 1<<22 can crash the range coder. Lower clamp values reduce the decoder speed
	// slightly on affected video.
	const int clamp_size = 1<<19;
	if ( out && length > clamp_size ){
		double factor = clamp_size;
		factor/=length;
		double temp[256];
		for ( int a=0;a<256;a++){
			temp[a]=((int)prob_ranges[a+1])*factor;
		}
		unsigned int total=0;
		for ( int a=0;a<256;a++){
			int scaled_val=(int)temp[a];
			if ( scaled_val== 0 && prob_ranges[a+1] ){
				scaled_val++;
			}
			total+=scaled_val;
			prob_ranges[a+1]=scaled_val;
		}
		int adjust = total<clamp_size?1:-1;
		int a=0;
		while ( total != clamp_size){
			if ( prob_ranges[a+1]>1){
				prob_ranges[a+1]+=adjust;
				total+=adjust;
			}
			a++;
			a&=255;
		}
		length = clamp_size;
	}

	unsigned int size=0;
	if ( out != NULL ){
		size = FibonacciEncode(&prob_ranges[1],out);
	}

	Scaleprob(length);

	return size;
}

// This function encapsilates compressing a byte stream.
// It applies a modified run length encoding if it saves enough bytes, 
// writes the frequency header, and range compresses the byte stream.

// !!!Warning!!! in[length] through in[length+3] will be thrashed
// in[length+8] must be readable
unsigned int CompressClass::Compact( unsigned char * in, unsigned char * out, const unsigned int length){

	unsigned int bytes_used=0;

	unsigned char * const buffer_1 = buffer;
	unsigned char * const buffer_2 = buffer+align_round(length*3/2+16,16);

	int rle=0;
	unsigned int size = TestAndRLE(in,buffer_1,buffer_2,length,&rle);

	out[0]=rle;
	if ( rle ){
		if (rle == -1 ){ // solid run of 0s, only 1st byte is needed
			out[0]=0xFF;
			out[1]=in[0];
			bytes_used= 2;

		} else {
			unsigned char * b2;
			if ( rle == 1 ){
				b2 = buffer_1;
			} else {
				b2 = buffer_2;
			}

			*(UINT32*)(out+1)=size;
			unsigned int skip = Calcprob(b2,size,out+5);


			skip += Encode(b2,out+5+skip,size) + 5;

			if ( size < skip ) { // RLE size is less than range compressed size
				out[0]+=4;
				memcpy(out+1,b2,size);
				skip=size+1;
			}
			bytes_used = skip;
		}
	} else {
		unsigned int skip = Calcprob(in, length, out+1);
		skip += Encode(in,out+skip+1,length) +1;
		bytes_used=skip;
	}
	assert(bytes_used>=2);
	assert(rle <=3 && rle >= -1);
	assert(out[0] == rle || rle == -1 || out[0]==rle+4 );

	return bytes_used;
}

// this function encapsulates decompressing a byte stream
void CompressClass::Uncompact( const unsigned char * in, unsigned char * out, const unsigned int length){

	int rle = in[0];
	if ( rle && ( rle < 8 || rle == 0xff ) ){ // any other values may indicate a corrupted RLE level from 1.3.0 or 1.3.1	
		if ( rle < 4 ){
			
			unsigned int size = *( UINT32 *)(in+1);
			if ( size >= length ){ // should not happen, most likely a corrupted 1.3.x encoding
				assert(false);
				int skip;
				skip = Readprob(in+1);
				if ( !skip )
					return;
				Decode_And_DeRLE(in+skip+1,out,length,0);
			} else {	
				int skip;
				skip = Readprob(in+5);

				if ( !skip )
					return;
				Decode_And_DeRLE(in+4+skip+1,out,length,in[0]);
				//zero::deRLE(buffer,out,length,in[0]);
			}
		} else {
			if ( rle == 0xff ){ // solid run of 0s, only need to set 1 byte
				memset(out,0,length);
				out[0]=in[1];
			} else { // RLE length is less than range compressed length
				rle-=4;
				if ( rle )
					deRLE(in+1,out,length,rle);
				else // uncompressed length is smallest...
					memcpy((void*)(in+1),out,length);
			}
		} 
	} else {
		
#ifndef LAGARITH_RELEASE
		if ( in[0] ){
			char msg[128];
			sprintf_s(msg,128,"Error! in[0] = %d",in[0]);
			MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
			memset(out,0,length);
			return;
		}
#endif

		int skip = *(int*)(in+1);

		// Adobe Premeire Pro tends to zero out the data when multi-threading is enabled...
		// This will help avoid errors
		if ( !skip ) 
			return;
		skip = Readprob(in+1);
		if ( !skip )
			return;
		Decode_And_DeRLE(in+skip+1,out,length,0);
	}
}

// initalized the buffers used by RLE and range coding routines
bool CompressClass::InitCompressBuffers(const unsigned int length){
	// buffer must be large enough to hold all 3 RLE levels at their worst case
	buffer = (unsigned char *)lag_aligned_malloc(buffer,length*3/2+length*5/4+32,8,"Compress::temp");
	if ( !buffer ){
		FreeCompressBuffers();
		return false;
	}
	return true;
}

// free the buffers used by RLE and range coding routines
void CompressClass::FreeCompressBuffers(){	
	lag_aligned_free( buffer,"Compress::buffer");
}

CompressClass::CompressClass(){
	buffer=NULL;
}

CompressClass::~CompressClass(){
	FreeCompressBuffers();
}

//MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);

#endif