#include "rvemu.h"

void mmu_load_elf(mmu_t *mmu, 
                  int fd) {

  u8 buf[sizeof(elf64_ehdr_t)];

  FILE *file = fdopen(fd, "rb");
  if (fread(buf, 1, sizeof(elf64_ehdr_t), fd) != sizeof(elf64_ehdr_t)) {
    fatal("file not elf, too small !!!");
  }

  elf64_ehdr_t *ehdr = (elf64_ehdr_t *)buf;
  
  // check magic number of ELF
  if (*(u32 *)ehdr != *(u32 *)ELFMAG) {  // 4 byte
    fatal("bad elf file");
  }

  // check is riscv and 64bit
  if (ehdr->e_machine != EM_RISCV || 
      ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
    fatal("only rv64 elf files are supported");
  }

  mmu->entry = (u64)ehdr->e_entry;


}