#ifndef _instr_h_
#define _instr_h_

#include "types.h"
#include <map>

using namespace std;

enum INSTRUCTION_TYPE: Byte{
  INST_NOOP = 11,
  INST_JUMP,
  INST_ONE_REG_DIR,
  INST_TWO_REG_DIR,
  INST_PUSH,
  INST_POP,
  INST_LOAD_STORE
};


enum ARG_VALUE_TYPE: Byte{
    ARG_LITERAL_VALUE = 18,
    ARG_SYMBOL_VALUE,
    ARG_SYMBOL_MEM_PCREL,
    ARG_REG_INDIR,
    ARG_REG_LITERAL_MEM,
    ARG_REG_SYMBOL_MEM
  };

  enum JMP_DEST_TYPE: Byte{
    JMP_ARG_SYMBOL_VALUE_PCREL = 24,
    JMP_ARG_LITERAL_MEM,
    JMP_ARG_SYMBOL_MEM,
    JMP_ARG_REG_DIR,
    JMP_ARG_REG_INDIR,
    JMP_ARG_REG_LITERAL_MEM,
    JMP_ARG_REG_SYMBOL_MEM
  };

  enum ADDRESING_TYPE{
    ADDR_IMMEDIATE = 0x00,
    ADDR_REG_DIR = 0x01,
    ADDR_REG_DIR_POM = 0x05,
    ADDR_REG_INDIR = 0x02,
    ADDR_REG_INDIR_POM = 0x03,
    ADDR_MEM = 0x04,
    ADDR_PC_REL = 0x06
  };

  enum UPDATE_REG_TYPE{
    UPDATE_REG_NULL = 0x00,
    UPDATE_REG_PRE_DEC = 0x01,
    UPDATE_REG_PRE_INC = 0x02,
    UPDATE_REG_POST_DEC = 0x03,
    UPDATE_REG_POST_INC = 0x04
  };

  enum OPERAND_TYPE{
    OPERAND_TYPE_REG,
    OPERAND_TYPE_LITERAL,
    OPERAND_TYPE_SYMBOL
  };

  struct Operand{
    OPERAND_TYPE type;
    string operand;
  };

  struct Instruction {
    INSTRUCTION_TYPE type;
    string name;
    Byte op_code;
    Operand* op1;
    Operand* op2;
    Operand* op3;
    ADDRESING_TYPE addresing;
    UPDATE_REG_TYPE update;
    bool is_relative;
  };

class Instruction_handler{
public:
 static void initializeInstrOpCodes();

 static map<string,Byte> instr_op_codes;

 static void handleInstruction(INSTRUCTION_TYPE type , void*l);

 static void getArguements(Instruction* instr, void*l);

 static Byte findRegCode(string text);

 static Word handleInstrSymbol(string symbol, Instruction* instr);

};


#endif