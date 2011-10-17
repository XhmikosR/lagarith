
#pragma once

int deRLE(const unsigned char * in, unsigned char * out, const int length, const int level);
unsigned int TestAndRLE(unsigned char * const __restrict in, unsigned char * const __restrict out1, unsigned char * const __restrict out3, const unsigned int length, int * level);
