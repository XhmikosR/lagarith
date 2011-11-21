
#pragma once

extern "C" {   // For use with convert.asm

/* RGB32toYUY2
* *src = pointer to rgb source frame (should be stored upside down)
* *dst = pointer to yuy2 frame for output
* src_pitch = pitch of RGB frame / 4
* dst_pitch = pitch of YUY2 frame / 2
* w = source image width in pixels
* h = source image height in pixels
*/

void mmx_ConvertRGB32toYUY2(const unsigned char *src,unsigned char *dst,int src_pitch, int dst_pitch,int w, int h);

/* YUY2toYV12
* *src = pointer to yuy2 source frame
* src_rowsize = size in bytes of one row of yuy2 image e.g. width in pixels * 2
* src_pitch = pitch of yuy2 frame
* *dstY = pointer to Y plane of destination YV12 image
* *dstU = pointer to U plane of destination YV12 image
* *dstV = pointer to V plane of destination YV12 image
* dst_pitch = pitch of Y plane
* dst_pitch_UV = pitch of U and V planes
* height = height of source image in pixels
*/

void isse_yuy2_to_yv12(const BYTE* src, int src_rowsize, int src_pitch, 
                    BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitch, int dst_pitchUV,
                    int height);

/* YV12toYUY2
* *srcY = pointer to Y plane of source YV12 image
* *srcU = pointer to U plane of source YV12 image
* *srcV = pointer to V plane of source YV12 image
* src_rowsize = width of source image in pixels
* src_pitch = pitch of Y plane
* src_pitch_uv = pitch of U and V planes
* *dst = pointer to destination yuy2 frame
* dst_pitch = pitch of yuy2 frame
* height = height of source image in pixels
*/

void isse_yv12_to_yuy2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_rowsize, 
				int src_pitch, int src_pitch_uv, BYTE* dst, int dst_pitch, int height);

/* YUY2toRGB32
* *src = pointer to souce YUY2 frame
* *dst = pointer to destination RGB32 frame
* *src_end = pointer to end of source YUY2 frame
* src_pitch = pitch of YUY2 frame
* rowsize = size in bytes of one row of yuy2 image e.g. width in pixels * 2
* rec709 = color coefficient matrix to use for transformation. 1=rec601 (equivalent to old method), 2=rec709, 4=pc_601, other = pc_709
*/

#ifdef X64_BUILD
void mmx_YUY2toRGB32(const BYTE* src, BYTE* dst, const BYTE* src_end, int src_pitch, int row_size,
				bool rec709);

void mmx_YUY2toRGB24(const BYTE* src, BYTE* dst, const BYTE* src_end, int src_pitch, int row_size,
				bool rec709);
#endif

}