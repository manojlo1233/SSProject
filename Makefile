flex --header-file=MyLexer.h lexer.lext

g++ assemblermain.cpp lex.yy.cc utility.cpp directives.cpp instr.cpp assembler.cpp -o asembler

./asembler sample1.s -o main1.txt

g++ linkermain.cpp linker.cpp utility.cpp assembler.cpp instr.cpp directives.cpp -o linker

./linker -relocateable main1.txt.o main2.txt.o

./linker -hex main4.txt.o -o linker4.out

g++ emulatormain.cpp emulator.cpp -o emulator

./emulator linker2.txt.hex.bin
