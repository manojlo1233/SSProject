#include "directives.h"
#include <iostream>
#include "assembler.h"
#include "utility.h"
#include "MyLexer.h"


void handleDirective( DIRECTIVES_TYPE type, void* l){
  yyFlexLexer* lex = (yyFlexLexer*)l;
  int ret = 0;
  switch (type)
  {
    case DIR_GLOBAL:{
      while(1){
          ret = lex->yylex();
          if ( ret == NEW_LINE) break;
          if ( lex->YYText() == "," || ret == ESCAPE_SPACES) continue;
          string symbol = lex->YYText();
          ELF_SYMT_Entry* entry = nullptr;
          if ( (entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)) != nullptr){
            entry->info = SYM_INFO_GLOBAL;
            if (stringExistsInVector(symbol, Assembler::global_symbols) == 0) Assembler::global_symbols.push_back(symbol);
            if (stringExistsInVector(symbol, Assembler::elf_file.strtab) == 0)Assembler::elf_file.strtab.push_back(symbol);
          }
          else if ( stringExistsInVector(symbol, Assembler::extern_symbols) == 0 && 
            stringExistsInVector(symbol, Assembler::global_symbols) == 0){
            Assembler::global_symbols.push_back(symbol);
            if (stringExistsInVector(symbol, Assembler::elf_file.strtab) == 0)Assembler::elf_file.strtab.push_back(symbol);
          }
          else cerr << "Symbol "<< symbol<< " already declared as extern or global" << endl;
            
          
        } 
        return;
      }
    case DIR_EXTERN: {
      while(1){
        ret = lex->yylex();
        if ( ret == NEW_LINE) break;
        if ( lex->YYText() == "," || ret == ESCAPE_SPACES) continue;
        
        string symbol = lex->YYText();
        if ( stringExistsInVector(symbol, Assembler::extern_symbols) == 1 || stringExistsInVector(symbol, Assembler::global_symbols) == 1){
          cerr << " Importing declared or defined symbol " << symbol << " in sample.s" << endl;
          exit(-1);
        }
        if ( findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl) == nullptr){
          if ( stringExistsInVector(symbol, Assembler::elf_file.strtab) == 0) Assembler::elf_file.strtab.push_back(symbol);
          ELF_SYMT_Entry new_symbol = ELF_SYMT_Entry();
          new_symbol.info = SYM_INFO_GLOBAL;
          new_symbol.name = 0;
          new_symbol.shndx = SECTION_NDX_UNDEF;
          new_symbol.type = SYM_TYPE_NOTYPE;
          new_symbol.value = 0;
          new_symbol.symndx = getLastIndexSYMTBL(&Assembler::elf_file.symtbl) + 1;
          new_symbol.sym_name = symbol;
          Assembler::elf_file.symtbl.push_back(new_symbol);
        }
        else{
            cerr << " Multiple declaration of symbol " << symbol << " in sample.s" << endl;
            exit(-1);
        }
        
      }
      return;
    }
    case DIR_SECTION:{

        ret = lex->yylex();    
        while( ret == ESCAPE_SPACES) ret = lex->yylex(); 
        
        string section = lex->YYText();
        Assembler::current_section = section;
        Assembler::location_counter = 0;
        if ( stringExistsInMap(section, Assembler::section_names_ndxs) == 0){
          Assembler::elf_file.strtab.push_back(section);
          Assembler::section_names_ndxs.insert(pair<string,int>(section, getLastIndexSECTBL() + 1));
          ELF_SHT_Entry new_section = ELF_SHT_Entry();
          
          new_section.offset = 0;
          new_section.symtbndx = getLastIndexSYMTBL(&Assembler::elf_file.symtbl) + 1;
          new_section.type = SH_TYPE_CODE;
          new_section.size = 0;
          
          Assembler::elf_file.shtable.push_back(new_section);
          Assembler::elf_file.elfHeader.sh_num_entries++;
          ELF_SYMT_Entry new_symbol = ELF_SYMT_Entry();
          new_symbol.info = (ELF_SYM_Info)(SYM_INFO_LOCAL);
          new_symbol.name = 0;
          new_symbol.shndx = getLastIndexSYMTBL(&Assembler::elf_file.symtbl) + 1;
          new_symbol.type = (ELF_SYM_Type)(SYM_TYPE_SECTION);
          new_symbol.value = 0;
          new_symbol.symndx = getLastIndexSYMTBL(&Assembler::elf_file.symtbl) + 1;
          new_symbol.sym_name = section;
          Assembler::elf_file.symtbl.push_back(new_symbol);

          Assembler::elf_file.sections_binary.insert(pair<string, vector<Byte>>(section, vector<Byte>()));
        }
        else {
            cerr << "Multiple declaration of section " << section << " in sample.s" << endl;
            exit(-1);
        }
      return;
    }
    case DIR_WORD:{
        
        while(1){
        ret = lex->yylex();
        if ( ret == NEW_LINE) break;
        if ( lex->YYText() == "," || ret == ESCAPE_SPACES) continue;
        string text = lex->YYText();
        if ( Assembler::current_section != ""){
          if ( ret == LITERAL){
            Word value = getLiteralValue(text);
            Assembler::writeToSectionBinary(value);
          }
          else if ( ret == SYMBOL){
            string symbol = lex->YYText();
            if ( stringExistsInVector(symbol, Assembler::extern_symbols) == 1 ){
              ELF_RELT_Entry rel_entry = ELF_RELT_Entry();
              rel_entry.offset = Assembler::location_counter;
              rel_entry.symbol = symbol;
              rel_entry.section_index = findSymbolInSymTBL(Assembler::current_section, &Assembler::elf_file.symtbl)->symndx;
              rel_entry.symbol_index = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)->symndx;
              rel_entry.type = REL_TYPE_ABS_16;
              rel_entry.addend = 0;
              Assembler::elf_file.reltbls.push_back(rel_entry);
              Assembler::writeToSectionBinary(0);
            }
            else{
              ELF_SYMT_Entry* entry = nullptr;
              if ( (entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)) != nullptr){
                if ( entry->info == SYM_INFO_GLOBAL){
                  ELF_RELT_Entry rel_entry = ELF_RELT_Entry();
                  rel_entry.offset = Assembler::location_counter;
                  rel_entry.symbol = symbol;
                  rel_entry.sym_info = SYM_INFO_GLOBAL;
                  rel_entry.section_index = findSymbolInSymTBL(Assembler::current_section, &Assembler::elf_file.symtbl)->symndx;
                  rel_entry.symbol_index = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)->symndx;
                  rel_entry.type = REL_TYPE_ABS_16;
                  rel_entry.addend = 0;
                  Assembler::elf_file.reltbls.push_back(rel_entry);
                   Assembler::writeToSectionBinary(0);
                }
                else{
                  ELF_RELT_Entry rel_entry = ELF_RELT_Entry();
                  rel_entry.offset = Assembler::location_counter;
                  rel_entry.symbol = symbol;
                  rel_entry.sym_info = SYM_INFO_LOCAL;
                  rel_entry.section_index = findSymbolInSymTBL(Assembler::current_section, &Assembler::elf_file.symtbl)->symndx;
                  rel_entry.symbol_index = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl)->shndx;
                  rel_entry.type = REL_TYPE_ABS_16;
                  rel_entry.addend = entry->value; 
                  Assembler::elf_file.reltbls.push_back(rel_entry);
                  Assembler::writeToSectionBinary(0);
                }
                
              }
              else {
                if ( stringExistsInVector(symbol, Assembler::global_symbols) == 0) Assembler::elf_file.strtab.push_back(symbol);
                ELF_FLINK_Entry* flink_entry = new ELF_FLINK_Entry();
                flink_entry->addr_type = FLINK_ADDR_TYPE_ABS;
                flink_entry->section = Assembler::current_section;
                flink_entry->section_location = Assembler::location_counter;
                Assembler::writeToSectionBinary(0);
                flink_entry->sym_name = symbol;
                if ( Assembler::flink_head == nullptr) Assembler::flink_head = flink_entry;
                else {
                  ELF_FLINK_Entry* find_last = Assembler::flink_head;
                  while ( find_last->next) find_last = find_last->next;
                  find_last->next = flink_entry; 
                }
               
              }
            }
        }
        else {
          cerr << "Bad usage of word directive" << endl;
          exit(-1);
        }
        
        
      }
      else {
            cerr << "Word directive used outside sections" << endl;
          }
      }
       return;
    }
    case DIR_SKIP:{
        ret = lex->yylex();
        while( ret == ESCAPE_SPACES) ret = lex->yylex(); 
        string text = lex->YYText();
        
        Word value = getLiteralValue(text);
      
        Assembler::skipIntoSectionBinary(value);
        return;
      
    }
    case DIR_ASCII:{
        ret = lex->yylex();
        while( ret == ESCAPE_SPACES) ret = lex->yylex(); 
        string text = lex->YYText();
        Assembler::writeStringToSectionBinary(text);
        return;
    }
    /*case DIR_EQU:{
      while(1){
        ret = lex->yylex();
        if ( ret == NEW_LINE) break;
        if ( lex->YYText() == "," || ret == ESCAPE_SPACES) continue;
        string symbol = lex->YYText();

        if ( stringExistsInVector(symbol, Assembler::elf_file.strtab) == 0){
            Assembler::elf_file.strtab.push_back(symbol);
            ELF_SYMT_Entry new_symbol = ELF_SYMT_Entry();
     
    
            new_symbol.info = SYM_INFO_GLOBAL;
            new_symbol.name = 0;
            new_symbol.shndx = findSectionIndex(Assembler::current_section, Assembler::section_names_ndxs);
            new_symbol.type = SYM_TYPE_NOTYPE;
            new_symbol.value = 0;
            new_symbol.symndx = Assembler::next_index++;
            new_symbol.sym_name = symbol;

            ELF_FLINK_TYPE flink_op_type = FLINK_TYPE_ADD; // postavljamo pocetnu operaciju na sabiranje
            int safety_counter = 0; // mora nesto da stoji iza zareza
            while( ret != NEW_LINE){
            
              ret = lex->yylex();

              if ( ret == ESCAPE_SPACES || lex->YYText() == ",") continue;
              if ( ret == PLUS) flink_op_type = FLINK_TYPE_ADD;
              if ( ret == MINUS) flink_op_type = FLINK_TYPE_SUB;
              if ( ret == SYMBOL){
                safety_counter++;
                string equ_symbol = lex->YYText();
                if ( stringExistsInVector(equ_symbol, Assembler::elf_file.strtab) == 0){
                Assembler::elf_file.strtab.push_back(equ_symbol);
                ELF_SYMT_Entry new_equ_symbol = ELF_SYMT_Entry();
               
               
                
                new_equ_symbol.info = SYM_INFO_LOCAL;
                new_equ_symbol.name = 0;
                new_equ_symbol.shndx = findSectionIndex(Assembler::current_section, Assembler::section_names_ndxs);
                new_equ_symbol.type = SYM_TYPE_NOTYPE;
                new_equ_symbol.value = 0;
                new_equ_symbol.symndx = Assembler::next_index++;
                new_equ_symbol.sym_name = equ_symbol;
                Assembler::elf_file.symtbl.push_back(new_equ_symbol);
                }
                else if ( stringExistsInVector(equ_symbol, Assembler::elf_file.strtab) == 1 && 
                      symbolDefinedInTable(equ_symbol, Assembler::elf_file.symtbl) == 0){
                  ELF_SYMT_Entry* entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl);
                  if ( entry->flink == nullptr){
                    ELF_FLINK_Entry* new_flink = new ELF_FLINK_Entry();
                    new_flink->flink_to_sym = YES;
                    new_flink->op_type = flink_op_type;
                    new_flink->sym_name = symbol;
                    new_flink->section = Assembler::current_section;
                    new_flink->section_location = Assembler::location_counter;
                    new_flink->next = nullptr;
                    entry->flink = new_flink;
                  }
                  else{
                    ELF_FLINK_Entry* new_flink = new ELF_FLINK_Entry();
                    new_flink->flink_to_sym = YES;
                    new_flink->op_type = flink_op_type;
                    new_flink->sym_name = symbol;
                    new_flink->section = Assembler::current_section;
                    new_flink->section_location = Assembler::location_counter;
                    new_flink->next = nullptr;
                    ELF_FLINK_Entry* find_last = entry->flink;
                    while(find_last->next){
                      find_last = find_last->next;
                    }
                    find_last->next = new_flink;
                  }
              
                }
                else if ( stringExistsInVector(equ_symbol, Assembler::elf_file.strtab) == 1 && 
                  symbolDefinedInTable(equ_symbol, Assembler::elf_file.symtbl) == 1){
                    ELF_SYMT_Entry* entry = findSymbolInSymTBL(equ_symbol, &Assembler::elf_file.symtbl);
                    if ( flink_op_type == FLINK_TYPE_ADD) new_symbol.value += entry->value;
                    if ( flink_op_type == FLINK_TYPE_SUB) new_symbol.value -= entry->value;
                }
              }
              if ( ret == LITERAL){
                safety_counter++;
                Word value = getLiteralValue(lex->YYText());
                if ( flink_op_type == FLINK_TYPE_ADD) new_symbol.value += value;
                if ( flink_op_type == FLINK_TYPE_SUB) new_symbol.value -= value;
              }
            }
            Assembler::elf_file.symtbl.push_back(new_symbol);
            return;
          }
          else if ( stringExistsInVector(symbol, Assembler::elf_file.strtab) == 1 && 
            symbolDefinedInTable(symbol, Assembler::elf_file.symtbl) == 0){

            ELF_SYMT_Entry* sym_entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl);

            ELF_FLINK_TYPE flink_op_type = FLINK_TYPE_ADD; // postavljamo pocetnu operaciju na sabiranje
            int safety_counter = 0; // mora nesto da stoji iza zareza
            while( ret != NEW_LINE){
            
              ret = lex->yylex();

              if ( ret == ESCAPE_SPACES || lex->YYText() == ",") continue;
              if ( ret == PLUS) flink_op_type = FLINK_TYPE_ADD;
              if ( ret == MINUS) flink_op_type = FLINK_TYPE_SUB;
              if ( ret == SYMBOL){
                safety_counter++;
                string equ_symbol = lex->YYText();
                if ( stringExistsInVector(equ_symbol, Assembler::elf_file.strtab) == 0){
                Assembler::elf_file.strtab.push_back(equ_symbol);
                ELF_SYMT_Entry new_equ_symbol = ELF_SYMT_Entry();
            
                new_equ_symbol.flink = new ELF_FLINK_Entry();

                new_equ_symbol.flink->flink_to_sym = YES;
                new_equ_symbol.flink->next = nullptr;
                new_equ_symbol.flink->op_type = flink_op_type;
                new_equ_symbol.flink->section = Assembler::current_section; // nebitno
                new_equ_symbol.flink->section_location = Assembler::location_counter; // nebitno
                new_equ_symbol.flink->sym_name = symbol;
                
                new_equ_symbol.info = SYM_INFO_LOCAL;
                new_equ_symbol.name = 0;
                new_equ_symbol.shndx = findSectionIndex(Assembler::current_section, Assembler::section_names_ndxs);
                new_equ_symbol.type = SYM_TYPE_NOTYPE;
                new_equ_symbol.value = 0;
                new_equ_symbol.symndx = Assembler::next_index++;
                new_equ_symbol.sym_name = equ_symbol;
                Assembler::elf_file.symtbl.push_back(new_equ_symbol);
                }
                else if ( stringExistsInVector(equ_symbol, Assembler::elf_file.strtab) == 1 && 
                      symbolDefinedInTable(equ_symbol, Assembler::elf_file.symtbl) == 0){
                  ELF_SYMT_Entry* entry = findSymbolInSymTBL(symbol, &Assembler::elf_file.symtbl);
                  if ( entry->flink == nullptr){
                    ELF_FLINK_Entry* new_flink = new ELF_FLINK_Entry();
                    new_flink->flink_to_sym = YES;
                    new_flink->op_type = flink_op_type;
                    new_flink->sym_name = symbol;
                    new_flink->section = Assembler::current_section;
                    new_flink->section_location = Assembler::location_counter;
                    new_flink->next = nullptr;
                    entry->flink = new_flink;
                  }
                  else{
                    ELF_FLINK_Entry* new_flink = new ELF_FLINK_Entry();
                    new_flink->flink_to_sym = YES;
                    new_flink->op_type = flink_op_type;
                    new_flink->sym_name = symbol;
                    new_flink->section = Assembler::current_section;
                    new_flink->section_location = Assembler::location_counter;
                    new_flink->next = nullptr;
                    ELF_FLINK_Entry* find_last = entry->flink;
                    while(find_last->next){
                      find_last = find_last->next;
                    }
                    find_last->next = new_flink;
                  }
              
                }
                else if ( stringExistsInVector(equ_symbol, Assembler::elf_file.strtab) == 1 && 
                  symbolDefinedInTable(equ_symbol, Assembler::elf_file.symtbl) == 1){
                    ELF_SYMT_Entry* entry = findSymbolInSymTBL(equ_symbol, &Assembler::elf_file.symtbl);
                    if ( flink_op_type == FLINK_TYPE_ADD) sym_entry->value += entry->value;
                    if ( flink_op_type == FLINK_TYPE_SUB) sym_entry->value -= entry->value;
                }
              }
              if ( ret == LITERAL){
                safety_counter++;
                Word value = getLiteralValue(lex->YYText());
                if ( flink_op_type == FLINK_TYPE_ADD) sym_entry->value += value;
                if ( flink_op_type == FLINK_TYPE_SUB) sym_entry->value -= value;
              }
            }
            return;
          }
          else if ( stringExistsInVector(symbol, Assembler::elf_file.strtab) == 1 && 
            symbolDefinedInTable(symbol, Assembler::elf_file.symtbl) == 1){
            cerr << "Multiple definition of symbol " << symbol << " in sample.s" << endl;
            exit(-1);
          }
      
      }
      return;
    }*/
    case DIR_END:{
      break;
    
    }
  }
}
void handleLabels( string label, void*l){

  yyFlexLexer* lex = (yyFlexLexer*)l;
  label.pop_back();
  ELF_SYMT_Entry* sym_entry = nullptr;
  if ( (sym_entry = findSymbolInSymTBL(label, &Assembler::elf_file.symtbl)) == nullptr){
    if ( stringExistsInVector(label, Assembler::global_symbols) == 1){
      if ( stringExistsInVector(label, Assembler::elf_file.strtab) == 0)Assembler::elf_file.strtab.push_back(label);
      ELF_SYMT_Entry new_symbol = ELF_SYMT_Entry();
      new_symbol.info = SYM_INFO_GLOBAL;
      new_symbol.name = 0;
      new_symbol.shndx = findSymbolInSymTBL(Assembler::current_section, &Assembler::elf_file.symtbl)->symndx;
      new_symbol.type = SYM_TYPE_NOTYPE;
      new_symbol.value = Assembler::location_counter;
      new_symbol.symndx = getLastIndexSYMTBL(&Assembler::elf_file.symtbl) + 1;
      new_symbol.sym_name = label;
      Assembler::elf_file.symtbl.push_back(new_symbol);
    }
    else if ( stringExistsInVector(label, Assembler::extern_symbols) == 1){
      cerr << "Imported symbol " << label << " defined in this file." << endl;
    }
    else {
      if ( stringExistsInVector(label,Assembler::elf_file.strtab) == 0) Assembler::elf_file.strtab.push_back(label);
      ELF_SYMT_Entry new_symbol = ELF_SYMT_Entry();
      new_symbol.info = SYM_INFO_LOCAL;
      new_symbol.name = 0;
      new_symbol.shndx = findSymbolInSymTBL(Assembler::current_section, &Assembler::elf_file.symtbl)->symndx;
      new_symbol.type = SYM_TYPE_NOTYPE;
      new_symbol.value = Assembler::location_counter;
      new_symbol.symndx = getLastIndexSYMTBL(&Assembler::elf_file.symtbl) + 1;
      new_symbol.sym_name = label;
      Assembler::elf_file.symtbl.push_back(new_symbol);
    }
      
  }
  else {
    cerr << "Multiple definition of symbol " << label << " in sample.s" << endl;
    exit(-1);
  }
      

}

