	section .text use32
	bits 32

WATER_WIDTH equ 512
WATER_HEIGHT equ 256

	global updateWaterAsm5_
updateWaterAsm5_:
	push ebx
	push ecx
	push edi
	push esi
	push ebp

	mov esi,eax
	mov edi,edx

	mov ecx,(WATER_WIDTH / 4) * (WATER_HEIGHT - 2) - 2
waterI:
		mov eax,[esi-1]
		mov ebx,[esi+1]
		add eax,[esi-WATER_WIDTH]
		add ebx,[esi+WATER_WIDTH]
		add eax,ebx
		shr eax,1
		and eax,0x7f7f7f7f
		mov edx,[edi]
		sub eax,edx

		mov ebx,eax
		and ebx,0x80808080

		mov edx,ebx
		shr edx,7

		mov ebp,ebx
		sub ebp,edx
		or ebx,ebp
		xor eax,ebx
		add eax,edx

		add esi,4
		mov [edi],eax
		add edi,4

	dec ecx
	jnz waterI

	pop ebp
	pop esi
	pop edi
	pop ecx
	pop ebx
	ret

	; vi:ft=nasm ts=8 sts=8 sw=8:
