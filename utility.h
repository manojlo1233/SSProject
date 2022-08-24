#ifndef _utility_h_
#define _utility_h_

#include "elf.h"
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <sstream>

#define END_FILE 1
#define NEW_LINE 2
#define ESCAPE_SPACES 46
#define LABEL 47
#define LITERAL 48
#define SYMBOL 49
#define PLUS 50
#define MINUS 51
#define OPEN_PARENTHESIS 52
#define CLOSE_PARENTHESIS 53
#define OPEN_PARENTHESIS_JMP 54
#define OPEN_COMMENT 55


using namespace std;

int stringExistsInVector(string str, vector<string> vec);

int stringExistsInMap(string str, map<string,int> sec_map);

ELF_SYMT_Entry* findSymbolInSymTBL(string label, vector<ELF_SYMT_Entry>* symtbl);

ELF_SYMT_Entry* findSymbolInSymTBLIndex(Word index, vector<ELF_SYMT_Entry>* symtbl);

Word getLiteralValue(string literal);

Word getLastIndexSYMTBL(vector<ELF_SYMT_Entry>* symtbl);

Word getLastIndexSECTBL();

Word wordExistsInVector(Word num, vector<Word> vec);

void createTXTFileUT(ofstream& file, ELF16_File elf_file);

string getTypeU(ELF_SYM_Type type);

string getInfoU(ELF_SYM_Info info);

string getRelTypeU(ELF_REL_Type type);

Word getSizeU(Word index, ELF16_File elf_file);

int findNumOfRelaSectionsU(ELF16_File elf_file);

string getSymbolForRelaU(ELF_RELT_Entry entry, ELF16_File elf_ile);

Word findOffsetInString(string sym , vector<string> table);

ELF_SYMT_Entry* findSectionInSymTBL(string section, vector<ELF_SYMT_Entry>* symtbl);

void createBinaryFile(FILE* file, ELF16_File elf_file);


#endif