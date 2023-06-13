/*
 * interpretor
 * first try interpret exec, when detect hot code, use JIT
*/

#include "rvemu.h"

static void func_empty(state_t *state, insn_t *insn) {}

typedef void (func_t)(state_t *, insn_t *);

// func <---> instru, 1 to 1
// a array of many function pointers, each one for an implementation of an
// instruction.
// total 133 instruction types, so array size should be 133, 
static func_t* funcs[] = {

  
};

// execute a block by interpreting
void exec_block_interp(state_t *state) {
  static insn_t insn = {0};
  
  while (true) {
    u32 data = *(u32 *) TO_HOST(state->pc);
    insn_decode(&insn, data);
    
    //printf("type:%d  data:%d\n", insn.type, data);
    
    funcs[insn.type](state, &insn);
    
    // if zero is changed
    state->gp_regs[zero] = 0;

    // if we meet a syscall, then jump out of this loop
    // manage the syscall in another loop, jump back here
    // when syscall is done.
    if (insn.cont) break;  // syscall or other reason

    state->pc += (insn.rvc ? 2 : 4);
  }

}