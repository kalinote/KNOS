; ===================================================================
;   bootloader/loader.asm
;	系统loader程序
; ===================================================================
org	10000h                      ; 指定代码起始地址为 0x10000
	jmp	Label_Start            ; 无条件跳转到16位代码入口 Label_Start

%include	"fat12.inc"           ; 包含 FAT12 文件系统相关常量和数据结构定义

; -----------------------------
; 内核文件加载相关地址定义
; -----------------------------
BaseOfKernelFile	equ	0x00         ; 内核文件在内存中的基地址（段基址，一般为0）
OffsetOfKernelFile	equ	0x100000    ; 内核文件加载到内存的偏移地址（1MB处）

BaseTmpOfKernelAddr	equ	0x00         ; 临时内核文件加载时使用的基地址（段基址，通常为0）
OffsetTmpOfKernelFile	equ	0x7E00      ; 临时内核文件加载偏移地址（一般放在低内存）

MemoryStructBufferAddr	equ	0x7E00   ; 内存结构信息存放缓冲区地址

; -----------------------------
; VBE（视频扩展 BIOS）相关地址定义
; -----------------------------
VBE_INFO_BLOCK     		equ 0x8000   ; 存放VBE信息块的内存物理地址（0:0x8000）
VBE_MODE_LIST_OFF  		equ 0x800E   ; VBE模式列表的偏移地址（在VBE信息块内）
VBE_MODE_LIST_SEG  		equ 0x8010   ; VBE模式列表的段地址（在VBE信息块内）
VBE_MODE_INFO_BLOCK 	equ 0x8200   ; 存放单个VBE模式信息的缓冲区地址

VBE_MODE_SELECTED_INFO	equ 0x8400	; [临时]存放最终选择的VBE模式信息缓冲区地址

DEFAULT_X_RES	equ	1024
DEFAULT_Y_RES	equ	768
DEFAULT_BPP		equ	32

; ===================================================================
; 32位保护模式下的全局描述符表 (GDT) 定义
; ===================================================================
[SECTION gdt]

LABEL_GDT:		
	dd	0,0                   ; 第0项：空描述符（必须项）
LABEL_DESC_CODE32:	
	dd	0x0000FFFF,0x00CF9A00  ; 32位代码段描述符，访问位和属性设定：可读、执行、存在等
LABEL_DESC_DATA32:	
	dd	0x0000FFFF,0x00CF9200  ; 32位数据段描述符，属性设定：可读写、存在等

GdtLen	equ	$ - LABEL_GDT       ; 计算GDT总长度
GdtPtr	dw	GdtLen - 1          ; GDT界限（长度-1）
	dd	LABEL_GDT             ; GDT在内存中的起始地址

; 用于选择子计算，选择子为描述符在GDT内的偏移
SelectorCode32	equ	LABEL_DESC_CODE32 - LABEL_GDT
SelectorData32	equ	LABEL_DESC_DATA32 - LABEL_GDT

; ===================================================================
; 64位长模式下的全局描述符表 (GDT64) 定义
; ===================================================================
[SECTION gdt64]

LABEL_GDT64:		
	dq	0x0000000000000000        ; 第0项：空描述符
LABEL_DESC_CODE64:	
	dq	0x0020980000000000        ; 64位代码段描述符，属性与基地址等信息预先设置
LABEL_DESC_DATA64:	
	dq	0x0000920000000000        ; 64位数据段描述符

GdtLen64	equ	$ - LABEL_GDT64     ; 计算64位GDT总长度
GdtPtr64	dw	GdtLen64 - 1        ; 64位GDT界限
		dd	LABEL_GDT64         ; 64位GDT起始地址

SelectorCode64	equ	LABEL_DESC_CODE64 - LABEL_GDT64
SelectorData64	equ	LABEL_DESC_DATA64 - LABEL_GDT64

; ===================================================================
; 16位实模式/过渡模式代码段（初始启动代码）
; ===================================================================
[SECTION .s16]
[BITS 16]

Label_Start:
	; ======= 设置段寄存器 =======
	mov	ax, cs              ; 将当前代码段选择子复制到 AX
	mov	ds, ax              ; DS 设为当前代码段（数据段使用与代码段相同）
	mov	es, ax              ; ES 同 DS 一样
	mov	ax, 0x00
	mov	ss, ax              ; SS 设为 0
	mov	sp, 0x7c00          ; 设置堆栈指针，通常从 0x7C00 开始

	; ======= 显示 "Start Loader" 信息在屏幕上 =======
	mov	ax, 1301h           ; BIOS显示字符串功能代码（带颜色等属性）
	mov	bx, 000fh           ; 显示颜色（白色）
	mov	dx, 0200h           ; 行号 2
	mov	cx, 12              ; 字符串长度
	push	ax
	mov	ax, ds
	mov	es, ax
	pop	ax
	mov	bp, StartLoaderMessage  ; 指向显示信息的字符串
	int	10h                 ; 调用BIOS视频中断

	; ======= 打开 A20 地址线（使能超过1MB内存访问） =======
	push	ax
	in	al, 92h             ; 从端口 0x92 读取当前状态
	or	al, 00000010b       ; 设置 A20 地址线使能位（bit1）
	out	92h, al            ; 写回端口 0x92
	pop	ax

	cli                     ; 关闭中断

	; ======= 加载32位保护模式下的GDT =======
	db	0x66               ; 操作数前缀，转换为32位操作数模式
	lgdt	[GdtPtr]          ; 加载GDT寄存器，准备进入保护模式

	; ======= 进入保护模式（设置CR0的保护模式位） =======
	mov	eax, cr0
	or	eax, 1              ; 设置CR0最低位为1，开启保护模式
	mov	cr0, eax

	; ======= 切换段寄存器到32位段描述符 =======
	mov	ax, SelectorData32  ; 将32位数据段选择子加载到AX
	mov	fs, ax              ; FS 寄存器设为数据段
	; 此处做个伪保护模式退出操作：
	mov	eax, cr0
	and	al, 11111110b      ; 清除最低位（保护模式位）
	mov	cr0, eax

	sti                     ; 重新开启中断

	; ======= 重置软驱（调用 int 13h，复位软驱控制器） =======
	xor	ah, ah
	xor	dl, dl
	int	13h

	; ======= 搜索内核文件 kernel.kal =======
	; 将根目录起始扇区号放入 [SectorNo]
	mov	word	[SectorNo],	RootDirStartSectors

Label_Search_In_Root_Dir_Begin:
	; 检查根目录剩余扇区是否遍历完毕
	cmp	word	[RootDirSizeForLoop],	0
	jz	Label_No_LoaderKal    ; 如果遍历完毕还没找到，则显示错误信息
	dec	word	[RootDirSizeForLoop]	
	; 读取当前根目录扇区到内存 0:8000h
	mov	ax, 00h
	mov	es, ax
	mov	bx, 8000h
	mov	ax,	[SectorNo]
	mov	cl,	1
	call	Func_ReadOneSector  ; 读取1个扇区
	; 准备比较文件名：内核文件名在 KernelFileName 中
	mov	si,	KernelFileName
	mov	di,	8000h
	cld
	mov	dx,	10h             ; 每个目录项大小或文件名长度（可能有多个目录项）

Label_Search_For_LoaderKal:
	; 循环比较目录中每个文件项的文件名
	cmp	dx,	0
	jz	Label_Goto_Next_Sector_In_Root_Dir
	dec	dx
	mov	cx,	11             ; 文件名长度为11字节（8.3格式）
Label_Cmp_FileName:
	cmp	cx,	0
	jz	Label_FileName_Found   ; 文件名匹配成功，跳转到文件加载流程
	dec	cx
	lodsb	                ; 从 DS:SI 加载下一个字符到 AL
	cmp	al,	byte	[es:di]   ; 比较当前字符与目录项内的字符
	jz	Label_Go_On         ; 若相等则继续比较
	jmp	Label_Different    ; 否则表示不同
Label_Go_On:
	inc	di                ; 指向下一个目录项字符
	jmp	Label_Cmp_FileName

Label_Different:
	; 若不匹配，则调整目录项指针到下一个文件项（每项长度为32字节）
	and	di, 0FFE0h
	add	di, 20h
	mov	si,	KernelFileName
	jmp	Label_Search_For_LoaderKal

Label_Goto_Next_Sector_In_Root_Dir:
	; 当前扇区文件项未匹配，读下一扇区
	add	word	[SectorNo],	1
	jmp	Label_Search_In_Root_Dir_Begin

; ======= 显示 "ERROR:No KERNEL Found" 错误信息 =======
Label_No_LoaderKal:
	mov	ax,	1301h
	mov	bx,	008Ch           ; 错误颜色设置
	mov	dx,	0300h           ; 在屏幕第3行显示
	mov	cx,	21              ; 字符串长度
	push	ax
	mov	ax,	ds
	mov	es, ax
	pop	ax
	mov	bp,	NoLoaderMessage ; 错误提示字符串
	int	10h
	jmp	$                   ; 死循环

; ======= 找到内核文件目录项，开始加载内核文件 =======
Label_FileName_Found:
	; 计算内核文件大小（存放在目录项中某处）
	mov	ax,	RootDirSectors
	; 将目录项指针调整到文件大小字段所在位置
	and	di,	0FFE0h
	add	di,	01Ah
	mov	cx,	word	[es:di]  ; 读取文件大小（以扇区计数或其它单位）
	push	cx
	add	cx,	ax
	add	cx,	SectorBalance  ; 计算文件所占扇区数（包含扇区平衡值）
	mov	eax,	BaseTmpOfKernelAddr	; 目标加载内存基地址
	mov	es,	eax
	mov	bx,	OffsetTmpOfKernelFile	; 目标加载内存偏移地址
	mov	ax,	cx

Label_Go_On_Loading_File:
	; 显示加载过程中打印一个点或标识字符'.'
	push	ax
	push	bx
	mov	ah,	0Eh
	mov	al,	'.'
	mov	bl,	0Fh
	int	10h
	pop	bx
	pop	ax

	; 读取内核文件的1个扇区到内存缓冲区
	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax

	; ======= 将临时加载的数据拷贝到最终内核加载区域 =======
	push	cx
	push	eax
	push	edi
	push	ds
	push	esi

	mov	cx,	200h             ; 复制 0x200 字节（一个扇区大小）
	mov	ax,	BaseOfKernelFile   ; 目标内核段基地址
	mov	fs,	ax
	mov	edi,	dword	[OffsetOfKernelFileCount]  ; 目标内核偏移地址计数器

	mov	ax,	BaseTmpOfKernelAddr  ; 源数据段基地址
	mov	ds,	ax
	mov	esi,	OffsetTmpOfKernelFile  ; 源数据偏移地址

Label_Mov_Kernel:	
	; 循环复制扇区数据到最终内核加载地址
	mov	al,	byte	[ds:esi]
	mov	byte	[fs:edi],	al
	inc	esi
	inc	edi
	loop	Label_Mov_Kernel

	; 切换数据段为0x1000（可能为内核段转换前的临时段）
	mov	eax,	0x1000
	mov	ds,	eax

	; 更新内核加载偏移计数器
	mov	dword	[OffsetOfKernelFileCount],	edi

	pop	esi
	pop	ds
	pop	edi
	pop	eax
	pop	cx

	; ======= 获取FAT表中的下一个扇区号 =======
	call	Func_GetFATEntry
	cmp	ax,	0FFFh        ; FAT结尾标记判断（0xFFF 表示最后一簇）
	jz	Label_File_Loaded    ; 如果达到结束，则跳转到加载结束处理
	push	ax
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance
	jmp	Label_Go_On_Loading_File

Label_File_Loaded:
	; 文件加载完成后，在屏幕上显示一个 'G' 字母（提示加载成功）
	mov	ax, 0B800h         ; 文本模式显存段地址
	mov	gs, ax
	mov	ah, 0Fh           ; 字符属性：黑底白字
	mov	al, 'G'
	; 在屏幕第0行，第39列显示字符
	mov	[gs:((80 * 0 + 39) * 2)], ax

KillMotor:
	; 关闭软驱马达（防止持续转动）
	push	dx
	mov	dx,	03F2h
	mov	al,	0	
	out	dx,	al
	pop	dx

	; ======= 获取内存结构信息（调用 INT 15h AH=0xE820） =======
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0400h          ; 在屏幕第4行显示
	mov	cx,	24
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetMemStructMessage  ; 显示 "Start Get Memory Struct."
	int	10h

	mov	ebx,	0
	mov	ax,	0x00
	mov	es,	ax
	mov	di,	MemoryStructBufferAddr  ; 内存结构信息缓冲区

Label_Get_Mem_Struct:
	; 调用 INT 15h, AH=0xE820 获取内存映射信息，ECX=20 表示缓冲区大小
	mov	eax,	0x0E820
	mov	ecx,	20
	mov	edx,	0x534D4150   ; “SMAP”签名
	int	15h
	jc	Label_Get_Mem_Fail     ; 如果出错，跳转到错误处理
	add	di,	20             ; 每次记录20字节的内存结构信息
	cmp	ebx,	0
	jne	Label_Get_Mem_Struct
	jmp	Label_Get_Mem_OK

Label_Get_Mem_Fail:
	; 获取内存结构失败，显示错误信息
	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0500h          ; 在屏幕第5行显示
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructErrMessage
	int	10h
	jmp	$

Label_Get_Mem_OK:
	; 获取内存结构成功，显示成功信息
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0600h          ; 在屏幕第6行显示
	mov	cx,	29
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructOKMessage
	int	10h	

	; ======= 获取 SVGA VBE 信息 =======
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0800h          ; 在屏幕第8行显示
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetSVGAVBEInfoMessage
	int	10h

	; 调用 INT 10h, AX=4F00h 获取VBE信息块
	; 准备512字节缓冲区（ES:DI=0:0x8000），并写入'VBE2'标志以表明支持VBE2.0+
	mov	ax,	0x00
	mov	es,	ax
	mov	di,	VBE_INFO_BLOCK
	; 写入'VBE2'标志
	mov word [es:di], 'VB'
	mov word [es:di+2], 'E2'
	mov cx, 512             ; 缓冲区大小为512字节
	mov	ax,	4F00h
	int	10h
	cmp	ax,	004Fh
	jz	.KO           ; 如果AX==004Fh表示成功，则跳转到 .KO 后续处理

	; ======= VBE获取失败，显示错误信息 =======
	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0900h          ; 在屏幕第9行显示
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAVBEInfoErrMessage
	int	10h
	jmp	$

.KO:
	; VESA标识
	mov al, 0dh
    call print_char
    mov al, 0ah
    call print_char
	mov al, [es:VBE_INFO_BLOCK]
    call print_char
	mov al, [es:VBE_INFO_BLOCK + 1]
    call print_char
	mov al, [es:VBE_INFO_BLOCK + 2]
    call print_char
	mov al, [es:VBE_INFO_BLOCK + 3]
    call print_char

	; 打印VBE版本信息
	mov al, ' '
    call print_char
	mov al, 'v'
    call print_char
	mov ax, [es:VBE_INFO_BLOCK + 0x4]
    call print_hex16
	mov al, 0dh
    call print_char
    mov al, 0ah
    call print_char

	; VBE信息块已成功存放在 0:0x8000
	; 获取模式列表：VBE信息块内偏移地址VBE_MODE_LIST_OFF和段地址VBE_MODE_LIST_SEG
	mov ax, 0
	mov es, ax             ; ES=0，指向物理地址 0x8000
	mov bx, [es:VBE_MODE_LIST_OFF]  ; 获取模式列表偏移量
	mov cx, [es:VBE_MODE_LIST_SEG]  ; 获取模式列表段地址
	; 切换到VBE模式数据段，设置 ES 为模式列表段地址，SI为偏移
	mov es, cx
	mov si, bx

; 开始循环遍历所有支持的SVGA模式
GetModeListLoop:
    ; 1) 读取当前模式号（16位模式号存储在ES:SI处）
    mov cx, [es:si]

    ; 2) 判断是否已经到模式列表结束标志 0xFFFF
    cmp cx, 0FFFFh
    jz EndModeListLoop
	
    ; 3) 打印当前模式号（16进制显示）
    push ax
    mov ax, cx
    call print_hex16   ; 调用打印16位十六进制数的函数
    pop ax

    ; 4) 打印一个空格分隔符
    mov al, ' '
    call print_char

    ; ---------------------------------
    ; 调用 INT 10h, AX=4F01h 获取模式信息
    ; ---------------------------------
    push es         ; 保存当前ES（VBE模式列表所在段）
    mov ax, 0
    mov es, ax      ; 切换ES=0，指向VBE_MODE_INFO_BLOCK所在物理地址
    mov bx, cx      ; BX = 当前模式号
    mov ax, 4F01h   ; VBE函数：获取模式信息
    mov di, VBE_MODE_INFO_BLOCK  ; 存放模式信息的缓冲区地址
    int 10h
    cmp ax, 0x004F
    jnz ModeInfoFail

    ; ---------------------------------
    ; 解析并显示模式信息
    ; ---------------------------------
    ; 显示分辨率宽度（存放在偏移0x12处）
    mov ax, [es:VBE_MODE_INFO_BLOCK + 0x12]
    call print_dec
    mov al, 'x'
    call print_char
    ; 显示分辨率高度（偏移0x14处）
    mov ax, [es:VBE_MODE_INFO_BLOCK + 0x14]
    call print_dec
    mov al, ','
    call print_char
    mov al, ' '
    call print_char
    ; 显示每像素位数（BitsPerPixel，偏移0x19处）
    mov al, [es:VBE_MODE_INFO_BLOCK + 0x19]
	mov ah, 0
    call print_dec
    ; 打印 "bpp" 字符串
    mov al, 'b'
    call print_char
    mov al, 'p'
    call print_char
    mov al, 'p'
    call print_char
	mov al, ' '
    call print_char
	; 显示线性帧缓冲区地址
	mov ax, [es:VBE_MODE_INFO_BLOCK + 0x2a]
    call print_hex16
	mov ax, [es:VBE_MODE_INFO_BLOCK + 0x28]
    call print_hex16
    ; 打印分割
	mov al, ' '
    call print_char
	mov al, '|'
    call print_char
	mov al, ' '
    call print_char

	; ---------------------------------
	; 判断模式号是否满足默认需求
	; ---------------------------------
    mov ax, [es:VBE_MODE_INFO_BLOCK + 0x12]     ; 读取 X 分辨率（VBE_MODE_INFO_BLOCK 偏移 0x12）
    cmp ax, DEFAULT_X_RES
    jne RestoreAndNextMode

    mov ax, [es:VBE_MODE_INFO_BLOCK + 0x14]     ; 读取 Y 分辨率（偏移 0x14）
    cmp ax, DEFAULT_Y_RES
    jne RestoreAndNextMode

    mov al, [es:VBE_MODE_INFO_BLOCK + 0x19]     ; 读取 BitsPerPixel（偏移 0x19）
    cmp al, DEFAULT_BPP
    jne RestoreAndNextMode

    mov [CandidateMode], cx		; 保存当前模式号到 CandidateMode

	; 保存模式信息
	push ds
	mov ax, 0
	mov ds, ax
	mov si, VBE_MODE_INFO_BLOCK
	mov di, VBE_MODE_SELECTED_INFO			; 目标地址
	mov cx, 256						; 复制 512 字节，即 256 个 WORD
	rep movsw
	pop ds

    jmp EndModeListLoop

RestoreAndNextMode:
    pop es         ; 恢复之前保存的ES（指向模式列表）
    ; ---------------------------------
    ; 指向下一个模式号（每个模式号占2字节）
    add si, 2
    jmp GetModeListLoop

ModeInfoFail:
	; 模式信息获取失败，显示错误信息
	mov ax, 1301h
	mov bx, 008Ch
	mov dx, 0000h   ; 在屏幕第0行显示错误信息
	mov cx, 24
	push ax
	mov ax, ds
	mov es, ax
	pop ax
	mov bp, GetSVGAModeInfoErrMessage
	int 10h
	jmp $

Label_SET_SVGA_Mode_VESA_VBE_FAIL:
	; 设置SVGA模式失败时显示错误信息
	mov ax, 1301h
	mov bx, 008Ch
	mov dx, 0100h   ; 在屏幕第1行显示
	mov cx, 19
	push ax
	mov ax, ds
	mov es, ax
	pop ax
	mov bp, SetSVGAModeErrMessage
	int 10h
	jmp	$

EndModeListLoop:	; 判断是否有符合的候选项
	cmp dword [CandidateMode], 0
	jz	Label_SET_SVGA_Mode_VESA_VBE_FAIL

	; 模式列表遍历完毕，显示成功信息
	mov ax, 1301h
	mov bx, 000Fh
	mov dx, 0000h   ; 在屏幕第0行显示
	mov cx, 30
	push ax
	mov ax, ds
	mov es, ax
	pop ax
	mov bp, GetSVGAModeInfoOKMessage
	int 10h

	; ======= 设置 SVGA 模式（调用 VBE函数 4F02h） =======
	mov	ax,	4F02h
	mov	bx,	[CandidateMode]	; 设置对应模式号
	add bx, 4000h			; 高分辨率扩展模式号需要以4开头，以启用线性帧缓冲
	int 	10h
	cmp	ax,	004Fh
	jnz	Label_SET_SVGA_Mode_VESA_VBE_FAIL

	; ======= 初始化 IDT、GDT 并切换到保护模式 =======
	cli			; 关闭中断

	db	0x66
	lgdt	[GdtPtr]         ; 重新加载32位GDT

	; 可选：加载IDT
	; db	0x66
	; lidt	[IDT_POINTER]

	mov	eax,	cr0
	or	eax,	1
	mov	cr0,	eax	
	; 跳转到伪保护模式下的临时代码入口（将进入32位代码段）
	jmp	dword SelectorCode32:GO_TO_TMP_Protect

; ===================================================================
; 32位保护模式代码段
; ===================================================================
[SECTION .s32]
[BITS 32]

GO_TO_TMP_Protect:
	; ======= 设置数据段寄存器，并调整堆栈 =======
	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	ss,	ax
	mov	esp,	7E00h

	; 检查 CPU 是否支持长模式（64位）
	call	support_long_mode
	test	eax,	eax
	jz	no_support         ; 如果不支持，则停在此处

	; ======= 初始化临时页表（物理地址0x90000处） =======
	; 以下设置几个页表项，具体含义：
	; [0x90000] 和 [0x90800] 分别为页目录项，对应不同内存区域
	; 属性0x91007 表示：存在、读写、用户权限等
	mov	dword	[0x90000],	0x91007
	mov	dword	[0x90004],	0x00000
	mov	dword	[0x90800],	0x91007
	mov	dword	[0x90804],	0x00000

	mov	dword	[0x91000],	0x92007
	mov	dword	[0x91004],	0x00000

	mov	dword	[0x92000],	0x000083
	mov	dword	[0x92004],	0x000000

	mov	dword	[0x92008],	0x200083
	mov	dword	[0x9200c],	0x000000

	mov	dword	[0x92010],	0x400083
	mov	dword	[0x92014],	0x000000

	mov	dword	[0x92018],	0x600083
	mov	dword	[0x9201c],	0x000000

	mov	dword	[0x92020],	0x800083
	mov	dword	[0x92024],	0x000000

	mov	dword	[0x92028],	0xa00083
	mov	dword	[0x9202c],	0x000000

	; ======= 加载64位GDT =======
	db	0x66
	lgdt	[GdtPtr64]
	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	ss,	ax
	mov	esp,	7E00h

	; ======= 开启PAE（物理地址扩展） =======
	mov	eax,	cr4
	bts	eax,	5          ; 将CR4的第5位（PAE使能位）置1
	mov	cr4,	eax

	; ======= 加载临时页表基址到CR3 =======
	mov	eax,	0x90000
	mov	cr3,	eax

	; ======= 开启长模式支持 =======
	; 读写MSR寄存器 IA32_EFER（0xC0000080），设置LME（Long Mode Enable，bit8）
	mov	ecx,	0C0000080h		; IA32_EFER MSR地址
	rdmsr
	bts	eax,	8           ; 设置第8位，开启长模式
	wrmsr

	; ======= 开启分页（PE） =======
	mov	eax,	cr0
	bts	eax,	0          ; 保持保护模式位
	bts	eax,	31         ; 设置分页使能位（PG，CR0的第31位）
	mov	cr0,	eax

	; ======= 跳转到64位内核入口 =======
	; 根据加载的内核文件，跳转到64位模式下的内核入口地址
	jmp	SelectorCode64:OffsetOfKernelFile

; ======= 检查 CPU 是否支持长模式的子程序 =======
support_long_mode:
	; 使用 CPUID 判断CPU支持情况
	mov	eax,	0x80000000
	cpuid
	cmp	eax,	0x80000001
	setnb	al	            ; 如果eax不低于0x80000001，则继续
	jb	support_long_mode_done
	mov	eax,	0x80000001
	cpuid
	bt	edx,	29         ; 测试EDX的第29位：是否支持长模式
	setc	al              ; 如果支持，则AL置1
support_long_mode_done:
	movzx	eax,	al         ; 将AL扩展到EAX返回
	ret

; ======= 如果不支持长模式，停在此处 =======
no_support:
	jmp	$

; ===================================================================
; 16位库函数部分（支持16位代码段下的辅助函数）
; ===================================================================
[SECTION .s16lib]
[BITS 16]

; -----------------------------
; Func_ReadOneSector: 从软驱读取1个扇区
; 使用 int 13h 读取扇区，内含错误重试循环
; 输入：cl中扇区数（一般为1），[SectorNo] 存放要读取的扇区号
; -----------------------------
Func_ReadOneSector:
	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl  ; 保存要读取的扇区数
	push	bx
	mov	bl,	[SectorsPreTrack]
	div	bl                ; 计算柱面、磁头、扇区参数
	inc	ah
	mov	cl,	ah
	mov	dh,	al
	shr	al,	1
	mov	ch,	al
	and	dh,	1
	pop	bx
	mov	dl,	[DriveNumber]   ; 软驱号
Label_Go_On_Reading:
	mov	ah,	2              ; INT 13h 读取扇区函数号
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading  ; 若出错则重试
	add	esp,	2
	pop	bp
	ret

; -----------------------------
; Func_GetFATEntry: 获取FAT表中的下一个文件簇号
; 根据当前读取的目录项计算对应FAT项位置并返回文件簇号
; -----------------------------
Func_GetFATEntry:
	push	es
	push	bx
	push	ax
	mov	ax,	00
	mov	es,	ax
	pop	ax
	mov	byte	[Odd],	0
	mov	bx,	3
	mul	bx
	mov	bx,	2
	div	bx
	cmp	dx,	0
	jz	Label_Even
	mov	byte	[Odd],	1
Label_Even:
	xor	dx,	dx
	mov	bx,	[BytesPreSector]
	div	bx
	push	dx
	mov	bx,	8000h
	add	ax,	FAT1StartSectors
	mov	cl,	2
	call	Func_ReadOneSector
	pop	dx
	add	bx,	dx
	mov	ax,	[es:bx]
	cmp	byte	[Odd],	1
	jnz	Label_Even_2
	shr	ax,	4
Label_Even_2:
	and	ax,	0FFFh        ; FAT12项低12位为文件簇号
	pop	bx
	pop	es
	ret

; -----------------------------
; 以下为打印函数，用于输出字符、16进制和10进制数字到屏幕
; -----------------------------

; print_char: 利用 BIOS int 10h, AH=0Eh 输出 AL 中的 ASCII 字符
print_char:
    mov ah, 0Eh
    int 10h
    ret

; print_hex16: 打印 AX 中的16进制数字（4位），例如0x1A2B将显示为 "1A2B"
print_hex16:
    push ax
    push bx
    push cx
    push dx
    mov bx, ax         ; 备份AX到BX
    mov cx, 4          ; 循环4次（每次输出一个16进制字符）
.hex_loop:
    mov dx, bx
    shr dx, 12         ; 取最高的4位（一个 nibble）
    and dx, 0x000F
    cmp dl, 9
    jbe .digit
    add dl, 'A' - 10
    jmp .out
.digit:
    add dl, '0'
.out:
    mov al, dl
    call print_char
    shl bx, 4         ; 左移4位，准备下一 nibble
    dec cx
    jnz .hex_loop
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; print_dec: 打印 AX 中的10进制无符号整数
; 例如 AX=1234 将显示 "1234"
print_dec:
    push ax
    push bx
    push cx
    push dx
    mov bx, 10        ; 除数10
    xor cx, cx        ; 初始化计数器
.convert_loop:
    xor dx, dx
    div bx            ; AX除以10，AX存商，DX存余数
    add dl, '0'       ; 将数字转换成ASCII字符
    push dx           ; 将余数保存到栈中
    inc cx
    test ax, ax
    jnz .convert_loop
.print_loop:
    pop ax            ; 弹出余数字符
    call print_char
    loop .print_loop
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; ===================================================================
; 临时 IDT 定义（未使用/占位）
; ===================================================================
IDT:
	times	0x50	dq	0            ; 分配0x50个双字，总共0x50*8字节的IDT空间
IDT_END:
IDT_POINTER:
		dw	IDT_END - IDT - 1  ; IDT界限
		dd	IDT              ; IDT基址

; ===================================================================
; 临时变量及显示消息定义
; ===================================================================
RootDirSizeForLoop	dw	RootDirSectors   ; 根目录扇区循环计数
SectorNo		dw	0                ; 当前读取的扇区号
Odd			db	0                ; 用于FAT项奇偶判断标志
OffsetOfKernelFileCount	dd	OffsetOfKernelFile  ; 内核加载时数据拷贝的偏移计数器
CandidateMode dw 0    ; 保存选中的 SVGA 模式号，初始为0表示未找到

StartLoaderMessage:	db	"Start Loader"
NoLoaderMessage:	    db	"ERROR:No KERNEL Found"
KernelFileName:		    db	"KERNEL  KAL",0    ; 需要匹配的内核文件名（8.3格式）
StartGetMemStructMessage:	db	"Start Get Memory Struct."
GetMemStructErrMessage:	    db	"Get Memory Struct ERROR"
GetMemStructOKMessage:	    db	"Get Memory Struct SUCCESSFUL!"
StartGetSVGAVBEInfoMessage:	db	"Start Get SVGA VBE Info"
GetSVGAVBEInfoErrMessage:	    db	"Get SVGA VBE Info ERROR"
GetSVGAModeInfoErrMessage:	    db	"Get SVGA Mode Info ERROR"
GetSVGAModeInfoOKMessage:	    db	"Get SVGA Mode Info SUCCESSFUL!"
SetSVGAModeErrMessage:		    db	"Set SVGA Mode ERROR"
