#include "emulator.h"
#include "instr.h"
#include <iomanip>

vector<signed short> Emulator::program_regs = vector<signed short>(9, 0);
vector<Byte> Emulator::memory = vector<Byte>(2<<16, 0);
Word Emulator::code_size = 0;
vector<Byte> Emulator::stack = vector<Byte>();

void Emulator::load_file(string input_file){

  FILE* fin = fopen(input_file.c_str() , "r");
  if ( fin == nullptr) {
    cerr << "Error opening the file" << endl;
    exit(-1);
  }

  
  fread(&Emulator::code_size, sizeof(Word), 1, fin);
  Byte byte;
  for ( int i = CODE_STARTING_POINT; i < code_size + CODE_STARTING_POINT; i++){
    fread(&byte, sizeof(Byte), 1, fin);
    Emulator::memory[i] = byte;
  }

  program_regs[PC] = CODE_STARTING_POINT;

}

void Emulator::emulate_code(){

  Word op_code;

  while(1){
    op_code = memory[program_regs[PC]++];
    if ( handle_op(op_code) == 1) break;
    cout << "OP_CODE" << std::setbase(16) << op_code << endl;
  }

}

int Emulator::handle_op(Byte op_code){
  
  switch ((Word)op_code)
      {
      case 0x00:{
        
        //halt
        cout << "------------------------------------------------" << endl;
        cout << "Emulated processor executed halt instruction" << endl;
        cout << "Emulated processor state: psw=0b" << std::setbase(2) << std::setw(16) << std::setfill('0')<< program_regs[8] << endl;
        cout << std::setw(0) << std::setfill(' ') << "r0=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[0] << endl;
        cout << std::setw(0) << std::setfill(' ') << "r1=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[1] << endl;
        cout << std::setw(0) << std::setfill(' ') << "r2=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[2] << endl;
        cout << std::setw(0) << std::setfill(' ') << "r3=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[3] << endl;
        cout << std::setw(0) << std::setfill(' ') << "r4=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[4] << endl;
        cout << std::setw(0) << std::setfill(' ') << "r5=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[5] << endl;
        cout << std::setw(0) << std::setfill(' ') << "r6=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[6] << endl;
        cout << std::setw(0) << std::setfill(' ') << "r7=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[7] << endl;
        return 1;
      }
      case 0x10:{
        // soft prekid
        break;
      }
      case 0x20:{
        // iret
        if ( Emulator::stack.size() < 2){
          // pozovi prekidnu rutinu za greske
        }
        program_regs[PSW] = Emulator::stack[program_regs[SP]--];
        program_regs[PC] = Emulator::stack[program_regs[PSW]--];
        break;
      }
      case 0x30:{
        // call, poziv potprograma
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Emulator::stack[program_regs[SP]++] = program_regs[PC];
            program_regs[PC] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Emulator::stack[program_regs[SP]++] = program_regs[PC];
            program_regs[PC] = memory[payload];
            break;
          }
          case ADDR_REG_DIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Emulator::stack[program_regs[SP]++] = program_regs[PC];
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg] + payload;
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Emulator::stack[program_regs[SP]++] = program_regs[PC];
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = memory[program_regs[src_reg] + payload];
            break;
         
          }
          case ADDR_REG_DIR:{
            Emulator::stack[program_regs[SP]++] = program_regs[PC];
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Emulator::stack[program_regs[SP]++] = program_regs[PC];
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = memory[program_regs[src_reg]];
            break;
          }
        }
        break;
      }
      case 0x40:{
        // ret
        program_regs[PC] = stack[program_regs[SP]--];
        break;
      }
      case 0x50:{
        // jmp
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            program_regs[PC] = CODE_STARTING_POINT + payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            program_regs[PC] = memory[payload];
            break;
          }
          case ADDR_REG_DIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg] + payload;
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = memory[program_regs[src_reg] + payload];
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = memory[program_regs[src_reg]];
            break;
          }
        }
        break;
      }
      case 0x51:{
        // jeq
        bool condition = false;
        if ( program_regs[PSW] & 0x0001) condition = true;
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = memory[payload];
            break;
          }
          case ADDR_REG_DIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = program_regs[src_reg] + payload;
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = memory[program_regs[src_reg] + payload];
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = memory[program_regs[src_reg]];
            break;
          }
        }
        break;
      }
      case 0x52:{
        // jne
        bool condition = true;
        if ( program_regs[PSW] & 0x0001) condition = false;
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = memory[payload];
            break;
          }
          case ADDR_REG_DIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = program_regs[src_reg] + payload;
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = memory[program_regs[src_reg] + payload];
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = memory[program_regs[src_reg]];
            break;
          }
        }
        break;
      }
      case 0x53:{
        //jgt
        bool condition = true;
        if ( program_regs[PSW] & 0x0008) condition = false;
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = memory[payload];
            break;
          }
          case ADDR_REG_DIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = program_regs[src_reg] + payload;
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = memory[program_regs[src_reg] + payload];
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = memory[program_regs[src_reg]];
            break;
          }
        }
        break;
      }
      case 0x60:{
        // xchg
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f);
        signed short temp = program_regs[dest_reg];
        program_regs[dest_reg] = program_regs[src_reg];
        program_regs[src_reg] = temp;
        break;
      }      
      case 0x70:{
        //add
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] + program_regs[src_reg];
        break;
      }
      case 0x71:{
        //sub
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] - program_regs[src_reg];
        break;
      }
      case 0x72:{
        //mul
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] * program_regs[src_reg];
        break;
      }
      case 0x73:{
        //div
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] / program_regs[src_reg];
        break;
      }
      case 0x74:{
        //cmp
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        signed short operand2 = 0 - program_regs[src_reg];
        signed short temp = program_regs[dest_reg] + operand2;
        if ( temp == 0) program_regs[PSW] = program_regs[PSW] | 0x0001;
        if ( temp < 0) program_regs[PSW] = program_regs[PSW] | 0x0008;
        if ( program_regs[dest_reg] < 0 && program_regs[src_reg] < 0 && temp > 0) program_regs[PSW] = program_regs[PSW] | 0x0002;
        if ( program_regs[dest_reg] > 0 && program_regs[src_reg] > 0 && temp < 0) program_regs[PSW] = program_regs[PSW] | 0x0002;
        unsigned int carry_check = program_regs[dest_reg] + operand2;
        if ( carry_check & 0x10000) program_regs[PSW] | 0x0004;
        break;
      }
      case 0x80:{
        //not 
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = - program_regs[dest_reg];

        break;
      }
      case 0x81:{
        //and
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] & program_regs[src_reg];
        break;
      }
      case 0x82:{
        //or
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] | program_regs[src_reg];
        break;
      }
      case 0x83:{
        //xor
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = (-program_regs[dest_reg] & program_regs[src_reg]) | (program_regs[dest_reg] & -program_regs[src_reg]);
        break;
      }
      case 0x84:{
        //text
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        signed short temp = program_regs[dest_reg] & program_regs[src_reg];
        if ( temp == 0) program_regs[PSW] = program_regs[PSW] | 0x0001;
        if ( temp < 0) program_regs[PSW] = program_regs[PSW] | 0x0008;
        break;
      }
      case 0x90:{
        //shl
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] << program_regs[src_reg];
        if ( program_regs[dest_reg] == 0) program_regs[PSW] = program_regs[PSW] | 0x0001;
        if ( program_regs[dest_reg] < 0) program_regs[PSW] = program_regs[PSW] | 0x0008;
        break;
      }
      case 0x91:{
        //shl
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] >> program_regs[src_reg];
        if ( program_regs[dest_reg] == 0) program_regs[PSW] = program_regs[PSW] | 0x0001;
        if ( program_regs[dest_reg] < 0) program_regs[PSW] = program_regs[PSW] | 0x0008;
        break;
      }
      case 0xA0:{
        //ldr
        
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_IMMEDIATE:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            program_regs[dest_reg] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            program_regs[dest_reg] = memory[payload];
            break;
          }
          case ADDR_REG_DIR_POM:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[dest_reg] = program_regs[src_reg] + payload;
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[dest_reg] = memory[program_regs[src_reg] + payload];
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[dest_reg] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[dest_reg] = memory[program_regs[src_reg]];
            break;
          }
        }
        break;
     
      }
      case 0xB0:{
        //str
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_IMMEDIATE:{
           //zove se prekidna rutina za gresku
          }
          case ADDR_MEM:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            memory[payload] = program_regs[dest_reg];
            break;
          }
          case ADDR_REG_DIR_POM:{
            // zove se prekidna rutina
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[dest_reg] = program_regs[src_reg] + payload;
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;
            memory[program_regs[dest_reg] + payload] = program_regs[src_reg];
            break;
         
          }
          case ADDR_REG_DIR:{
           
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte src_reg = reg_descr & 0x0f;
            cout << "REG" << program_regs[dest_reg] << endl;
            program_regs[dest_reg] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte src_reg = reg_descr & 0x0f;
            memory[program_regs[dest_reg]] = program_regs[src_reg];
            break;
          }
        }
        break;
      }
      default:{
        cout << "UNDEFINED INSTR" << endl;
        exit(-1);
      }
      
      }
  return 0;
}

