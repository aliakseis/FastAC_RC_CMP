#ifndef enlog_h
#define enlog_h

bool EncodeData(int* source, int data_size, int min, int max, unsigned char* result, int& res_size);
void DecodeData(unsigned char* data, int data_size, int* decoded, int& decoded_size);

#endif