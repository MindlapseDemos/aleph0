	.section .rodata

	.globl font_glyphmap
	.globl font_glyphmap_size
font_glyphmap:
	.incbin "font.glyphmap"
font_glyphmap_end:

	.align 4
font_glyphmap_size:
	.long font_glyphmap_end - font_glyphmap
