#ifndef _emulator_h_
#define _emulator_h_

#include "types.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <atomic>

using namespace std;

#define CODE_STARTING_POINT 0x10
#define ISR_ERROR_STARTING_CODE 0x02

#define TIMER_CFG 0xFF10
#define ISR_TIMER_IVT 0x04
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
  static vector<Word> program_regs;
  static vector<Byte> memory;


  static void load_file(string input_file);
  static void emulate_code();
  static int handle_op(Byte op_code);

  static thread timer_thread;
  static bool timer_finished;
  static atomic<bool> timer_set;

  static void callTimer();

  static void timer();
};

#endif