#include "../lib/debug.h"
#include "../lib/elf.h"

// Define page table entry flags
#define PTE_U 0x4  // User-accessible
#define PTE_P 0x1  // Present
#define PTE_W 0x2  // Writable
#include "../lib/string.h"
#include "../lib/types.h"
#include "../lib/x86.h"
#include "../lib/pmap.h"
#include "../lib/gcc.h"

#define PAGESIZE 4096 // Define page size as 4KB

#define VM_TOP		0xffffffff
#define VM_USERHI	0xf0000000
#define VM_DYNLINK	0xe0000000
#define VM_STACKHI	0xd0000000
#define VM_USERLO	0x40000000
#define VM_BOTTOM	0x00000000
#ifndef ELF_H
#define ELF_H

#define ELF_SHN_UNDEF 0
#define ELF_SHT_STRTAB 3

typedef struct {
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
} sechdr;

#include <stdint.h>

typedef struct {
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_va;
	uint32_t p_pa;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} proghdr;

#define ELF_MAGIC 0x464C457FU /* "\x7FELF" in little endian */
#define ELF_PROG_LOAD 1       /* Program header type for loadable segments */
#define ELF_PROG_FLAG_WRITE 0x2 /* Writable segment flag */

typedef struct {
    uint32_t e_magic;  // Must equal ELF_MAGIC
    uint8_t e_ident[12];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elfhdr;

#endif // ELF_H
/*
 * Load elf execution file exe to the virtual address space pmap.
 */
void
elf_load (void *exe_ptr, int pid)
{
	elfhdr *eh;
	proghdr *ph, *eph;
	sechdr *sh, *esh;
	char *strtab;
	uintptr_t exe = (uintptr_t) exe_ptr;

	eh = (elfhdr *) exe;

	KERN_ASSERT(eh->e_magic == ELF_MAGIC);
	KERN_ASSERT(eh->e_shstrndx != ELF_SHN_UNDEF);

	sh = (sechdr *) ((uintptr_t) eh + eh->e_shoff);
	esh = sh + eh->e_shnum;

	strtab = (char *) (exe + sh[eh->e_shstrndx].sh_offset);
	KERN_ASSERT(sh[eh->e_shstrndx].sh_type == ELF_SHT_STRTAB);

	ph = (proghdr *) ((uintptr_t) eh + eh->e_phoff);
	eph = ph + eh->e_phnum;

	for (; ph < eph; ph++)
	{
		uintptr_t fa;
		uint32_t va, zva, eva, perm;

		if (ph->p_type != ELF_PROG_LOAD)
			continue;

		fa = (uintptr_t) eh + rounddown (ph->p_offset, PAGESIZE);
		va = rounddown (ph->p_va, PAGESIZE);
		zva = ph->p_va + ph->p_filesz;
		eva = roundup (ph->p_va + ph->p_memsz, PAGESIZE);

		perm = PTE_U | PTE_P;
		if (ph->p_flags & ELF_PROG_FLAG_WRITE)
			perm |= PTE_W;

		for (; va < eva; va += PAGESIZE, fa += PAGESIZE)
		{
			alloc_page (pid, va, perm);

			if (va < rounddown (zva, PAGESIZE))
			{
				/* copy a complete page */
				pt_copyout ((void *) fa, pid, va, PAGESIZE);
			}
			else if (va < zva && ph->p_filesz)
			{
				/* copy a partial page */
				pt_memset (pid, va, 0, PAGESIZE);
				pt_copyout ((void *) fa, pid, va, zva - va);
			}
			else
			{
				/* zero a page */
				pt_memset (pid, va, 0, PAGESIZE);
			}
		}
	}

}

uintptr_t
elf_entry (void *exe_ptr)
{
	uintptr_t exe = (uintptr_t) exe_ptr;
	elfhdr *eh = (elfhdr *) exe;
	KERN_ASSERT(eh->e_magic == ELF_MAGIC);
	return (uintptr_t) eh->e_entry;
}
