BITS 64

%include "amd64inc.asm"

;=============================================================================
; Constants
;=============================================================================
%define ofs_x0000_0000_0010_0010  0
%define ofs_x0080_0080_0080_0080  8
%define ofs_x00FF_00FF_00FF_00FF  16
%define ofs_x00002000_00002000 24
%define ofs_xFF000000_FF000000 32
%define ofs_cy 40
%define ofs_crv 48
%define ofs_cgu_cgv 56
%define ofs_cbu 64

;=============================================================================
; Read only data
;=============================================================================

ALIGN 64
%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

align 8
yuv2rgb_constants_rec601:
	dq	0x0000000000100010
	dq	0x0080008000800080
mask1:
	dq	0x00FF00FF00FF00FF
	dq	0x0000200000002000
	dq	0xFF000000FF000000                    
	dq	0x00004A8500004A85
	dq	0x3313000033130000
	dq	0xE5FCF377E5FCF377
	dq	0x0000408D0000408D
yuv2rgb_constants_PC_601:
	dq	0x0000000000000000		;     0       
	dq	0x0080008000800080		;   128       
	dq	0x00FF00FF00FF00FF                    
	dq	0x0000200000002000		;  8192        = (0.5)<<14
	dq	0xFF000000FF000000                    
	dq	0x0000400000004000		; 16384        = (1.)<<14+0.5                                
	dq	0x2D0B00002D0B0000		; 11531        = ((1-0.299)*255./127.)<<13+0.5                      
	dq	0xE90FF4F2E90FF4F2		; -5873, -2830 = (((K-1)*K/0.587)*255./127.)<<13-0.5, K=(0.299, 0.114)
	dq	0x000038ED000038ED		;        14573 = ((1-0.114)*255./127.)<<13+0.5                      
yuv2rgb_constants_rec709:
	dq	0x0000000000100010		;    16       
	dq	0x0080008000800080		;   128       
	dq	0x00FF00FF00FF00FF                    
	dq	0x0000200000002000		;  8192        = (0.5)<<14
	dq	0xFF000000FF000000                    
	dq	0x00004A8500004A85		; 19077        = (255./219.)<<14+0.5
	dq	0x3960000039600000		; 14688        = ((1-0.2125)*255./112.)<<13+0.5
	dq	0xEEF5F930EEF5F930		; -4363, -1744 = ((K-1)*K/0.7154*255./112.)<<13-0.5, K=(0.2125, 0.0721)
	dq	0x0000439B0000439B		;        17307        = ((1-0.0721)*255./112.)<<13+0.5       
yuv2rgb_constants_PC_709:
	dq	0x0000000000000000		;     0       
	dq	0x0080008000800080		;   128       
	dq	0x00FF00FF00FF00FF                    
	dq	0x0000200000002000		;  8192        = (0.5)<<14
	dq	0xFF000000FF000000                    
	dq	0x0000400000004000		; 16384        = (1.)<<14+0.5                                
	dq	0x3299000032990000		; 12953        = ((1-0.2125)*255./127.)<<13+0.5                      
	dq	0xF0F8F9FEF0F8F9FE		; -3848, -1538 = (((K-1)*K/0.7154)*255./127.)<<13-0.5, K=(0.2125, 0.0721)
	dq	0x00003B9F00003B9F		;        15263 = ((1-0.0721)*255./127.)<<13+0.5
mask2:
	dq	0xFF00FF00FF00FF00
add_ones:
	dq  0x0101010101010101
rgb_mask:
	dq	0x00FFFFFF00FFFFFF
fraction:
	dq	0x0000000000084000
add_32:
	dq	0x000000000000000020
rb_mask:
	dq	0x0000FFFF0000FFFF
y1y2_mult:
	dq	0x0000000000004A85
fpix_add:
	dq	0x0080800000808000
fpix_mul:
	dq	0x0000503300003F74
low32_mask:
	dq	0x000000000ffffffff
cybgr_64:
	dq	0x000020DE40870c88
chroma_mask2:
	dq	0xffff0000ffff0000


;=============================================================================
; Macros
;=============================================================================

%macro GET_Y 2  ; mma, uyvy
%if %2
	psrlw %1, 8
%else
	pand %1, [rdx+ofs_x00FF_00FF_00FF_00FF]
%endif
%endmacro

%macro GET_UV 2  ; mma, uyvy
	GET_Y %1, (1-%2)
%endmacro

%macro YUV2RGB_INNER_LOOP 3  ; uyvy, rgb32, no_next_pixel
	movd		mm0,[rsi]
	 movd		 mm5,[rsi+4]
	movq		mm1,mm0
	GET_Y		mm0,%1	; mm0 = __________Y1__Y0
	 movq		 mm4,mm5
	GET_UV		mm1,%1	; mm1 = __________V0__U0
	 GET_Y		 mm4,%1
	movq		mm2,mm5		; *** avoid reload from [esi+4]
	 GET_UV		 mm5,%1
	psubw		mm0,[rdx+ofs_x0000_0000_0010_0010]
	 movd		 mm6,[rsi+8-4*%3]
	GET_UV		mm2,%1	; mm2 = __________V2__U2
	 psubw		 mm4,[rdx+ofs_x0000_0000_0010_0010]
	paddw		mm2,mm1
	 GET_UV		 mm6,%1
	psubw		mm1,[rdx+ofs_x0080_0080_0080_0080]
	 paddw		 mm6,mm5
	psllq		mm2,32
	 psubw		 mm5,[rdx+ofs_x0080_0080_0080_0080]
	punpcklwd	mm0,mm2		; mm0 = ______Y1______Y0
	 psllq		 mm6,32
	pmaddwd		mm0,[rdx+ofs_cy]	; mm0 scaled
	 punpcklwd	 mm4,mm6
	paddw		mm1,mm1
	 pmaddwd	 mm4,[rdx+ofs_cy]
	 paddw		 mm5,mm5
	paddw		mm1,mm2		; mm1 = __V1__U1__V0__U0 * 2
	paddd		mm0,[rdx+ofs_x00002000_00002000]
	 paddw		 mm5,mm6
	movq		mm2,mm1
	 paddd		 mm4,[rdx+ofs_x00002000_00002000]
	movq		mm3,mm1
	 movq		 mm6,mm5
	pmaddwd		mm1,[rdx+ofs_crv]
	 movq		 mm7,mm5
	paddd		mm1,mm0
	 pmaddwd	 mm5,[rdx+ofs_crv]
	psrad		mm1,14		; mm1 = RRRRRRRRrrrrrrrr
	 paddd		 mm5,mm4
	pmaddwd		mm2,[rdx+ofs_cgu_cgv]
	 psrad		 mm5,14
	paddd		mm2,mm0
	 pmaddwd	 mm6,[rdx+ofs_cgu_cgv]
	psrad		mm2,14		; mm2 = GGGGGGGGgggggggg
	 paddd		 mm6,mm4
	pmaddwd		mm3,[rdx+ofs_cbu]
	 psrad		 mm6,14
	paddd		mm3,mm0
	 pmaddwd	 mm7,[rdx+ofs_cbu]
       add	       rsi, byte 8
       add	       rdi, byte 12+4*%2
%if (%3==0)
       cmp	       rsi,rcx
%endif
	psrad		mm3,14		; mm3 = BBBBBBBBbbbbbbbb
	 paddd		 mm7,mm4
	pxor		mm0,mm0
	 psrad		 mm7,14
	packssdw	mm3,mm2	; mm3 = GGGGggggBBBBbbbb
	 packssdw	 mm7,mm6
	packssdw	mm1,mm0	; mm1 = ________RRRRrrrr
	 packssdw	 mm5,mm0	; *** avoid pxor mm4,mm4
	movq		mm2,mm3
	 movq		 mm6,mm7
	punpcklwd	mm2,mm1	; mm2 = RRRRBBBBrrrrbbbb
	 punpcklwd	 mm6,mm5
	punpckhwd	mm3,mm1	; mm3 = ____GGGG____gggg
	 punpckhwd	 mm7,mm5
	movq		mm0,mm2
	 movq		 mm4,mm6
	punpcklwd	mm0,mm3	; mm0 = ____rrrrggggbbbb
	 punpcklwd	 mm4,mm7
%if (%2==0)
	psllq		mm0,16
	 psllq		 mm4,16
%endif
	punpckhwd	mm2,mm3	; mm2 = ____RRRRGGGGBBBB
	 punpckhwd	 mm6,mm7
	packuswb	mm0,mm2	; mm0 = __RRGGBB__rrggbb <- ta dah!
	 packuswb	 mm4,mm6

%if %2
	por mm0, [rdx+ofs_xFF000000_FF000000]	 ; set alpha channels "on"
	 por mm4, [rdx+ofs_xFF000000_FF000000]
	movq	[rdi-16],mm0
	 movq	 [rdi-8],mm4
%else
	psrlq	mm0,8		; pack the two quadwords into 12 bytes
	psllq	mm4,8		; (note: the two shifts above leave
	movd	[rdi-12],mm0	; mm0,4 = __RRGGBBrrggbb__)
	psrlq	mm0,32
	por	mm4,mm0
	movd	[rdi-8],mm4
	psrlq	mm4,32
	movd	[rdi-4],mm4
%endif
%endmacro

%macro YUV2RGB_PROC 3  ; procname, uyvy, rgb32
cglobal %1
%1:
	firstpush	rsi
	pushreg rsi
	push	rdi
	pushreg rdi
	push rbx
	pushreg rbx
	endprolog
	
;	mov	eax,[esp+16+8]
;	mov	esi,[esp+12+8]		; read source bottom-up
;	mov	edi,[esp+8+8]
	mov eax, r9d  ; int stride/src_pitch
	mov r9, rcx  ; unsigned char* src
	mov rsi, r8  ; const unsigned char* src_end
	mov rdi, rdx  ; unsigned char* dst
	mov ebx, [rsp+40+24] ; row_size
;	mov	edx, yuv2rgb_constants
	lea rdx, [yuv2rgb_constants_rec601 wrt rip]
	test	byte [rsp+48+24],1
	jz	.%1_loop0
	lea	rdx, [yuv2rgb_constants_rec709 wrt rip]

	test	byte [rsp+48+24],2
	jz	.%1_loop0
	lea	rdx, [yuv2rgb_constants_PC_601 wrt rip]

	test	byte [rsp+48+24],4
	jz	.%1_loop0
	lea	rdx, [yuv2rgb_constants_PC_709 wrt rip]

.%1_loop0:
	sub	rsi,rax
	lea rcx, [rsi+rbx-8]

;align 32
.%1_loop1:
	YUV2RGB_INNER_LOOP	%2,%3,0
	jb	.%1_loop1

	YUV2RGB_INNER_LOOP	%2,%3,1

	sub	rsi,rbx
	cmp	rsi, r9
	ja	.%1_loop0

	emms
	pop rbx
	pop	rdi
	pop	rsi
	ret
	endfunc
%endmacro



;=============================================================================
; Code
;=============================================================================

SECTION .text
cglobal isse_yuy2_to_yv12

YUV2RGB_PROC	mmx_YUY2toRGB24,0,0
YUV2RGB_PROC	mmx_YUY2toRGB32,0,1
;YUV2RGB_PROC	mmx_UYVYtoRGB24,1,0
;YUV2RGB_PROC	mmx_UYVYtoRGB32,1,1


isse_yuy2_to_yv12:
; ( const BYTE* src,
;	int src_rowsize,
;	int src_pitch,
;	BYTE* dstY,
;	BYTE* dstU,
;	BYTE* dstV,
;	int dst_pitchY,
;	int dst_pitchUV,
;	int height);

	firstpush rdi
	pushreg rdi
	push rsi
	pushreg rsi
	push r12
	pushreg r12
	push r13
	pushreg r13
	push r14
	pushreg r14
	endprolog
	
;rcx = srcp
;edx = src_rowsize
;r8d = src_pitch
;r9 = dst_y

	; rsp offset = 8*5 = 40
	mov r10, [rsp+40+40]
	mov r11, [rsp+40+48]
	mov r12d, [rsp+40+56]
	mov r13d, [rsp+40+64]
	mov r14d, [rsp+40+72]

	add edx, byte 3
	shr edx, 2 ; src_rowsize = (src_rowsize+3)/4;

	lea rsi, [rcx+r8]  ; srcp+src_pitch
	lea rdi, [r9+r12] ; dsty+pitch_y

	movq mm7, [mask2 wrt rip]
	movq mm4, [mask1 wrt rip]
.yloop:
	xor eax, eax
;align 16
.xloop:
	movq mm0, [rcx+rax*4]
	movq mm1, [rsi+rax*4]
	movq mm6, mm0
	movq mm2, [rcx+rax*4+8]
	movq mm3, [rsi+rax*4+8]
	movq mm5, mm2
	pavgb mm6, mm1
	pavgb mm5, mm3
	pand mm0, mm4
	psrlq mm5, 8
	pand mm1, mm4
	psrlq mm6, 8
	pand mm2, mm4
	pand mm3, mm4
	pand mm5, mm4
	pand mm6, mm4
	packuswb mm0, mm2
	packuswb mm6, mm5
	packuswb mm1, mm3
	movq mm5, mm6
	pand mm5, mm7
	pand mm6, mm4
	psrlq mm5, 8
	packuswb mm5, mm7
	packuswb mm6, mm7
	movq [r9+2*rax], mm0
	movq [rdi+2*rax], mm1

	movd [r10+rax], mm6
	movd [r11+rax], mm5

	add eax, byte 4
	cmp eax, edx
	jl .xloop
	lea rcx, [rcx+2*r8]
	lea rsi, [rsi+2*r8]
	lea r9, [r9+2*r12]
	lea rdi, [rdi+2*r12]
	lea r10, [r10+r13]
	lea r11, [r11+r13]

	sub r14d, byte 2
	jnz .yloop
	sfence
	emms
	pop r14
	pop r13
	pop r12
	pop rsi
	pop rdi
	ret
	endfunc

cglobal isse_yv12_to_yuy2
isse_yv12_to_yuy2:
;(const BYTE* srcY, 8
; const BYTE* srcU, 16
; const BYTE* srcV, 24
; int src_rowsize, 32
; int src_pitch, 40
; int src_pitch_uv, 48
; BYTE* dst, 56
; int dst_pitch, 64
; int height) 72

	firstpush rdi
	pushreg rdi
	push rsi
	pushreg rsi
	push r12
	pushreg r12
	push r13
	pushreg r13
	push r14
	pushreg r14
	push r15
	pushreg r15
	push rbx
	pushreg rbx
	endprolog
;rsp offset = 56
	; What? At least I left rbp spare....

	mov r14d, [rsp+56+72] ; height
	mov r12d, [rsp+56+64] ; dst_pitch
	mov rdi, [rsp+56+56] ; dst
	mov r10d, [rsp+56+40] ; src_pitch
	mov r11d, [rsp+56+48] ; src_pitch_uv
	lea rsi, [rdi+r12] ; dst+dst_pitch
	lea r13, [rcx+r10] ; srcY+src_pitch
	inc r9d
	shr r9d, 1 ; src_rowsize = (src_rowsize+1)/2

	movq mm6, [add_ones wrt rip]

	xor eax, eax
;align 16
.top_xloop:
	movd mm1, [rdx+rax] ; U
	movq mm0, [rcx+2*rax] ; Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
	punpcklbw mm1, [r8+rax] ; V3U3V2U2 V1U1V0U0
	movq mm4, mm0
	punpcklbw mm0, mm1 ; V1 Y3 U1 Y2 V0 Y1 U0 Y0
	punpckhbw mm4, mm1 ; V3 Y7 U3 Y6 V2 Y5 U2 Y4
	movq [rdi+4*rax], mm0
	movq [rdi+4*rax+8], mm4

	add eax, byte 4
	cmp eax, r9d
	jl .top_xloop

	lea rcx, [rcx+2*r10] ; increment srcY
	lea rdi, [rdi+2*r12] ; increment dst

	; swap rcx, r13 and rdi, rsi from this point
	; since we've done one line of Y plane/output.

	lea rbx, [rdx+r11] ; srcU + src_pitch_uv
	lea r15, [r8+r11] ; srcV + src_pitch_uv
	
	sub r14d, byte 2 ; the top and bottom lines aren't interpolated
	jz .done
;Top line taken care of - do the bottom line after the main loop

; MAIN LOOP

.yloop:
	xor eax, eax
;align 16
.xloop:
	movd mm1, [rdx+rax]
	movq mm0, [r13+2*rax] ; YYYY YYYY current line
	punpcklbw mm1, [r8+rax] ; V3U3V2U2 V1U1V0U0
	movd mm2, [rbx+rax] ; 0000UUUU next
	movq mm7, [rcx+2*rax] ; YYYY YYYY next line
	movq mm5, mm1
	punpcklbw mm2, [r15+rax] ; V7U7V6U6 V5U5V4U4
	movq mm3, mm0  ; YYYY YYYY use high dword
	pavgb mm1, mm2 ; 50/50
	movq mm4, mm7
	psubusb mm1, mm6 ; for rounding
	pavgb mm5, mm1 ; 75/25 for current line
	pavgb mm2, mm1 ; 25/75 for next line
	punpcklbw mm0, mm5 ; for output V1Y3U1Y2 V0Y1U0Y0
	punpcklbw mm7, mm2 ; for output line 2 V5Y3U5Y2 V4Y2U4Y0
	punpckhbw mm3, mm5 ; for output V3Y7U3Y6 V2Y5U2Y4
	punpckhbw mm4, mm2 ; for output line 2 V7Y7U7Y6 V6Y5U6Y4
	movq [rsi+4*rax], mm0
	movq [rdi+4*rax], mm7
	movq [rsi+4*rax+8], mm3
	movq [rdi+4*rax+8], mm4
	
	add eax, byte 4
	cmp eax, r9d
	jl .xloop
	
	lea rcx, [rcx+2*r10]
	add rdx, r11
	add r8, r11
	lea rdi, [rdi+2*r12]
	lea rsi, [rsi+2*r12]
	lea r13, [r13+2*r10]
	add rbx, r11
	add r15, r11
	sub r14, byte 2
	
	jnz .yloop
	
.done: ; copy chroma directly for the bottom line
	xor eax, eax
;align 16
.bottom_xloop:
	movd mm1, [rdx+rax] ; 0000 UUUU
	movq mm0, [r13+2*rax] ; Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
	punpcklbw mm1, [r8+rax] ; V3U3V2U2 V1U1V0U0
	movq mm4, mm0
	punpcklbw mm0, mm1 ; V1 Y3 U1 Y2 V0 Y1 U0 Y0
	punpckhbw mm4, mm1 ; V3 Y7 U3 Y6 V2 Y5 U2 Y4
	movq [rsi+4*rax], mm0
	movq [rsi+4*rax+8], mm4

	add eax, byte 4
	cmp eax, r9d
	jl .bottom_xloop
	
	sfence ; ???
	emms
	
	pop rbx
	pop r15
	pop r14
	pop r13
	pop r12
	pop rsi
	pop rdi
	ret
	endfunc
	
cglobal mmx_ConvertRGB32toYUY2
mmx_ConvertRGB32toYUY2:
	mov r10, rdx
	shl r8d, 2
	mov r11d, [rsp+48] ; height
	mov eax, r11d
	dec eax
	mul r8d ; eax = src_pitch*(height-1)
	mov edx, [rsp+40] ; width
	shl edx, 1 ; width<<1 // get yuy2 width in bytes
	add rcx, rax ; src += src_pitch*(h-1);
	
.yloop:
	xor eax, eax
	movq mm0, [rcx]
	punpcklbw mm1, mm0
	movq mm4, [cybgr_64 wrt rip]
.re_enter:
		punpckhbw mm2,mm0				; mm2= 0000 R200 G200 B200
		 movq mm3,[fraction wrt rip]
		psrlw mm1,8							; mm1= 0000 00R1 00G1 00B1
	  psrlw mm2,8							; mm2= 0000 00R2 00G2 00B2 
		 movq mm6,mm1						; mm6= 0000 00R1 00G1 00B1 (shifter unit stall)
		pmaddwd mm1,mm4						; mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
		 movq mm7,mm2						; mm7= 0000 00R2 00G2 00B2		 
		  movq mm0,[rb_mask wrt rip]
		pmaddwd mm2,mm4						; mm1= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
		 paddd mm1,mm3						; Add rounding fraction
		  paddw mm6,mm7					; mm6 = accumulated RGB values (for b_y and r_y) 
		paddd mm2,mm3						; Add rounding fraction (lower dword only)
		 movq mm4,mm1
		movq mm5,mm2
		pand mm1,[low32_mask wrt rip]
		 psrlq mm4,32
		pand mm6,mm0						; Clear out accumulated G-value mm6= 0000 RRRR 0000 BBBB
		pand mm2,[low32_mask wrt rip]
		 psrlq mm5,32
		paddd mm1,mm4				    ;mm1 Contains final y1 value (shifted 15 up)
		 psllq mm6, 14					; Shift up accumulated R and B values (<<15 in C)
		paddd mm2,mm5					  ;mm2 Contains final y2 value (shifted 15 up)
		 psrlq mm1,15
		movq mm3,mm1
		 psrlq mm2,15
		movq mm4,[add_32 wrt rip]
		 paddd mm3,mm2					;mm3 = y1+y2
		movq mm5,[y1y2_mult wrt rip]
		 psubd mm3,mm4					; mm3 = y1+y2-32 		mm0,mm4,mm5,mm7 free
    movq mm0,[fpix_add wrt rip]			; Constant that should be added to final UV pixel
	   pmaddwd mm3,mm5				; mm3=scaled_y (latency 2 cycles)
      movq mm4,[fpix_mul wrt rip]		; Constant that should be multiplied to final UV pixel	  
		  psllq mm2,16					; mm2 Y2 shifted up (to clear fraction) mm2 ready
		punpckldq mm3,mm3						; Move scaled_y to upper dword mm3=SCAL ED_Y SCAL ED_Y 
		psubd mm6,mm3								; mm6 = b_y and r_y (stall)
		psrad mm6,14									; Shift down b_y and r_y (>>10 in C-code) 
		movq mm3, [chroma_mask2 wrt rip]
		 por mm1,mm2								; mm1 = 0000 0000 00Y2 00Y1
		pmaddwd mm6,mm4							; Mult b_y and r_y 
		 pxor mm2,mm2
		paddd mm6, mm0							; Add 0x808000 to r_y and b_y 
	add eax, byte 4
	pand mm6, mm3
	cmp eax, edx
	jge .outloop
	packuswb mm6,mm6
	movq mm0, [rcx+2*rax]
	por mm6, mm1
	movq mm4, [cybgr_64 wrt rip]
	movd [r10+rax-4], mm6
	punpcklbw mm1, mm0
	jmp .re_enter
.outloop:
	packuswb mm6, mm6
	por mm1, mm6
	movd [r10+rax-4], mm1
	sub rcx, r8
	lea r10, [r10+r9*4]
	dec r11d
	jnz .yloop
	
	emms
	ret