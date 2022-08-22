#include "instr.h"
#include "MyLexer.h"
#include "assembler.h"
#include "utility.h"
#include "emulator.h"
#include "types.h"


map<string,Byte> Instruction_handler::instr_op_codes = map<string,Byte>();

void Instruction_handler::handleInstruction(INSTRUCTION_TYPE type, void* l){
  
  yyFlexLexer* lex = (yyFlexLexer*)l;
  int ret = 0;
  switch (type)
  {
  case INST_NOOP:{
    Byte op_code = instr_op_codes.find(lex->YYText())->second;
    Assembler::writeInstrNoArgToSectonBinary(op_code);
    break;
  }
  case INST_ONE_REG_DIR:{
    Instruction instr = Instruction();
    instr.type = INST_ONE_REG_DIR;
    instr.name = lex->YYText();
    instr.op_code = instr_op_codes.find(instr.name)->second;
    instr.op1 = nullptr;
    instr.op2 = nullptr;
    instr.op3 = nullptr;
    Instruction_handler::getArguements(&instr, lex);
    Assembler::writeInstrtToSectionBinary(&instr);
    break;
  }
  case INST_TWO_REG_DIR:{
    Instruction instr = Instruction();
    instr.type = INST_TWO_REG_DIR;
    instr.name = lex->YYText();
    instr.op_code = instr_op_codes.find(instr.name)->second;
    instr.op1 = nullptr;
    instr.op2 = nullptr;
    instr.op3 = nullptr;
    Instruction_handler::getArguements(&instr, lex);
    Assembler::writeInstrtToSectionBinary(&instr);
    break;
  }
  case INST_LOAD_STORE:{
  
    Instruction instr = Instruction();
    instr.type = INST_LOAD_STORE;
    instr.name = lex->YYText();
    instr.op_code = instr_op_codes.find(instr.name)->second;
    instr.op1 = nullptr;
    instr.op2 = nullptr;
    instr.op3 = nullptr;
   
    Instruction_handler::getArguements(&instr, lex);
    
    Assembler::writeInstrtToSectionBinary(&instr);
     
    break;
  }
  case INST_JUMP:{
    Instruction instr = Instruction();
    instr.type = INST_JUMP;
    instr.name = lex->YYText();
    instr.op_code = instr_op_codes.find(instr.name)->second;
    instr.op1 = nullptr;
    instr.op2 = nullptr;
    instr.op3 = nullptr;
    Instruction_handler::getArguements(&instr, lex);
    Assembler::writeInstrtToSectionBinary(&instr);
    break;
  }

  
  }
 
}

void Instruction_handler::initializeInstrOpCodes(){
 
  //HALT
  instr_op_codes.insert(pair<string,Byte>("halt", 0x00));
  //INT
  instr_op_codes.insert(pair<string,Byte>("int", 0x10));
  //IRET
  instr_op_codes.insert(pair<string,Byte>("iret", 0x20));
  //CALL
  instr_op_codes.insert(pair<string,Byte>("call", 0x30));
  //RET
  instr_op_codes.insert(pair<string,Byte>("ret", 0x40));
  //JMP
  instr_op_codes.insert(pair<string,Byte>("jmp", 0x50));
  //JEQ
  instr_op_codes.insert(pair<string,Byte>("jeq", 0x51));
  //JNE
  instr_op_codes.insert(pair<string,Byte>("jne", 0x52));
  //JGT
  instr_op_codes.insert(pair<string,Byte>("jgt", 0x53));
  //XCHG
  instr_op_codes.insert(pair<string,Byte>("xchg", 0x60));
  //ADD
  instr_op_codes.insert(pair<string,Byte>("add", 0x70));
  //SUB
  instr_op_codes.insert(pair<string,Byte>("sub", 0x71));
  //MUL
  instr_op_codes.insert(pair<string,Byte>("mul", 0x72));
  //DIV
  instr_op_codes.insert(pair<string,Byte>("div", 0x73));
  //CMP
  instr_op_codes.insert(pair<string,Byte>("cmp", 0x74));
  //NOT
  instr_op_codes.insert(pair<string,Byte>("not", 0x80));
  //AND
  instr_op_codes.insert(pair<string,Byte>("and", 0x81));
  //OR
  instr_op_codes.insert(pair<string,Byte>("or", 0x82));
  //XOR
  instr_op_codes.insert(pair<string,Byte>("xor", 0x83));
  //TEST
  instr_op_codes.insert(pair<string,Byte>("test", 0x84));
  //SHL
  instr_op_codes.insert(pair<string,Byte>("shl", 0x90));
  //SHR
  instr_op_codes.insert(pair<string,Byte>("shr", 0x91));
  //LDR
  instr_op_codes.insert(pair<string,Byte>("ldr", 0xA0));
  //STR
  instr_op_codes.insert(pair<string,Byte>("str", 0xB0));
}

void Instruction_handler::getArguements(Instruction* instr, void*l){
  
  yyFlexLexer* lex = (yyFlexLexer*)l;
  int ret = 0;
  switch (instr->type)
  {
    case INST_TWO_REG_DIR:{
      
      while(1){
        ret = lex->yylex();
        if ( ret == NEW_LINE) break;
        else if ( ret == ESCAPE_SPACES || lex->YYText() == ",") continue;
        else if ( ret >= REG_RX && ret <= REG_PSW) {
          if ( instr->op1 == nullptr){
            instr->op1 = new Operand();
            instr->op1->type = OPERAND_TYPE_REG;
            instr->op1->operand = lex->YYText();
          }
          else if (instr->op2 == nullptr){
            instr->op2 = new Operand();
            instr->op2->type = OPERAND_TYPE_REG;
            instr->op2->operand = lex->YYText();
          }
          else cerr << "Too many arguments for " << instr->name << " instruction" << endl;
        }
        else {
          
          cerr << "Error in sample.s\n Bad line writing" << endl;
          exit (-1);
        }
      }
      if ( instr->op1 == nullptr && instr->op2 == nullptr) {
        cerr << "No arguments provided to " << instr->name << " instruction" << endl;
        exit(-1);
      }
      break;
    }
    case INST_ONE_REG_DIR:{
      while(1){
        ret = lex->yylex();
        if ( ret == NEW_LINE) break;
        else if ( ret == ESCAPE_SPACES || lex->YYText() == ",") continue;
        else if ( ret >= REG_RX && ret <= REG_PSW) {
          if ( instr->op1 == nullptr){
            instr->op1 = new Operand();
            instr->op1->type = OPERAND_TYPE_REG;
            instr->op1->operand = lex->YYText();
          }
          else cerr << "Too many arguments for " << instr->name << " instruction" << endl;
        }
        else {
          cerr << "Error in sample.s\n Bad line writing" << endl;
          exit (-1);
        }
      }
      if ( instr->op1 == nullptr && instr->op2 == nullptr) {
       
        cerr << "No arguments provided to " << instr->name << " instruction" << endl;
        exit(-1);
      }
      break;
    }
    case INST_LOAD_STORE:{
      
      // get REGISTER
      ret = lex->yylex();
      while ( ret == ESCAPE_SPACES) ret = lex->yylex();

      if ( ret >= REG_RX && ret <= REG_PSW){
        instr->op1 = new Operand();
        instr->op1->type = OPERAND_TYPE_REG;
        instr->op1->operand = lex->YYText();
      }
      else {
        cerr << "Invalid instruction writing.\nExited\n" << endl;
        exit(-1);
      }
      //GET SECOND OPERAND
      
      while(1){
        ret = lex->yylex();
        
        if ( ret == NEW_LINE) break;
        else if ( ret == ESCAPE_SPACES) continue;
        else if ( ret == ARG_LITERAL_VALUE){
          instr->op2 = new Operand();
          instr->op2->operand = ((string)lex->YYText()).erase(0,1);
          instr->op2->type = OPERAND_TYPE_LITERAL;
          instr->addresing = ADDR_IMMEDIATE;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret >= REG_RX && ret <= REG_PSW){
          instr->op2 = new Operand();
          instr->op2->operand = lex->YYText();
          instr->op2->type = OPERAND_TYPE_REG;
          instr->addresing = ADDR_REG_DIR;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == LITERAL){
          instr->op2 = new Operand();
          instr->op2->operand = lex->YYText();
          instr->op2->type = OPERAND_TYPE_LITERAL;
          instr->addresing = ADDR_MEM;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == SYMBOL){
          instr->op2 = new Operand();
          instr->op2->operand = lex->YYText();
          instr->op2->type = OPERAND_TYPE_SYMBOL;
          instr->addresing = ADDR_MEM;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == ARG_SYMBOL_VALUE){
          instr->op2 = new Operand();
          instr->op2->operand = ((string)lex->YYText()).erase(0,1);
          instr->op2->type = OPERAND_TYPE_SYMBOL;
          instr->addresing = ADDR_IMMEDIATE;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == ARG_SYMBOL_MEM_PCREL){
          instr->op2 = new Operand();
          instr->op2->operand = ((string)lex->YYText()).erase(0,1);
          instr->op2->type = OPERAND_TYPE_SYMBOL;
          instr->addresing = ADDR_MEM;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = true;
        }
        else if ( ret == OPEN_PARENTHESIS){
          int one_operand = 1;
          while(ret != NEW_LINE){
            ret = lex->yylex();
            if ( ret == ESCAPE_SPACES) continue;
            if ( ret == PLUS) one_operand = 0;
            else if ( ret >= REG_RX && ret <= REG_PSW) {
              instr->op2 = new Operand();
              instr->op2->operand = lex->YYText();
              instr->op2->type = OPERAND_TYPE_REG;
              instr->addresing = ADDR_REG_INDIR;
              instr->update = UPDATE_REG_NULL;
              instr->is_relative = false;
            }
            else if ( one_operand == 0 && ret == LITERAL){
              instr->op3 = new Operand();
              instr->op3->operand = lex->YYText();
              instr->op3->type = OPERAND_TYPE_LITERAL;
              instr->addresing = ADDR_REG_INDIR_POM;
              instr->update = UPDATE_REG_NULL;
              instr->is_relative = false;
            }
            else if ( one_operand == 0 && ret == SYMBOL){
              instr->op3 = new Operand();
              instr->op3->operand = lex->YYText();
              instr->op3->type = OPERAND_TYPE_SYMBOL;
              instr->addresing = ADDR_REG_INDIR_POM;
              instr->update = UPDATE_REG_NULL;
              instr->is_relative = false;
            }
          }
          return;
          
        }
        else {
        
          cerr << "Error in sample.s\nBad line writing" << endl;
          exit (-1);
        }
      }
      
      if ( instr->op1 == nullptr || instr->op2 == nullptr) {
        cerr << "No arguments provided to " << instr->name << " instruction" << endl;
        exit(-1);
      }
      break;
    }
    case INST_JUMP:{
    while(1){
        ret = lex->yylex();

        if ( ret == NEW_LINE) break;
        else if ( ret == ESCAPE_SPACES) continue;
        else if ( ret == LITERAL){//
          instr->op1 = new Operand();
          instr->op1->operand = lex->YYText();
          instr->op1->type = OPERAND_TYPE_LITERAL;
          instr->addresing = ADDR_IMMEDIATE;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == JMP_ARG_REG_DIR){
          instr->op1 = new Operand();
          instr->op1->operand = ((string)lex->YYText()).erase(0,1);
          instr->op1->type = OPERAND_TYPE_REG;
          instr->addresing = ADDR_REG_DIR;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == SYMBOL){
          instr->op1 = new Operand();
          instr->op1->operand = lex->YYText();
          instr->op1->type = OPERAND_TYPE_SYMBOL;
          instr->addresing = ADDR_IMMEDIATE;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == ARG_SYMBOL_MEM_PCREL){
          instr->op1 = new Operand();
          instr->op1->operand = ((string)lex->YYText()).erase(0,1);
          instr->op1->type = OPERAND_TYPE_SYMBOL;
          instr->addresing = ADDR_IMMEDIATE;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = true;
        }
        else if ( ret == JMP_ARG_LITERAL_MEM){
          instr->op1 = new Operand();
          instr->op1->operand = ((string)lex->YYText()).erase(0,1);
          instr->op1->type = OPERAND_TYPE_LITERAL;
          instr->addresing = ADDR_MEM;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == JMP_ARG_SYMBOL_MEM){
          instr->op1 = new Operand();
          instr->op1->operand = ((string)lex->YYText()).erase(0,1);
          instr->op1->type = OPERAND_TYPE_SYMBOL;
          instr->addresing = ADDR_MEM;
          instr->update = UPDATE_REG_NULL;
          instr->is_relative = false;
        }
        else if ( ret == OPEN_PARENTHESIS_JMP){
          int one_operand = 1;
          while(ret != NEW_LINE){
            ret = lex->yylex();
            if ( ret == ESCAPE_SPACES) continue;
            if ( ret == PLUS) one_operand = 0;
            else if ( ret >= REG_RX && ret <= REG_PSW) {
              instr->op1 = new Operand();
              instr->op1->operand = lex->YYText();
              instr->op1->type = OPERAND_TYPE_REG;
              instr->addresing = ADDR_REG_INDIR;
              instr->update = UPDATE_REG_NULL;
              instr->is_relative = false;
            }
            else if ( one_operand == 0 && ret == LITERAL){
              instr->op2 = new Operand();
              instr->op2->operand = lex->YYText();
              instr->op2->type = OPERAND_TYPE_LITERAL;
              instr->addresing = ADDR_REG_INDIR_POM;
              instr->update = UPDATE_REG_NULL;
              instr->is_relative = false;
            }
            else if ( one_operand == 0 && ret == SYMBOL){
              instr->op2 = new Operand();
              instr->op2->operand = lex->YYText();
              instr->op2->type = OPERAND_TYPE_SYMBOL;
              instr->addresing = ADDR_REG_INDIR_POM;
              instr->update = UPDATE_REG_NULL;
              instr->is_relative = false;
            }
          }
          return;
          
        }
        else {
        
          cerr << "Error in sample.s\nBad line writing" << endl;
          exit (-1);
        }
      }
      
      if ( instr->op1 == nullptr) {
        cerr << "No arguments provided to " << instr->name << " instruction" << endl;
        exit(-1);
      }
      break;
    }
  
  }

}

Byte Instruction_handler::findRegCode(string text){
  
  if ( text == "pc"){
    return 7;
  }
  else if ( text == "sp"){
    return 5;
  }
  else if ( text == "psw"){
    return 8;
  }
  else { 
    return text.at(1) - '0';
  }

}

Word Instruction_handler::handleInstrSymbol(string symbol, Instruction* instr){

  if ( stringExistsInVector(symbol, Assembler::extern_symbols) == 1 ){

    ELF_RELT_Entry rel_entry = ELF_RELT_Entry();
    rel_entry.offset = Assembler::location_counter + 3;
    rel_entry.symbol = symbol;
    rel_entry.sym_info = SYM_INFO_GLOBAL;
    rel_entry.section_index = findSymbolInSymTBL(Assembler::current_section, &Assembler::elf_file.symtbl)->symndx;
    rel_entry.symbol_index = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)->symndx;
    if ( instr->is_relative){
      rel_entry.type = REL_TYPE_PC_REL_16;
      rel_entry.addend = -2;
    }
    else{
      rel_entry.type = REL_TYPE_ABS_16;
      rel_entry.addend = 0;
    }
    
    Assembler::elf_file.reltbls.push_back(rel_entry);
    return 0;
  }
  else{
   
    ELF_SYMT_Entry* sym_entry = nullptr;
    if ( (sym_entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)) != nullptr){
       if ( instr->is_relative && sym_entry->shndx == findSymbolInSymTBL(Assembler::current_section,&Assembler::elf_file.symtbl)->symndx){
          Word offset =  sym_entry->value - (Assembler::location_counter + 5) ;
          return offset;
        }
      if ( sym_entry->info == SYM_INFO_GLOBAL){
        
        ELF_RELT_Entry rel_entry = ELF_RELT_Entry();
        rel_entry.offset = Assembler::location_counter + 3;
        rel_entry.symbol = symbol;
        rel_entry.sym_info = SYM_INFO_GLOBAL;
        rel_entry.section_index = findSymbolInSymTBL(Assembler::current_section, &Assembler::elf_file.symtbl)->symndx;
        rel_entry.symbol_index = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)->symndx;
        if ( instr->is_relative){
          rel_entry.type = REL_TYPE_PC_REL_16;
          rel_entry.addend = -2;
        }
        else{
        rel_entry.type = REL_TYPE_ABS_16;
        rel_entry.addend = 0;
        }
       
        Assembler::elf_file.reltbls.push_back(rel_entry);
        return 0;
      }
      else{
        
        ELF_RELT_Entry rel_entry = ELF_RELT_Entry();
        rel_entry.offset = Assembler::location_counter+3;
        rel_entry.symbol = symbol;
        rel_entry.sym_info = SYM_INFO_LOCAL;
        rel_entry.section_index = findSymbolInSymTBL(Assembler::current_section, &Assembler::elf_file.symtbl)->symndx;
        rel_entry.symbol_index = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)->shndx;
        if ( instr->is_relative){
          rel_entry.type = REL_TYPE_PC_REL_16;
          rel_entry.addend = sym_entry->value - 2;
        }
        else{
          rel_entry.type = REL_TYPE_ABS_16;
          rel_entry.addend = sym_entry->value;
        }
        Assembler::elf_file.reltbls.push_back(rel_entry);
        return 0;
      }
      
    }
    else {
      
      if ( stringExistsInVector(symbol, Assembler::elf_file.strtab) == 0) Assembler::elf_file.strtab.push_back(symbol);
      ELF_FLINK_Entry* flink_entry = new ELF_FLINK_Entry();
      if (instr->is_relative) flink_entry->addr_type = FLINK_ADDR_TYPE_PC_REL; // po ovome znam da ide u relok, da je pisalo PC_REL potencijalno bismo odmah razresili
      else flink_entry->addr_type = FLINK_ADDR_TYPE_ABS;
      flink_entry->section = Assembler::current_section;
      flink_entry->section_location = Assembler::location_counter + 3;
      flink_entry->sym_name = symbol;
      if ( Assembler::flink_head == nullptr) Assembler::flink_head = flink_entry;
      else {
        ELF_FLINK_Entry* find_last = Assembler::flink_head;
        while ( find_last->next) find_last = find_last->next;
        find_last->next = flink_entry; 
      }
      return 0;
    }
  }

}