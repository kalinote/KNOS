OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(0x5);  /* 代码段：读和执行 (R+X) */
    data PT_LOAD FLAGS(0x6);  /* 数据段：读和写 (R+W) */
}

SECTIONS
{
    /* 将链接地址设置为高端内核虚拟地址 + 偏移 */
    . = 0xffff800000000000 + 0x100000;

    /* 代码段：只读+可执行 */
    .text : {
        _text = .;
        *(.text)
        _etext = .;
    } :text

    . = ALIGN(8);

    /* 数据段：只读+可写 */
    .data : {
        _data = .;
        *(.data)
        _edata = .;
    } :data

    /* BSS 段也归入 data 程序头 */
    .bss : {
        _bss = .;
        *(.bss)
        _ebss = .;
    } :data

    _end = .;
}
