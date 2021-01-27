#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

#include "instruction.h"
#include "printRoutines.h"

/* Reads one byte from memory, at the specified address. Stores the
   read value into *value. Returns 1 in case of success, or 0 in case
   of failure (e.g., if the address is larger than the limit of the
   memory size). */
int memReadByte(machine_state_t *state,	uint64_t address, uint8_t *value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if (address > state->programSize){
    return 0;
  }
  *value = state->programMap[address];
  return 1;
}

/* Reads one quad-word (64-bit number) from memory in little-endian
   format, at the specified starting address. Stores the read value
   into *value. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is larger than the limit of the memory size). */
int memReadQuadLE(machine_state_t *state, uint64_t address, uint64_t *value) {
  
  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if (address+7 > state->programSize){
    return 0; 
  }
  *value = state->programMap[address];
 // *value = __builtin_bswap64(state->programMap[address]);
  return 1;
}

/* Stores the specified one-byte value into memory, at the specified
   address. Returns 1 in case of success, or 0 in case of failure
   (e.g., if the address is larger than the limit of the memory
   size). */
int memWriteByte(machine_state_t *state,  uint64_t address, uint8_t value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if (address > state->programSize){
    return 0;
  }
  state->programMap[address] = value ;
  return 1;
}

/* Stores the specified quad-word (64-bit) value into memory, at the
   specified start address, using little-endian format. Returns 1 in
   case of success, or 0 in case of failure (e.g., if the address is
   larger than the limit of the memory size). */
int memWriteQuadLE(machine_state_t *state, uint64_t address, uint64_t value) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if (address > state->programSize){
    return 0; 
  }
  state->programMap[address] = __builtin_bswap64(value);
  return 1;
}

/* Fetches one instruction from memory, at the address specified by
   the program counter. Does not change the machine's state. The
   resulting instruction is stored in *instr. Returns 1 if the
   instruction is a valid non-halt instruction, or 0 (zero)
   otherwise. If the instruction is invalid an error is printed. */
int fetchInstruction(machine_state_t *state, y86_instruction_t *instr) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  uint64_t address = state->programCounter;
  instr->location = address;
  
  //for parsing
  uint8_t onebyte;

  memReadByte(state,address,&onebyte);
  //parse the first and second half of the byte
  instr->icode = onebyte/16;
  instr->ifun = onebyte%16;
  //halt
  if(instr->icode == I_HALT){
    return 0; 
  }
  address += 1;
  //check icode
  if(instr->icode>0xB){
    printf("Error: Instruction(icode) is invalid!");
    return 0;
  }
  //check ifun
  if( instr->icode==2 || instr->icode==6 || instr->icode==7){
    if(instr->ifun>6){
      printf("Error: Instruction(ifun) is invalid!");
      return 0;
    }
  }
  else{
    if(instr->ifun!=0){
      printf("Error: Instruction(ifun) is invalid!");
      return 0;
    }
  }

  if( (instr->icode>=2 && instr->icode<=6) || instr->icode==0xA || instr->icode==0xB){
    memReadByte(state,address,&onebyte);
    //parse the first and second half of the byte
    instr->rA = onebyte/16;
    instr->rB = onebyte%16;
    address += 1;

    //check registers
    if(instr->icode==3){
      if( instr->rA!=0xF ){
        printf("Error: Instruction(regA) is invalid!");
        return 0;
      }
    }
    else{
      if(instr->rA>=0xF){
        printf("Error: Instruction(regA) is invalid!");
        return 0;
      }
    }
    if(instr->icode==0xA || instr->icode==0xB){
      if( instr->rB!=0xF ){
        printf("Error: Instruction(regB) is invalid!");
        return 0;
      }
    }
    else{
      if(instr->rB>=0xF){
        printf("Error: Instruction(regB) is invalid!");
        return 0;
      }
    }

    //icode 3-5
    if(instr->icode>=3 && instr->icode<=5){
      memReadQuadLE(state,address,&(instr->valC));
      address += 8;
    }
  }
  //icode 7-8
  else if(instr->icode==7 || instr->icode==8){
    memReadQuadLE(state,address,&(instr->valC));
    address += 8;
  }

  instr->valP = address;
  return 1;
}

/* Executes the instruction specified by *instr, modifying the
   machine's state (memory, registers, condition codes, program
   counter) in the process. Returns 1 if the instruction was executed
   successfully, or 0 if there was an error. The error is printed to
   the standard output. Typical errors include an invalid instruction
   or a memory access to an invalid address. In case of error the
   machine's state is not modified. */
int executeInstruction(machine_state_t *state, y86_instruction_t *instr) {

  /* THIS PART TO BE COMPLETED BY THE STUDENT */
  if(instr->icode == I_HALT){
    return 1;
  }
  //flags
  int zeroflag = 0;
  int signflag = 0;
  //nop
  if(instr->icode == I_NOP){
    state->programCounter = instr->valP;
    return 1;
  }
  //rrmovxx
  else if(instr->icode == I_RRMVXX){
    switch(instr->ifun){
      case(C_NC):
        state->registerFile[instr->rB] = state->registerFile[instr->rA];
        break;

      case(C_LE):
        if(zeroflag || signflag){
          state->registerFile[instr->rB] = state->registerFile[instr->rA];
        }
        break;

      case(C_L):
        if(signflag){
          state->registerFile[instr->rB] = state->registerFile[instr->rA];
        }
        break;

      case(C_E):
        if(zeroflag){
          state->registerFile[instr->rB] = state->registerFile[instr->rA];
        }
        break;

      case(C_NE):
        if(!zeroflag){
          state->registerFile[instr->rB] = state->registerFile[instr->rA];
        }
        break;
      
      case(C_GE):
        if(!signflag){
          state->registerFile[instr->rB] = state->registerFile[instr->rA];
        }
        break;

      case(C_G):
        if(!zeroflag && !signflag){
          state->registerFile[instr->rB] = state->registerFile[instr->rA];
        }
        break;
    }
    state->programCounter = instr->valP;
    return 1;
  }
  //irmovq
  else if(instr->icode == I_IRMOVQ){
    state->registerFile[instr->rB] = instr->valC;
    state->programCounter = instr->valP;
    return 1;
  }
  //rmmovq
  else if(instr->icode == I_RMMOVQ){   
    if(memWriteQuadLE(state, state->programMap[state->registerFile[instr->rB]]+instr->valC, 
    state->registerFile[instr->rA])){
      state->programCounter = instr->valP;
      return 1;
    }
    return 0;
  }
  //mrmovq
  else if(instr->icode == I_MRMOVQ){
    if(memReadQuadLE(state, state->programMap[state->registerFile[instr->rB]]+instr->valC, &(state->registerFile[instr->rA]))){
      state->programCounter = instr->valP;
      return 1;
    }
  }
  //alu op
  else if(instr->icode == I_OPQ){
    switch(instr->ifun){
      case(A_ADDQ):
        state->registerFile[instr->rB] = state->registerFile[instr->rB] + state->registerFile[instr->rA];
        break;

      case(A_SUBQ):
        state->registerFile[instr->rB] = state->registerFile[instr->rB] - state->registerFile[instr->rA];
        break;

      case(A_ANDQ):
        state->registerFile[instr->rB] = state->registerFile[instr->rB] & state->registerFile[instr->rA];
        break;
      
      case(A_XORQ):
        state->registerFile[instr->rB] = state->registerFile[instr->rB] ^ state->registerFile[instr->rA];
        break;

      case(A_MULQ):
        state->registerFile[instr->rB] = state->registerFile[instr->rB] * state->registerFile[instr->rA];
        break;
      
      case(A_DIVQ):
        state->registerFile[instr->rB] = state->registerFile[instr->rB] / state->registerFile[instr->rA];
        break;

      case(A_MODQ):
        state->registerFile[instr->rB] = state->registerFile[instr->rB] % state->registerFile[instr->rA];
        break;
    }
    //set flags
    zeroflag = (state->programMap[instr->rB] == 0);
    signflag = (state->programMap[instr->rB] > 0x7FFFFFFFFFFFFFFF);
    state->programCounter = instr->valP;
    return 1;
  }
  //conditional jumps
  else if(instr->icode == I_JXX){
    switch(instr->ifun){
      case(C_NC):
        state->programCounter = instr->valC;
        break;

      case(C_LE):
        if(zeroflag || signflag){
          state->programCounter = instr->valC;
        }
        break;

      case(C_L):
        if(signflag){
          state->programCounter = instr->valC;
        }
        break;

      case(C_E):
        if(zeroflag){
          state->programCounter = instr->valC;
        }
        break;

      case(C_NE):
        if(!zeroflag){
          state->programCounter = instr->valC;
        }
        break;
      
      case(C_GE):
        if(!signflag){
          state->programCounter = instr->valC;
        }
        break;

      case(C_G):
        if(!zeroflag && !signflag){
          state->programCounter = instr->valC;
        }
        break;
    }
    return 1;
  }
  //call
  else if(instr->icode == I_CALL){
    state->registerFile[R_RSP] += 8; 
    if(memWriteQuadLE(state, state->programMap[state->registerFile[R_RSP]]-8, instr->valP)){
      state->programCounter = instr->valC;
      return 1;
    }
    state->registerFile[R_RSP] -= 8;
    return 0; 
  }
  //return
  else if(instr->icode == I_RET){
    if(memReadQuadLE(state, state->programMap[state->registerFile[R_RSP]]-8, &(state->programCounter))){
      state->registerFile[R_RSP] -= 8;
      return 1;
    }
    return 0;
  }
  //push
  else if(instr->icode == I_PUSHQ){
    state->registerFile[R_RSP] += 8; 
    if(memWriteQuadLE(state, state->programMap[state->registerFile[R_RSP]]-8, state->registerFile[instr->rA])){
      state->programCounter = instr->valC;
      return 1;
    }
    state->registerFile[R_RSP] -= 8;
    return 0; 
  }
  //pop
  else if(instr->icode == I_POPQ){
    if(memReadQuadLE(state, state->programMap[state->registerFile[R_RSP]]-8, &(state->registerFile[instr->rA]))){
      state->registerFile[R_RSP] -= 8;
      return 1;
    }
    return 0;
  }

  return 0;

}
