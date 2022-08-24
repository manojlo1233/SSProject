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
#include <termios.h>

using namespace std;

#define CODE_STARTING_POINT 0x10
#define ISR_ERROR_STARTING_CODE 0x01

#define ISR_TERMINAL_IVT 0x03
#define TERM_OUT 0xFF00
#define TERM_IN 0xFF02

#define TIMER_CFG 0xFF10
#define ISR_TIMER_IVT 0x02
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
  static atomic<bool> timer_finished;
  static atomic<bool> timer_set;

  static thread term_thread;
  static atomic<bool> term_finished;
  static atomic<bool> term_set;

  static termios terminal_handler;
  static termios terminal_reset;

  static void initialize_terminal();
  static void terminal();
  static void callTerm();

  static void callTimer();
  static void timer();
};

#endif