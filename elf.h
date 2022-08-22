#ifndef _elf_h_
#define _elf_h_

#include "types.h"
#include <string>
#include <map>
#include <vector>

using namespace std;



enum ELF_SH_TYPE{
  SH_TYPE_SYMTBL = 300,
  SH_TYPE_STRTBL,
  SH_TYPE_RELA,
  SH_TYPE_CODE
};


enum ELF_SYM_Info: Byte{
  SYM_INFO_LOCAL = 0,
  SYM_INFO_GLOBAL

};

enum ELF_SYM_Type: Byte{
  SYM_TYPE_NOTYPE = 0,
  SYM_TYPE_SECTION,
};

enum ELF_REL_Type: Byte{
  REL_TYPE_ABS_16 = 0,
  REL_TYPE_PC_REL_16
};

enum ELF_FLINK_ADDR_TYPE: Byte{
  FLINK_ADDR_TYPE_PC_REL,
  FLINK_ADDR_TYPE_ABS
};

struct ELF_Header{
  Offs shoff;
  Word sh_num_entries;
  Word symtblndx;
  Word strndx;
  Word relndx;
};

struct ELF_RELT_Entry{
  Offs offset;
  Offs addend;
  Word symbol_index;
  Word section_index;
  ELF_REL_Type type;
  ELF_SYM_Info sym_info;
  string symbol;
};

struct ELF_FLINK_Entry{
  string section;
  int section_location;
  ELF_FLINK_Entry* next;
  ELF_FLINK_ADDR_TYPE addr_type;
  string sym_name;
};

struct ELF_SYMT_Entry{
  Word name;
  Word value;
  ELF_SYM_Info info;
  ELF_SYM_Type type;
  Word shndx;
  string sym_name;
  Word symndx;
};

struct ELF_SHT_Entry{
  Word symtbndx;
  Offs offset;
  ELF_SH_TYPE type;
  Word size;
};

struct ELF16_File{
    public:

    ELF16_File(){
      
    }
    ELF_Header elfHeader;
    map<string,vector<Byte>> sections_binary;
    vector<string> strtab;
    vector<ELF_SYMT_Entry> symtbl;
    vector<ELF_RELT_Entry> reltbls;
    vector<ELF_SHT_Entry> shtable;

  };

#endif