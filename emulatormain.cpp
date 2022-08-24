#include "emulator.h"
#include <iomanip>

int main(int argc, char** argv){

  string input_file;

  if ( argc != 2) {
    cerr << "Invalid input" << endl;
  }
  else{
    input_file = argv[1];
  }

  Emulator::load_file(input_file);

  Emulator::initialize_terminal();
  
  Emulator::emulate_code();

  cout << "Emulator finished succesfully" << endl;
  return 0;
}