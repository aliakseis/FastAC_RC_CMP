#ifndef _rancode_h
#define _rancode_h

void Decode(unsigned char* code, int code_size, int* result, int& result_size);
void Encode(int* source, int source_size, int max_value, int min_value, unsigned char* result, int& result_size);

#endif
