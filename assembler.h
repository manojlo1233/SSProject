#ifndef _assembler_h_
#define _assembler_h_

#include "elf.h"
#include <vector>
#include <string>
#include "instr.h"
#include <map>
#include <fstream>
#include <sstream>


using namespace std;



class Assembler{
public:

  Assembler(){}
  
  static ELF16_File elf_file;

  static int asm_pass(void* l);

  static void initializeFile();

  static void buffer(){}
  

  static map<string,int> section_names_ndxs;  

  static int location_counter;
  static string current_section;
  
  static vector<string> extern_symbols;
  static vector<string> global_symbols;

  static void writeToSectionBinary(Word value);
  static void skipIntoSectionBinary(Word value);
  static void writeStringToSectionBinary(string text);

  static void writeInstrNoArgToSectonBinary(Byte op_code);
  static void writeInstrtToSectionBinary(Instruction* instr);

  static ELF_FLINK_Entry* flink_head;

  static int symbolExistsInSymbolTBL(string symbol);

  static void finalizeFLINKS();
  static void deleteFlinkList();
  static void finalizeRelTBL();
  static void finalizeSectionSize();

  static void createTXTFile(ofstream& file);
  static void createBinaryFile(FILE* file);

  static Word getSize(Word index);
  static string getType(ELF_SYM_Type type);
  static string getInfo(ELF_SYM_Info info);
  static string getRelType(ELF_REL_Type type);
  static string getSymbolForRela(ELF_RELT_Entry entry);
  static void validateSymbols();

  static int findNumOfRelaSections();

  static void fixLocalSymbols();

};

  enum SECTION_NDX
{
    SECTION_NDX_UNDEF = 0,
    SECTION_NDX_SYMTAB,
    SECTION_NDX_STRTAB

};



#endif