;
; Huffyuv v2.1.1, by Ben Rudiak-Gould.
; http://www.math.berkeley.edu/~benrg/huffyuv.html
;
; This file is copyright 2000 Ben Rudiak-Gould, and distributed under
; the terms of the GNU General Public License, v2 or later.  See
; http://www.gnu.org/copyleft/gpl.html.
;

;
; This file makes heavy use of macros to define a bunch of almost-identical
; functions -- see huffyuv_a.h.
;

	.586
	.mmx
	.model	flat

; alignment has to be 'page' so that I can use 'align 32' below

_TEXT64	segment	page public use32 'CODE'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

HUFF_CODEC_PROC_START	MACRO

	push	ebp
	push	edi
	push	esi
	push	ebx

	mov	esi,[esp+4+16]
	mov	edi,[esp+8+16]
	mov	ebp,[esp+12+16]
	mov	eax,[esi]

	ENDM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	PUBLIC	C _mmx_RowDiff
;
;void __cdecl mmx_RowDiff(
;	[esp+ 4] unsigned char* src,
;	[esp+ 8] unsigned char* dst,
;	[esp+12] unsigned char* src_end,
;	[esp+16] int stride);
;
;_mmx_RowDiff	PROC
;
;	push	ebp
;	push	edi
;	push	esi
;	push	ebx
;
;	mov	esi,[esp+4+16]
;	mov	edi,[esp+8+16]
;	mov	ecx,[esp+16+16]
;	add	ecx,esi
;
;	cmp	esi,edi
;	je	diff
;
;	; make sure we're 8-byte aligned
;loop0:
;	test	edi,7
;	jz	endloop0
;	mov	al,[esi]
;	inc	esi
;	mov	[edi],al
;	inc	edi
;	jmp	loop0
;endloop0:
;
;	; copy the (rest of the) first row
;loop1:
;	movq	mm0,[esi]
;	movq	mm1,[esi+8]
;	add	esi,16
;	movq	[edi],mm0
;	movq	[edi+8],mm1
;	add	edi,16
;	cmp	esi,ecx
;	jb	loop1
;
;	; diff the remaining rows
;diff:
;	mov	esi,[esp+12+16]
;	mov	ecx,[esp+4+16]
;	mov	edi,[esp+8+16]
;	mov	ebx,[esp+16+16]
;	add	edi,esi
;	sub	edi,ecx
;	add	ecx,ebx
;	neg	ebx
;
;	; align again (sigh...)
;loop2:
;	test	edi,7
;	jz	endloop2
;	mov	al,[esi-1]
;	sub	al,[esi+ebx-1]
;	dec	esi
;	mov	[edi-1],al
;	dec	edi
;	jmp	loop2
;endloop2:
;
;	mov	edx,32
;	sub	esi,edx
;	sub	edi,edx
;	align	32
;loop3:
;	movq	mm3,[esi+24]
;	movq	mm2,[esi+16]
;	movq	mm6,[esi+ebx+16]
;	psubb	mm3,[esi+ebx+24]	; 2
;	psubb	mm2,mm6
;	movq	mm1,[esi+8]
;	movq	[edi+24],mm3		; 2
;	movq	mm5,[esi+ebx+8]
;	movq	mm0,[esi]
;	movq	[edi+16],mm2		; 2
;	psubb	mm1,mm5
;	psubb	mm0,[esi+ebx]		; 2
;	sub	esi,edx
;	movq	[edi+8],mm1		; 2
;	cmp	esi,ecx
;	movq	[edi],mm0		; 2
;	lea	edi,[edi-32]
;	jae	loop3
;
;	; and more alignment
;	add	esi,edx
;	add	edi,edx
;loop4:
;	cmp	esi,ecx
;	jbe	endloop4
;	mov	al,[esi-1]
;	sub	al,[esi+ebx-1]
;	dec	esi
;	mov	[edi-1],al
;	dec	edi
;	jmp	loop4
;endloop4:
;
;	emms
;	pop	ebx
;	pop	esi
;	pop	edi
;	pop	ebp
;	retn
;
;_mmx_RowDiff	ENDP
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	PUBLIC	C _mmx_RowAccum
;
;void __cdecl mmx_RowAccum(
;	[esp+ 4] unsigned char* buf,
;	[esp+ 8] unsigned char* buf_end,
;	[esp+12] int stride);
;
;_mmx_RowAccum	PROC
;
;	push	ebp
;	push	esi
;	push	ebx
;
;	mov	esi,[esp+4+12]
;	mov	ebx,[esp+12+12]
;	add	esi,ebx
;	neg	ebx
;
;loop0:
;	test	esi,7
;	jz	endloop0
;	mov	al,[esi+ebx]
;	add	[esi],al
;	inc	esi
;	jmp	loop0
;endloop0:
;
;	mov	ecx,[esp+8+12]
;	sub	ecx,32
;	align	32
;loop1:
;	movq	mm0,[esi]
;	movq	mm1,[esi+8]
;	movq	mm5,[esi+ebx+24]
;	paddb	mm0,[esi+ebx]
;	movq	mm2,[esi+16]
;	movq	mm4,[esi+ebx+16]
;	paddb	mm1,[esi+ebx+8]
;	movq	mm3,[esi+24]
;	paddb	mm2,mm4
;	movq	[esi],mm0
;	paddb	mm3,mm5
;	movq	[esi+8],mm1
;	movq	[esi+16],mm2
;	movq	[esi+24],mm3
;	add	esi,32
;	cmp	esi,ecx
;	jbe	loop1
;
;	; cleanup end in case of misalignment
;	add	ecx,32
;loop2:
;	cmp	esi,ecx
;	jae	endloop2
;	mov	al,[esi+ebx]
;	add	[esi],al
;	inc	esi
;	jmp	loop2
;endloop2:
;
;	emms
;	pop	ebx
;	pop	esi
;	pop	ebp
;	retn
;
;_mmx_RowAccum	ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

YUV_SHIFT	MACRO	mmb,mma,uyvy	; clobbers mm4,5

; mma:mmb = ABCDEFGHIJKLMNOP (VYUYVYUY...) - backwards from mem order
;   we want mmb = EDGFIHKJ (prev pixel of same channel)

	movq	mm4,mmb
	punpcklbw	mmb,mma		; mm4:mmb = AIBJCKDLEMFNGOHP
	punpckhbw	mm4,mma
	movq	mm5,mmb
	punpcklbw	mmb,mm4		; mm5:mmb = AEIMBFJNCGKODHLP
	punpckhbw	mm5,mm4
	movq	mm4,mmb
	punpcklbw	mmb,mm5		; mm4:mmb = ACEGIKMOBDFHJLNP
	punpckhbw	mm4,mm5
	psllq	mmb,8+8*&uyvy		; mm4:mmb = EGIKMO__DFHJLNP_
	psllq	mm4,16-8*&uyvy
	punpckhbw	mmb,mm4		; mmb = EDGFIHKJ (for YUY2; different for UYVY)

	ENDM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MEDIAN_PREDICT_PROC	MACRO	procname,uyvy

	PUBLIC	C _&procname

;void __cdecl mmx_MedianPredict(
;	[esp+ 4] unsigned char* src,
;	[esp+ 8] unsigned char* dst,
;	[esp+12] unsigned char* src_end,
;	[esp+16] int stride);

_&procname	PROC

	push	ebp
	mov	ebp,esp
	push	edi
	push	esi
	push	ebx

	; do the first row
	mov	esi,[ebp+4+4]
	mov	edi,[ebp+8+4]
	mov	ebx,[ebp+16+4]
	lea	ecx,[ebx+esi+8]
	neg	ebx

	pxor	mm2,mm2
	movq	mm3,[esi]	; for use in next loop

loop0:
	movq	mm0,[esi]
	YUV_SHIFT	mm2,mm0,uyvy
	add	esi,8
	movq	mm1,mm0
	psubb	mm1,mm2
	movq	[edi],mm1
	movq	mm2,mm0
	add	edi,8
	cmp	esi,ecx
	jb	loop0

	mov	ecx,[ebp+8+4]	; recopy first group of four, just for consistency with other compression methods
	movd	dword ptr [ecx],mm3

	; do the remaining rows
	mov	ecx,[ebp+12+4]
	; mm2,3 are already initialized from previous loop

	align	32

	; pixel arrangement:
	;    mm3 mm1
	;    mm2 mm0

loop1:
	; mm2,3 <- appropriate left and above-left pixels
	movq	mm0,[esi]
	movq	mm1,[esi+ebx]

	YUV_SHIFT	mm2,mm0,uyvy	; note: clobbers mm4,5
	add	esi,8
	YUV_SHIFT	mm3,mm1,uyvy

	; mm4 <- median of mm1,mm2,(mm1+mm2-mm3)

	movq	mm4,mm2		; (mm2,mm4) <- (min(mm1,mm2),max(mm1,mm2))
	movq	mm5,mm2		; mm5 <- mm1+mm2-mm3
	psubusb	mm4,mm1
	paddb	mm5,mm1
	psubb	mm2,mm4
	psubb	mm5,mm3
	paddb	mm4,mm1

	psubusb	mm2,mm5		; mm2 = max(mm2,mm5)
	paddb	mm2,mm5

	movq	mm5,mm4		; mm4 = min(mm2,mm4)
	psubusb	mm5,mm2
	psubb   mm4,mm5		; now mm4 = median

	; write out the result and loop
	movq	mm2,mm0
	movq	mm3,mm1
	psubb	mm0,mm4
	movq	[edi],mm0
	cmp	esi,ecx
	lea	edi,[edi+8]
	jb	loop1

	emms
	pop	ebx
	pop	esi
	pop	edi
	pop	ebp
	retn

_&procname	ENDP

	ENDM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;MEDIAN_PREDICT_PROC	mmx_MedianPredictYUY2,0
;MEDIAN_PREDICT_PROC	mmx_MedianPredictUYVY,1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

salc	MACRO		; see http://www.df.lth.se/~john_e/gems/gem0004.html
	db	0d6h
	ENDM

MEDIAN_RESTORE	MACRO	ofs1,ofs2,increment

	mov	ah,[esi+&ofs2]
	mov	ch,[esi+ebx+&ofs1]
	mov	dh,[esi+ebx+&ofs2]
	neg	dh		; compute ah+ch-dh
	mov	cl,ch		; (interleaved) exchange ah,ch if necessary so ah<ch
	add	dh,ch
	add	dh,ah
	sub	cl,ah
	salc
	and	cl,al
	sub	ch,cl
	add	ah,cl
	mov	cl,dh		; exchange ch,dh (but toss dh)
	sub	cl,ch
	salc
IF &increment
	add	esi,&increment
ENDIF
	and	cl,al
	add	ch,cl
	mov	cl,ch		; exchange ah,ch (but toss ah)
	sub	cl,ah
	salc
	and	cl,al
	sub	ch,cl		; now ch = median
	add	[esi+&ofs1-&increment],ch

	ENDM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	PUBLIC	C _asm_MedianRestore

;void __cdecl asm_MedianRestore(
;	[esp+ 4] unsigned char* buf,
;	[esp+ 8] unsigned char* buf_end,
;	[esp+12] int stride);

_asm_MedianRestore	PROC

	push	ebp
	mov	ebp,esp
	push	esi
	push	edi
	push	ebx

	mov	esi,[ebp+4+4]
	mov	ebx,[ebp+12+4]
	lea	edi,[esi+ebx+8]
	add	esi,4
	neg	ebx

	; process first row (left predict)

loop0:
	mov	al,[esi-2]
	mov	ah,[esi-3]
	mov	dl,[esi]
	mov	dh,[esi-1]
	add	dl,al
	add	[esi+1],ah
	mov	[esi],dl
	add	[esi+2],dl
	add	[esi+3],dh
	add	esi,4
	cmp	esi,edi
	jb	loop0

	; process remaining rows

	mov	edi,[ebp+8+4]

	align	32
loop1:
	MEDIAN_RESTORE	0,-2,0
	MEDIAN_RESTORE	1,-3,2
	cmp	esi,edi
	jb	loop1

	pop	ebx
	pop	edi
	pop	esi
	pop	ebp
	retn

_asm_MedianRestore	ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	PUBLIC	C _asm_SwapFields
;
;_asm_SwapFields	PROC
;
;void __cdecl asm_SwapFields(
;	[esp+ 4] unsigned char* buf,
;	[esp+ 8] unsigned char* buf_end,
;	[esp+12] int stride);
;
;	push	ebp
;	push	esi
;	push	ebx
;
;	mov	esi,[esp+4+12]
;	mov	ebp,[esp+8+12]
;	mov	ebx,[esp+12+12]
;
;loop0:
;	mov	ecx,ebx
;	shr	ecx,2
;loop1:
;	mov	eax,[esi]
;	mov	edx,[esi+ebx]
;	dec	ecx
;	mov	[esi+ebx],eax
;	mov	[esi],edx
;	lea	esi,[esi+4]
;	jnz	loop1
;	add	esi,ebx
;	cmp	esi,ebp
;	jb	loop0
;
;	pop	ebx
;	pop	esi
;	pop	ebp
;	retn
;
;_asm_SwapFields	ENDP
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	align	8

yuv2rgb_constants:

x0000_0000_0010_0010	dq	00000000000100010h
x0080_0080_0080_0080	dq	00080008000800080h
x00FF_00FF_00FF_00FF	dq	000FF00FF00FF00FFh
x00002000_00002000	dq	00000200000002000h
cy			dq	000004A8500004A85h
crv			dq	03313000033130000h
cgu_cgv			dq	0E5FCF377E5FCF377h
cbu			dq	00000408D0000408Dh

ofs_x0000_0000_0010_0010 = 0
ofs_x0080_0080_0080_0080 = 8
ofs_x00FF_00FF_00FF_00FF = 16
ofs_x00002000_00002000 = 24
ofs_cy = 32
ofs_crv = 40
ofs_cgu_cgv = 48
ofs_cbu = 56

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GET_Y	MACRO	mma,uyvy
IF &uyvy
	psrlw		mma,8
ELSE
	pand		mma,[edx+ofs_x00FF_00FF_00FF_00FF]
ENDIF
	ENDM

GET_UV	MACRO	mma,uyvy
	GET_Y		mma,1-uyvy
	ENDM

YUV2RGB_INNER_LOOP	MACRO	uyvy,rgb32,no_next_pixel

;; This YUV422->RGB conversion code uses only four MMX registers per
;; source dword, so I convert two dwords in parallel.  Lines corresponding
;; to the "second pipe" are indented an extra space.  There's almost no
;; overlap, except at the end and in the two lines marked ***.

	movd		mm0,dword ptr [esi]
	 movd		 mm5,dword ptr [esi+4]
	movq		mm1,mm0
	GET_Y		mm0,&uyvy	; mm0 = __________Y1__Y0
	 movq		 mm4,mm5
	GET_UV		mm1,&uyvy	; mm1 = __________V0__U0
	 GET_Y		 mm4,&uyvy
	movq		mm2,mm5		; *** avoid reload from [esi+4]
	 GET_UV		 mm5,&uyvy
	psubw		mm0,[edx+ofs_x0000_0000_0010_0010]
	 movd		 mm6,dword ptr [esi+8-4*(no_next_pixel)]
	GET_UV		mm2,&uyvy	; mm2 = __________V2__U2
	 psubw		 mm4,[edx+ofs_x0000_0000_0010_0010]
	paddw		mm2,mm1
	 GET_UV		 mm6,&uyvy
	psubw		mm1,[edx+ofs_x0080_0080_0080_0080]
	 paddw		 mm6,mm5
	psllq		mm2,32
	 psubw		 mm5,[edx+ofs_x0080_0080_0080_0080]
	punpcklwd	mm0,mm2		; mm0 = ______Y1______Y0
	 psllq		 mm6,32
	pmaddwd		mm0,[edx+ofs_cy]	; mm0 scaled
	 punpcklwd	 mm4,mm6
	paddw		mm1,mm1
	 pmaddwd	 mm4,[edx+ofs_cy]
	 paddw		 mm5,mm5
	paddw		mm1,mm2		; mm1 = __V1__U1__V0__U0 * 2
	paddd		mm0,[edx+ofs_x00002000_00002000]
	 paddw		 mm5,mm6
	movq		mm2,mm1
	 paddd		 mm4,[edx+ofs_x00002000_00002000]
	movq		mm3,mm1
	 movq		 mm6,mm5
	pmaddwd		mm1,[edx+ofs_crv]
	 movq		 mm7,mm5
	paddd		mm1,mm0
	 pmaddwd	 mm5,[edx+ofs_crv]
	psrad		mm1,14		; mm1 = RRRRRRRRrrrrrrrr
	 paddd		 mm5,mm4
	pmaddwd		mm2,[edx+ofs_cgu_cgv]
	 psrad		 mm5,14
	paddd		mm2,mm0
	 pmaddwd	 mm6,[edx+ofs_cgu_cgv]
	psrad		mm2,14		; mm2 = GGGGGGGGgggggggg
	 paddd		 mm6,mm4
	pmaddwd		mm3,[edx+ofs_cbu]
	 psrad		 mm6,14
	paddd		mm3,mm0
	 pmaddwd	 mm7,[edx+ofs_cbu]
       add	       esi,8
       add	       edi,12+4*rgb32
IFE &no_next_pixel
       cmp	       esi,ecx
ENDIF
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
IFE &rgb32
	psllq		mm0,16
	 psllq		 mm4,16
ENDIF
	punpckhwd	mm2,mm3	; mm2 = ____RRRRGGGGBBBB
	 punpckhwd	 mm6,mm7
	packuswb	mm0,mm2	; mm0 = __RRGGBB__rrggbb <- ta dah!
	 packuswb	 mm4,mm6

IF &rgb32
	movd	dword ptr [edi-16],mm0	; store the quadwords independently
	 movd	 dword ptr [edi-8],mm4	; (and in pieces since we may not be aligned)
	psrlq	mm0,32
	 psrlq	 mm4,32
	movd	dword ptr [edi-12],mm0
	 movd	 dword ptr [edi-4],mm4
ELSE
	psrlq	mm0,8		; pack the two quadwords into 12 bytes
	psllq	mm4,8		; (note: the two shifts above leave
	movd	dword ptr [edi-12],mm0	; mm0,4 = __RRGGBBrrggbb__)
	psrlq	mm0,32
	por	mm4,mm0
	movd	dword ptr [edi-8],mm4
	psrlq	mm4,32
	movd	dword ptr [edi-4],mm4
ENDIF

	ENDM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

YUV2RGB_PROC	MACRO	procname,uyvy,rgb32

	PUBLIC	C _&procname

;;void __cdecl procname(
;;	[esp+ 4] const unsigned char* src,
;;	[esp+ 8] unsigned char* dst,
;;	[esp+12] const unsigned char* src_end,
;;	[esp+16] int stride);

_&procname	PROC

	push	esi
	push	edi

	mov	eax,[esp+16+8]
	mov	esi,[esp+12+8]		; read source bottom-up
	mov	edi,[esp+8+8]
	mov	edx,offset yuv2rgb_constants

loop0:
	lea	ecx,[esi-8]
	sub	esi,eax

	align 32
loop1:
	YUV2RGB_INNER_LOOP	uyvy,rgb32,0
	jb	loop1

	YUV2RGB_INNER_LOOP	uyvy,rgb32,1

	sub	esi,eax
	cmp	esi,[esp+4+8]
	ja	loop0

	emms
	pop	edi
	pop	esi
	retn

_&procname	ENDP

	ENDM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

YUV2RGB_PROC	mmx_YUY2toRGB24,0,0
YUV2RGB_PROC	mmx_YUY2toRGB32,0,1
;YUV2RGB_PROC	mmx_UYVYtoRGB24,1,0
;YUV2RGB_PROC	mmx_UYVYtoRGB32,1,1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	END
