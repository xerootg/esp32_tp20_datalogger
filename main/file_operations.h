#include <stdio.h>

FILE * setup_fs();
int f_puts(const char* strToWrite, FILE* filePtr); //When the string was written successfuly, it returns number of character encoding units written to the file. When the function failed due to disk full or any error, an EOF (-1) will be returned.
