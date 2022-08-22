#include "assembler.h"
#include "utility.h"
#include <iostream>
#include <iomanip>


int stringExistsInVector(string str, vector<string> vec){

    for ( int i = 0; i < vec.size(); i++){
        if ( str == vec[i]) return 1;
    }

    return 0;

}

int stringExistsInMap(string str, map<string,int> sec_map){

    map<string,int>::iterator iter;

    for ( iter = sec_map.begin(); iter!= sec_map.end(); iter++ ){
        if ( str == iter->first) return 1;
    }   

    return 0;

}

ELF_SYMT_Entry* findSymbolInSymTBL(string label, vector<ELF_SYMT_Entry>* symtbl){

    vector<ELF_SYMT_Entry>::iterator it;
    for ( it = symtbl->begin(); it!= symtbl->end(); it++){
        if ( it->sym_name == label) return &(*it);
    }
    return nullptr;
}

ELF_SYMT_Entry* findSymbolInSymTBLIndex( Word index, vector<ELF_SYMT_Entry>* symtbl){
    
    vector<ELF_SYMT_Entry>::iterator it;
    for ( it = symtbl->begin(); it!= symtbl->end(); it++){
        if ( it->symndx == index) return &(*it);
    }
    return nullptr;
}

Word getLiteralValue(string literal){
    if (literal[0] == '0') {
        if (literal.length() == 1) {
            return 0;
        } else if (literal[1] == 'x' || literal[1] == 'X') {
            return stoi(literal.substr(2), nullptr, 16);
        } else if (literal[1] == 'b' || literal[1] == 'B') {
            return stoi(literal.substr(2), nullptr, 2);
        } else {
            return stoi(literal.substr(2), nullptr, 8);
        }
    } else {
        return stoi(literal);
    }
}

Word getLastIndexSYMTBL(vector<ELF_SYMT_Entry>* symtbl){
    return symtbl->size()-1;
}

Word getLastIndexSECTBL(){
    return Assembler::section_names_ndxs.size() - 1;
}

Word wordExistsInVector(Word num, vector<Word> vec){


    for ( int i = 0; i < vec.size(); i++){
        if ( num == vec[i]) return 1;
    }

    return 0;

}

void createTXTFileUT(ofstream& file, ELF16_File elf_file){

    file << std::setfill(' ') << std::setbase(10);
    string little_space = "   ";
    string bigger_space = "       ";
    int ls = 3;
    int bs = 7;
    file << endl;
    file << "--------------------------------- Symbol Table ---------------------------------" << endl;
    file << little_space<< "Num" << bigger_space<<"Value" << bigger_space << "Size" << bigger_space << "Type";
    file << bigger_space << "Bind" << bigger_space << "Index" << bigger_space  << bigger_space << "Name" << little_space << endl;
  
    for ( int i = 0 ; i < elf_file.symtbl.size(); i++){
      ELF_SYMT_Entry sym_entry = elf_file.symtbl[i];
      file << std::setw(3 + ls) <<sym_entry.symndx << std::setw(5 + bs) << std::hex << sym_entry.value;
      file << std::setw(4 + bs) << std::hex << getSizeU(sym_entry.symndx, elf_file) << std::setw(4 + bs) << getTypeU(sym_entry.type);
      file << std::setw(4 + bs) << getInfoU(sym_entry.info) << std::setw(5 + bs) << std::dec;
      
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
   
  
    for ( int i = 0; i < elf_file.reltbls.size(); i++){
        string section;
        if ( stringExistsInVector(section = findSymbolInSymTBLIndex(elf_file.reltbls[i].section_index, &elf_file.symtbl)->sym_name, searched_sections) == 0){
    
        searched_sections.push_back(section);
        Word sec = elf_file.reltbls[i].section_index;
        file << endl;
        file << "# rela " << findSymbolInSymTBLIndex(sec, &elf_file.symtbl)->sym_name << endl;
        
        file << little_space << "Offset" << bigger_space << bigger_space << "Type" << bigger_space << bigger_space << "Symbol" << bigger_space << "Addend" << endl;
        
        for ( int j = i; j < elf_file.reltbls.size(); j++){
            
            ELF_RELT_Entry rel_entry = elf_file.reltbls[j];
            if ( sec == rel_entry.section_index){
            file << std::setw(6 + ls) << std::hex << rel_entry.offset << std::setw(4 + bs*2) << getRelTypeU(rel_entry.type);
            file << std::setw(6 + bs*2) <<  getSymbolForRelaU(rel_entry, elf_file)<<std::dec << std::setw(6 + bs) << rel_entry.addend << endl;
            }
        }

        }
        
    }
    
    
    file << endl;
    file << "----------------------- Binary Section Content --------------------------" << endl;
    
    for ( map<string,vector<Byte>>::iterator it = elf_file.sections_binary.begin(); it != elf_file.sections_binary.end(); it++){
      file << "# Content of section " << it->first << endl;
      for ( int i = 0; i < it->second.size(); i++){
        if ( i > 0 && i % 4 == 0) file << " ";
        if ( i > 0 && i % 8 == 0) file << endl;
        
        file << std::setbase(16) << std::setfill('0') << std::setw(2) << (Word)it->second[i] << " ";
        
      }
      file << endl << endl;
      
    }
    
}

string getTypeU(ELF_SYM_Type type){
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

string getInfoU(ELF_SYM_Info info){
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

string getRelTypeU(ELF_REL_Type type){
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

Word getSizeU(Word index, ELF16_File elf_file){

  for ( int i = 0 ; i < elf_file.shtable.size(); i++){
    if ( index == elf_file.shtable[i].symtbndx) return elf_file.shtable[i].size;
  }
  return 0;

}

int findNumOfRelaSectionsU(ELF16_File elf_file){
  vector<string> rela_sections = vector<string>();
  for ( int i = 0; i < elf_file.reltbls.size(); i++){
    if ( stringExistsInVector(findSymbolInSymTBLIndex(elf_file.reltbls[i].section_index, &elf_file.symtbl)->sym_name, rela_sections) == 0){
     
      rela_sections.push_back(findSymbolInSymTBLIndex(elf_file.reltbls[i].section_index, &elf_file.symtbl)->sym_name);
    }
  }
  return rela_sections.size();
}

string getSymbolForRelaU(ELF_RELT_Entry entry, ELF16_File elf_file){
  
  return entry.symbol;
}

Word findOffsetInString(string sym , vector<string> table){

  for ( int i = 0; i < table.size() ;i++){
    if ( sym == table[i]) return i;
  }

  return -1;

}

ELF_SYMT_Entry* findSectionInSymTBL(string section, vector<ELF_SYMT_Entry>* symtbl){

  vector<ELF_SYMT_Entry>::iterator it;
  for ( it = symtbl->begin(); it!= symtbl->end(); it++){
      if ( it->sym_name == section && it->type == SYM_TYPE_SECTION) return &(*it);
  }
  return nullptr;

}

void createBinaryFile(FILE* file, ELF16_File elf_file){
  elf_file.elfHeader.shoff = sizeof(ELF_Header);
  unsigned long next_section = elf_file.elfHeader.sh_num_entries * sizeof(ELF_SHT_Entry) + sizeof(ELF_Header);
  unsigned long next_header = sizeof(ELF_Header);
  
  elf_file.elfHeader.symtblndx = 0;
  elf_file.elfHeader.strndx = 1;
  elf_file.elfHeader.relndx = 2;

  fwrite(&elf_file.elfHeader, sizeof(ELF_Header), 1, file);

  //ubacivanje strtabel header
  for ( int i = 0; i < elf_file.shtable.size(); i++){
    
    if ( elf_file.shtable[i].type == SH_TYPE_SYMTBL) {

      elf_file.shtable[i].offset = next_section;
      Word size = 0;
      for ( int j = 0; j < elf_file.symtbl.size(); j++){
        size += sizeof(ELF_SYMT_Entry);
      }
      elf_file.shtable[i].size = size;

      fwrite(&elf_file.shtable[i], sizeof(ELF_SHT_Entry), 1, file);

      // ubacivanje sekcije symtbl

      fseek(file, next_section, SEEK_SET);
      for ( int i = 0; i < elf_file.symtbl.size(); i++){
        fwrite(&elf_file.symtbl[i].info, sizeof(ELF_SYM_Info), 1, file );
        fwrite(&elf_file.symtbl[i].name, sizeof(Word), 1, file );
        fwrite(&elf_file.symtbl[i].shndx, sizeof(Word), 1, file );
        for ( int j = 0; j < elf_file.symtbl[i].sym_name.size(); j++){
          fwrite(&elf_file.symtbl[i].sym_name[j], sizeof(char), 1, file );
        }
        fwrite("\0", sizeof(char),1 , file);

        fwrite(&elf_file.symtbl[i].symndx, sizeof(Word), 1, file );
        fwrite(&elf_file.symtbl[i].type, sizeof(ELF_SYM_Type), 1, file );
        fwrite(&elf_file.symtbl[i].value, sizeof(Word), 1, file );
      }
      next_section += size;
    }
  }
  
  next_header+= sizeof(ELF_SHT_Entry);

   // ubacivanje symtabel headera

  fseek(file, next_header, SEEK_SET);
  
  for ( int i = 0; i < elf_file.shtable.size(); i++){
    if ( elf_file.shtable[i].type == SH_TYPE_STRTBL) {
      elf_file.shtable[i].offset = next_section;
      Word size = 0;
      for ( int j = 0; j < elf_file.strtab.size(); j++){
        size += elf_file.strtab[j].size() + 1;
      }
      elf_file.shtable[i].size = size;
      fwrite(&elf_file.shtable[i], sizeof(ELF_SHT_Entry), 1, file);
       // ubacivanje sekcije strtab
      fseek(file, next_section, SEEK_SET);
      for ( int i = 0; i < elf_file.strtab.size(); i++){
        for ( int j = 0; j < elf_file.strtab[i].size(); j++){
          fwrite(&elf_file.strtab[i][j], sizeof(char), 1, file );
          
        }
        fwrite("\0", sizeof(char),1 , file);
        
      }
      next_section += size;
    }
  }

  next_header+= sizeof(ELF_SHT_Entry);
   // ubacivanje rela headera
  fseek(file, next_header, SEEK_SET);

  for ( int i = 0; i < elf_file.shtable.size(); i++){
    if ( elf_file.shtable[i].type == SH_TYPE_RELA) {
      elf_file.shtable[i].offset = next_section;
      Word size = 0;
      for ( int j = 0; j < elf_file.reltbls.size(); j++){
        size += sizeof(ELF_RELT_Entry);
      }
      elf_file.shtable[i].size = size;
      fwrite(&elf_file.shtable[i], sizeof(ELF_SHT_Entry), 1, file);

      // ubacivanje sekcije rela

      fseek(file, next_section, SEEK_SET);
      for ( int i = 0; i < elf_file.reltbls.size(); i++){
        fwrite(&elf_file.reltbls[i].addend, sizeof(Offs), 1, file);
        fwrite(&elf_file.reltbls[i].offset, sizeof(Offs), 1, file);
        fwrite(&elf_file.reltbls[i].section_index, sizeof(Word), 1, file);
        fwrite(&elf_file.reltbls[i].sym_info, sizeof(ELF_SYM_Info), 1, file);
        for ( int j = 0; j < elf_file.reltbls[i].symbol.size(); j++){
          fwrite(&elf_file.reltbls[i].symbol[j], sizeof(char), 1, file );
        }
        fwrite("\0", sizeof(char),1 , file);
        fwrite(&elf_file.reltbls[i].symbol_index, sizeof(Word), 1, file);
        fwrite(&elf_file.reltbls[i].type, sizeof(ELF_REL_Type), 1, file);
      
      }
      next_section += size;
    }
  }
  next_header+= sizeof(ELF_SHT_Entry);
   // ubacivanje headera


  for ( int i = 0; i < elf_file.shtable.size();i++){
    if ( elf_file.shtable[i].type == SH_TYPE_CODE && elf_file.shtable[i].symtbndx > 0) {
      fseek(file, next_header, SEEK_SET);

      elf_file.shtable[i].offset = next_section;
      
      //size je vec sredjen
      fwrite(&elf_file.shtable[i], sizeof(ELF_SHT_Entry), 1, file);
      next_header+= sizeof(ELF_SHT_Entry);
     
      // ubacivanje sekcije 
      fseek(file, next_section, SEEK_SET);

   
      map<string,vector<Byte>>::iterator it;
      
      string sec_name;
      for ( it = elf_file.sections_binary.begin(); it != elf_file.sections_binary.end(); it++){
        if ( it->first == findSymbolInSymTBLIndex(elf_file.shtable[i].symtbndx, &elf_file.symtbl)->sym_name){
         
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
      next_section += elf_file.shtable[i].size + sec_name.size() + 1;
    }
    
  }
  
}




