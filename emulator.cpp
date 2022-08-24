#include "emulator.h"
#include "instr.h"
#include <iomanip>

using namespace std::literals::chrono_literals;
vector<Word> Emulator::program_regs = vector<Word>(9, 0);
vector<Byte> Emulator::memory = vector<Byte>(2<<16, 0);
Word Emulator::code_size = 0;
thread Emulator::timer_thread = thread();
atomic<bool> Emulator::timer_finished = ATOMIC_VAR_INIT(false);
atomic<bool> Emulator::timer_set = ATOMIC_VAR_INIT(false);
thread Emulator::term_thread = thread();
atomic<bool> Emulator::term_finished = ATOMIC_VAR_INIT(false);
atomic<bool> Emulator::term_set = ATOMIC_VAR_INIT(false);
termios Emulator::terminal_handler = termios();
termios Emulator::terminal_reset = termios();

void Emulator::initialize_terminal(){
  
  tcgetattr(0, &Emulator::terminal_handler);
  tcgetattr(0, &Emulator::terminal_reset);
  Emulator::terminal_handler.c_lflag &= ~ECHO;
  
  Emulator::terminal_handler.c_lflag &= ~ICANON;
  Emulator::terminal_handler.c_cc[VMIN] = 0;
  Emulator::terminal_handler.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW ,&Emulator::terminal_handler);
  
}

void Emulator::load_file(string input_file){
  
  FILE* fin = fopen(input_file.c_str() , "r");
  if ( fin == nullptr) {
    cerr << "Error opening the file" << endl;
    exit(-1);
  }

  fread(&Emulator::code_size, sizeof(Word), 1, fin);
  Byte byte;
  for ( int i = 0; i < code_size; i++){
    fread(&byte, sizeof(Byte), 1, fin);
    Emulator::memory[i] = byte;
  }
  Byte pc_low = memory[0x00];
  Byte pc_high = memory[0x01];
  program_regs[PC] = ((Word)pc_high << 8) | pc_low;

}

void Emulator::emulate_code(){

  //Emulator::timer_thread = thread(Emulator::timer);
  Emulator::term_thread = thread(Emulator::terminal);
  Word op_code;
  while(1){
    
    /*if ( Emulator::timer_set) {
    Emulator::callTimer();
    Emulator::timer_set = false;
    
    }*/
    if ( Emulator::term_set && ((program_regs[PSW] & 0x8000) >> 15) == 0){
      Emulator::callTerm();
      Emulator::term_set = false;
    }

    op_code = memory[program_regs[PC]++];

    if ( handle_op(op_code) == 1) break;
   
  }
  timer_finished = true;
  term_finished = true;
  //Emulator::timer_thread.join();
  Emulator::term_thread.join();
  tcsetattr(1, TCSANOW, &Emulator::terminal_reset);
}

void Emulator::terminal(){

  char c = '\0';
  while ( term_finished == false){
    
    read(0, &c, 1);
    if ( c != '\0'){
      tcflush(0, TCIOFLUSH);
      memory[TERM_IN] = (Byte)c;
      c = '\0';
      term_set = true;
    }
    
  }
 
  
}

void Emulator::callTerm(){
  
  Byte psw_high = (program_regs[PSW] & 0xff00) >> 8;
  Byte psw_low =  (program_regs[PSW] & 0x00ff);
  
  memory[--program_regs[SP]] = psw_high;
  memory[--program_regs[SP]] = psw_low;
  
  Byte high_pc = (program_regs[PC] & 0xff00) >> 8;
  Byte low_pc =  (program_regs[PC] & 0x00ff);
  memory[--program_regs[SP]] = high_pc;
  memory[--program_regs[SP]] = low_pc;
  Byte new_pc_low = memory[ISR_TERMINAL_IVT % 0x8 * 2];
  Byte new_pc_high = memory[ISR_TERMINAL_IVT % 0x8 * 2 + 1];

  program_regs[PC] = ((Word)new_pc_high) << 8 | new_pc_low;
  program_regs[PSW] = program_regs[PSW] | 0x8000;
    
  
}
void Emulator::timer(){
  Word waiting_time;
  Byte timer_low = memory[TIMER_CFG];
  Byte timer_high = memory[TIMER_CFG + 1];
  waiting_time = ((Word)timer_high) << 8 | timer_low;
  int millisec = 0;
  switch (waiting_time)
  {
  case 0x0:{
    millisec = 500;
    break;
  }
  case 0x1:{
    millisec = 1000;
    break;
  }
  case 0x2:{
    millisec = 1500;
    break;
  }
  case 0x3:{
    millisec = 2000;
    break;
  }
  case 0x4:{
    millisec = 2500;
    break;
  }
  case 0x5:{
    millisec = 3000;
    break;
  }
  case 0x6:{
    millisec = 3500;
    break;
  }
  case 0x7:{
    millisec = 4000;
    break;
  }
  }
  while( Emulator::timer_finished == false){
    if ( Emulator::timer_set) {
    
      std::this_thread::sleep_for(3s);
    }
    else {
      Emulator::timer_set = true;
     
      std::this_thread::sleep_for(std::chrono::milliseconds(millisec));
    }
  }

  
 
}

void Emulator::callTimer(){
  if ( ((program_regs[PSW] & 0x8000) >> 15) == 0){
    Byte psw_high = (program_regs[PSW] & 0xff00) >> 8;
    Byte psw_low =  (program_regs[PSW] & 0x00ff);
    memory[--program_regs[SP]] = psw_high;
    memory[--program_regs[SP]] = psw_low;
    Byte pc_high = (program_regs[PC] & 0xff00) >> 8;
    Byte pc_low =  (program_regs[PC] & 0x00ff);
    memory[--program_regs[SP]] = pc_high;
    memory[--program_regs[SP]] = pc_low;
    Byte low_pc_ivt = memory[ISR_TIMER_IVT];
    Byte high_pc_ivt = memory[ISR_TIMER_IVT+1];
    
    program_regs[PC] = ((Word)high_pc_ivt) << 8 | low_pc_ivt;
   
    program_regs[PSW] = program_regs[PSW] | 0x8000;
  } 
  
  
  
}

int Emulator::handle_op(Byte op_code){
  
  switch ((Word)op_code)
      {
      case 0x00:{
        Emulator::timer_finished = true;
        //halt
        cout << "------------------------------------------------" << endl;
        cout << "Emulated processor executed halt instruction" << endl;
        cout << "Emulated processor state: psw=0x" << std::setbase(4) << std::setw(4) << std::setfill('0')<< program_regs[8] << endl;
        cout << "r0=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[0] << endl;
        cout << "r1=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[1] << endl;
        cout << "r2=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[2] << endl;
        cout << "r3=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[3] << endl;
        cout << "r4=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[4] << endl;
        cout << "r5=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[5] << endl;
        cout << "r6=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[6] << endl;
        cout << "r7=0x" << std::setbase(16) << std::setw(4) << std::setfill('0') << program_regs[7] << endl;
        return 1;
      }
      case 0x10:{
        // soft prekid
        //push psw; pc<=mem16[(reg[DDDD] mod 8)*2]
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest = (reg_descr & 0xf0) >> 4;

        Byte psw_high = (program_regs[PSW] & 0xff00) >> 8;
        Byte psw_low =  (program_regs[PSW] & 0x00ff);
        
        memory[--program_regs[SP]] = psw_high;
        memory[--program_regs[SP]] = psw_low;
       
        Byte high_pc = (program_regs[PC] & 0xff00) >> 8;
        Byte low_pc =  (program_regs[PC] & 0x00ff);
        memory[--program_regs[SP]] = high_pc;
        memory[--program_regs[SP]] = low_pc;
        Byte new_pc_low = memory[(program_regs[dest] % 0x8) * 2];
        Byte new_pc_high = memory[(program_regs[dest] % 0x8) * 2 + 1];
      
       
        program_regs[PC] = ((Word)new_pc_high) << 8 | new_pc_low;
        program_regs[PSW] = program_regs[PSW] | 0x8000;
        
        break;
      }
      case 0x20:{
        // iret
       
        Byte pc_low = Emulator::memory[program_regs[SP]++];
        Byte pc_high = Emulator::memory[program_regs[SP]++];
       
        
        Byte psw_low = Emulator::memory[program_regs[SP]++];
        Byte psw_high = Emulator::memory[program_regs[SP]++];
        program_regs[PSW] = ((Word)psw_high) << 8 | psw_low;
        program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
        program_regs[PSW] = program_regs[PSW] & 0x7fff;
        
        
        break;
      }
      case 0x30:{
        // call, poziv potprograma
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
           case ADDR_PC_REL:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            signed short payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte pc_high = (program_regs[PC] & 0xff00) >> 8;
            Byte pc_low = program_regs[PC] & 0x00ff;
            memory[--program_regs[SP]] = pc_high;
            memory[--program_regs[SP]] = pc_low;
            program_regs[PC] = program_regs[PC] + payload;
            break;
          }
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte pc_high = (program_regs[PC] & 0xff00) >> 8;
            Byte pc_low = program_regs[PC] & 0x00ff;
            memory[--program_regs[SP]] = pc_high;
            memory[--program_regs[SP]] = pc_low;
            program_regs[PC] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte pc_stack_high = (program_regs[PC] & 0xff00) >> 8;
            Byte pc_stack_low = program_regs[PC] & 0x00ff;
            memory[--program_regs[SP]] = pc_stack_high;
            memory[--program_regs[SP]] = pc_stack_low;
            Byte pc_low = memory[payload];
            Byte pc_high = memory[payload + 1];
            program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
            break;
          }
          case ADDR_REG_DIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte pc_stack_high = (program_regs[PC] & 0xff00) >> 8;
            Byte pc_stack_low = program_regs[PC] & 0x00ff;
            memory[--program_regs[SP]] = pc_stack_high;
            memory[--program_regs[SP]] = pc_stack_low;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg] + payload;
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte pc_stack_high = (program_regs[PC] & 0xff00) >> 8;
            Byte pc_stack_low = program_regs[PC] & 0x00ff;
            memory[--program_regs[SP]] = pc_stack_high;
            memory[--program_regs[SP]] = pc_stack_low;
            Byte src_reg = reg_descr & 0x0f;
            Byte pc_low = memory[program_regs[src_reg] + payload];
            Byte pc_high = memory[program_regs[src_reg] + payload + 1];
            program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
            
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte pc_high = (program_regs[PC] & 0xff00) >> 8;
            Byte pc_low = program_regs[PC] & 0x00ff;
            memory[--program_regs[SP]] = pc_high;
            memory[--program_regs[SP]] = pc_low;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte pc_stack_high = (program_regs[PC] & 0xff00) >> 8;
            Byte pc_stack_low = program_regs[PC] & 0x00ff;
            memory[--program_regs[SP]] = pc_stack_high;
            memory[--program_regs[SP]] = pc_stack_low;
            Byte src_reg = reg_descr & 0x0f;
            Byte pc_low = memory[program_regs[src_reg]];
            Byte pc_high = memory[program_regs[src_reg] + 1];
            program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
            break;
          }
        }
        break;
      }
      case 0x40:{
        // ret
        Byte pc_low = Emulator::memory[program_regs[SP]++];
        Byte pc_high = Emulator::memory[program_regs[SP]++];
        program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
       
        break;
      }
      case 0x50:{
        // jmp
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_PC_REL:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            signed short payload = ((Word)payload_high) << 8 | (Word)payload_low;
            program_regs[PC] = program_regs[PC] + payload;
            break;
          }
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            signed short payload = ((Word)payload_high) << 8 | (Word)payload_low;
            program_regs[PC] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte pc_low = memory[payload];
            Byte pc_high = memory[payload + 1];
            program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
          
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
            Byte pc_low = memory[program_regs[src_reg] + payload];
            Byte pc_high = memory[program_regs[src_reg] + payload + 1];
            program_regs[PC] = ((Word)pc_high) << 8 | pc_low;

            break;
         
          }
          case ADDR_REG_DIR:{
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte src_reg = reg_descr & 0x0f;
            Byte pc_low = memory[program_regs[src_reg]];
            Byte pc_high = memory[program_regs[src_reg] + 1];
            program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
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
          case ADDR_PC_REL:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            signed short payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = program_regs[PC] + payload;
            break;
          }
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte pc_low = memory[payload];
            Byte pc_high = memory[payload + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
            
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
            Byte pc_low = memory[program_regs[src_reg] + payload];
            Byte pc_high = memory[program_regs[src_reg] + payload + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
           
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte src_reg = reg_descr & 0x0f;
            Byte pc_low = memory[program_regs[src_reg]];
            Byte pc_high = memory[program_regs[src_reg] + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
           
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
           case ADDR_PC_REL:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            signed short payload = ((Word)payload_high) << 8 | (Word)payload_low;
            program_regs[PC] = program_regs[PC] + payload;
            break;
          }
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] =  payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
             Byte pc_low = memory[payload];
            Byte pc_high = memory[payload + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
           
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
            Byte pc_low = memory[program_regs[src_reg] + payload];
            Byte pc_high = memory[program_regs[src_reg] + payload + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
          
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte src_reg = reg_descr & 0x0f;
            if ( condition) program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte src_reg = reg_descr & 0x0f;
            Byte pc_low = memory[program_regs[src_reg]];
            Byte pc_high = memory[program_regs[src_reg] + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
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
           case ADDR_PC_REL:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            signed short payload = ((Word)payload_high) << 8 | (Word)payload_low;
            program_regs[PC] = program_regs[PC] + payload;
            break;
          }
          case ADDR_IMMEDIATE:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            if ( condition) program_regs[PC] = payload;
            break;
          }
          case ADDR_MEM:{
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte pc_low = memory[payload];
            Byte pc_high = memory[payload + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
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
            Byte pc_low = memory[program_regs[src_reg] + payload];
            Byte pc_high = memory[program_regs[src_reg] + payload + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
            break;
         
          }
          case ADDR_REG_DIR:{
            Byte src_reg = reg_descr & 0x0f;
            program_regs[PC] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte src_reg = reg_descr & 0x0f;
            Byte pc_low = memory[program_regs[src_reg]];
            Byte pc_high = memory[program_regs[src_reg] + 1];
            if ( condition) program_regs[PC] = ((Word)pc_high) << 8 | pc_low;
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
        else program_regs[PSW] = program_regs[PSW] & 0xfffe;
        if ( temp < 0) program_regs[PSW] = program_regs[PSW] | 0x0008;
        else program_regs[PSW] = program_regs[PSW] & 0xfff7;
        if ( program_regs[dest_reg] < 0 && program_regs[src_reg] < 0 && temp > 0) program_regs[PSW] = program_regs[PSW] | 0x0002;
        else program_regs[PSW] = program_regs[PSW] & 0xfffd;
        if ( program_regs[dest_reg] > 0 && program_regs[src_reg] > 0 && temp < 0) program_regs[PSW] = program_regs[PSW] | 0x0002;
        else program_regs[PSW] = program_regs[PSW] & 0xfffd;
        unsigned int carry_check = program_regs[dest_reg] + operand2;
        if ( carry_check & 0x10000) program_regs[PSW] = program_regs[PSW] | 0x0004;
        else program_regs[PSW] = program_regs[PSW] & 0xfffb;
        break;
      }
      case 0x80:{
        //not 
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = ~program_regs[dest_reg];

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
        program_regs[dest_reg] = (~program_regs[dest_reg] & program_regs[src_reg]) | (program_regs[dest_reg] & ~program_regs[src_reg]);
        break;
      }
      case 0x84:{
        //test
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        signed short temp = program_regs[dest_reg] & program_regs[src_reg];

        if ( temp == 0) program_regs[PSW] = program_regs[PSW] | 0x0001;
        else program_regs[PSW] = program_regs[PSW] & 0xfffe;

        if ( temp < 0) program_regs[PSW] = program_regs[PSW] | 0x0008;
        else program_regs[PSW] = program_regs[PSW] & 0xfff7;

        
        break;
      }
      case 0x90:{
        //shl
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] << program_regs[src_reg];

        if ( program_regs[dest_reg] == 0) program_regs[PSW] = program_regs[PSW] | 0x0001;
        else program_regs[PSW] = program_regs[PSW] & 0xfffe;
        if ( program_regs[dest_reg] < 0) program_regs[PSW] = program_regs[PSW] | 0x0008;
        else program_regs[PSW] = program_regs[PSW] & 0xfff7;

        break;
      }
      case 0x91:{
        //shl
        Byte reg_descr = memory[program_regs[PC]++];
        Byte dest_reg = (reg_descr & 0xf0) >> 4;
        Byte src_reg = (reg_descr & 0x0f); 
        program_regs[dest_reg] = program_regs[dest_reg] >> program_regs[src_reg];
        if ( program_regs[dest_reg] == 0) program_regs[PSW] = program_regs[PSW] | 0x0001;
        else program_regs[PSW] = program_regs[PSW] & 0xfffe;
        if ( program_regs[dest_reg] < 0) program_regs[PSW] = program_regs[PSW] | 0x0008;
        else program_regs[PSW] = program_regs[PSW] & 0xfff7;
        break;
      }
      case 0xA0:{
        //ldr
     
        Byte reg_descr = memory[program_regs[PC]++];
        Byte addr_mode = memory[program_regs[PC]++];
        switch (addr_mode & 0x0f)
        {
          case ADDR_PC_REL:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            signed short payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte reg_low = memory[program_regs[PC] + payload];
            Byte reg_high = memory[program_regs[PC] + payload + 1];
            program_regs[dest_reg] = ((Word)reg_high) << 8 | reg_low;
            break;
          }
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

            Byte reg_low = memory[payload];
            Byte reg_high = memory[payload + 1];
            program_regs[dest_reg] = ((Word)reg_high) << 8 | reg_low;
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
            Byte update = ((Word)addr_mode) >> 4;
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;

            if ( (Word)update == 0x01) program_regs[src_reg]--;
            if ( (Word)update == 0x02) program_regs[src_reg]++;

            if ( (Word)src_reg == 0x06) {
              Byte reg_low = memory[program_regs[src_reg] + payload];
              Byte reg_high = memory[program_regs[src_reg] + payload + 1]; 
              program_regs[dest_reg] = ((Word)reg_high) << 8 | reg_low;
            }
            else{
              Byte reg_low = memory[program_regs[src_reg] + payload];
              Byte reg_high = memory[program_regs[src_reg] + payload + 1]; 
              program_regs[dest_reg] = ((Word)reg_high) << 8 | reg_low;
            }

           

            if ( (Word)update == 0x03) program_regs[src_reg]--;
            if ( (Word)update == 0x04) program_regs[src_reg]++;

            break;
         
          }
          case ADDR_REG_DIR:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte src_reg = reg_descr & 0x0f;
            program_regs[dest_reg] = program_regs[src_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte update = ((Word)addr_mode) >> 4;
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte src_reg = reg_descr & 0x0f;

            if ( (Word)update == 0x01) program_regs[src_reg] = program_regs[src_reg] - 2;
            if ( (Word)update == 0x02) program_regs[src_reg] = program_regs[src_reg] + 2;

           
            Byte reg_low = memory[program_regs[src_reg]];
            Byte reg_high = memory[program_regs[src_reg] + 1]; 
            program_regs[dest_reg] = ((Word)reg_high) << 8 | reg_low;
          
           
            if ( (Word)update == 0x03) program_regs[src_reg] = program_regs[src_reg] - 2;
            if ( (Word)update == 0x04) program_regs[src_reg] = program_regs[src_reg] + 2;

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
          case ADDR_PC_REL:
          case ADDR_IMMEDIATE:{
            Byte low_pc = memory[ISR_ERROR_STARTING_CODE];
            Byte high_pc = memory[ISR_ERROR_STARTING_CODE+1];
            program_regs[PC] = ((Word)high_pc) << 8 | low_pc;
          }
          case ADDR_MEM:{
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Word payload = ((Word)payload_high) << 8 | (Word)payload_low;
            
            memory[payload] = (program_regs[dest_reg] & 0x00ff);
            memory[payload + 1] = (program_regs[dest_reg]&0xff00) >> 8;
            if ( payload == TERM_OUT) {
              
              cout << (char)memory[payload] << endl;
            }
            
            break;
          }
          case ADDR_REG_DIR_POM:{
            Byte low_pc = memory[ISR_ERROR_STARTING_CODE];
            Byte high_pc = memory[ISR_ERROR_STARTING_CODE+1];
            program_regs[PC] = ((Word)high_pc) << 8 | low_pc;
            
            break;
          }
          case ADDR_REG_INDIR_POM:{
            Byte update = ((Word)addr_mode) >> 4;
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte payload_low = memory[program_regs[PC]++];
            Byte payload_high = memory[program_regs[PC]++];
            Offs payload = ((Word)payload_high) << 8 | (Word)payload_low;
            Byte src_reg = reg_descr & 0x0f;

            if ( (Word)update == 0x01) program_regs[src_reg] = program_regs[src_reg] - 2;
            if ( (Word)update == 0x02) program_regs[src_reg] = program_regs[src_reg] + 2;

         
            memory[program_regs[src_reg] + payload] = (program_regs[dest_reg] & 0x00ff);
            memory[program_regs[src_reg] + payload + 1] = (program_regs[dest_reg]&0xff00) >> 8;
            if ( (program_regs[src_reg] + payload) == TERM_OUT) cout << (char)memory[program_regs[src_reg] + payload];
      
            if ( (Word)update == 0x03) program_regs[src_reg] = program_regs[src_reg] - 2;
            if ( (Word)update == 0x04) program_regs[src_reg] = program_regs[src_reg] + 2;
            break;
         
          }
          case ADDR_REG_DIR:{
           
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte src_reg = reg_descr & 0x0f;
            
            program_regs[src_reg] = program_regs[dest_reg];
            break;
          }
          case ADDR_REG_INDIR:{
            Byte update = (addr_mode & 0xf0) >> 4;
            Byte dest_reg = (reg_descr & 0xf0) >> 4;
            Byte src_reg = reg_descr & 0x0f;
          
            if ( (Word)update == 0x01) program_regs[src_reg] = program_regs[src_reg] - 2;
            if ( (Word)update == 0x02) program_regs[src_reg] = program_regs[src_reg] + 2;
           
            
            
            memory[program_regs[src_reg]] = (program_regs[dest_reg] & 0x00ff);
            memory[program_regs[src_reg] + 1] = (program_regs[dest_reg]&0xff00) >> 8;
            if ( program_regs[src_reg] == TERM_OUT) cout << (char)memory[program_regs[src_reg]];
            

            if ( (Word)update == 0x03) program_regs[src_reg] = program_regs[src_reg] - 2;
            if ( (Word)update == 0x04) program_regs[src_reg] = program_regs[src_reg] + 2;
            break;
          }
        }
        break;
      }
      default:{
        
        Byte low_pc = memory[ISR_ERROR_STARTING_CODE];
        Byte high_pc = memory[ISR_ERROR_STARTING_CODE+1];
        program_regs[PC] = ((Word)high_pc) << 8 | low_pc;
      }
      
      }
  return 0;
}

