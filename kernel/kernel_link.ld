/* 
 *  kernel/kernel_link.ld
 *  内核链接脚本
 */
OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);   /* R-X */
    data PT_LOAD FLAGS(6);   /* RW- */
    rodata PT_LOAD FLAGS(4); /* R-- */
}

SECTIONS
{

	. = 0xffff800000000000 + 0x100000;
	.text :
	{
		_text = .;
		*(.text)

		_etext = .;
	}: text
	. = ALIGN(8);
	.data :
	{
		_data = .;
		*(.data)
		
		_edata = .;
	}: data
	.bss :
	{
		_bss = .;
		*(.bss)
		_ebss = .;
	}: data

	_end = .;
}
