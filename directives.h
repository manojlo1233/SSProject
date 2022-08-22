#ifndef _directives_h_
#define _directives_h_

#include "types.h"
#include <string>
using namespace std;

  enum DIRECTIVES_TYPE: Byte{
      DIR_GLOBAL = 3,
      DIR_EXTERN,
      DIR_SECTION,
      DIR_WORD,
      DIR_SKIP,
      DIR_ASCII,
      DIR_EQU,
      DIR_END
    };

void handleDirective( DIRECTIVES_TYPE type, void* l);

void handleLabels( string label, void*l);



#endif