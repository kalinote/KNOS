; ----------------
;   boot.asm
;   引导扇区文件
;   KNOS
; ----------------

    org 0x7c00

BaseOfStack		equ		0x7c00

; FAT12格式
;	jmp	short start				; 跳转到启动代码
;	nop
;	BS_OEMName		db	'KNOSBOOT'			; 启动区名称(8字节)
;	BPB_BytesPerSec	dw	512					; 每个扇区大小为0x200
;	BPB_SecPerClus	db	1					; 每个簇大小为一个扇区
;	BPB_RsvdSecCnt	dw	1					; FAT起始位置(boot记录占用扇区数)
;	BPB_NumFATs		db	2          		    ; FAT表数
;	BPB_RootEntCnt	dw	224					; 根目录最大文件数
;	BPB_TotSec16	dw	2880				; 磁盘大小(总扇区数，如果这里扇区数为0，则由下面给出)
;	BPB_Media		db	0xf0                ; 磁盘种类
;	BPB_FATSz16		dw	9					; FAT长度(每个FAT表占用扇区数)
;	BPB_SecPerTrk	dw	18					; 每个磁道的扇区数
;	BPB_NumHeads	dw	2					; 磁头数(面数)
;	BPB_HiddSec		dd	0                   ; 不使用分区(隐藏扇区数)
;	BPB_TotSec32	dd	0					; 重写一遍磁盘大小(如果上面的扇区数为0，则由这里给出)
;	BS_DrvNum		db	0                   ; INT 13H的驱动器号
;	BS_Reserved1	db	0					; 保留
;	BS_BootSig		db	0x29                ; 扩展引导标记(29h)磁盘名称(11字节)
;	BS_VolID		dd	0                   ; 卷序列号
;	BS_VolLab		db	'KNOS       '       ; 磁盘名称(11字节)
;	BS_FileSysType	db	'FAT12   '			; 磁盘格式名称(8字节)

; 启动代码
start:
    ; 初始化
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, BaseOfStack

; 在屏幕上显示信息
	; 显示start booting KNOS
	; 清屏(ah=0x06, al=0x00)
	mov ah, 06h		; int 10中断，使用0x06 ah功能号时，按照指定范围滚动窗口
	mov al, 00h		; al为0时表示进行清屏操作
	int 10h

	; 设置光标位置
	mov ah, 02h		; int 10h的02h功能号设定光标位置
	mov dx, 0000h	; dh为列数，dl为行数
	mov bh, 00h		; 页码
	int 10h

	; 显示start booting KNOS
	mov	ah,	13h		; 显示字符串
	mov al, 01h		; 完成显示后将光标移动至字符串末尾
	mov	bh,	00h		; 页码(第0页)
	mov bl, 0fh		; 字符属性(白色高亮，黑色背景不闪烁)
	mov	dh,	00h		; 游标行号
	mov dl, 00h		; 游标列号
	mov	cx,	18		; 字符串长度
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartBootMessage		; es:bp 指向字符串地址
	int	10h

; 栈
StartBootMessage:	db	"Start booting KNOS"

; 填充至512字节，并设置启动扇区识别符
	times	510 - ($ - $$)	db	0
	dw	0xaa55
