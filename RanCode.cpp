// RanCode.cpp : Defines the entry point for the console application.
//
//This is DEMO version of arithmetic/range encoder written for research purposes by 
//Andrew Polar (www.ezcodesample.com, Jan. 10, 2007). The algorithm was published by
//G.N.N. Martin. In March 1979. Video & Data Recording Conference, Southampton, UK. 
//The program was tested for many statistically different data samples, however you 
//use it on own risk. Author do not accept any responsibility for use or misuse of 
//this program. Any data sample that cause crash or wrong result can be sent to author 
//for research. The e-mail may be obtained by WHOIS www.ezcodesample.com.
//Last correction Feb. 24, 2007.

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

///////////////////////////////////////////////////////////////////
// End of data preparation start of encoding, decoding functions
///////////////////////////////////////////////////////////////////

const unsigned MAX_BASE = 15;   //value is optional but 15 is recommended
unsigned       current_byte;    //position of current byte in result data
unsigned char  current_bit;     //postion of current bit in result data

//Some look up tables for fast processing
static int           output_mask[8][32];
static unsigned char bytes_plus [8][64];

static unsigned char edge_mask[8]   = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
static unsigned char shift_table[8] = {24, 16, 8, 0};

static long long     overflow_indicator = 0xffffffffffffffff << (MAX_BASE * 2 - 1);

void make_look_ups() {

	int sign_mask[8] = {
		0xffffffff, 0x7fffffff, 0x3fffffff, 0x1fffffff,
		0x0fffffff, 0x07ffffff, 0x03ffffff, 0x01ffffff
	};
	
	for (int i=0; i<8; i++) {
		for (int j=0; j<32; j++) {
			output_mask[i][j] = sign_mask[i];
			output_mask[i][j] >>= (32-i-j);
			output_mask[i][j] <<= (32-i-j);
		}
	}

	for (int i=0; i<8; i++) {
		for (int j=0; j<64; j++) {
			bytes_plus[i][j] = j/8;
			if ((i + j%8) > 7)
				++bytes_plus[i][j];
		}
	}
}

//The key function that takes product of x*y and convert it to 
//mantissa and exponent. Mantissa have number of bits equal to
//length of the mask.
inline int SafeProduct(int x, int y, int mask, int& shift) {

	int p = x * y;
	if ((p >> shift) > mask) {
		p >>= shift;
		while (p > mask) {
			p >>= 1;
			++shift;
		}
	}
	else {
		while (shift >= 0) {
			--shift;
			if ((p >> shift) > mask) {
				break;
			}
		}
		++shift;
		p >>= shift;
	}
	return p;
}

inline int readBits(int operation_buffer, int bits, unsigned char* code) {

	unsigned end_byte = current_byte + bytes_plus[current_bit][bits];
	int buffer = (code[current_byte] & edge_mask[current_bit]);
	unsigned char total_bits = 8 - current_bit;
	for (unsigned i=current_byte+1; i<=end_byte; ++i) {
		(buffer <<= 8) |= code[i];
		total_bits += 8;
	}
	buffer >>= (total_bits - bits);
	operation_buffer <<= bits;
	operation_buffer |= buffer;
	current_byte = end_byte;
	current_bit  = (bits + current_bit)%8; 
	return operation_buffer;
}

inline void writeBits(long long operation_buffer, int bits, unsigned char* result) {

	int buffer = (int)((operation_buffer >> current_bit) >> 32);
	buffer &= output_mask[current_bit][bits];
	unsigned bytes_plus2 = bytes_plus[current_bit][bits]; 
	current_bit = (bits + current_bit)%8;
	for (unsigned i=0; i<=bytes_plus2; ++i) {
		result[current_byte + i] |= (buffer >> shift_table[i]);
	}
	current_byte += bytes_plus2;
}

//The result buffer should be allocated outside the function
//The data must be non-negative integers less or equal to max_value
//The actual size of compressed buffer is returned in &result_size
//Initially result_size must contain size of result buffer
void Encode(int* source, int source_size, int max_value, int min_value, unsigned char* result, int& result_size) {
	
	memset(result, 0x00, result_size);
	current_byte = 0;
	current_bit  = 0;

	if (min_value != 0) {
		for (int i=0; i<source_size; i++) {
			source[i] -= min_value;
		}
	}

	//collecting frequencies
	int range = (max_value + 1) - min_value + 1; //we add one more value for flag
	int* frequency = (int*)malloc(range*sizeof(int));
	memset(frequency, 0x00, range * sizeof(int));
	for (int i=0; i<source_size; ++i)
		++frequency[source[i]+1]; //we make source vary from 1 to max-min+1
	frequency[0] = 1; //we use it as flag, virtual 0 value occured once
	
	//correction by bringing them in certain range
	int Denominator = source_size + 1; //we increased size by 1 
	unsigned base = 0;
	while (Denominator > 1) {
		Denominator >>= 1;
		base++;
	}
	if (base > MAX_BASE)
		base = MAX_BASE;
	Denominator = (1 << base);

	double coeff = (double)(Denominator)/((double)(source_size));
	for (int i=1; i<range; i++) {  //we start from 1 on purpose
		if (frequency[i] > 0) {
			double ff = (double)(frequency[i]);
			ff *= coeff;
			frequency[i] = (int)(ff);
			if (frequency[i] == 0)
				frequency[i] = 1;
		}
	}
	int sm = 0;
	for (int i=0; i<range; i++) {
		sm += frequency[i];
	}
	int diff = Denominator - sm;
	if (diff > 0) {
		int mx = frequency[0];
		int ps = 0;
		for (int i=0; i<range; i++) {
			if (frequency[i] > mx) {
				mx = frequency[i];
				ps = i;
			}
		}
		frequency[ps] += diff;
	}
	if (diff < 0) {
		int i = 0;
		while (diff < 0) {
			if (frequency[i] > 1) {
				frequency[i]--;
				diff++;
			}
			i++;
			if (i == (range-1))
				i = 0;
		}
	}
	//end of correction

	//saving frequencies in result buffer
	int offset = 0;
	memcpy(result + offset, &range, 4);
	offset += 4;
	for (int m=0; m<range; ++m) {
		memcpy(result + offset, &frequency[m], 2);
		offset += 2;
	}
	current_byte += offset;

	//making cumulative frequencies
	int* cumulative = (int*)malloc((range+1)*sizeof(int));
	memset(cumulative, 0x00, (range+1)*sizeof(int));
	cumulative[0] = 0;
	for (int i=1; i<range; i++) {
		cumulative[i] = cumulative[i-1] + frequency[i-1];
	}
	cumulative[range] = Denominator;
	//data is ready

	//encoding
	make_look_ups();
	int mask = 0xFFFFFFFF >> (32 - base);
	//First 64 bits are not used for data, we use them for saving data_size and
	//any other 32 bit value that we want to save
	long long operation_buffer = source_size;
	operation_buffer <<= 32;
	//we save minimum value always as positive number and use last
	//decimal position as sign indicator
	int saved_min = min_value * 10;
	if (saved_min < 0) {
		saved_min = 1 - saved_min;
	}
	operation_buffer |= saved_min; //we use another 32 bits of first 64 bit buffer 
	/////////////////////
	int product = 1;
	int shift = 0;
	for (int i=0; i<source_size; ++i) {

		while (((operation_buffer << (base - shift)) & overflow_indicator) == overflow_indicator) {
			printf("Possible buffer overflow is corrected at value %d\n", i);
			//this is zero flag output in order to avoid buffer overflow
			//rarely happen, cumulative[0] = 0, frequency[0] = 1
			writeBits(operation_buffer, base-shift, result); 
			operation_buffer <<= (base - shift);
			operation_buffer += cumulative[0] * product;
			product = SafeProduct(product, frequency[0], mask, shift);
			//in result of this operation shift = 0
		}

		writeBits(operation_buffer, base-shift, result); 
		operation_buffer <<= (base - shift);
		operation_buffer += cumulative[source[i]+1] * product;
		product = SafeProduct(product, frequency[source[i]+1], mask, shift);
	}
	//flushing remained 64 bits
	writeBits(operation_buffer, 24, result);
	operation_buffer <<= 24;
	writeBits(operation_buffer, 24, result);
	operation_buffer <<= 24;
	writeBits(operation_buffer, 16, result);
	operation_buffer <<= 16;
	result_size = current_byte + 1;
	//end encoding

	if (min_value != 0) {
		for (int i=0; i<source_size; i++) {
			source[i] += min_value;
		}
	}

	if (cumulative)
		free(cumulative);

	if (frequency)
		free(frequency);
}

//result buffer must be allocated outside of function
//result size must contain the correct size of the buffer
void Decode(unsigned char* code, int code_size, int* result, int& result_size) {

	current_byte = 0;
	current_bit  = 0;

	//reading frequencies
	int range  = 0;
	memcpy(&range, code, 4);

	short* frequency = (short*)malloc(range * sizeof(short));
	memcpy(frequency, code + 4, range * 2);
	int Denominator = 0;
	for (int i=0; i<range; ++i) {
		Denominator += frequency[i];
	}
	current_byte += range * 2 + 4;
	unsigned base = 0;
	while (Denominator > 1) {
		Denominator >>= 1;
		base++;
	}
	Denominator = (1 << base);

	//making cumulative frequencies
	int* cumulative = (int*)malloc((range+1)*sizeof(int));
	memset(cumulative, 0x00, (range+1)*sizeof(int));
	cumulative[0] = 0;
	for (int i=1; i<range; i++) {
		cumulative[i] = cumulative[i-1] + frequency[i-1];
	}
	cumulative[range] = Denominator;
	//data is ready

	//prepare data for decoding
	int* symbol = (int*)malloc(Denominator*sizeof(int));
	memset(symbol, 0x00, Denominator*sizeof(int));
	for (int k=0; k<range; ++k) {
		for (int i=cumulative[k]; i<cumulative[k+1]; i++) {
			symbol[i] = k;
		}
	}
	//data is ready

	//decoding
	make_look_ups();
	int mask = 0xFFFFFFFF >> (32 - base);
	long long ID;
	int product = 1;
	int shift = 0;
	int operation_buffer = 0;
	//we skip first 64 bits they contain size and minimum value
	operation_buffer = readBits(operation_buffer, 32, code);
	result_size = (int)operation_buffer; //First 32 bits is data_size 
	operation_buffer = 0;
	operation_buffer = readBits(operation_buffer, 32, code);
	int min_value = (int)operation_buffer;  //Second 32 bits is minimum value;
	//we find sign according to our convention
	if ((min_value % 10) > 0)
		min_value = - min_value;
	min_value /= 10;
	operation_buffer = 0;
	////////////////////////////////////////
	int cnt = 0;
	while (cnt < result_size) {
		operation_buffer = readBits(operation_buffer, base-shift, code);
		ID = operation_buffer/product;
		operation_buffer -= product * cumulative[symbol[ID]];
		product = SafeProduct(product, frequency[symbol[ID]], mask, shift);
		result[cnt] = symbol[ID] + min_value - 1;
		if (result[cnt] >= min_value)
			cnt++;
	}
	//end decoding

	if (symbol)
		free(symbol);

	if (cumulative)
		free(cumulative);

	if (frequency)
		free(frequency);
}
