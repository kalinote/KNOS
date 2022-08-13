org 90000h

    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7c00

	; 显示Entered Loader
	mov	ah,	13h		; 显示字符串
	mov al, 01h		; 完成显示后将光标移动至字符串末尾
	mov	bh,	00h		; 页码(第0页)
	mov bl, 0fh		; 字符属性(白色高亮，黑色背景不闪烁)
	mov	dh,	02h		; 游标行号
	mov dl, 00h		; 游标列号
	mov	cx,	14		; 字符串长度
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartBootMessage		; es:bp 指向字符串地址
	int	10h

    jmp $

StartBootMessage:   db  "Entered Loader"