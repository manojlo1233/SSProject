#ifndef _emulator_h_
#define _emulator_h_

#include "types.h"
#include <vector>
#include <fstream>
#include <iostream>

using namespace std;

#define CODE_STARTING_POINT 4
#define SP 6
#define PC 7
#define PSW 8

enum OP_REG: Byte{
  REG_RX = 200,
  REG_SP,
  REG_PC,
  REG_PSW
};



class Emulator{
public:

  static Word code_size;
  static vector<signed short> program_regs;
  static vector<Byte> memory;
  static vector<Byte> stack;

  static void load_file(string input_file);
  static void emulate_code();
  static int handle_op(Byte op_code);
};

#endif