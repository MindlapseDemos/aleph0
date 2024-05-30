	section .text use32
	bits 32
; foo_ are watcom functions, _foo are djgpp functions

	global get_cs
	global _get_cs
	global get_cs_
get_cs:
_get_cs:
get_cs_:
	xor eax, eax
	mov ax, cs
	ret

	global get_msr
	global _get_msr
get_msr:
_get_msr:
	push ebp
	mov ebp, esp
	push ebx
	mov ecx, [ebp + 8]
	rdmsr
	mov ebx, [ebp + 12]
	mov [ebx], eax
	mov ebx, [ebp + 16]
	mov [ebx], edx
	pop ebx
	pop ebp
	ret

	global get_msr_
get_msr_:
	push ebx
	push edx
	mov ecx, eax
	rdmsr
	pop ebx
	mov [ebx], eax
	pop ebx
	mov [ebx], edx
	ret

	global set_msr
	global _set_msr
set_msr:
_set_msr:
	mov ecx, [esp + 4]
	mov eax, [esp + 8]
	mov edx, [esp + 12]
	rdmsr
	ret

	global set_msr_
set_msr_:
	mov ecx, eax
	mov eax, edx
	mov edx, ebx
	wrmsr
	ret

	global memcpy64_mmx_
memcpy64_mmx_:
	movq mm0, [edx]
	movq [eax], mm0
	add edx, 8
	add eax, 8
	dec ebx
	jnz memcpy64_mmx_
	emms
	ret

	global memcpy64_nommx_
memcpy64_nommx_:
	push esi
	push edi
	push ecx
	mov esi, edx
	mov edi, eax
	mov ecx, ebx
	shl ecx, 1
	rep movsd
	pop ecx
	pop edi
	pop esi
	ret

	global memcpy64_mmx
	global _memcpy64_mmx
memcpy64_mmx:
_memcpy64_mmx:
	mov edx, [esp + 4]
	mov eax, [esp + 8]
	mov ecx, [esp + 12]
.cploop:
	movq mm0, [eax]
	movq [edx], mm0
	add eax, 8
	add edx, 8
	dec ecx
	jnz .cploop
	emms
	ret

	global memcpy64_nommx
	global _memcpy64_nommx
memcpy64_nommx:
_memcpy64_nommx:
	push esi
	push edi
	mov esi, [esp + 16]
	mov edi, [esp + 12]
	mov ecx, [esp + 20]
	shl ecx, 1
	rep movsd
	pop edi
	pop esi
	ret

; vi:ft=nasm:
