; =========================
;   bootloader/boot.asm
;   引导扇区程序
;   KNOS
; =========================

    org 0x7c00

StackBase		equ		0x7c00

; Loader程序的相关配置
	LoaderBase		equ		0x1000	; Loader程序的基地址
	LoaderOffset	equ		0x0000	; Loader程序的偏移地址
	; 通过上面两个地址可以计算出Loader在内存中的物理地址为0x10000

; FAT12格式
	jmp	short start				; 跳转到启动代码
	nop

%include	"fat12.inc"

; 启动代码
start:
    ; 初始化
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, StackBase

; 在屏幕上显示信息
	; 显示start booting KNOS
	; 清屏(ah=0x06, al=0x00)
	mov ah, 06h		; int 10中断，使用0x06 ah功能号时，按照指定范围滚动窗口
	mov al, 00h		; al为0时表示进行清屏操作
	mov	bx,	0700h
	mov	cx,	0
	mov	dx,	0184fh
	int 10h

	; 设置光标位置
	mov ah, 02h		; int 10h的02h功能号设定光标位置
	mov dx, 0000h	; dh为列数，dl为行数
	mov bh, 00h		; 页码
	int 10h

	; 显示start booting KNOS
	; 为了缩减MBR程序，去掉显示该字符串(反正也看不到)
	; mov	ah,	13h		; 显示字符串
	; mov al, 01h		; 完成显示后将光标移动至字符串末尾
	; mov	bh,	00h		; 页码(第0页)
	; mov bl, 0fh		; 字符属性(白色高亮，黑色背景不闪烁)
	; mov	dh,	00h		; 游标行号
	; mov dl, 00h		; 游标列号
	; mov	cx,	12		; 字符串长度
	; push	ax
	; mov	ax,	ds
	; mov	es,	ax
	; pop	ax
	; mov	bp,	StartBootMessage		; es:bp 指向字符串地址
	; int	10h

; ————————————重置软盘驱动器————————————
	xor ah, ah
	xor dl, dl
	int 13h

; ————————————搜索loader.kal————————————
	mov word [SectorNumber], RootDirStartSectors		; 将SectorNumber变量指向根目录开始的扇区
SearchRootDir:
	cmp word [RootDirSearchLoop], 0			; 如果RootDirSearchLoop不是0，则说明根目录扇区还没有遍历完
	jz LoaderNotFound						; 如果RootDirSearchLoop已经为0，说明根目录区没有找到loader，跳转到未找到
	dec word [RootDirSearchLoop]			; 剩余扇区-1
	; 下面调用封装的读取软盘程序
	mov ax, 0000h
	mov es, ax
	mov bx, 7e00h				; 将读取的内容保存在内存0x0000:0x7e00处(es:bx)
	mov ax, [SectorNumber]		; 读取位置
	mov cl, 1					; 一次读一个扇区(这里可以灵活调整)
	call ReadOneSectorFunction
	mov si, LoaderFileName
	mov di, 7e00h
	cld
	mov dx, 10h					; 每扇区容纳目录项个数(512 / 32 = 16)
SearchLoader:
	cmp dx, 0					; 该扇区目录项已搜索完毕
	jz SearchNextDirSector		
	dec dx
	mov cx, 11					; 文件名长度(8字节文件名+3字节扩展名)
CompareFileName:
	cmp cx, 0
	jz FindLoaderFile
	dec cx
	lodsb						; (在这段程序中)这条指令可以把DS:SI指定的内存地址读入al(文件名)
	cmp al, byte [es:di]
	jz ContinueComparison		; 此次匹配，继续进行匹配，知道11字节匹配完毕
	jmp FileNameNotMatch		; 文件名不匹配，搜索下一条目录
ContinueComparison:
	inc di
	jmp CompareFileName

FileNameNotMatch:
	and di, 0ffe0h
	add di, 20h
	mov si, LoaderFileName
	jmp SearchLoader

; ————————————该扇区目录搜索完毕，继续读取下一扇区————————————
SearchNextDirSector:
	add word [SectorNumber], 1			; 需要读取的扇区号+1
	jmp SearchRootDir

; ————————————没有找到loader，显示错误信息然后循环hlt————————————
LoaderNotFound:
	mov	ah,	13h		; 显示字符串
	mov al, 01h		; 完成显示后将光标移动至字符串末尾
	mov	bh,	00h		; 页码(第0页)
	mov bl, 8ch		; 字符属性(红色高亮，黑色背景闪烁)
	mov	dh,	01h		; 游标行号
	mov dl, 00h		; 游标列号
	mov	cx,	21		; 字符串长度
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderFoundMessage		; es:bp 指向字符串地址
	int	10h
	jmp LoopHLTFunction		; 跳转到死循环

; ————————————在根目录表中找到loader.kal————————————
FindLoaderFile:
	mov ax, RootDirSectors
	and di, 0ffe0h
	add di, 01ah
	mov cx, word [es:di]
	push cx
	add cx, ax
	add cx, SectorBalance
	mov ax, LoaderBase
	mov es, ax
	mov bx, LoaderOffset
	mov ax, cx
LoadingFile:
	push ax
	push bx
	mov ah, 0eh
	mov al, '*'				; 每加载一个扇区，就打印一个"*"
	mov bl, 0fh
	int 10h
	pop bx
	pop ax

	mov cl, 1
	call ReadOneSectorFunction
	pop ax
	call DecodeFAT
	cmp ax, 0fffh
	jz LoaderLoadSuccess
	push ax
	mov dx, RootDirSectors
	add ax, dx
	add ax, SectorBalance
	add bx, [BytesPreSector]
	jmp LoadingFile
LoaderLoadSuccess:
	jmp LoaderBase:LoaderOffset			; 跳转到loader继续执行，正常流程走下来，这一步应该是引导程序的最后一步

; ————————————软盘驱动器处理程序(封装函数)————————————

; 函数：ReadOneSectorFunction
	; 从软盘读取数据(这一部分代码是对int 13h的2号功能的简单封装，使其更容易使用)
	; 参数：
	; 	AX - 待读取的起始扇区号(LAB)
	;	CL - 读取扇区数量
	;	ES:BX - 目标缓冲区起始地址
ReadOneSectorFunction:
	; 在调用int 13h前需要先把LAB转换成CHS
	push bp
	mov bp, sp
	sub esp, 2
	mov byte [bp - 2], cl		; 找个地方存需要读取的扇区数
	push bx
	mov bl,	[SectorsPreTrack]
	div bl
	inc ah
	mov cl, ah
	mov dh, al
	shr al, 1
	mov ch, al
	and dh, 1
	pop bx
	mov dl, [DriveNumber]
ReRead:
	mov	ah,	2
	mov	al,	byte [bp - 2]
	int	13h
	jnc ReadSuccess		; 读取成功，跳到success执行，否则继续向下执行
	add si, 1			; 每次出错si加1
	cmp si, 5			
	jae ReadError		; 如果重试5次还是出错，则跳转到ReadError
	jmp ReRead			; 不足5次继续重试
ReadSuccess:			; 读取成功，恢复调用现场
	add	esp, 2
	pop	bp
	ret
ReadError:		; 读取数据出现错误(这里想的是先重试，重试5次都不行就报错
	jmp LoopHLTFunction

; 函数: LoopHLTFunction
	; 调用此函数以进入hlt循环，常用于出现错误时使用
LoopHLTFunction:		; 出错时跳转到这里进行死循环
	hlt
	jmp LoopHLTFunction

; 函数 DecodeFAT
DecodeFAT:
	push es
	push bx
	push ax
	mov ax, 0000h
	mov es, ax
	pop ax
	mov byte [Odd], 0
	mov bx, 3
	mul bx
	mov bx, 2
	div	bx
	cmp	dx,	0
	jz Even
	mov byte [Odd], 1
Even:
	xor dx, dx
	mov bx, [BytesPreSector]
	div bx
	push dx
	mov bx, 7e00h
	add ax, FAT1StartSectors
	mov cl, 2
	call ReadOneSectorFunction
	pop dx
	add bx, dx
	mov ax, [es:bx]
	cmp byte [Odd],	1
	jnz Even2
	shr ax, 4
Even2:
	and	ax,	0fffh
	pop	bx
	pop	es
	ret

; 变量空间
RootDirSearchLoop		dw	RootDirSectors						; 通过根目录占用扇区大小来设置循环搜索次数
SectorNumber			dw	0									; 扇区号变量(找个内存地址保存相关变量值)
Odd						db	0									; 这个变量标记了奇偶性(奇数为1，偶数为0)
; 栈
; StartBootMessage:		db	"Booting KNOS"				; 启动显示信息
LoaderFileName:			db	"LOADER  KAL",0						; 需要搜索的Loader程序文件名
NoLoaderFoundMessage:	db	"LOADER.KAL not found!"				; loader.kal未找到的错误信息

; 填充至512字节，并设置启动扇区识别符
	times 510 - ($ - $$) db 0
	dw 0xaa55
