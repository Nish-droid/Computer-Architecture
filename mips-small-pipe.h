/*
 *     $Author: yeung $
 */

#define MAXMEMORY 16384 /* maximum number of data words in memory */
#define NUMREGS 32 /* number of machine registers */
#define MAXLINELENGTH 1000

#define OP_SHIFT   26
#define R1_SHIFT  21
#define R2_SHIFT  16
#define R3_SHIFT   11

#define OP_MASK    0x3F
#define REG_MASK   0x1F
#define FUNC_MASK  0x7FF
#define IMMEDIATE_MASK 0xFFFF

#define LW_OP       0x23
#define SW_OP       0x2B
#define ADDI_OP     0x8
#define REG_REG_OP  0x0
#define BEQZ_OP     0x4
#define HALT_OP     0x3F

#define ADD_FUNC  0x20
#define SLL_FUNC  0x4
#define SRL_FUNC  0x6
#define SUB_FUNC  0x22
#define AND_FUNC  0x24
#define OR_FUNC   0x25

#define FROM_REGISTER_FILE 0
#define FROM_TARGET_AHEAD_1 1
#define FROM_TARGET_AHEAD_2 2
#define FROM_TARGET_AHEAD_3 3

#define NOPINSTRUCTION 0x20

typedef struct IFIDStruct {
  int instr;
  int pcPlus1;
} IFID_t;

typedef struct IDEXStruct {
  int instr;
  int pcPlus1;
  int readRegA;
  int readRegB;
  int offset;
} IDEX_t;

typedef struct EXMEMStruct {
  int instr;
  int aluResult;
  int readRegB;
} EXMEM_t;

typedef struct MEMWBStruct {
  int instr;
  int writeData;
} MEMWB_t;

typedef struct WBENDStruct {
  int instr;
  int writeData;
} WBEND_t;

typedef struct stateStruct {
  int pc;
  unsigned int instrMem[MAXMEMORY];
  unsigned int dataMem[MAXMEMORY];
  int reg[NUMREGS];
  int numMemory;
  IFID_t IFID;
  IDEX_t IDEX;
  EXMEM_t EXMEM;
  MEMWB_t MEMWB;
  WBEND_t WBEND;
  int cycles; /* number of cycles run so far */
} state_t, *Pstate;

void run(Pstate);

int opcode(int);
int func(int);
int field_r1(int);
int field_r2(int);
int field_r3(int);
int field_imm(int);
int offset(int);
int convertNum(int);

void printState(Pstate);
void printInstruction(int);
void print_rtype(int, char *);
void print_itype(int, char *);

#define memaddr(x) ((x) >> 2)
