.386p

	NAME    resample
        EXTRN   _rvolsel :WORD
        EXTRN   _lvolsel :WORD

	.model small,c

DGROUP  GROUP   _DATA

_TEXT   SEGMENT DWORD PUBLIC USE32 'CODE'
        ASSUME  CS:_TEXT ,DS:DGROUP,SS:DGROUP

        PUBLIC  AsmStereoNormal_
        PUBLIC  AsmMonoNormal_
        
SS2     MACRO index
        even
        mov   edx,ebx
        sar   edx,0bh
        mov   al,[esi+edx]
        add   ebx,ecx
        mov   edx,es:[eax*4]
        add   (index*8)[edi],edx
        mov   edx,fs:[eax*4]
        add   (4+(index*8))[edi],edx
        ENDM

SM2     MACRO index
        even
        mov   edx,ebx
        add   ebx,ecx
        sar   edx,0bh
        mov   al,[esi+edx]
        mov   edx,es:[eax*4]
        add   (index*4)[edi],edx
        ENDM


AsmStereoNormal_ proc USES ebp fs es
        mov    ax,_lvolsel
        mov    es,ax                       ; voltab selector naar fs
        mov    ax,_rvolsel
        mov    fs,ax
        xor    eax,eax
        push   edx
        shr    edx,4
        jz     sskip16
        mov    ebp,edx
sagain16:
        SS2    0
        SS2    1
        SS2    2
        SS2    3
        SS2    4
        SS2    5
        SS2    6
        SS2    7
        SS2    8
        SS2    9
        SS2    10
        SS2    11
        SS2    12
        SS2    13
        SS2    14
        SS2    15
        add    edi,(16*8)
        dec    ebp
        jnz    sagain16
sskip16:
        pop    edx
        and    edx,15
        jz     sskip1
        mov    ebp,edx
sagain1:
        SS2    0
        add    edi,8
        dec    ebp
        jnz    sagain1
sskip1:
	ret
AsmStereoNormal_ endp


AsmMonoNormal_ proc USES ebp es
        mov    ax,_lvolsel
        mov    es,ax                       ; voltab selector naar fs
        xor    eax,eax
        push   edx
        shr    edx,4
        jz     mskip16
        mov    ebp,edx
magain16:
        SM2    0
        SM2    1
        SM2    2
        SM2    3
        SM2    4
        SM2    5
        SM2    6
        SM2    7
        SM2    8
        SM2    9
        SM2    10
        SM2    11
        SM2    12
        SM2    13
        SM2    14
        SM2    15
        add    edi,(16*4)
        dec    ebp
        jnz    magain16
mskip16:
        pop    edx
        and    edx,15
        jz     mskip1
        mov    ebp,edx
magain1:
        SM2    0
        add    edi,4
        dec    ebp
        jnz    magain1
mskip1:
	ret
AsmMonoNormal_ endp

_TEXT   ENDS

_DATA   SEGMENT DWORD PUBLIC USE32 'DATA'
_DATA   ENDS

        END
