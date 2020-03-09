/*
 *     $Author: yeung $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mips-small-pipe.h"


/************************************************************/
int
main(int argc, char *argv[])
{
  short i;
  char line[MAXLINELENGTH];
  state_t state;
  FILE *filePtr;


  if (argc != 2) {
    printf("error: usage: %s <machine-code file>\n", argv[0]);
    exit(1);
  }

  memset(&state, 0, sizeof(state_t));

  state.pc=state.cycles=0;
  state.IFID.instr = state.IDEX.instr = state.EXMEM.instr =
    state.MEMWB.instr = state.WBEND.instr = NOPINSTRUCTION; /* nop */

  /* read machine-code file into instruction/data memory (starting at address 0) */

  filePtr = fopen(argv[1], "r");
  if (filePtr == NULL) {
    printf("error: can't open file %s\n", argv[1]);
    perror("fopen");
    exit(1);
  }

  for (state.numMemory=0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
       state.numMemory++) {
    if (sscanf(line, "%x", &state.dataMem[state.numMemory]) != 1) {
      printf("error in reading address %d\n", state.numMemory);
      exit(1);
    }
    state.instrMem[state.numMemory] = state.dataMem[state.numMemory];
    printf("memory[%d]=%x\n", 
	   state.numMemory, state.dataMem[state.numMemory]);
  }

  printf("%d memory words\n", state.numMemory);

  printf("\tinstruction memory:\n");
  for (i=0; i<state.numMemory; i++) {
    printf("\t\tinstrMem[ %d ] = ", i);
    printInstruction(state.instrMem[i]);
  }

  run(&state);

  return 0;
}
/************************************************************/

/************************************************************/
void
run(Pstate state)
{
  state_t new;
  int readRegA;
  int readRegB;
  int instr;
  int funct;
  int s1, s2;
  int prev1, prev2, prev3;
  int opPrev1, opPrev2, opPrev3;
  int extraCycles;
  int off;
  extraCycles = 0;
  
  memset(&new, 0, sizeof(state_t));

  while (1) {

    printState(state);

    /* copy everything so all we have to do is make changes.
       (this is primarily for the memory and reg arrays) */
    memcpy(&new, state, sizeof(state_t));

    new.cycles++;

    /* --------------------- IF stage --------------------- */
    new.IFID.instr = new.instrMem[new.pc/4];
    instr = new.IFID.instr;
    new.pc+=4;
    
    if ((opcode(new.IFID.instr) == BEQZ_OP)) {
      off = offset(instr);
      
      if (off > 0) {
	/*Not taken*/
	new.IFID.pcPlus1 = new.pc;
      }
      else {
	/*Taken*/
	new.pc += off;
	new.IFID.pcPlus1 = new.pc - off;
      }
    } else {
      new.IFID.pcPlus1 = new.pc;
    }

    /* --------------------- ID stage --------------------- */
    new.IDEX.instr = state->IFID.instr;
    instr = new.IDEX.instr;
      
    new.IDEX.offset = offset(instr);
    new.IDEX.pcPlus1 = state->IFID.pcPlus1;
    new.IDEX.readRegA = new.reg[field_r1(instr)];
    new.IDEX.readRegB = new.reg[field_r2(instr)];;

    if ((opcode(instr) == REG_REG_OP) &&
	(opcode(state->IDEX.instr) == LW_OP)) {
      if ((field_r1(instr) == field_r2(state->IDEX.instr)) ||
	  (field_r2(instr) == field_r2(state->IDEX.instr))) {
	new.IDEX.instr = NOPINSTRUCTION;
	new.IDEX.pcPlus1 = 0;
	new.IDEX.readRegA = new.IDEX.readRegB = 0;
	new.IDEX.offset = offset(NOPINSTRUCTION);

	new.IFID.instr = state->IFID.instr;
	instr = new.IFID.instr;
	new.pc -= 4;
	new.IFID.pcPlus1 = new.pc;

	extraCycles += 1;
      }
    }
    else if ((opcode(instr) != HALT_OP) &&
	     (opcode(state->IDEX.instr) == LW_OP)) {
      if (field_r1(instr) == field_r2(state->IDEX.instr)) {
	new.IDEX.instr = NOPINSTRUCTION;
	new.IDEX.pcPlus1 = 0;
	new.IDEX.readRegA = new.IDEX.readRegB = 0;
	new.IDEX.offset = offset(NOPINSTRUCTION);

	new.IFID.instr = state->IFID.instr;
	instr = new.IFID.instr;
	new.pc -= 4;
	new.IFID.pcPlus1 = new.pc;

	extraCycles += 1;
      }
    }
				   
    /* --------------------- EX stage --------------------- */
    
      
    new.EXMEM.instr = state->IDEX.instr;
    instr = new.EXMEM.instr;

    /**                            Forwarding check      */
    funct = func(instr);
    
    prev1 = state->EXMEM.instr;
    opPrev1 = opcode(prev1);
    prev2 = state->MEMWB.instr;
    opPrev2 = opcode(prev2);
    prev3 = state->WBEND.instr;
    opPrev3 = opcode(prev3);

    readRegA = new.reg[field_r1(instr)];
    readRegB = new.reg[field_r2(instr)];

    /*printf("prev1: %d & prev2: %d & prev3: %d\n", prev1, prev2, prev3);
      printf("BEFORE: tempRegA: %d & tempRegB: %d\n", readRegA, readRegB);
    */
       
    s1 = field_r1(instr);
    s2 = field_r2(instr);    

    if (opPrev3 == REG_REG_OP) {
      	
      if (s1!=0 && s1 == field_r3(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	readRegA = state->WBEND.writeData;
      }
      if (s2!=0 && s2 == field_r3(prev3)  && opPrev3 != opcode(NOPINSTRUCTION)) {
	readRegB = state->WBEND.writeData;
      }
    }
    else if (opPrev3 == LW_OP ||
	     (opPrev3 == ADDI_OP) || opPrev3 == BEQZ_OP)
      {
		
	if (s1!=0 && s1 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->WBEND.writeData;
	}
	if (s2!=0 && s2 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	  readRegB = state->WBEND.writeData;
	}				 
      }
    else if (opPrev3 == SW_OP) {
      if (s2!=0 && s2 == field_r2(prev3) && opPrev3 != opcode(NOPINSTRUCTION)) {
	readRegB = state->EXMEM.aluResult;
      }
    }
    if (opPrev2 == REG_REG_OP) {
      	
      if (s1!=0 && s1 == field_r3(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	readRegA = state->MEMWB.writeData;
      }
      if (s2!=0 && s2 == field_r3(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	readRegB = state->MEMWB.writeData;
      }
    }
    else if (opPrev2 == LW_OP ||
	     opPrev2 == ADDI_OP || opPrev2 == BEQZ_OP)
      {
	
	if (s1!=0 && s1 == field_r2(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->MEMWB.writeData;
	}
	if (s2!=0 && s2 == field_r2(prev2) && opPrev2 != opcode(NOPINSTRUCTION)) {
	  readRegB = state->MEMWB.writeData;
	}
      }
    if (opPrev1 == REG_REG_OP) {
      	
      if (s1!=0 && s1 == field_r3(prev1)) {
	readRegA = state->EXMEM.aluResult;
      }
      if (s2!=0 && s2 == field_r3(prev1)) {
	readRegB = state->EXMEM.aluResult;
      }
    }
    else if (opPrev1 == LW_OP ||
	     opPrev1 == ADDI_OP || opPrev1 == BEQZ_OP)
      {		
	if (s1!=0 && s1 == field_r2(prev1) && opPrev1 != opcode(NOPINSTRUCTION)) {
	  readRegA = state->EXMEM.aluResult;
	}
	if (s2!=0 && s2 == field_r2(prev1) && opPrev1 != opcode(NOPINSTRUCTION) &&
	    opcode(instr) != opPrev1) {
	  readRegB = state->EXMEM.aluResult;
	}
      }    
    else {

    }

    /**
       In Execute, Decoded.... Now execute
    */
    if (opcode(instr) == REG_REG_OP) {
      if (instr == NOPINSTRUCTION) {
	new.EXMEM.aluResult = 0;
	new.EXMEM.readRegB = 0;
      }
      else if (funct == ADD_FUNC) {
	new.EXMEM.aluResult = readRegA + readRegB;
	new.EXMEM.readRegB =  readRegB;

      } else if (funct == SLL_FUNC) {
	new.EXMEM.aluResult = readRegA << readRegB;
	new.EXMEM.readRegB = readRegB;

      } else if (funct == SRL_FUNC) {
	new.EXMEM.aluResult = ((unsigned int) readRegA) >> readRegB;
	new.EXMEM.readRegB = readRegB;
  
      } else if (funct == SUB_FUNC) {
	new.EXMEM.aluResult = readRegA - readRegB;
	new.EXMEM.readRegB = readRegB;

      } else if (funct == AND_FUNC) {
	new.EXMEM.aluResult = readRegA & readRegB;
	new.EXMEM.readRegB = readRegB;

      } else if (funct == OR_FUNC) {
	new.EXMEM.aluResult = readRegA | readRegA;
	new.EXMEM.readRegB = readRegB;

      }
            
    } else if (opcode(instr) == ADDI_OP) {
      new.EXMEM.aluResult = readRegA + offset(instr);

      new.EXMEM.readRegB = state->IDEX.readRegB;
      
    } else if (opcode(instr) == LW_OP) {
      new.EXMEM.aluResult = readRegA + field_imm(instr);
      new.EXMEM.readRegB = new.reg[field_r2(instr)];
      
    } else if (opcode(instr) == SW_OP) {
      new.EXMEM.readRegB = readRegB;
      new.EXMEM.aluResult = readRegA + field_imm(instr);
      
    } else if (opcode(instr) == BEQZ_OP) {
      
      if ((state->IDEX.offset > 0 && readRegA==0) ||
	  (state->IDEX.offset < 0 && readRegA!=0)) {
	new.IFID.instr = NOPINSTRUCTION;
	new.IDEX.instr = NOPINSTRUCTION;
	new.pc = state->IDEX.offset + state->IDEX.pcPlus1;
	
	new.IFID.pcPlus1 = new.IDEX.pcPlus1 = 0;
	new.IDEX.readRegA = new.IDEX.readRegB = 0;
	new.IDEX.offset = 32;

	new.EXMEM.aluResult = state->IDEX.pcPlus1 + offset(instr);
	new.EXMEM.readRegB = new.reg[field_r2(instr)];
      } else {
	new.EXMEM.readRegB = new.reg[field_r2(instr)];
	new.EXMEM.aluResult = state->IDEX.pcPlus1 + offset(instr);
      }
      
      
      
    } else {
      new.EXMEM.aluResult = 0;
      new.EXMEM.readRegB = 0;
    }
    /* --------------------- MEM stage --------------------- */
      
    new.MEMWB.instr = state->EXMEM.instr;
    instr = new.MEMWB.instr;
    
    if (opcode(instr) == REG_REG_OP) {
      new.MEMWB.writeData = state->EXMEM.aluResult;
    } else if (opcode(instr) == LW_OP) {
      new.MEMWB.writeData = state->dataMem[state->EXMEM.aluResult/4];
    } else if (opcode(instr) == SW_OP) {
      new.MEMWB.writeData = state->EXMEM.readRegB;
      new.dataMem[(field_imm(instr) + new.reg[field_r1(instr)])/4] =
	new.MEMWB.writeData;
    } else if (opcode(instr) == ADDI_OP) {
      new.MEMWB.writeData = state->EXMEM.aluResult;
    } else if (opcode(instr) == BEQZ_OP) {
      new.MEMWB.writeData = state->EXMEM.aluResult;
    } else if (opcode(instr) == HALT_OP) {
      new.MEMWB.writeData = 0;
    }
      
    		   
    /* --------------------- WB stage --------------------- */
    
    new.WBEND.instr = state->MEMWB.instr;
    instr = new.WBEND.instr;
      
    if (opcode(instr) == REG_REG_OP) {
      new.reg[field_r3(instr)] = state->MEMWB.writeData;
      new.WBEND.writeData = state->MEMWB.writeData;
      
    } else if (opcode(instr) == LW_OP) {
      new.WBEND.writeData = state->MEMWB.writeData;
      new.reg[field_r2(instr)] = new.WBEND.writeData;
    } else if (opcode(instr) == SW_OP) {
      new.WBEND.writeData = state->MEMWB.writeData;
    } else if (opcode(instr) == ADDI_OP) {
      new.WBEND.writeData = state->MEMWB.writeData;
      new.reg[field_r2(instr)] = new.WBEND.writeData;
    } else if (opcode(instr) == BEQZ_OP) {
      new.WBEND.writeData = state->MEMWB.writeData;
    } else if (opcode(instr) == HALT_OP) {
      printf("machine halted\n");
      printf("total of %d cycles executed\n", state->cycles) ;
      exit(0);
    } else {

    }
      
    /* --------------------- end stage --------------------- */

    readRegA = 0; readRegB = 0;
    /* transfer new state into current state */
    memcpy(state, &new, sizeof(state_t));
  }
}
/************************************************************/

/************************************************************/
int
opcode(int instruction)
{
  return (instruction >> OP_SHIFT) & OP_MASK;
}
/************************************************************/

/************************************************************/
int
func(int instruction)
{
  return (instruction & FUNC_MASK);
}
/************************************************************/

/************************************************************/
int
field_r1(int instruction)
{
  return (instruction >> R1_SHIFT) & REG_MASK;
}
/************************************************************/

/************************************************************/
int
field_r2(int instruction)
{
  return (instruction >> R2_SHIFT) & REG_MASK;
}
/************************************************************/

/************************************************************/
int
field_r3(int instruction)
{
  return (instruction >> R3_SHIFT) & REG_MASK;
}
/************************************************************/

/************************************************************/
int
field_imm(int instruction)
{
  return (instruction & IMMEDIATE_MASK);
}
/************************************************************/

/************************************************************/
int
offset(int instruction)
{
  /* only used for lw, sw, beqz */
  return convertNum(field_imm(instruction));
}
/************************************************************/

/************************************************************/
int
convertNum(int num)
{
  /* convert a 16 bit number into a 32-bit Sun number */
  if (num & 0x8000) {
    num -= 65536;
  }
  return(num);
}
/************************************************************/

/************************************************************/
void
printState(Pstate state)
{
  short i;
  printf("@@@\nstate before cycle %d starts\n", state->cycles);
  printf("\tpc %d\n", state->pc);

  printf("\tdata memory:\n");
  for (i=0; i<state->numMemory; i++) {
    printf("\t\tdataMem[ %d ] %d\n", 
	   i, state->dataMem[i]);
  }
  printf("\tregisters:\n");
  for (i=0; i<NUMREGS; i++) {
    printf("\t\treg[ %d ] %d\n", 
	   i, state->reg[i]);
  }
  printf("\tIFID:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IFID.instr);
  printf("\t\tpcPlus1 %d\n", state->IFID.pcPlus1);
  printf("\tIDEX:\n");
  printf("\t\tinstruction ");
  printInstruction(state->IDEX.instr);
  printf("\t\tpcPlus1 %d\n", state->IDEX.pcPlus1);
  printf("\t\treadRegA %d\n", state->IDEX.readRegA);
  printf("\t\treadRegB %d\n", state->IDEX.readRegB);
  printf("\t\toffset %d\n", state->IDEX.offset);
  printf("\tEXMEM:\n");
  printf("\t\tinstruction ");
  printInstruction(state->EXMEM.instr);
  printf("\t\taluResult %d\n", state->EXMEM.aluResult);
  printf("\t\treadRegB %d\n", state->EXMEM.readRegB);
  printf("\tMEMWB:\n");
  printf("\t\tinstruction ");
  printInstruction(state->MEMWB.instr);
  printf("\t\twriteData %d\n", state->MEMWB.writeData);
  printf("\tWBEND:\n");
  printf("\t\tinstruction ");
  printInstruction(state->WBEND.instr);
  printf("\t\twriteData %d\n", state->WBEND.writeData);
}
/************************************************************/

/************************************************************/
void
printInstruction(int instr)
{

  if (opcode(instr) == REG_REG_OP) {

    if (func(instr) == ADD_FUNC) {
      print_rtype(instr, "add");
    } else if (func(instr) == SLL_FUNC) {
      print_rtype(instr, "sll");
    } else if (func(instr) == SRL_FUNC) {
      print_rtype(instr, "srl");
    } else if (func(instr) == SUB_FUNC) {
      print_rtype(instr, "sub");
    } else if (func(instr) == AND_FUNC) {
      print_rtype(instr, "and");
    } else if (func(instr) == OR_FUNC) {
      print_rtype(instr, "or");
    } else {
      printf("data: %d\n", instr);
    }

  } else if (opcode(instr) == ADDI_OP) {
    print_itype(instr, "addi");
  } else if (opcode(instr) == LW_OP) {
    print_itype(instr, "lw");
  } else if (opcode(instr) == SW_OP) {
    print_itype(instr, "sw");
  } else if (opcode(instr) == BEQZ_OP) {
    print_itype(instr, "beqz");
  } else if (opcode(instr) == HALT_OP) {
    printf("halt\n");
  } else {
    printf("data: %d\n", instr);
  }
}
/************************************************************/

/************************************************************/
void
print_rtype(int instr, char *name)
{
  printf("%s %d %d %d\n", 
	 name, field_r3(instr), field_r1(instr), field_r2(instr));
}
/************************************************************/

/************************************************************/
void
print_itype(int instr, char *name)
{
  printf("%s %d %d %d\n", 
	 name, field_r2(instr), field_r1(instr), offset(instr));
}
/************************************************************/
