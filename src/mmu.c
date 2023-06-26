#include "rvemu.h"

static void load_phdr(elf64_phdr_t *phdr, 
                      elf64_ehdr_t *ehdr, 
                               i64 i, 
                              FILE *file) {
  
  // get i_th program header (base offset + i * one ph size)
  // ps: e_phentsize == sizeof(elf64_phdr_t)
  if (fseek(file, ehdr->e_phoff + i * ehdr->e_phentsize, SEEK_SET) != 0) 
    fatal("seek phdr failed");

  if (fread((void *)phdr, 1, sizeof(elf64_phdr_t), file) != sizeof(elf64_phdr_t))
    fatal("failed to read phdr, file too small !!!");

}

static int pflags_to_prot(u32 flags) {
  return (flags & PF_R ? PROT_READ : 0) |
         (flags & PF_W ? PROT_WRITE : 0) |
         (flags & PF_X ? PROT_EXEC : 0);
}

static void mmu_load_segment(mmu_t *mmu,
                      elf64_phdr_t *phdr,
                               int fd) {
  
  // host prog => high address (0x7ff....)
  // guest prog => low address (GUEST_MEMORY_OFFSET + VirtAddr of a segment)
  int page_size = getpagesize();
  u64 offset = phdr->p_offset;  // Offset of the segment in the file image.
  
  u64 vaddr = TO_HOST(phdr->p_vaddr);
  u64 aligned_vaddr = ROUNDDOWN(vaddr, page_size);
  
  u64 filesz = phdr->p_filesz + (vaddr - aligned_vaddr);
  u64 memsz  = phdr->p_memsz  + (vaddr - aligned_vaddr);

  int prot = pflags_to_prot(phdr->p_flags);

  // mmap only accepts aligned address, so we need ROUNDDOWN 
  // align_offset : 
  //    vaddr - aligned_addr == offset - ROUNDDOWN(offset, page_size)
  // load to aligned address (maybe load something else (align_offset) but okay)
  
  u64 addr = (u64)mmap((void *)aligned_vaddr, filesz, prot, 
                       MAP_PRIVATE | MAP_FIXED, 
                       fd, ROUNDDOWN(offset, page_size));
  assert(addr == aligned_vaddr);

  // Section to Segment mapping:
  //   Segment Sections...
  //   00     .riscv.attributes 
  //   01     .text .rodata 
  //   02     .eh_frame .init_array .fini_array .data .sdata .sbss .bss 
  
  // sbss, bss are last sections, 0 size in file but filled with 0000 in mem
  // mmap filesz to mem, tail with 000 for bss

  // when mem need more pages
  u64 remaining_bss = ROUNDUP(memsz, page_size) - ROUNDUP(filesz, page_size);
  if (remaining_bss > 0) {
    u64 addr = (u64)mmap((void *)(aligned_vaddr + ROUNDUP(filesz, page_size)), remaining_bss, prot,
                          MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
                          -1, 0);
    assert(addr == aligned_vaddr + ROUNDUP(filesz, page_size));
  }

  // host:  [    ELF   ][  stack  ][|  mem alloced while running   |]
  //                                  | <- host_alloc
  //                              | sp
  // guest:             | <- base      | <- alloc
  mmu->host_alloc = MAX(mmu->host_alloc, (aligned_vaddr + ROUNDUP(memsz, page_size)));
  
  mmu->base = mmu->alloc = TO_GUEST(mmu->host_alloc); 

}


void mmu_load_elf(mmu_t *mmu, 
                    int fd) {

  u8 buf[sizeof(elf64_ehdr_t)];  // read bytes

  FILE *file = fdopen(fd, "rb");
  if (fread(buf, 1, sizeof(elf64_ehdr_t), file) != sizeof(elf64_ehdr_t)) 
    fatal("failed to read ehdr, too small !!!");

  elf64_ehdr_t *ehdr = (elf64_ehdr_t *)buf;
  
  // check magic number of ELF
  if (*(u32 *)ehdr != *(u32 *)ELFMAG)  // 4 byte
    fatal("bad elf file");

  // check is riscv and 64bit
  if (ehdr->e_machine != EM_RISCV || 
      ehdr->e_ident[EI_CLASS] != ELFCLASS64) 
    fatal("only rv64 elf files are supported");

  mmu->entry = (u64)ehdr->e_entry;

  elf64_phdr_t phdr;
  for (i64 i = 0; i < ehdr->e_phnum; ++i) {
    load_phdr(&phdr, ehdr, i, file);

    // only need to load segments with type LOAD
    if (phdr.p_type == PT_LOAD) {
      mmu_load_segment(mmu, &phdr, fd);
    }
  }

}

// stack 32M
//  |top
// [               | argc argv envp auxv]
//                 ^ stack bottom(initial sp)


u64 mmu_alloc(mmu_t *mmu, 
                i64 sz) { // i64: can be neg, to free mem
  int page_size = getpagesize();
  u64 base = mmu->alloc;
  assert(base >= mmu->base);

  mmu->alloc += sz;
  assert(mmu->alloc >= mmu->base);
  // implement with mmap, host_alloc always aligns to page_size
  // if there are remaining spaces, we don't have to alloc a new page
  if (sz > 0 && mmu->alloc > TO_GUEST(mmu->host_alloc)) {
    if (mmap((void *)mmu->host_alloc, ROUNDUP(sz, page_size),
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0) == MAP_FAILED)
      fatal("alloc mmap failed");
    mmu->host_alloc += ROUNDUP(sz, page_size);
  }
  // free more than one page
  else if (sz < 0 && ROUNDUP(mmu->alloc, page_size) < TO_GUEST(mmu->host_alloc)) {
    u64 len = TO_GUEST(mmu->host_alloc) - ROUNDUP(mmu->alloc, page_size);
    
    if (munmap((void *)ROUNDUP(mmu->alloc, page_size), len) == -1) 
      fatal(strerror(errno));

    mmu->host_alloc -= len;
  }

  return base; // guest space addr
}

