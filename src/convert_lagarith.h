
void ConvertRGB24toYV16_SSE2(const unsigned char *src, unsigned char *ydst, unsigned char *udst, unsigned char *vdst, unsigned int w, unsigned int h);
void ConvertRGB32toYV16_SSE2(const unsigned char *src, unsigned char *ydst, unsigned char *udst, unsigned char *vdst, unsigned int w, unsigned int h);
void ConvertRGB32toYV12_SSE2(const unsigned char *src, unsigned char *ydst, unsigned char *udst, unsigned char *vdst, unsigned int w, unsigned int h);
void ConvertRGB24toYV12_SSE2(const unsigned char *src, unsigned char *ydst, unsigned char *udst, unsigned char *vdst, unsigned int w, unsigned int h);
//void ConvertYUY2ToYV12(const unsigned char * src, unsigned char * ydest, unsigned char * udest, unsigned char *  vdest, const unsigned int w, const unsigned int h);
