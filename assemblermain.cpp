#include "assembler.h"
#include "MyLexer.h"
#include <iostream>


using namespace std;

int main(int argc, char** argv)
{
	if ( argc != 2 && argc != 4){
		printf("Invalid parameter input\n");
		return -1;
	}

	string inputFile;
	string outputFile;

	if ( argc == 2){
		
		inputFile = argv[1];
		outputFile = "asm.txt";
	}
	if ( argc == 4){
		
		
		if ( (string)argv[1] == "-o"){
			inputFile = argv[3];
			outputFile = argv[2];
		}
		else if ( (string)argv[2] == "-o"){
			
			inputFile = argv[1];
			outputFile = argv[3];
		}
	}

	

	int status = 0;
	ifstream file;
	file.open(inputFile);
	if ( !file.is_open()){
		printf("Invalid file as input.\n");
		return -1;
	}
	
	string data = "";

	
	while(!file.eof()){
		string tmp;
		getline(file,tmp); //Get 1 line from the file and place into temp
		data = data + tmp + '\n';

	}
	

	file.close();
	istringstream  flex_data(data);
	yyFlexLexer l(&flex_data, &cout);

	Assembler::asm_pass(&l);

	

	ofstream file_output(outputFile);
	if ( file_output.is_open()){
		Assembler::createTXTFile(file_output);
	}
	file_output.close();

	string output_file_bin = outputFile + ".o";

	FILE* fout = fopen( output_file_bin.c_str() , "w");
	if ( fout == nullptr) cerr << "Error opening the file" << endl;
	
	Assembler::createBinaryFile(fout);
	
	fclose(fout);

	
	cout << "Assembler finished succesfully" << endl;

	return 0;
		
}