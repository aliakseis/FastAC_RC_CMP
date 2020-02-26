#ifndef _runcoder15_h
#define _runcoder15_h

int Encode15(int* data, int data_size, int alphabet, unsigned char* result);
int Decode15(unsigned char* code, int alphabet, int* decode);

#endif
