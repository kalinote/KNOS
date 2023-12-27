RootDirSectors		equ		14		; 根目录占用的扇区数，其计算方法为：(根目录数 * 32 + 每扇区字节数 - 1) / 每扇区的字节数
RootDirStartSectors	equ		19		; 根目录起始扇区，其计算方法为：保留扇区数 + FAT表数 * 每个FAT表占用扇区
FAT1StartSectors	equ		1		; FAT表1起始扇区
SectorBalance		equ		17		; 根目录起始扇区-2，方便计算FAT

	OEMName				db	'KNOSBOOT'			; 启动区名称(8字节)
	BytesPreSector		dw	512					; 每个扇区大小为0x200
	SectorsPreCluster	db	1					; 每个簇大小为一个扇区
	FATStartCluster		dw	1					; FAT起始位置(boot记录占用扇区数)
	FATNumber			db	2          		    ; FAT表数
	MaxRootDirNumber	dw	224					; 根目录最大文件数
	TotalSectorsNumber	dw	2880				; 磁盘大小(总扇区数，如果这里扇区数为0，则由下面给出)
	MediaType			db	0xf0                ; 磁盘种类
	FATSize				dw	9					; FAT长度(每个FAT表占用扇区数)
	SectorsPreTrack		dw	18					; 每个磁道的扇区数
	HeadsNumber			dw	2					; 磁头数(面数)
	UnusedSector		dd	0                   ; 不使用分区(隐藏扇区数)
	TotalSectorsNumber2	dd	0					; 重写一遍磁盘大小(如果上面的扇区数为0，则由这里给出)
	DriveNumber			db	0                   ; INT 13H的驱动器号
	Reserve				db	0					; 保留
	Bootsign			db	0x29                ; 扩展引导标记(29h)磁盘名称(11字节)
	VolId				dd	0                   ; 卷序列号
	VolLab				db	'KNOS       '       ; 磁盘名称(11字节)
	FileSystemType		db	'FAT12   '			; 磁盘格式名称(8字节)
