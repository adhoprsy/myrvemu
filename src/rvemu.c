#include "rvemu.h"

int main(int argc, char *argv[]) {
  
  machine_t machine = {0};
  machine_load_program(&machine, argv[1]);

  printf("machine address: 0x%lx\n", (u64)&machine);

  printf("loaded elf entry: %lx\n", machine.mmu.entry);
  printf("host alloc: %lx\n", machine.mmu.host_alloc);
  printf("running prog entry: %llx \n", TO_HOST(machine.mmu.entry));
  

  return 0;

}