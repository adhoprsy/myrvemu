/*
 * instruction decode 
*/

#include "rvemu.h"

#define QUADRANT(data) (((data) >> 0) & 0x3) // lowest 2 bits

// read raw bitcode, decode into insn_t
void insn_decode(insn_t *insn, u32 data) {
  u32 quadrant = QUADRANT(data);
  switch(quadrant) {
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    default: unreachable();
  }
}

