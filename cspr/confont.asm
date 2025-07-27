	section .text USE32
	global cs_confont
	global _cs_confont
	global cs_confont_
cs_confont:
_cs_confont:
	mov eax, [esp + 12]
;	mov ecx, 640
;	mul ecx
	mov ecx, eax
	shl eax, 9
	shl ecx, 7
	add eax, ecx
	add eax, [esp + 8]
	add eax, [esp + 8]
	add eax, [esp + 4]
	mov edx, eax
	mov eax, [esp + 16]
	jmp [titletab + eax * 4]

cs_confont_:
	push eax
	mov eax, ebx
	shl eax, 9
	shl ebx, 7
	add eax, ebx
	shl edx, 1
	add eax, edx
	pop edx
	add edx, eax
	jmp [titletab + ecx * 4]

titletab:
	dd tile0
	dd tile1
	dd tile2
	dd tile3
	dd tile4
	dd tile5
	dd tile6
	dd tile7
	dd tile8
	dd tile9
	dd tile10
	dd tile11
	dd tile12
	dd tile13
	dd tile14
	dd tile15
	dd tile16
	dd tile17
	dd tile18
	dd tile19
	dd tile20
	dd tile21
	dd tile22
	dd tile23
	dd tile24
	dd tile25
	dd tile26
	dd tile27
	dd tile28
	dd tile29
	dd tile30
	dd tile31
	dd tile32
	dd tile33
	dd tile34
	dd tile35
	dd tile36
	dd tile37
	dd tile38
	dd tile39
	dd tile40
	dd tile41
	dd tile42
	dd tile43
	dd tile44
	dd tile45
	dd tile46
	dd tile47
	dd tile48
	dd tile49
	dd tile50
	dd tile51
	dd tile52
	dd tile53
	dd tile54
	dd tile55
	dd tile56
	dd tile57
	dd tile58
	dd tile59
	dd tile60
	dd tile61
	dd tile62
	dd tile63
	dd tile64
	dd tile65
	dd tile66
	dd tile67
	dd tile68
	dd tile69
	dd tile70
	dd tile71
	dd tile72
	dd tile73
	dd tile74
	dd tile75
	dd tile76
	dd tile77
	dd tile78
	dd tile79
	dd tile80
	dd tile81
	dd tile82
	dd tile83
	dd tile84
	dd tile85
	dd tile86
	dd tile87
	dd tile88
	dd tile89
	dd tile90
	dd tile91
	dd tile92
	dd tile93
	dd tile94
	dd tile95

tile0:
	ret
tile1:
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile2:
	add edx, 2
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	ret
tile3:
	add edx, 2
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	ret
tile4:
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 644
	mov word [edx + 0], 0xffff
	ret
tile5:
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile6:
	add edx, 2
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile7:
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile8:
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	ret
tile9:
	add edx, 2
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	ret
tile10:
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	ret
tile11:
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile12:
	add edx, 1924
	mov dword [edx + 0], 0xffffffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	ret
tile13:
	add edx, 1920
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
tile14:
	add edx, 3202
	mov dword [edx + 0], 0xffffffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	ret
tile15:
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile16:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 6
	mov dword [edx + 0], 0xffffffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile17:
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile18:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
tile19:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile20:
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile21:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 648
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile22:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile23:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile24:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile25:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile26:
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	add edx, 1280
	mov dword [edx + 0], 0xffffffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	ret
tile27:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	add edx, 1280
	mov dword [edx + 0], 0xffffffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	ret
tile28:
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	ret
tile29:
	add edx, 1280
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 1280
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
tile30:
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	ret
tile31:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 1280
	mov word [edx + 0], 0xffff
	ret
tile32:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov dword [edx + 0], 0xffffffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile33:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile34:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile35:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile36:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile37:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
tile38:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile39:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 6
	mov dword [edx + 0], 0xffffffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile40:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile41:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile42:
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile43:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile44:
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
tile45:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	add edx, 6
	mov dword [edx + 0], 0xffffffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile46:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 6
	mov dword [edx + 0], 0xffffffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile47:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile48:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile49:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	add edx, 6
	mov word [edx + 0], 0xffff
	ret
tile50:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile51:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile52:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile53:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile54:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile55:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	ret
tile56:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile57:
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile58:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 648
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
tile59:
	add edx, 4
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile60:
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile61:
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile62:
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile63:
	add edx, 3840
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
tile64:
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	ret
tile65:
	add edx, 1282
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile66:
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile67:
	add edx, 1282
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile68:
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile69:
	add edx, 1282
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile70:
	add edx, 4
	mov dword [edx + 0], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile71:
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile72:
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile73:
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile74:
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	ret
tile75:
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile76:
	add edx, 2
	mov dword [edx + 0], 0xffffffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile77:
	add edx, 1280
	mov dword [edx + 0], 0xffffffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	ret
tile78:
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 4
	mov dword [edx + 0], 0xffffffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile79:
	add edx, 1282
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile80:
	add edx, 1280
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile81:
	add edx, 1282
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile82:
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 4
	mov dword [edx + 0], 0xffffffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile83:
	add edx, 1282
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 632
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	ret
tile84:
	add edx, 2
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 6
	mov word [edx + 0], 0xffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	ret
tile85:
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 6
	mov dword [edx + 0], 0xffffffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	add edx, 6
	mov word [edx + 0], 0xffff
	ret
tile86:
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	ret
tile87:
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 636
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	ret
tile88:
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 634
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	ret
tile89:
	add edx, 1280
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 632
	mov word [edx + 0], 0xffff
	add edx, 8
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 634
	mov dword [edx + 0], 0xffffffff
	mov word [edx + 4], 0xffff
	ret
tile90:
	add edx, 1280
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 646
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 638
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
tile91:
	add edx, 6
	mov dword [edx + 0], 0xffffffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 642
	mov dword [edx + 0], 0xffffffff
	ret
tile92:
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	ret
tile93:
	mov dword [edx + 0], 0xffffffff
	add edx, 644
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 642
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 640
	mov word [edx + 0], 0xffff
	add edx, 636
	mov dword [edx + 0], 0xffffffff
	ret
tile94:
	add edx, 2
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 4
	mov word [edx + 0], 0xffff
	add edx, 638
	mov word [edx + 0], 0xffff
	ret
tile95:
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	add edx, 640
	mov dword [edx + 0], 0xffffffff
	mov dword [edx + 4], 0xffffffff
	mov word [edx + 8], 0xffff
	ret
