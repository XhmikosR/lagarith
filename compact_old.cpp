#pragma once

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


// !!!DO NOT CHANGE THIS FILE!!!
// !!!DO NOT USE ANY FUNCTIONS OTHER THAN ObsoleteUncompact!!!
// This file is maintained for backwards compatiblity with 1.0.x versions of Lagarith.
// It must remain bug-for-bug compatable; and changing it will have no benifits for
// current encodings.

#include <stdlib.h>
#include <memory.h>

int ObsoleteDeRLEZero( unsigned char * in, unsigned char * out, int length,int level){

	int a=0;
	int b=0;
	if ( level == 1 ){
		for ( ; b < length; a++ ){
			if ( in[a] ){
				out[b++]=in[a];
			} else {
				int x;
				int run=in[a+1];
				if ( run < 0x80 ){
					run*=2;
				} else{ 
					run=-run;
					run*=2;
					run--;
					run&=0xff;
				}
				for ( x = 0; x<=run;x++)
					out[b+x]=0;
				b+=x;
				a++;
			}
		}
		return a;
	} else if ( level == 2 ){
		while ( b < length){
			if ( in[a] ) {
				out[b++]=in[a++];
			} else if ( in[a+1] ){
				out[b]=0;
				out[b+1]=in[a+1];
				b+=2;
				a+=2;
			} else {
				int run=in[a+2];
				if ( run < 0x80 ){
					run*=2;
				} else{ 
					run=-run;
					run*=2;
					run--;
					run&=0xff;
				}
				int x=0;
				for (; x < run+2;x++)
					out[b+x]=0;
				b+=x;
				a+=3;	
			}
		}
		return a;
	} else if (level==3){
		while ( b < length){
			if ( in[a] ) {
				out[b++]=in[a++];
			} else if ( in[a+1] ){
				out[b]=0;
				out[b+1]=in[a+1];
				b+=2;
				a+=2;
			} else if ( in[a+2] ){
				out[b]=0;
				out[b+1]=0;
				out[b+2]=in[a+2];
				b+=3;
				a+=3;
			} else {
				int run=in[a+3];
				if ( run < 0x80 ){
					run*=2;
				} else{ 
					run=-run;
					run*=2;
					run--;
					run&=0xff;
				}
				int x=0;
				for (; x < run+3;x++)
					out[b+x]=0;
				b+=x;
				a+=4;
			}
		}
		return a;
	}
	return 0;
}

void ObsoleteDecode(const unsigned char *xin, unsigned char *xout, int length, unsigned int * ranges, int scale){

	xin++;
	const unsigned int Top_value = 0x80000000;
	const unsigned int Bottom_value = 0x00800000;
	const unsigned int Shift_bits = 23;
	const unsigned int Extra_bits = 7;

	unsigned int buffer = (*xin++);
    unsigned int xlow = buffer >> ( 8 - Extra_bits );
    unsigned int range = 1 << Extra_bits;
	unsigned char * ending = xout+length;

	//decoder renormalize
	for ( ; xout < ending; xout++){
		while ( range <= Bottom_value ) {
			xlow = ( xlow << 8 ) | ( ( buffer << Extra_bits ) & 0xFF );
			buffer = (*xin++);
			xlow = xlow | ( buffer >> ( 8 - Extra_bits) );
			range <<= 8;
		}

		unsigned int help = range >> scale;
		unsigned int tmp = xlow / help;

		int x  = ( tmp >= ranges[128] )*128;
			x += ( tmp >= ranges[x+64])*64;
			x += ( tmp >= ranges[x+32])*32;
			x += ( tmp >= ranges[x+16])*16;
			x += ( tmp >= ranges[x+8] )*8;
			x += ( tmp >= ranges[x+4] )*4;
			x += ( tmp >= ranges[x+2] )*2;
			x += ( tmp >= ranges[x+1] );

		*xout=x;

		//update decode state
		tmp = help * ranges[x];
		xlow -= tmp;
		if (ranges[x+1] < ranges[256] ){
			range = help * (ranges[x+1]-ranges[x]);
		} else {
			range -= tmp;
		}
	}
}

#define readBit() { \
	bit = bitpos & in[pos];\
	bitpos >>=1;\
	if ( !bitpos ){\
		pos++;\
		bitpos = 0x80;\
	}\
}

int ObsoleteFibonacciDecode( const unsigned char * in, int * out, int length){
	int pos =0;
	int bit;
	int bitpos = 0x80;
	int series[]={1,2,3,5,8,13,21,34};
	char input[32];
	memset(input,0,32);

	for ( int b =0; b < length; b++ ){
		bit =0;
		int prevbit=0;
		int bits=0;
		memset(input,0,32);
		for ( int a =0; !(prevbit && bit); a++ ){
			prevbit = bit;
			readBit();
			input[a]=bit;
			if ( bit && !prevbit )
				bits+=series[a];
		}
		bits--;
		int value=1;
		for ( int a= 0;a < bits; a++ ){
			readBit();
			value<<=1;
			if ( bit )
				value++;
		}
		out[b]= value-1;
	}
	return pos+1;
}

int ObsoleteDeRLE( const unsigned char * in, unsigned char * out, int length,int level){
	int a=0;
	int b=0;
	int prev = !in[0];
	if ( level == 1 ){
		for ( ; b < length; a+=2 ){
			int x;
			int run=in[a+1];
			if ( run < 0x80 ){
				run*=2;
			} else{ 
				run=-run;
				run*=2;
				run--;
				run&=0xff;
			}
			for ( x = 0; x<=run;x++)
				out[b+x]=in[a];
			b+=x;
		}
		return a;
	} else if ( level == 2 ){
		while ( b < length){
			if ( in[a]!= prev ) {
				out[b++]=in[a];
				prev = in[a];
				a++;
			} else {
				int run=in[a+1];
				if ( run < 0x80 ){
					run*=2;
				} else{ 
					run=-run;
					run*=2;
					run--;
					run&=0xff;
				}
				int x=0;
				for (; x <= run;x++)
					out[b+x]=prev;
				b+=x;
				a+=2;
			}
		}
		return a;
	} else if (level==3){
		while ( b < length){
			if ( in[a]!=prev ) {
				out[b++]=in[a];
				prev = in[a++];
			} else if ( in[a+1]!=prev ){
				out[b]=in[a];
				out[b+1]=in[a+1];
				prev = in[a+1];
				b+=2;
				a+=2;
			} else {

				int run=in[a+2];
				if ( run < 0x80 ){
					run*=2;
				} else{ 
					run=-run;
					run*=2;
					run--;
					run&=0xff;
				}
				int x=0;
				for (; x < run+2;x++)
					out[b+x]=prev;
				b+=x;
				a+=3;
			}
		}
		return a;
	}
	return 0;
}

int ObsoleteReadProb(const unsigned char * in, unsigned int * ranges, int * scale){
	int length=0;
	ranges[0]=0;
	int bytecounts[260];
	int skip;
	if ( in[0] ){
		int size = *(int*)(in+1);
		unsigned char header[4096];
		skip = ObsoleteDeRLE(in+5,header,size,in[0]);
		skip += 5;
		ObsoleteFibonacciDecode(header,bytecounts,256);
	} else {
		skip= ObsoleteFibonacciDecode(in+1,bytecounts,256)+1;
	}

	int a;
	for ( a =0; a < 256; a++ )
		ranges[a+1]=bytecounts[a];

	for ( a =1;a< 257; a++)
		length+=ranges[a];

	int temp = length;
	while ( temp > 1 )
		temp/=2;
	while ( temp < length )
		temp*=2;
	
	double factor = temp/(double)length;

	for ( a = 1; a < 257; a++ )
		ranges[a]= (int)(ranges[a]*factor);
	int newlen=0;
	for ( a = 1; a < 257; a++ )
		newlen+=ranges[a];

	newlen= temp-newlen;
	if ( newlen < 0 ){
		ranges[1]+=newlen;
		newlen=0;
	}
	
	a =0;
	unsigned char b=0;
	while ( newlen  ){
		ranges[b]++;
		newlen--;
		a++;
		if ( a%2 )
			b= -((a+1)/2);
		else 
			b=a/2;
		if ( a == 256 ){
			a=0; 
			b=0;
		}
	}
	*scale = temp;
	temp=0;
	for ( a =1; a < 257; a++ )
		temp+=ranges[a];

	for ( a =0; *scale; a++)
		*scale/=2;
	*scale =a-1;
	for ( a = 1; a <= 257; a++ )
		ranges[a]+=ranges[a-1];
	return skip;
}

void ObsoleteUncompact( const unsigned char * in, unsigned char * out, int length, unsigned char * buffer){

	unsigned int ranges[260];
	int scale;

	if ( in[0] ){
		int size = *(int*)(in+1);
		int skip = ObsoleteReadProb(in+5,&ranges[0],&scale);
		ObsoleteDecode(in+4+skip,buffer,size,&ranges[0],scale);
		ObsoleteDeRLEZero(buffer,out,length,in[0]);
	} else {
		int skip = ObsoleteReadProb(in+1,&ranges[0],&scale);
		ObsoleteDecode(in+skip,out,length,&ranges[0],scale);
	}
}