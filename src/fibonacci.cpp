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

#ifndef _FIBONACCI_CPP
#define _FIBONACCI_CPP

#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <windows.h>
#include <assert.h>

// This file is used to compress the header without needing a header for the header...
// This is acheived due to the fact that the byte frequencies will be concentrated around
// 0 and fall off rapidly, resulting in the majority of the byte frequencies being single
// digit numbers or zero. Fibonacci coding is a varible bit method for encoding integers, with
// large numbers taking more bits than small numbers. Here, 0 takes two bits to encode,
// while a number like 200,000 takes about 28 bits. Additonally, if a frequency of zero is
// encountered, the number of zero freqencies that follows are encoded and the encode position
// moved accordingly. This is pretty much the same as the zero RLE done on the byte streams 
// themselves, except the run is encoded using Fibonacci coding.

#define writeBit(x) { \
	if ( x) \
		out[pos]|=bitpos;\
	bitpos>>=1;\
	if ( !bitpos){ \
		bitpos=0x80;\
		pos++;\
		out[pos]=0;\
	}\
}

unsigned int FibonacciEncode(unsigned int * in, unsigned char * out){
	unsigned char output[12];
	unsigned int pos=0;
	unsigned int bitpos=0x80;
	out[0]=0;
	unsigned int series[]={1,2,3,5,8,13,21};
	for ( unsigned int b =0; b < 256; b++ ){
		memset(output,0,12);
		unsigned int num = in[b]+1; // value needs to be >= 1
		
		// calculate Fibonacci part
		unsigned int a;
		unsigned int bits =32;
		for( ; !(num & 0x80000000) && bits; bits-- )
			num<<=1;
		num<<=1;
		assert(bits);
		unsigned int bits2=bits;

		while(bits){
			for ( a =0; series[a]<= bits; a++ );
			a--;
			output[a]=1;
			bits-=series[a];
		}
		for ( a =7; !output[a]; a-- );
		a++;
		output[a]=1;
		for ( unsigned int x = 0; x <= a; x++ )
			writeBit( output[x] );
		bits2--;

		//write numerical part
		for (unsigned int place = 0x80000000 ; bits2; bits2--){
			writeBit( num&place);
			place>>=1;
		}
		if ( in[b]==0 ){
			unsigned int i;
			for ( i=1;i+b<256&&in[b+i]==0;i++);
			b+=i-1;
			memset(output,0,12);
			
			// calculate Fibonacci part
			bits =32;
			num=i;
			for( ; !(num & 0x80000000) && bits; bits-- )
				num<<=1;

			assert(bits > 0 );

			num<<=1;
			bits2=bits;
			while(bits){
				for ( a =0; series[a]<= bits; a++ );
				a--;
				output[a]=1;
				bits-=series[a];
			}
			for ( a =7; !output[a]; a-- );
			a++;
			output[a]=1;
			for ( unsigned int x = 0; x <= a; x++ )
				writeBit( output[x] );
			bits2--;

			//write numerical part
			for (unsigned int place = 0x80000000 ; bits2; bits2--){
				writeBit( num&place);
				place>>=1;
			}
		}
	}
	return pos+(bitpos!=0x80);
}

#define readBit() { \
	bit = bitpos & in[pos];\
	bitpos >>=1;\
	if ( !bitpos ){\
		pos++;\
		bitpos = 0x80;\
	}\
}

unsigned int FibonacciDecode(const unsigned char * in, unsigned int * out){
	unsigned int pos =0;
	unsigned int bit;
	unsigned int bitpos = 0x80;
	unsigned int series[]={1,2,3,5,8,13,21};

	for ( unsigned int b =0; b < 256; b++ ){
		bit =0;
		unsigned int prevbit=0;
		unsigned int bits=0;
		unsigned int a=0;
		for ( ; !(prevbit && bit ); a++ ){

			prevbit = bit;
			readBit();
			if ( bit && !prevbit )
				bits+=series[a];
		}
		bits--;
		unsigned int value=1;
		for ( a= 0;a < bits; a++ ){
			readBit();
			value<<=1;
			if ( bit )
				value++;
		}
		value--;
		out[b]= value;
		if ( value == 0){

			bit =0;
			prevbit=0;
			bits=0;
			for ( a =0; !(prevbit && bit); a++ ){
				prevbit = bit;
				readBit();

				if ( bit && !prevbit )
					bits+=series[a];
			}
			bits--;
			value=1;

			for ( a= 0;a < bits; a++ ){
				readBit();
				value<<=1;
				if ( bit )
					value++;
			}
			if ( b+value >= 257 )
				value = 256-b;
			b+=value-1;	
		}
	}
	return pos+(bitpos!=0x80);
}
#endif