#include <stdio.h>
#include <stdlib.h>

#include "helpers.h"


void alloc_error() {
    fprintf(stderr, "błąd alokacji\n");
    exit(EXIT_FAILURE);
}

void fatal_error(char * info) {
    fprintf(stderr, info);
    exit(EXIT_FAILURE);
}

// for(int i=0; i< iter; i++) {
//  for (int j=0;get_function[j]._token != NULL;j++) {
//      if (get_function[j]._token == logical_operators[i]) {

//      }
//  }
// }


