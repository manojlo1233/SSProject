#include "assembler.h"
#include <iostream>
#include "MyLexer.h"
#include "directives.h"
#include "utility.h"
#include <iomanip>


using namespace std;

ELF16_File Assembler::elf_file = ELF16_File();
string Assembler::current_section = "";
int Assembler::location_counter = 0;
ELF_FLINK_Entry* Assembler::flink_head = nullptr;

vector<string> Assembler::extern_symbols = vector<string>();
vector<string> Assembler::global_symbols = vector<string>();

map<string,int> Assembler::section_names_ndxs = map<string,int>();


void Assembler::initializeFile(){


  elf_file.elfHeader.shoff = 0;
  elf_file.elfHeader.sh_num_entries = 3;
 
  elf_file.elfHeader.symtblndx = SECTION_NDX_SYMTAB;
  elf_file.elfHeader.strndx = SECTION_NDX_STRTAB;

  elf_file.strtab.push_back(".undef");
  elf_file.strtab.push_back(".symtab");
  elf_file.strtab.push_back(".strtab");

  Assembler::section_names_ndxs.insert(pair<string,int>(".undef", 0));
  Assembler::section_names_ndxs.insert(pair<string,int>(".symtab", 1));
  Assembler::section_names_ndxs.insert(pair<string,int>(".strtab", 2));

  ELF_SHT_Entry und_sec = ELF_SHT_Entry();
  und_sec.offset = 0;
  und_sec.size = 0;
  und_sec.symtbndx = 0;
  und_sec.type = SH_TYPE_CODE;
  elf_file.shtable.push_back(und_sec);

  ELF_SHT_Entry sym_section = ELF_SHT_Entry();
  sym_section.offset = 0;
  sym_section.size = 0;
  sym_section.symtbndx = -1;
  sym_section.type = SH_TYPE_SYMTBL;
  elf_file.shtable.push_back(sym_section);

  ELF_SHT_Entry strtab_section = ELF_SHT_Entry();
  
  strtab_section.offset = 0; // na kraju se svakako mora ponovo postaviti jer se menja velicina elf_filei
  sym_section.symtbndx = -1;
  strtab_section.size = 0;
  strtab_section.type = SH_TYPE_STRTBL;
  elf_file.shtable.push_back(strtab_section);

  ELF_SYMT_Entry undef_sym = ELF_SYMT_Entry();
  undef_sym.info = SYM_INFO_LOCAL;
  undef_sym.name = 0;
  undef_sym.shndx = 0;
  undef_sym.sym_name = "UND";
  undef_sym.symndx = 0;
  undef_sym.type = SYM_TYPE_NOTYPE;
  undef_sym.value = 0;

  elf_file.symtbl.push_back(undef_sym);

  elf_file.sections_binary = map<string,vector<Byte>>();
 
  Instruction_handler::initializeInstrOpCodes();

}

int Assembler::asm_pass(void* l){

  Assembler::initializeFile();

  yyFlexLexer* lex = (yyFlexLexer*)l;
  int ret = 0;
  bool dir_end_exists = false;
	
	while ( ret != END_FILE )
	{

			ret = lex->yylex();
			if (ret == -1){
				printf("\nExited\n");
				exit(-1);
			} 
			if ( ret == NEW_LINE) {
				ret = 0;
				continue;
			}
      if ( ret >= DIR_GLOBAL && ret < DIR_END){
        handleDirective((DIRECTIVES_TYPE)ret, l);
      }
      if ( ret == DIR_END) {
        dir_end_exists = true;
        break;
      }

      if ( ret >= INST_NOOP && ret <= INST_LOAD_STORE){
        Instruction_handler::handleInstruction((INSTRUCTION_TYPE)ret, l);
      }

      if ( ret == LABEL) {
        handleLabels(lex->YYText(),l);
      }
			
	}
  if ( dir_end_exists == false){
    cerr << "End directive is missing in sample.s" << endl;
    exit(-1);
  }
  
  Assembler::validateSymbols();
  Assembler::finalizeFLINKS();
  Assembler::finalizeRelTBL();
  Assembler::finalizeSectionSize();
  Assembler::fixLocalSymbols(); 
  
  return 0;
}

void Assembler::writeToSectionBinary(Word value){
  vector<Byte>* write_vector;
  map<string,vector<Byte>>::iterator it;
  for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
    if ( it->first == Assembler::current_section) write_vector = &(it->second);
  }
  
  //little-endien

  Byte lower = value & 0xff;
  Byte upper = (value &0xff00) >> 8;

  write_vector->emplace(write_vector->begin() + Assembler::location_counter++,lower);
  write_vector->emplace(write_vector->begin() + Assembler::location_counter++, upper);

}

void Assembler::skipIntoSectionBinary(Word value){
  vector<Byte>* write_vector;
  map<string,vector<Byte>>::iterator it;
  for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
    if ( it->first == Assembler::current_section) write_vector = &(it->second);
  }
  for ( int i = 0 ; i < value; i++){
    write_vector->emplace(write_vector->begin() + Assembler::location_counter,0);
    Assembler::location_counter++;
  }
 
}

void Assembler::writeStringToSectionBinary(string text){

  vector<Byte>* write_vector;
  map<string,vector<Byte>>::iterator it;
  for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
    if ( it->first == Assembler::current_section) write_vector = &(it->second);
  }
  const char* tmp = text.c_str();
  for ( ; *tmp != '\0'; tmp++){
    if ( *tmp == '"') continue;
    write_vector->emplace(write_vector->begin() + Assembler::location_counter++,(unsigned char)*tmp);
  }
  write_vector->emplace(write_vector->begin() + Assembler::location_counter++,(unsigned char)0);
}

void Assembler::writeInstrNoArgToSectonBinary(Byte op_code){

  vector<Byte>* write_vector;
  map<string,vector<Byte>>::iterator it;
  for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
    if ( it->first == Assembler::current_section) write_vector = &(it->second);
  }
  write_vector->emplace(write_vector->begin() + Assembler::location_counter++, op_code);

}

void Assembler::writeInstrtToSectionBinary(Instruction* instr){

  switch (instr->type)
  {
  case INST_TWO_REG_DIR:{
    
    vector<Byte>* write_vector;
    map<string,vector<Byte>>::iterator it;
    for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
      if ( it->first == Assembler::current_section) write_vector = &(it->second);
    }
  
    Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
    Byte src_reg = Instruction_handler::findRegCode(instr->op2->operand);
    Byte data = src_reg | (dest_reg << 4);
    write_vector->emplace(write_vector->begin() + Assembler::location_counter++, instr->op_code);
    write_vector->emplace(write_vector->begin() + Assembler::location_counter++, data);
    delete instr->op1;
    delete instr->op2;

    instr->op1 = nullptr;
    instr->op2 = nullptr;
    instr->op3 = nullptr;
    break;
  }
  case INST_ONE_REG_DIR:{
    vector<Byte>* write_vector;
    map<string,vector<Byte>>::iterator it;
    for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
      if ( it->first == Assembler::current_section) write_vector = &(it->second);
    }
    if ( instr->name == "push"){
      Byte byte1 = Instruction_handler::instr_op_codes.find("str")->second;
      Byte byte2 = ( 0x1 << 4) | 0x2;
      Byte src_reg = 0x6; // sp
      Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
      Byte byte3 = (dest_reg << 4) | src_reg;
      write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
      write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
      write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2);  
    }
    else if ( instr->name == "pop"){
      Byte byte1 = Instruction_handler::instr_op_codes.find("ldr")->second;
      Byte byte2 = ( 0x4 << 4) | 0x2;
      Byte src_reg = 0x6; // sp
      Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
      Byte byte3 = (dest_reg << 4) | src_reg;
      write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
      write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
      write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2);  
    }
    else{
      Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
      Byte data = 0xf | (dest_reg << 4);
      write_vector->emplace(write_vector->begin() + Assembler::location_counter++, instr->op_code);
      write_vector->emplace(write_vector->begin() + Assembler::location_counter++, data);
    }
    
    delete instr->op1;
    instr->op1 = nullptr;
    instr->op2 = nullptr;
    instr->op3 = nullptr;
    break;
  }
  case INST_LOAD_STORE:{
    
    vector<Byte>* write_vector;
    map<string,vector<Byte>>::iterator it;
    for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
      if ( it->first == Assembler::current_section) write_vector = &(it->second);
    }
    switch (instr->addresing)
    { 
      case ADDR_IMMEDIATE:
      case ADDR_MEM:
      case ADDR_REG_INDIR_POM:
      case ADDR_REG_DIR_POM:{
       
        if ( instr->op3 == nullptr){
      
          if ( instr->op2->type == OPERAND_TYPE_LITERAL){
            Word value = getLiteralValue(instr->op2->operand);
            Byte byte1 = value & 0xff;
            Byte byte2 = (value & 0xff00) >> 8;
            Byte byte3 = (instr->update << 4) | (instr->addresing);
            Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
            Byte src_reg = 0xf;
            Byte byte4 = (dest_reg << 4) | src_reg;
            Byte byte5 = instr->op_code;
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte5);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte4);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2); 
          }
          if ( instr->op2->type == OPERAND_TYPE_SYMBOL){
          
            Word value = Instruction_handler::handleInstrSymbol(instr->op2->operand, instr);
            Byte byte1 = value & 0xff;
            Byte byte2 = (value & 0xff00) >> 8;
            Byte byte3 = (instr->update << 4) | (instr->addresing);
            Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
            Byte src_reg = 0xf;
            Byte byte4 = (dest_reg << 4) | src_reg;
            Byte byte5 = instr->op_code;
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte5);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte4);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2);    
          }
          
        }
        else{
           if ( instr->op3->type == OPERAND_TYPE_LITERAL){
            Word value = getLiteralValue(instr->op3->operand);
            Byte byte1 = value & 0xff;
            Byte byte2 = (value & 0xff00) >> 8;
            Byte byte3 = (instr->update << 4) | (instr->addresing);
            Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
            Byte src_reg = Instruction_handler::findRegCode(instr->op2->operand);
            Byte byte4 = (dest_reg << 4) | src_reg;
            Byte byte5 = instr->op_code;
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte5);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte4);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2); 
          }
          if ( instr->op3->type == OPERAND_TYPE_SYMBOL){
            
            Word value = Instruction_handler::handleInstrSymbol(instr->op3->operand, instr);
            Byte byte1 = value & 0xff;
            Byte byte2 = (value & 0xff00) >> 8;
            Byte byte3 = (instr->update << 4) | (instr->addresing);
            Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
            Byte src_reg = Instruction_handler::findRegCode(instr->op2->operand);
            Byte byte4 = (dest_reg << 4) | src_reg;
            Byte byte5 = instr->op_code;
           
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte5);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte4);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2);    
          }
         
        }
        delete instr->op1;
        delete instr->op2;
        delete instr->op3;
        instr->op1 = nullptr;
        instr->op2 = nullptr;
        instr->op3 = nullptr;
        
        return;
      }
      case ADDR_REG_DIR:
      case ADDR_REG_INDIR:{
         
        vector<Byte>* write_vector;
        map<string,vector<Byte>>::iterator it;
        for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
          if ( it->first == Assembler::current_section) write_vector = &(it->second);
        }
        
        Byte byte1 = (instr->update << 4) | (instr->addresing);
        Byte dest_reg = Instruction_handler::findRegCode(instr->op1->operand);
        Byte src_reg = Instruction_handler::findRegCode(instr->op2->operand);
        Byte byte2 = (dest_reg << 4) | src_reg;
        Byte byte3 = instr->op_code;
        
        write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
        write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2);
        write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
        
       
        delete instr->op1;
        delete instr->op2;
        instr->op1 = nullptr;
        instr->op2 = nullptr;
        
        return;
      }
      
    }
    
  }
  case INST_JUMP:{
    vector<Byte>* write_vector;
    map<string,vector<Byte>>::iterator it;
    for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
      if ( it->first == Assembler::current_section) write_vector = &(it->second);
    }
    switch (instr->addresing)
    { 
      case ADDR_IMMEDIATE:
      case ADDR_MEM:
      case ADDR_REG_INDIR_POM:
      case ADDR_REG_DIR_POM:{
        if ( instr->op2 == nullptr){
          if ( instr->op1->type == OPERAND_TYPE_LITERAL){
            Word value = getLiteralValue(instr->op1->operand);
            Byte byte1 = value & 0xff;
            Byte byte2 = (value & 0xff00) >> 8;
            Byte byte3 = (instr->update << 4) | (instr->addresing);
            Byte dest_reg = 0xf;
            Byte src_reg = 0xf;
            Byte byte4 = (dest_reg << 4) | src_reg;
            Byte byte5 = instr->op_code;
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte5);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte4);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2); 
          }
          if ( instr->op1->type == OPERAND_TYPE_SYMBOL){
            Word value = Instruction_handler::handleInstrSymbol(instr->op1->operand, instr);
            Byte byte1 = value & 0xff;
            Byte byte2 = (value & 0xff00) >> 8;
            Byte byte3 = (instr->update << 4) | (instr->addresing);
            Byte dest_reg = 0xf;
            Byte src_reg = 0xf;
            Byte byte4 = (dest_reg << 4) | src_reg;
            Byte byte5 = instr->op_code;
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte5);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte4);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2);    
          }
      
        }
        else{
          if ( instr->op2->type == OPERAND_TYPE_LITERAL){
            Word value = getLiteralValue(instr->op2->operand);
            Byte byte1 = value & 0xff;
            Byte byte2 = (value & 0xff00) >> 8;
            Byte byte3 = (instr->update << 4) | (instr->addresing);
            Byte dest_reg = 0xf;
            Byte src_reg = Instruction_handler::findRegCode(instr->op1->operand);
            Byte byte4 = (dest_reg << 4) | src_reg;
            Byte byte5 = instr->op_code;
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte5);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte4);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2); 
          }
          if ( instr->op2->type == OPERAND_TYPE_SYMBOL){
            
            Word value = Instruction_handler::handleInstrSymbol(instr->op2->operand, instr);
            Byte byte1 = value & 0xff;
            Byte byte2 = (value & 0xff00) >> 8;
            Byte byte3 = (instr->update << 4) | (instr->addresing);
            Byte dest_reg = 0xf;
            Byte src_reg = Instruction_handler::findRegCode(instr->op1->operand);
            Byte byte4 = (dest_reg << 4) | src_reg;
            Byte byte5 = instr->op_code;
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte5);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte4);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
            write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2);    
          }
        }
        
        
        delete instr->op1;
        delete instr->op2;
    
        instr->op1 = nullptr;
        instr->op2 = nullptr;
     
        break;
      }
      case ADDR_REG_DIR:
      case ADDR_REG_INDIR:{
        vector<Byte>* write_vector;
        map<string,vector<Byte>>::iterator it;
        for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
          if ( it->first == Assembler::current_section) write_vector = &(it->second);
        }
        Byte byte1 = (instr->update << 4) | (instr->addresing);
        Byte dest_reg = 0xf;
        Byte src_reg = Instruction_handler::findRegCode(instr->op1->operand);
        Byte byte2 = (dest_reg << 4) | src_reg;
        Byte byte3 = instr->op_code;
        write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte3);
        write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte2);
        write_vector->emplace(write_vector->begin() + Assembler::location_counter++, byte1);
        
       
        delete instr->op1;
      
        instr->op1 = nullptr;
   
       break;
      }
      
    }
  }
  } 
 
}
  
void Assembler::finalizeFLINKS(){

  for ( ELF_FLINK_Entry* curr = Assembler::flink_head; curr; curr = curr->next){
    
    ELF_SYMT_Entry* sym_entry = nullptr;
    if ( (sym_entry = findSymbolInSymTBL(curr->sym_name, &Assembler::elf_file.symtbl)) != nullptr){
      if ( sym_entry->shndx == findSymbolInSymTBL(curr->section, &Assembler::elf_file.symtbl)->symndx
       && curr->addr_type == FLINK_ADDR_TYPE_PC_REL){
        
        vector<Byte>* write_vector;
        map<string,vector<Byte>>::iterator it;
        
        for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
          if ( it->first == curr->section) write_vector = &(it->second);
        }
        
        Word value = sym_entry->value - (curr->section_location + 2);
        Byte byte1 = value & 0xff;
        Byte byte2 = (value & 0xff00) >> 8;
        
        (*write_vector)[curr->section_location++] = byte1;
        (*write_vector)[curr->section_location] = byte2;
        
        
      }
      else if ( sym_entry->info == SYM_INFO_GLOBAL){
        
        ELF_RELT_Entry rel_entry = ELF_RELT_Entry();
        rel_entry.offset = curr->section_location;
        rel_entry.symbol = curr->sym_name;
        rel_entry.sym_info = SYM_INFO_GLOBAL;
        rel_entry.section_index = findSymbolInSymTBL(curr->section, &Assembler::elf_file.symtbl)->symndx;
        rel_entry.symbol_index = findSymbolInSymTBL(curr->sym_name, &Assembler::elf_file.symtbl)->symndx;
        if ( curr->addr_type == FLINK_ADDR_TYPE_PC_REL){
          rel_entry.type = REL_TYPE_PC_REL_16;
          rel_entry.addend = -2;
        }
        else{
          rel_entry.type = REL_TYPE_ABS_16;
          rel_entry.addend = 0;
        }
        
        Assembler::elf_file.reltbls.push_back(rel_entry);
        }
      else{
          ELF_RELT_Entry rel_entry = ELF_RELT_Entry();
          rel_entry.offset = curr->section_location;
          rel_entry.symbol = curr->sym_name;
          rel_entry.sym_info = SYM_INFO_LOCAL;
          rel_entry.section_index = findSymbolInSymTBL(curr->section, &Assembler::elf_file.symtbl)->symndx;
          rel_entry.symbol_index = findSymbolInSymTBL(curr->sym_name, &Assembler::elf_file.symtbl)->shndx;
          if ( curr->addr_type == FLINK_ADDR_TYPE_PC_REL){
            rel_entry.type = REL_TYPE_PC_REL_16;
            rel_entry.addend = sym_entry->value - 2;
          }
          else{
            rel_entry.type = REL_TYPE_ABS_16;
            rel_entry.addend = sym_entry->value;
          }
          Assembler::elf_file.reltbls.push_back(rel_entry);
          
        }
      
    }
    else{
      cerr << "Symbol " << curr->sym_name << " was not declared extern but used without definition in sample.s" << endl;
      exit(-1);
    }
  }
  
  deleteFlinkList();

}

void Assembler::deleteFlinkList(){

  for ( ELF_FLINK_Entry* curr = Assembler::flink_head; curr; ){
    if ( curr == flink_head) flink_head = nullptr;
    ELF_FLINK_Entry* next = curr;
    if ( curr->next) curr = curr->next;
    else curr = nullptr;
    next->next = nullptr;
    delete next;
  }

}

void Assembler::finalizeRelTBL(){

  vector<ELF_RELT_Entry>* rel_tabel = &Assembler::elf_file.reltbls;

  for ( int i = 0; i < (*rel_tabel).size(); i++){
    ELF_RELT_Entry* entry = &(*rel_tabel)[i];
    if ( entry->sym_info == SYM_INFO_LOCAL && stringExistsInVector(entry->symbol, Assembler::global_symbols) == 1){
      entry->sym_info = SYM_INFO_GLOBAL;
      if ( entry->type == REL_TYPE_ABS_16){
        entry->addend = 0;
        entry->symbol_index = findSymbolInSymTBL(entry->symbol, &Assembler::elf_file.symtbl)->symndx;
      }
      else {
        entry->addend = -2;
        entry->symbol_index = findSymbolInSymTBL(entry->symbol, &Assembler::elf_file.symtbl)->symndx;
      }
    }
  }

  ELF_SHT_Entry rel_sec = ELF_SHT_Entry();
  rel_sec.offset = 0;
  rel_sec.size = 0;
  rel_sec.symtbndx = -1;
  rel_sec.type = SH_TYPE_RELA;
  Assembler::elf_file.shtable.push_back(rel_sec);
  Assembler::elf_file.elfHeader.sh_num_entries++;

}

void Assembler::validateSymbols(){

  for ( int i = 3; i < Assembler::elf_file.strtab.size(); i++){
    string symbol = Assembler::elf_file.strtab[i];
    ELF_SYMT_Entry* sym_entry = nullptr;
    
    if ( stringExistsInVector(symbol, Assembler::extern_symbols) == 1 && 
    stringExistsInVector(symbol, Assembler::global_symbols) == 1){
      cerr << "Extern symbol " << symbol << " cannot be declared as global in sample.s" << endl;
      exit(-1);
    }
    else if ( stringExistsInVector(symbol, Assembler::global_symbols) == 1 &&
    (sym_entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)) == nullptr){
      cerr << "Undefined symbol " << symbol << " in sample.s" << endl;
      exit(-1);
    }
    else if (stringExistsInVector(symbol, Assembler::extern_symbols) == 1 &&
    (sym_entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)) == nullptr){
      cerr << "Undefined symbol " << symbol << " in sample.s" << endl;
      exit(-1);
    }
    else if (stringExistsInVector(symbol, Assembler::extern_symbols) == 0 &&
    stringExistsInVector(symbol, Assembler::global_symbols) == 0 &&
    (sym_entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)) == nullptr){
      cerr << "Undefined symbol " << symbol << " in sample.s" << endl;
      exit(-1);
    }
  }

}

void Assembler::createTXTFile(ofstream& file){

    string little_space = "   ";
    string bigger_space = "       ";
    int ls = 3;
    int bs = 7;

    file << "--------------------------------- Symbol Table ---------------------------------" << endl;
    file << little_space<< "Num" << bigger_space<<"Value" << bigger_space << "Size" << bigger_space << "Type";
    file << bigger_space << "Bind" << bigger_space << "Index" << bigger_space  << bigger_space << "Name" << little_space << endl;

    for ( int i = 0 ; i < elf_file.symtbl.size(); i++){
      ELF_SYMT_Entry sym_entry = elf_file.symtbl[i];
      file << std::setw(3 + ls) <<sym_entry.symndx << std::setw(5 + bs) << std::hex << sym_entry.value;
      file << std::setw(4 + bs) << std::hex << getSize(sym_entry.symndx) << std::setw(4 + bs) << getType(sym_entry.type);
      file << std::setw(4 + bs) << getInfo(sym_entry.info) << std::setw(5 + bs) << std::dec;
      
      if ( sym_entry.shndx == 0 ){
        file<< "UND";
      }
      else{
        file << sym_entry.shndx;
      }
       file << std::setw(4 + bs*2) << sym_entry.sym_name << endl;
    }
    file << endl;
    file << "------------------- Relocation tables -------------------" << endl;
    
   vector<string> searched_sections = vector<string>();
   
  
    int num_of_rela_section = findNumOfRelaSections();
    
    while( searched_sections.size() != num_of_rela_section ){
     
      for ( int i = 0; i < Assembler::elf_file.reltbls.size(); i++){
        string section;
        if ( stringExistsInVector( section = findSymbolInSymTBLIndex(Assembler::elf_file.reltbls[i].section_index, &Assembler::elf_file.symtbl)->sym_name, searched_sections) == 0){
          
          searched_sections.push_back(section);
          Word sec = Assembler::elf_file.reltbls[i].section_index;
          file << endl;
          
          file << "# rela " << findSymbolInSymTBLIndex(sec, &Assembler::elf_file.symtbl)->sym_name << endl;
         
          
          file << little_space << "Offset" << bigger_space << bigger_space << "Type" << bigger_space << bigger_space << "Symbol" << bigger_space << "Addend" << endl;
          
          for ( int j = i; j < Assembler::elf_file.reltbls.size(); j++){
            
            ELF_RELT_Entry rel_entry = Assembler::elf_file.reltbls[j];
            if ( sec == rel_entry.section_index){
              file << std::setw(6 + ls) << std::hex << rel_entry.offset << std::setw(4 + bs*2) << Assembler::getRelType(rel_entry.type);
              file << std::setw(6 + bs*2) <<  getSymbolForRela(rel_entry)<<std::dec << std::setw(6 + bs) << rel_entry.addend << endl;
            }
          }

        }
        
      }
    }
    
    file << endl;
    file << "----------------------- Binary Section Content --------------------------" << endl;
    
    for ( map<string,vector<Byte>>::iterator it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
      file << "# Content of section " << it->first << endl;
      for ( int i = 0; i < it->second.size(); i++){
        if ( i > 0 && i % 4 == 0) file << " ";
        if ( i > 0 && i % 8 == 0) file << endl;
        
        file << std::setbase(16) << std::setfill('0') << std::setw(2) << (Word)it->second[i] << " ";
        
      }
      file << endl << endl;
    }
    
}

void Assembler::finalizeSectionSize(){

  for ( int i = 0; i < Assembler::elf_file.shtable.size(); i++){
    ELF_SYMT_Entry* section = findSymbolInSymTBLIndex(Assembler::elf_file.shtable[i].symtbndx, &Assembler::elf_file.symtbl);
    if ( section == nullptr) continue;
    map<string, vector<Byte>>::iterator it;
    for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
      if ( section->sym_name == it->first){
        Assembler::elf_file.shtable[i].size = it->second.size();
      }
    }
  }

}

Word Assembler::getSize(Word index){

  for ( int i = 0 ; i < Assembler::elf_file.shtable.size(); i++){
    if ( index == Assembler::elf_file.shtable[i].symtbndx) return Assembler::elf_file.shtable[i].size;
  }
  return 0;

}

string Assembler::getType(ELF_SYM_Type type){
  switch (type)
  {
  case SYM_TYPE_NOTYPE:
    return "NOTYP";
    break;
  
  case SYM_TYPE_SECTION:
    return "SCTN";
    break;
  }
  return "";
}

string Assembler::getInfo(ELF_SYM_Info info){
  switch (info)
  {
  case SYM_INFO_LOCAL:
    return "LOC";
    break;
  
  case SYM_INFO_GLOBAL:
    return "GLOB";
    break;
  }
  return "";
}

string Assembler::getRelType(ELF_REL_Type type){
  switch (type)
  {
  case REL_TYPE_ABS_16:
    return "ABS_16";
    break;
  
  case REL_TYPE_PC_REL_16:
    return "PC_REL_16";
    break;
  }
  return "";
}

string Assembler::getSymbolForRela(ELF_RELT_Entry entry){
  if ( findSymbolInSymTBL(entry.symbol, &Assembler::elf_file.symtbl) == nullptr){
    return findSymbolInSymTBLIndex (entry.section_index,&Assembler::elf_file.symtbl)->sym_name;
  }
  else return entry.symbol;

}

void Assembler::createBinaryFile(FILE* file){
  Assembler::elf_file.elfHeader.shoff = sizeof(ELF_Header);
  unsigned long next_section = (Assembler::elf_file.elfHeader.sh_num_entries-1) * sizeof(ELF_SHT_Entry) + sizeof(ELF_Header);
  unsigned long next_header = sizeof(ELF_Header);
  Assembler::elf_file.elfHeader.sh_num_entries--;
  Assembler::elf_file.elfHeader.symtblndx = 0;
  Assembler::elf_file.elfHeader.strndx = 1;
  Assembler::elf_file.elfHeader.relndx = 2;
  fwrite(&Assembler::elf_file.elfHeader, sizeof(ELF_Header), 1, file);

  //ubacivanje strtabel header
  for ( int i = 0; i < Assembler::elf_file.shtable.size(); i++){
    
    if ( Assembler::elf_file.shtable[i].type == SH_TYPE_SYMTBL) {

      Assembler::elf_file.shtable[i].offset = next_section;
      Word size = 0;
      for ( int j = 0; j < Assembler::elf_file.symtbl.size(); j++){
        size += sizeof(ELF_SYMT_Entry);
      }
      Assembler::elf_file.shtable[i].size = size;

      fwrite(&Assembler::elf_file.shtable[i], sizeof(ELF_SHT_Entry), 1, file);

      // ubacivanje sekcije symtbl

      fseek(file, next_section, SEEK_SET);
      for ( int i = 0; i < Assembler::elf_file.symtbl.size(); i++){
        fwrite(&Assembler::elf_file.symtbl[i].info, sizeof(ELF_SYM_Info), 1, file );
        fwrite(&Assembler::elf_file.symtbl[i].name, sizeof(Word), 1, file );
        fwrite(&Assembler::elf_file.symtbl[i].shndx, sizeof(Word), 1, file );
        for ( int j = 0; j < Assembler::elf_file.symtbl[i].sym_name.size(); j++){
          fwrite(&Assembler::elf_file.symtbl[i].sym_name[j], sizeof(char), 1, file );
        }
        fwrite("\0", sizeof(char),1 , file);

        fwrite(&Assembler::elf_file.symtbl[i].symndx, sizeof(Word), 1, file );
        fwrite(&Assembler::elf_file.symtbl[i].type, sizeof(ELF_SYM_Type), 1, file );
        fwrite(&Assembler::elf_file.symtbl[i].value, sizeof(Word), 1, file );
      }
      next_section += size;
    }
  }
  
  next_header+= sizeof(ELF_SHT_Entry);

   // ubacivanje symtabel headera

  fseek(file, next_header, SEEK_SET);
  
  for ( int i = 0; i < Assembler::elf_file.shtable.size(); i++){
    if ( Assembler::elf_file.shtable[i].type == SH_TYPE_STRTBL) {
      Assembler::elf_file.shtable[i].offset = next_section;
      Word size = 0;
      for ( int j = 0; j < Assembler::elf_file.strtab.size(); j++){
        size += Assembler::elf_file.strtab[j].size() + 1;
      }
      Assembler::elf_file.shtable[i].size = size;
      fwrite(&Assembler::elf_file.shtable[i], sizeof(ELF_SHT_Entry), 1, file);
       // ubacivanje sekcije strtab
      fseek(file, next_section, SEEK_SET);
      for ( int i = 0; i < Assembler::elf_file.strtab.size(); i++){
        for ( int j = 0; j < Assembler::elf_file.strtab[i].size(); j++){
          fwrite(&Assembler::elf_file.strtab[i][j], sizeof(char), 1, file );
          
        }
        fwrite("\0", sizeof(char),1 , file);
        
      }
      next_section += size;
    }
  }

  next_header+= sizeof(ELF_SHT_Entry);
   // ubacivanje rela headera
  fseek(file, next_header, SEEK_SET);

  for ( int i = 0; i < Assembler::elf_file.shtable.size(); i++){
    if ( Assembler::elf_file.shtable[i].type == SH_TYPE_RELA) {
      Assembler::elf_file.shtable[i].offset = next_section;
      Word size = 0;
      for ( int j = 0; j < Assembler::elf_file.reltbls.size(); j++){
        size += sizeof(ELF_RELT_Entry);
      }
      Assembler::elf_file.shtable[i].size = size;
      fwrite(&Assembler::elf_file.shtable[i], sizeof(ELF_SHT_Entry), 1, file);

      // ubacivanje sekcije rela

      fseek(file, next_section, SEEK_SET);
      for ( int i = 0; i < Assembler::elf_file.reltbls.size(); i++){
        fwrite(&Assembler::elf_file.reltbls[i].addend, sizeof(Offs), 1, file);
        fwrite(&Assembler::elf_file.reltbls[i].offset, sizeof(Offs), 1, file);
        fwrite(&Assembler::elf_file.reltbls[i].section_index, sizeof(Word), 1, file);
        fwrite(&Assembler::elf_file.reltbls[i].sym_info, sizeof(ELF_SYM_Info), 1, file);
        for ( int j = 0; j < Assembler::elf_file.reltbls[i].symbol.size(); j++){
          fwrite(&Assembler::elf_file.reltbls[i].symbol[j], sizeof(char), 1, file );
        }
        fwrite("\0", sizeof(char),1 , file);
        fwrite(&Assembler::elf_file.reltbls[i].symbol_index, sizeof(Word), 1, file);
        fwrite(&Assembler::elf_file.reltbls[i].type, sizeof(ELF_REL_Type), 1, file);
      
      }
      next_section += size;
    }
  }
  next_header+= sizeof(ELF_SHT_Entry);
   // ubacivanje headera


  for ( int i = 0; i < Assembler::elf_file.shtable.size();i++){
    if ( Assembler::elf_file.shtable[i].type == SH_TYPE_CODE && Assembler::elf_file.shtable[i].symtbndx > 0) {
      fseek(file, next_header, SEEK_SET);

      Assembler::elf_file.shtable[i].offset = next_section;
      
      //size je vec sredjen
      fwrite(&Assembler::elf_file.shtable[i], sizeof(ELF_SHT_Entry), 1, file);
      next_header+= sizeof(ELF_SHT_Entry);
     
      // ubacivanje sekcije 
      fseek(file, next_section, SEEK_SET);

   
      map<string,vector<Byte>>::iterator it;
      
      string sec_name;
      for ( it = Assembler::elf_file.sections_binary.begin(); it != Assembler::elf_file.sections_binary.end(); it++){
        if ( it->first == findSymbolInSymTBLIndex(Assembler::elf_file.shtable[i].symtbndx, &Assembler::elf_file.symtbl)->sym_name){
         
          sec_name = it->first;
          for ( int j = 0; j < it->first.size(); j++){
          fwrite(&it->first[j], sizeof(char), 1, file );
          }
          fwrite("\0", sizeof(char),1 , file);
          for ( int i = 0; i < it->second.size(); i++){
            fwrite(&it->second[i], sizeof(Byte), 1, file);
          }
        }
      }
      next_section += Assembler::elf_file.shtable[i].size + sec_name.size() + 1;
    }
    
  }
  
}

int Assembler::findNumOfRelaSections(){
  vector<string> rela_sections = vector<string>();
  for ( int i = 0; i < Assembler::elf_file.reltbls.size(); i++){
    if ( stringExistsInVector(findSymbolInSymTBLIndex(Assembler::elf_file.reltbls[i].section_index, &Assembler::elf_file.symtbl)->sym_name, rela_sections) == 0){
     
      rela_sections.push_back(findSymbolInSymTBLIndex(Assembler::elf_file.reltbls[i].section_index, &Assembler::elf_file.symtbl)->sym_name);
    }
  }
  return rela_sections.size();
}

void Assembler::fixLocalSymbols(){
  bool change = true;



  while( change ){
    change = false;
    for ( vector<ELF_SYMT_Entry>::iterator i = Assembler::elf_file.symtbl.begin(); i != Assembler::elf_file.symtbl.end(); i++){
      if ( i->info == SYM_INFO_LOCAL && i->type == SYM_TYPE_NOTYPE && i->sym_name != "UND"){
        
        for ( int j = 0; j < Assembler::elf_file.reltbls.size(); j++){

          if ( Assembler::elf_file.reltbls[j].symbol == i->sym_name){
            Assembler::elf_file.reltbls[j].symbol = findSymbolInSymTBLIndex(i->shndx, &Assembler::elf_file.symtbl)->sym_name;
          }

        }

        for ( int j = i->symndx + 1; j < Assembler::elf_file.symtbl.size(); j++){
          if ( Assembler::elf_file.symtbl[j].type == SYM_TYPE_SECTION){
             for ( int k = 0; k < Assembler::elf_file.shtable.size(); k++){
              if ( Assembler::elf_file.symtbl[j].symndx == Assembler::elf_file.shtable[k].symtbndx) Assembler::elf_file.shtable[k].symtbndx--;
             }
          }
          Assembler::elf_file.symtbl[j].symndx--;
          if ( i->symndx < Assembler::elf_file.symtbl[j].shndx) Assembler::elf_file.symtbl[j].shndx--;
        }
        for ( int j = 0; j < Assembler::elf_file.reltbls.size(); j++){
          if ( Assembler::elf_file.reltbls[j].symbol_index > i->symndx) Assembler::elf_file.reltbls[j].symbol_index--;
          if ( i->symndx < Assembler::elf_file.reltbls[j].section_index) Assembler::elf_file.reltbls[j].section_index--;
        }



        Assembler::elf_file.symtbl.erase(i);
        change = true;
        break;
      }
    }
  }
  for ( vector<ELF_SYMT_Entry>::iterator i = Assembler::elf_file.symtbl.begin(); i != Assembler::elf_file.symtbl.end(); i++){
    if ( i->type == SYM_TYPE_SECTION && i->shndx != 0){
       for ( int j = 0; j < Assembler::elf_file.reltbls.size(); j++){
          if ( i->sym_name == Assembler::elf_file.reltbls[j].symbol){
            Assembler::elf_file.reltbls[j].symbol_index = i->symndx;
          }
       }
    }
  }
  

}
