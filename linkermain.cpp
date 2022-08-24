#include <iostream>

#include "linker.h"
#include "utility.h"
#include "assembler.h"
#include "instr.h"
#include "directives.h"
#include <iomanip>

using namespace std;

int main(int argc, char** argv){

  bool is_hex = false;
  bool is_reloc = false;

  for ( int i = 1; i < argc; i++){
    if ( (string)argv[i] == "-hex"){
      is_hex = true;
    }
    if ( (string)argv[i] == "-relocateable"){
      is_reloc = true;
    }
    
  }

  if ( is_hex && is_reloc) cerr << "Wrong input for linker" << endl;
  if ( is_hex == false && is_reloc == false) return 0;

  string output_file;
  vector<string> input_files;
  if ( is_hex ){

    for ( int i = 1; i < argc; i++){
      if ( (string)argv[i] == "-hex" ) continue;
      if ( (string)argv[i] == "-o" && i < argc - 1){
        output_file = argv[i+1];
        i++;
      }
      else {
        string input = (string)argv[i];
        bool place = false;
        bool count = false;
        int counter = 0;
        for ( int j = 0; j < input.size(); j++){
          if ( input[j] == '='){
            count = true;
            continue;
          }
          if ( input[j] == '@') {
            place = true;
            break;
          }
          if (count) counter++;
        }
        if ( place == false) input_files.push_back((string)argv[i]);
        else {
          string section = input.substr(7,counter);
          Word sec_pl = stoi(input.substr(7 + counter + 3), nullptr, 16);
          Linker::sections_place.insert(pair<string, Word>(section, sec_pl));
        }
      }
    }
    
  
    Linker::loadFiles(input_files);

    Linker::checkErrors();
    
    Linker::linkFiles();

    Linker::placeSections();

    Linker::relocateSymbols();
   
    string output_file_hex = output_file;

    ofstream file_output_hex(output_file_hex);
    if ( file_output_hex.is_open()){
      /*for ( map<string,ELF16_File>::iterator it= Linker::elf_files.begin(); it != Linker::elf_files.end(); it++){
        createTXTFileUT(file_output, it->second);
      }*/
      Linker::createHex(file_output_hex,&Linker::elf_files.begin()->second);
    }
    file_output_hex.close();
    
    
    ofstream file_output(output_file + ".rel");
    if ( file_output.is_open()){
      /*for ( map<string,ELF16_File>::iterator it= Linker::elf_files.begin(); it != Linker::elf_files.end(); it++){
        createTXTFileUT(file_output, it->second);
      }*/
       
      createTXTFileUT(file_output, Linker::elf_files.begin()->second);
    }
    file_output.close();

    string output_file_bin = output_file + ".bin";

    FILE* fout = fopen( output_file_bin.c_str() , "w");
    if ( fout == nullptr) cerr << "Error opening the file" << endl;
    
    Linker::createHexBinary(fout, &Linker::elf_files.begin()->second);

    fclose(fout);

  }
  else{

    for ( int i = 1; i < argc; i++){
      if ( (string)argv[i] == "-relocateable" ) continue;
      if ( (string)argv[i] == "-o" && i < argc - 1){
        output_file = argv[i+1];
        i++;
      }
      else {
        string input = (string)argv[i];
        for ( int j = 0; j < input.size(); j++){
          if ( input[j] == '@'){
            cerr << "Cannot include both place and relocateable option" << endl;
            exit(-1);
          }
        }
        input_files.push_back((string)argv[i]);
      }
    }

    Linker::loadFiles(input_files);
    
    Linker::checkErrors();
    
    Linker::linkFiles();

    ofstream file_output(output_file);
    if ( file_output.is_open()){
      /*for ( map<string,ELF16_File>::iterator it= Linker::elf_files.begin(); it != Linker::elf_files.end(); it++){
        createTXTFileUT(file_output, it->second);
      }*/
     
      createTXTFileUT(file_output, Linker::elf_files.begin()->second);
    }
    file_output.close();

    string output_file_bin = output_file + ".bin";

    FILE* fout = fopen( output_file_bin.c_str() , "w");
    if ( fout == nullptr) cerr << "Error opening the file" << endl;
    
    createBinaryFile(fout, Linker::elf_files.begin()->second);
    
    fclose(fout);


  }

  cout << "Linker finished succesfully" << endl;
  return 0;
}