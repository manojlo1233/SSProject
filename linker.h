#ifndef _linker_h_
#define _linker_h_

#include "elf.h"
#include <fstream>

class Linker{
  public:

  static map<string, ELF16_File> elf_files;
  static map<string, Word> sections_place;

  static void linkFiles();
  static void loadFiles(vector<string> files);
  static void fixStrings(ELF16_File* elf_file);
  static void checkErrors();
  static void relocateSymbols();
  static void placeSections();
  static void createHex(ofstream& fout,ELF16_File* elf_file);
  static void createHexBinary(FILE* fout,ELF16_File* elf_file);
};

#endif