#include "linker.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include "utility.h"

using namespace std;

vector<pair<string,ELF16_File>> Linker::elf_files = vector<pair<string,ELF16_File>>();
map<string, Word> Linker::sections_place = map<string,Word>();

void Linker::linkFiles(){

  ELF16_File* base_elf_file = &(Linker::elf_files.begin()->second);
  
  vector<pair<string,ELF16_File>>::iterator it = Linker::elf_files.begin();
  it++;
  // Sections
  for ( ; it != Linker::elf_files.end(); it++){ // za sve elf_datoteke

    for ( vector<ELF_SYMT_Entry>::iterator sym_entry = it->second.symtbl.begin(); sym_entry != it->second.symtbl.end(); sym_entry++){ // za svaki simbol u tabeli simbola
     
      if ( sym_entry->type == SYM_TYPE_SECTION && findSectionInSymTBL(sym_entry->sym_name, &base_elf_file->symtbl) == nullptr){
        
        base_elf_file->sections_binary.insert(pair<string,vector<Byte>>(sym_entry->sym_name, vector<Byte>()));
        vector<Byte>* write_vector = &base_elf_file->sections_binary.find(sym_entry->sym_name)->second;
        vector<Byte>* read_vector = &it->second.sections_binary.find(sym_entry->sym_name)->second;
        
        for ( int i = 0; i < read_vector->size();i++){
          write_vector->push_back((*read_vector)[i]);
        }

  
        base_elf_file->strtab.push_back(sym_entry->sym_name);
        
        ELF_SHT_Entry new_section = ELF_SHT_Entry();
        
        new_section.offset = 0;
        new_section.symtbndx = getLastIndexSYMTBL(&base_elf_file->symtbl) + 1;
        new_section.type = SH_TYPE_CODE;
        new_section.size = 0;
        for ( int i = 0; i < it->second.shtable.size(); i++){
          if ( sym_entry->symndx == it->second.shtable[i].symtbndx) new_section.size = it->second.shtable[i].size;
        }

        Word section_index = new_section.symtbndx;
        
        base_elf_file->shtable.push_back(new_section);
        base_elf_file->elfHeader.sh_num_entries++;
        
        ELF_SYMT_Entry new_symbol = ELF_SYMT_Entry();
        new_symbol.info = (ELF_SYM_Info)(SYM_INFO_LOCAL);
        new_symbol.name = 0;
        new_symbol.shndx = getLastIndexSYMTBL(&base_elf_file->symtbl)  + 1;
        new_symbol.type = (ELF_SYM_Type)(SYM_TYPE_SECTION);
        new_symbol.value = 0;
        new_symbol.symndx = getLastIndexSYMTBL(&base_elf_file->symtbl)  + 1;
        new_symbol.sym_name = sym_entry->sym_name;
        base_elf_file->symtbl.push_back(new_symbol);

        for ( vector<ELF_SYMT_Entry>::iterator section_symbols_search = it->second.symtbl.begin(); section_symbols_search != it->second.symtbl.end(); section_symbols_search++){
          if ( section_symbols_search->shndx == sym_entry->symndx && section_symbols_search->sym_name != sym_entry->sym_name){
            ELF_SYMT_Entry* existing_symbol;
            if ( (existing_symbol = findSymbolInSymTBL( section_symbols_search->sym_name, &base_elf_file->symtbl)) != nullptr ){
              existing_symbol->shndx = section_index;
              existing_symbol->value = section_symbols_search->value;
            }
            else{
              ELF_SYMT_Entry new_symbol = *section_symbols_search;
              new_symbol.shndx = section_index;
              new_symbol.symndx = getLastIndexSYMTBL(&base_elf_file->symtbl) + 1;
              base_elf_file->symtbl.push_back(new_symbol);
             
              base_elf_file->strtab.push_back(new_symbol.sym_name);
            }
            
          }
        }

        for ( vector<ELF_RELT_Entry>::iterator section_rels = it->second.reltbls.begin(); section_rels != it->second.reltbls.end(); section_rels++){
          if ( section_rels->section_index == sym_entry->symndx){
            ELF_RELT_Entry new_rel = *section_rels;
            new_rel.symbol_index = -1;
            new_rel.section_index = section_index; // index nove sekcije
            base_elf_file->reltbls.push_back(new_rel);
          }
        }

      }
      else if ( sym_entry->type == SYM_TYPE_SECTION && findSectionInSymTBL(sym_entry->sym_name, &base_elf_file->symtbl) != nullptr){
        
        ELF_SYMT_Entry* existing_section = findSectionInSymTBL(sym_entry->sym_name, &base_elf_file->symtbl);

        for ( int sh_entry = 0; sh_entry < base_elf_file->shtable.size(); sh_entry++){
          if ( base_elf_file->shtable[sh_entry].symtbndx == existing_section->symndx){
            for ( int i = 0; i < it->second.shtable.size(); i++){
              if ( sym_entry->symndx == it->second.shtable[i].symtbndx) base_elf_file->shtable[sh_entry].size += it->second.shtable[i].size;
            }
          }
        }
        


        vector<Byte>* write_vector = &base_elf_file->sections_binary.find(sym_entry->sym_name)->second;
        vector<Byte>* read_vector = &it->second.sections_binary.find(sym_entry->sym_name)->second;
        
        Word sec_offset = write_vector->size();

        for ( int i = 0; i < read_vector->size();i++){
          write_vector->push_back((*read_vector)[i]);
        }

         for ( vector<ELF_SYMT_Entry>::iterator section_symbols_search = it->second.symtbl.begin(); section_symbols_search != it->second.symtbl.end(); section_symbols_search++){
          if ( section_symbols_search->shndx == sym_entry->symndx && section_symbols_search->sym_name != sym_entry->sym_name){
            ELF_SYMT_Entry* existing_symbol;
            if ( (existing_symbol = findSymbolInSymTBL( section_symbols_search->sym_name, &base_elf_file->symtbl)) != nullptr ){
              existing_symbol->shndx = existing_section->symndx;
              existing_symbol->value = section_symbols_search->value + sec_offset;
            }
            else{
              ELF_SYMT_Entry new_symbol = *section_symbols_search;
              new_symbol.shndx = existing_section->symndx;
              new_symbol.symndx = getLastIndexSYMTBL(&base_elf_file->symtbl) + 1;
              new_symbol.value += sec_offset;
              base_elf_file->symtbl.push_back(new_symbol);
              
              base_elf_file->strtab.push_back(new_symbol.sym_name);
            }
            
          }
        }

        for ( vector<ELF_RELT_Entry>::iterator section_rels = it->second.reltbls.begin(); section_rels != it->second.reltbls.end(); section_rels++){
          if ( section_rels->section_index == sym_entry->symndx){
            ELF_RELT_Entry new_rel = *section_rels;
            if ( new_rel.sym_info == SYM_INFO_LOCAL) new_rel.addend += sec_offset;
            new_rel.offset += sec_offset;
            new_rel.symbol_index = -1;
            new_rel.section_index = existing_section->symndx; // indeks postojece sekcije
            base_elf_file->reltbls.push_back(new_rel);
          }
        }

      }
      else if ( sym_entry->info == SYM_INFO_GLOBAL && sym_entry->shndx == 0){ // extern symbol iz desnog elf_fajla
        if ( findSymbolInSymTBL(sym_entry->sym_name, &base_elf_file->symtbl) == nullptr){
          base_elf_file->symtbl.push_back(*sym_entry);
        }
      }
    }
  }

  for ( vector<ELF_RELT_Entry>::iterator rel_entry = base_elf_file->reltbls.begin(); rel_entry != base_elf_file->reltbls.end(); rel_entry++){
    if ( rel_entry->symbol_index == 65535){ // zato sto je to -1 za unsigned
      rel_entry->symbol_index = findSymbolInSymTBL(rel_entry->symbol, &base_elf_file->symtbl)->symndx;
    }
  }

  for ( vector<ELF_SHT_Entry>::iterator sh_entry = base_elf_file->shtable.begin(); sh_entry != base_elf_file->shtable.end(); sh_entry++){
    if ( sh_entry->symtbndx > 0 && sh_entry->symtbndx != 65535 && stringExistsInMap(findSymbolInSymTBLIndex(sh_entry->symtbndx, &base_elf_file->symtbl)->sym_name, Linker::sections_place) == 0){
      
      Linker::sections_place.insert(pair<string, Word>(findSymbolInSymTBLIndex(sh_entry->symtbndx, &base_elf_file->symtbl)->sym_name, -1));
    }
  }

}

void Linker::loadFiles(vector<string> files){
 
  for ( int i = 0 ; i < files.size(); i++){
    ELF16_File elf_file = ELF16_File();
     
    FILE* fin = fopen(files[i].c_str() , "r");
    if ( fin == nullptr) {
      cerr << "Error opening the file" << endl;
      exit(-1);
    }
    ELF_Header header; // HEADER
    //loading header
    fread(&header, sizeof(ELF_Header), 1, fin);

    
    vector<ELF_SHT_Entry> sh_table = vector<ELF_SHT_Entry>(); // SH_TABLE
   

    //loading sh_table
    for ( int i = 0; i < header.sh_num_entries; i++){
      ELF_SHT_Entry sh_entry;
      fread(&sh_entry, sizeof(ELF_SHT_Entry), 1, fin);
      sh_table.push_back(sh_entry);
    }

      
    // loading string_table
    fseek(fin, sh_table[header.strndx].offset, SEEK_SET);
    vector<string> strtab;
    string dummy;
    for ( int i = 0; i < sh_table[1].size; i++){
      char c;
      fread(&c, sizeof(char), 1, fin);
      dummy.push_back(c);
      if ( c == '\0'){
        strtab.push_back(dummy);
        dummy.clear();
      }
    }
  
    // loading symtbl
    fseek(fin, sh_table[header.symtblndx].offset, SEEK_SET);
    vector<ELF_SYMT_Entry> symtbl = vector<ELF_SYMT_Entry>();
    int sym_size = sh_table[0].size / sizeof(ELF_SYMT_Entry); 
    for ( int i = 0; i < sym_size; i++){
      ELF_SYMT_Entry sym_entry;
      fread(&sym_entry.info, sizeof(ELF_SYM_Info), 1, fin );
      fread(&sym_entry.name, sizeof(Word), 1, fin );
      fread(&sym_entry.shndx, sizeof(Word), 1, fin );
      string sym_name;
      char c = 'A';
      while ( c != '\0'){
       fread(&c, sizeof(char), 1, fin );
       sym_name.push_back(c);
      }
      sym_entry.sym_name = sym_name;
      fread(&sym_entry.symndx, sizeof(Word), 1, fin );
      fread(&sym_entry.type, sizeof(ELF_SYM_Type), 1, fin );
      fread(&sym_entry.value, sizeof(Word), 1, fin );
      symtbl.push_back(sym_entry);
    }
  
    //loading rel_tbl
    fseek(fin, sh_table[header.relndx].offset, SEEK_SET);
    vector<ELF_RELT_Entry> reltbl = vector<ELF_RELT_Entry>();
    int rel_size = sh_table[2].size / sizeof(ELF_RELT_Entry); 
    for ( int i = 0; i < rel_size; i++){
      ELF_RELT_Entry rel_entry;
      fread(&rel_entry.addend, sizeof(Offs), 1, fin);
      fread(&rel_entry.offset, sizeof(Offs), 1, fin);
      fread(&rel_entry.section_index, sizeof(Word), 1, fin);
      fread(&rel_entry.sym_info, sizeof(ELF_SYM_Info), 1, fin);
      string sym_name;
      char c = 'A';
      while ( c != '\0'){
       fread(&c, sizeof(char), 1, fin );
       sym_name.push_back(c);
      }
      rel_entry.symbol = sym_name;
      fread(&rel_entry.symbol_index, sizeof(Word), 1, fin);
      fread(&rel_entry.type, sizeof(ELF_REL_Type), 1, fin);
      reltbl.push_back(rel_entry);
      
    }
    
    //loading sections_binary
    map<string,vector<Byte>> sections_binary = map<string, vector<Byte>>();
    for ( int i = 3; i < sh_table.size(); i++){
      
      fseek(fin, sh_table[i].offset, SEEK_SET);
      string sec_name;
      char c = 'A';
      while ( c != '\0'){
      
       fread(&c, sizeof(char), 1, fin );
       sec_name.push_back(c);
       
      }

      sec_name.pop_back();
      sections_binary.insert(pair<string,vector<Byte>>(sec_name, vector<Byte>()));
      
      for ( int j = 0 ; j < sh_table[i].size; j++){
        
        Byte byte;
        fread(&byte, sizeof(Byte), 1, fin);
        sections_binary.find(sec_name)->second.push_back(byte) ;
      }
    }
    
   
    // load elf_file
    elf_file.elfHeader = header;
    elf_file.reltbls = reltbl;
    elf_file.sections_binary = sections_binary;
    elf_file.shtable = sh_table;
    elf_file.strtab = strtab;
    elf_file.symtbl = symtbl;

    Linker::fixStrings(&elf_file);
   
    Linker::elf_files.push_back(pair<string, ELF16_File>(files[i], elf_file));
    fclose(fin);

   

  }

  

}

void Linker::fixStrings(ELF16_File* elf_file){

  for ( int i = 0; i < elf_file->strtab.size(); i++){
    elf_file->strtab[i].pop_back();
  }

  for ( int i = 0; i < elf_file->reltbls.size(); i++){
    elf_file->reltbls[i].symbol.pop_back();
  }

  for ( int i = 0; i < elf_file->symtbl.size(); i++){
    elf_file->symtbl[i].sym_name.pop_back();
    elf_file->symtbl[i].name = findOffsetInString(elf_file->symtbl[i].sym_name, elf_file->strtab);
  }


}

void Linker::checkErrors(){
  vector<string> extern_symbols;
  vector<string> global_symbols;

  for ( vector<pair<string,ELF16_File>>::iterator it= Linker::elf_files.begin(); it != Linker::elf_files.end(); it++){
    for ( int j = 0; j < it->second.symtbl.size(); j++){
      if ( it->second.symtbl[j].info == SYM_INFO_GLOBAL && it->second.symtbl[j].type == SYM_TYPE_NOTYPE && it->second.symtbl[j].shndx != 0){
        if ( stringExistsInVector(it->second.symtbl[j].sym_name, global_symbols) == 0) global_symbols.push_back(it->second.symtbl[j].sym_name);
        else {
          cerr << "Multiple export of symbol " << it->second.symtbl[j].sym_name << endl;
          exit(-1);
        }
      }
      if ( it->second.symtbl[j].info == SYM_INFO_GLOBAL && it->second.symtbl[j].type == SYM_TYPE_NOTYPE && it->second.symtbl[j].shndx == 0){
        extern_symbols.push_back(it->second.symtbl[j].sym_name);
      }
    }
  }

  for ( int i = 0; i < extern_symbols.size(); i++){
    if ( stringExistsInVector(extern_symbols[i], global_symbols) == 0){
      cerr << "Imported symbol " << extern_symbols[i] << " cannot be resolved" << endl;
      exit(-1);
    }
  }

  

}

void Linker::relocateSymbols(){

  ELF16_File* base_elf_file = &Linker::elf_files.begin()->second;

  for ( vector<ELF_RELT_Entry>::iterator rel_entry = base_elf_file->reltbls.begin(); rel_entry != base_elf_file->reltbls.end(); rel_entry++){
    vector<Byte>* write_vector = &base_elf_file->sections_binary.find(findSymbolInSymTBLIndex(rel_entry->section_index, &base_elf_file->symtbl)->sym_name)->second;
    Word value;
    if ( rel_entry->type == REL_TYPE_ABS_16){
      value = rel_entry->addend + findSymbolInSymTBL(rel_entry->symbol, &base_elf_file->symtbl)->value;
    }
    else {
      Offs real_offset = findSymbolInSymTBLIndex(rel_entry->section_index, &base_elf_file->symtbl)->value + rel_entry->offset;
      value = rel_entry->addend + findSymbolInSymTBL(rel_entry->symbol, &base_elf_file->symtbl)->value - real_offset;
    }
  
    Byte byte1 = value & 0xff;
    Byte byte2 = (value & 0xff00) >> 8;

    (*write_vector)[rel_entry->offset] = byte1;
    (*write_vector)[rel_entry->offset + 1] = byte2;
  }

}

void Linker::placeSections(){

  ELF16_File* base_elf_file = &Linker::elf_files.begin()->second;
  Word next_section_place = placePredefinedSections();
  
  for ( vector<ELF_SHT_Entry>::iterator sh_entry = base_elf_file->shtable.begin(); sh_entry != base_elf_file->shtable.end(); sh_entry++){
    if ( sh_entry->symtbndx > 0 && sh_entry->symtbndx != 65535){

      if ( Linker::sections_place.find(findSymbolInSymTBLIndex(sh_entry->symtbndx, &base_elf_file->symtbl)->sym_name)->second != 65535){

        findSymbolInSymTBLIndex(sh_entry->symtbndx, &base_elf_file->symtbl)->value = Linker::sections_place.find(findSymbolInSymTBLIndex(sh_entry->symtbndx, &base_elf_file->symtbl)->sym_name)->second;

        for ( vector<ELF_SYMT_Entry>::iterator section_symbols = base_elf_file->symtbl.begin(); section_symbols != base_elf_file->symtbl.end(); section_symbols++){

          if ( section_symbols->shndx > 0 && section_symbols->shndx == sh_entry->symtbndx && section_symbols->symndx != sh_entry->symtbndx){
            section_symbols->value += Linker::sections_place.find(findSymbolInSymTBLIndex(sh_entry->symtbndx, &base_elf_file->symtbl)->sym_name)->second;
          }

        }
        continue;
      } 
      Linker::sections_place.find(findSymbolInSymTBLIndex(sh_entry->symtbndx, &base_elf_file->symtbl)->sym_name)->second = next_section_place;
      findSymbolInSymTBLIndex(sh_entry->symtbndx, &base_elf_file->symtbl)->value = next_section_place;
      for ( vector<ELF_SYMT_Entry>::iterator section_symbols = base_elf_file->symtbl.begin(); section_symbols != base_elf_file->symtbl.end(); section_symbols++){
        if ( section_symbols->shndx > 0 && section_symbols->shndx == sh_entry->symtbndx && section_symbols->symndx != sh_entry->symtbndx){
          section_symbols->value += next_section_place;
        }
      }
      next_section_place += sh_entry->size;
    }
  }
  
}

void Linker::createHex(ofstream& fout, ELF16_File* elf_file){

  bool bad_placing = true;

  for ( map<string, Word>::iterator placed_sections = Linker::sections_place.begin(); placed_sections != Linker::sections_place.end(); placed_sections++){
    if ( placed_sections->second == 0) bad_placing = false;
  }

  if ( bad_placing){
    cerr << "No section placed at memory start(must be ivt)" << endl;
    exit(-1);
  }

  ELF16_File* base_elf_file = &Linker::elf_files.begin()->second;

  int size = 0;
  
  string highest_section = "";
  Word highest_section_place = 0;
  for ( map<string, Word>::iterator placed_sections = Linker::sections_place.begin(); placed_sections != Linker::sections_place.end(); placed_sections++){
    if ( highest_section_place < placed_sections->second){
      highest_section_place = placed_sections->second;
      highest_section = placed_sections->first;
    }
  }
  /*int binary_size = 0;
  for ( map<string, vector<Byte>>::iterator section = elf_file->sections_binary.begin(); section != elf_file->sections_binary.end(); section++ ){
    binary_size += section->second.size();
  }*/
  for ( vector<ELF_SHT_Entry>::iterator sh_entry = base_elf_file->shtable.begin(); sh_entry != base_elf_file->shtable.end(); sh_entry++){
    if ( sh_entry->symtbndx == findSymbolInSymTBL(highest_section, &base_elf_file->symtbl)->symndx){
      size = Linker::sections_place.find(highest_section)->second + sh_entry->size;
    }
  }

  vector<Byte> content = vector<Byte>(size, 0);
  vector<bool> place_taken = vector<bool>(size, false);

  Word next_place = 0;
  
 
  for ( map<string, Word>::iterator placed_sections = Linker::sections_place.begin(); placed_sections != Linker::sections_place.end(); placed_sections++){
   
    vector<Byte> read_vector = elf_file->sections_binary.find(placed_sections->first)->second;
    for ( int i = placed_sections->second; i < read_vector.size() + placed_sections->second; i++){
      if ( place_taken[i] == false){
         content[i] = read_vector[i - placed_sections->second];
         place_taken[i] = true;
      }
      else{
        cerr << "Sections are overlaping" << endl;
        exit(-1);
      }
     
    }
    
    next_place += read_vector.size();

  }
 
  
  
  
  int next_address = 0;
  fout << "---------------- Memory content ----------------" << endl;
  fout << std::setbase(16);
  for ( int i = 0; i < content.size() ; i++){
    if ( i % 8 == 0) {
      fout << endl;
      fout << std::setw(4) << std::setfill('0') << next_address;
      fout << ": ";
      next_address += 8;
    }
    fout << std::setw(2) << std::setfill('0') << (Word)content[i] << " ";
  }

}

void Linker::createHexBinary(FILE* fout,ELF16_File* elf_file){
  
  /*int binary_size = 0;
  for ( map<string, vector<Byte>>::iterator section = elf_file->sections_binary.begin(); section != elf_file->sections_binary.end(); section++ ){
    binary_size += section->second.size();
  }

  vector<Byte> content = vector<Byte>(binary_size, 0);

  Word next_place = 0;
  
  
  for ( map<string, Word>::iterator placed_sections = Linker::sections_place.begin(); placed_sections != Linker::sections_place.end(); placed_sections++){
    
      vector<Byte> read_vector = elf_file->sections_binary.find(placed_sections->first)->second;
      for ( int i = placed_sections->second; i < read_vector.size() + placed_sections->second; i++){
        content[i] = read_vector[i - placed_sections->second];
      }

  }*/

  bool bad_placing = true;

  for ( map<string, Word>::iterator placed_sections = Linker::sections_place.begin(); placed_sections != Linker::sections_place.end(); placed_sections++){
    if ( placed_sections->second == 0) bad_placing = false;
  }

  if ( bad_placing){
    cerr << "No section placed at memory start(must be ivt)" << endl;
    exit(-1);
  }

  ELF16_File* base_elf_file = &Linker::elf_files.begin()->second;

  int size = 0;
  
  string highest_section = "";
  Word highest_section_place = 0;
  for ( map<string, Word>::iterator placed_sections = Linker::sections_place.begin(); placed_sections != Linker::sections_place.end(); placed_sections++){
    if ( highest_section_place < placed_sections->second){
      highest_section_place = placed_sections->second;
      highest_section = placed_sections->first;
    }
  }
  /*int binary_size = 0;
  for ( map<string, vector<Byte>>::iterator section = elf_file->sections_binary.begin(); section != elf_file->sections_binary.end(); section++ ){
    binary_size += section->second.size();
  }*/
  for ( vector<ELF_SHT_Entry>::iterator sh_entry = base_elf_file->shtable.begin(); sh_entry != base_elf_file->shtable.end(); sh_entry++){
    if ( sh_entry->symtbndx == findSymbolInSymTBL(highest_section, &base_elf_file->symtbl)->symndx){
      size = Linker::sections_place.find(highest_section)->second + sh_entry->size;
    }
  }

  vector<Byte> content = vector<Byte>(size, 0);


  Word next_place = 0;
  
 
  for ( map<string, Word>::iterator placed_sections = Linker::sections_place.begin(); placed_sections != Linker::sections_place.end(); placed_sections++){
   
    vector<Byte> read_vector = elf_file->sections_binary.find(placed_sections->first)->second;
    for ( int i = placed_sections->second; i < read_vector.size() + placed_sections->second; i++){
      content[i] = read_vector[i - placed_sections->second];
    }
    
    next_place += read_vector.size();

  }
 

  Word size_bin = content.size();
  fwrite(&size_bin, sizeof(Word), 1, fout);
  for ( int i = 0; i < content.size(); i++){
    fwrite(&content[i], sizeof(Byte), 1, fout);
  }
  
}

Word Linker::placePredefinedSections(){

  ELF16_File* base_elf_file = &Linker::elf_files.begin()->second;

  Word next_section_place = 0;

  Word highest_start = 0;
  for ( map<string, Word>::iterator it = Linker::sections_place.begin(); it != Linker::sections_place.end(); it++ ){
    if ( highest_start <= it->second && it->second != 65535) {
      highest_start = it->second;
      Word sec_symtbl_index = findSymbolInSymTBL(it->first, &base_elf_file->symtbl)->symndx;
      for ( vector<ELF_SHT_Entry>::iterator sh_entry = base_elf_file->shtable.begin(); sh_entry != base_elf_file->shtable.end(); sh_entry++){
        if ( sh_entry->symtbndx == sec_symtbl_index) next_section_place = highest_start + sh_entry->size;
      }
      
    }
  }
  
  return next_section_place;
}