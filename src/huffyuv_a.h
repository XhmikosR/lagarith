
#pragma once

// from Ben Rudiak-Gould's Huffyuv source code
extern "C" {

void __cdecl mmx_MedianPredictYUY2(const unsigned char* src, unsigned char* dst, const unsigned char* src_end, int stride);

void __cdecl mmx_YUY2toRGB24(const unsigned char* src, unsigned char* dst, const unsigned char* src_end, int stride);
void __cdecl mmx_YUY2toRGB32(const unsigned char* src, unsigned char* dst, const unsigned char* src_end, int stride);

}
