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


; vi:ft=nasm:
