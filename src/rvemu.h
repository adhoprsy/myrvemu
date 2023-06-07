#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "types.h"
#include "elfdef.h"
#include "reg.h"

#define fatalf(fmt, ...) (fprintf(stderr, "\033[;41mfatal\033[0m: %s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__), exit(1))
#define fatal(msg) fatalf("%s", msg)
#define unreachable() (fatal("unreachable !"), __builtin_unreachable())

#define ROUNDDOWN(x, k) ((x) & -(k))  //binary round down
#define ROUNDUP(x, k) (((x) + (k)-1) & -(k))
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define GUEST_MEMORY_OFFSET 0x088800000000ULL

#define TO_HOST(addr)  (addr + GUEST_MEMORY_OFFSET)
#define TO_GUEST(addr) (addr - GUEST_MEMORY_OFFSET)


/*
 * mmu.c
 * memory manage unit
 * using a LINEAR mmu, easy to implement and fast
 * mmap, no need for additional permission check
*/
typedef struct {
   u64 entry;
   u64 host_alloc;
   u64 alloc;       // guest
   u64 base;        // guest
}mmu_t;

void mmu_load_elf(mmu_t *, int);


/*
 * state.c
 * rv general purpose reg
*/
enum exit_reason_t {
  none,           // no jump
  direct_branch,  // 
  indirect_branch,
  ecall,          // syscall
};
typedef struct {
  enum exit_reason_t exit_reason;
  u64 gp_regs[32];
  u64 pc;  //pc -> first instru before run
}state_t;


/*
 * machine.c
 * state of the whole emu machine
*/
typedef struct {
  state_t state;
  mmu_t mmu;
}machine_t;

void machine_load_program(machine_t *, char *);
enum exit_reason_t machine_step(machine_t *);

/*
 * instructions
*/
enum insn_type_t {
  insn_addi,
  num_insns,  // how many instrus
};
typedef struct {
  i8 rd;
  i8 rs1, rs2;
  i32 imm;
  enum insn_type_t type;
  bool rvc;   // rv c-extension
  bool cont;  // continue execution (mark for branch or calls or syscalls)
}insn_t;

/*
 * interp.c
*/
void exec_block_interp(state_t *);

/*
 * decode.c
*/
void insn_decode(insn_t *, u32);