#include "rvemu.h"

int main(int argc, char *argv[]) {
  
  machine_t machine = {0};
  machine_load_program(&machine, argv[1]);
  machine_setup(&machine, argc, argv);

  printf("machine address: 0x%lx\n", (u64)&machine);

  printf("loaded elf entry: %lx\n", machine.mmu.entry);
  printf("host alloc: %lx\n", machine.mmu.host_alloc);
  printf("running prog entry: %llx \n", TO_HOST(machine.mmu.entry));
  
  while (true) {
    enum exit_reason_t reason = machine_step(&machine);
    assert(reason == ecall);
    // rv: reg a7 syscall number
    //     a0~a6  syscall args
    //fatal("get a syscall");
    u64 syscall = machine_get_gp_reg(&machine, a7);
    //fatalf("syscall: %ld",syscall);
    u64 ret = do_syscall(&machine, syscall);
    machine_set_gp_reg(&machine, a0, ret);
  }

  return 0;

}