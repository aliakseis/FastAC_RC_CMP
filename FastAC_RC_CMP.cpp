// FastAC_RC_CMP.cpp : Defines the entry point for the console application.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Simple file compression using arithmetic coding                           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Version 1.00  -  April 25, 2004                                           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                                  WARNING                                  -
//                                 =========                                 -
//                                                                           -
// The only purpose of this program is to demonstrate the basic principles   -
// of arithmetic coding. It is provided as is, without any express or        -
// implied warranty, without even the warranty of fitness for any particular -
// purpose, or that the implementations are correct.                         -
//                                                                           -
// Permission to copy and redistribute this code is hereby granted, provided -
// that this warning and copyright notices are not removed or altered.       -
//                                                                           -
// Copyright (c) 2004 by Amir Said (said@ieee.org) &                         -
//                       William A. Pearlman (pearlw@ecse.rpi.edu)           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <windows.h>

#include "arithmetic_codec.h"
#include "rancode.h"
extern "C" {
#include "range.h"
}
#include "enlog.h"
#include "runcoder15.h"

#include "model_ex.h"
#include "coder.h"

// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const char * W_MSG = "cannot write to file";
const char * R_MSG = "cannot read from file";

//does not make much difference in speed but provide adaption.
//we use 1 for static encoding for speed comparison, original code has 16 (comment by Andrew Polar)
const unsigned NumModels  = 1; //MUST be power of 2

const unsigned FILE_ID    = 0xB8AA3B29U;

const unsigned BufferSize = 65536;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Prototypes  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Encode_File(char * data_file_name,
                 char * code_file_name);

void Decode_File(char * code_file_name,
                 char * data_file_name);

void Encode_Data(unsigned char* source, int size, unsigned char* data, int& data_size);
void Decode_Data(unsigned char* code,   int size, unsigned char* result, int res_size);

///////////////////////////////////////////////////////////////////////
// Test data preparation
///////////////////////////////////////////////////////////////////////

double entropy24(unsigned char* data, int size) {
	
	int max_size = 256;
	int counter;
	int* buffer;
	double entropy;
	double log2 = log(2.0);
	double prob;

	buffer = (int*)malloc(max_size*sizeof(int));
	memset(buffer, 0x00, max_size*sizeof(int));
	for (counter=0; counter<size; counter++) {
		buffer[data[counter]]++;
	}

	entropy = 0.0;
	for (counter=0; counter<max_size; counter++) {
		if (buffer[counter] > 0) {
			prob = (double)buffer[counter];
			prob /= (double)size;
			entropy += log(prob)/log2*prob;
		}
	}
	entropy *= -1.0;

	if (buffer)
		free(buffer);

	return  entropy;
}

double round(double x) {
	double y  = floor(x);
	double yy = x - y;
	if (yy >= 0.5)
		return ceil(x);
	else
		return floor(x);
}

double MakeData(unsigned char* data, int SIZE, int MIN, int MAX, int redundancy_factor) {

	int counter, cnt, high, low;
	double buf;

	if (redundancy_factor <= 1)
		redundancy_factor = 1;

	if (MAX <= MIN)
		MAX = MIN + 1;

	srand((unsigned)time(0)); 
	for (counter=0; counter<SIZE; counter++) {
		buf = 0;
		for (cnt=0; cnt<redundancy_factor; cnt++) {
			buf += (double)(rand()%256);
		}
		data[counter] = ((int)buf)/redundancy_factor;
	}

	low  = data[0];
	high = data[0];
	for (counter=0; counter<SIZE; counter++) {
		if (data[counter] > high)
			high = data[counter];
		if (data[counter] < low)
			low = data[counter];
	}

	for (counter=0; counter<SIZE; counter++) {
		buf = (double)(data[counter] - low);
		buf /= (double)(high - low);
		buf *= (double)(MAX - MIN);
		buf = round(buf);
		data[counter] = (int)buf + MIN;
	}

	return entropy24(data, SIZE);
}

class Timer {
private:
	LARGE_INTEGER startTime, stopTime;
	LARGE_INTEGER freq;
public:
	Timer() {
		QueryPerformanceFrequency(&freq);
		startTime.LowPart = startTime.HighPart = 0;
		stopTime.LowPart = stopTime.HighPart = 0;
	}

	// start the timer
	void start() {
		QueryPerformanceCounter(&startTime);
	}

	// stop the timer
	void stop() {
		QueryPerformanceCounter(&stopTime);
	}

	// returns the duration of the timer (in seconds)
	double duration() {
		return (double)(stopTime.QuadPart - startTime.QuadPart) / freq.QuadPart;
	}
};


void main() {
	
	printf("Making random data ... \n");
	int SIZE = 4000000;
	unsigned char* source = (unsigned char*)malloc(SIZE);
	int* source2 = (int*)malloc(SIZE*sizeof(int));

	double entropy = MakeData(source, SIZE, 0, 255, 4);
	int Bytes = (int)(ceil((double)(SIZE) * entropy)/8.0);

	BOOL bOK = SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	for (int i=0; i<SIZE; i++)
		source2[i] = source[i];

	printf("Entropy %f, theoretical compressed size %d bytes\n", entropy, Bytes);
	printf("Data size %d\n", SIZE);
	printf("Alphabet  %d\n\n", 256);
	printf("FastAC round trip ---------------------------------------\n");

	Timer timer;
	printf("Encoding started\n");
	timer.start();

	unsigned char* result = (unsigned char*)malloc(SIZE);
	int result_size = 0;
	Encode_Data(source, SIZE, result, result_size);

	timer.stop();
	printf("Encoding ended,   time %2.3f sec., size %d bytes\n", timer.duration(), result_size);
	timer.start();

	unsigned char* test = (unsigned char*)malloc(SIZE + SIZE/4);
	Decode_Data(result, result_size, test, SIZE);

	timer.stop();
	printf("Decoding ended,   time %2.3f sec.\n", timer.duration());

	bool isCorrect = true;
	for (int i=0; i<SIZE; i++) {
		if (test[i] != source[i]) {
			printf("%d %d %d\n", i, test[i], source[i]);
			isCorrect = false;
			break;
		}
	}

	if (isCorrect == true)
		printf("Round trip OK\n\n");
	else
		printf("Data mismatch\n\n");

	printf("Andrew Polar's range encoder round trip -----------------\n");

	printf("Encoding started\n");
	timer.start();

	unsigned char* rcode = (unsigned char*)malloc(SIZE + SIZE/2);
	int rcode_size = SIZE + SIZE/2;

	Encode(source2, SIZE, 255, 0, rcode, rcode_size);

	timer.stop();
	printf("Encoding ended,   time %2.3f sec., size %d bytes\n", timer.duration(), rcode_size);
	timer.start();
	
	int* test2 = (int*)malloc((SIZE + SIZE/2)*sizeof(int));
	int test2_size = SIZE + SIZE/2;
	Decode(rcode, rcode_size, test2, test2_size);
	
	timer.stop();
	printf("Decoding ended,   time %2.3f sec.\n", timer.duration());

	isCorrect = true;
	for (int i=0; i<SIZE; i++) {
		if (test2[i] != source[i]) {
			printf("%d %d %d\n", i, test2[i], source[i]);
			isCorrect = false;
			break;
		}
	}

	if (isCorrect == true)
		printf("Round trip OK\n\n");
	else
		printf("Data mismatch\n\n");

	printf("Subbotin's range encoder, modified by Mikael Lundqvist --\n");
	printf("Encoding started\n");
	timer.start();
#define SYMS 256

	int BSIZE = SIZE;
	int CSIZE = BSIZE + BSIZE/100;

	Uint ftbl[SYMS];  // The frequency table 

	Sint sym;
	Uint n;
	uc *op, *s;
	rc_encoder rc;
	rc_decoder rd;
	rc_model rcm;
	
	op = (uc *)malloc(CSIZE);
	s = (uc *)malloc(BSIZE+1);

	for (int i=0; i<BSIZE; i++)
		s[i] = source[i];
		
	n = ModelInit (&rcm, SYMS, ftbl, NULL, 4, (Uint)1<<16, ADAPT);
	if (n != RC_OK) {
		fprintf(stderr, "Error: maxFreq or totFreq > 65536\n");
		free(op);
		free(s);
		exit(1);
	}
	fflush(stdout);
	StartEncode (&rc, NULL, op, CSIZE);
	for (int i = 0; i < BSIZE; i++) {
		EncodeSymbol (&rc, &rcm, s[i]);
		switch (rc.error) {
			case RC_OK:
				break;
			case RC_ERROR:
				fprintf(stderr, "Encode error: Symbol outside range\n");
				free(op);
				free(s);
				exit(1);
			case RC_IO_ERROR:
				fprintf(stderr, "Encode error: Could not output compressed data\n");
				free(op);
				free(s);
				exit(1);
		}
	}
	FinishEncode (&rc);

	timer.stop();
	printf("Encoding ended,   time %2.3f sec., size %d bytes\n", timer.duration(), rc.passed);
	timer.start();

	n = ModelInit (&rcm, SYMS, ftbl, NULL, 4, (Uint)1<<16, ADAPT);
	if (n != RC_OK) {
		fprintf(stderr, "Error: maxFreq or totFreq > 65536\n");
		free(op);
		free(s);
		exit(1);
	}
	fflush(stdout);
	StartDecode (&rd, NULL, op, CSIZE);
	n = 0;
	int i = 0;
	for (i = 0; i < BSIZE; i++) {
		sym = DecodeSymbol (&rd, &rcm);
		switch (rd.error) {
			case RC_OK:
				break;
			case RC_ERROR:
				fprintf(stderr, "Decode error: Symbol outside range\n");
				free(op);
				free(s);
				exit(1);
			case RC_IO_ERROR:
				fprintf(stderr, "Decode error: Could not input compressed data\n");
				free(op);
				free(s);
				exit(1);
		}
		if (s[n++] != sym) break;
	}
		
	timer.stop();
	printf("Decoding ended,   time %2.3f sec.\n", timer.duration());

	fflush(stdout);
	if (i == BSIZE) {
		printf("Round trip OK\n\n");
	}
	else
		fprintf(stderr, "Strings differ at %d...\n", i);

	free(op);
	free(s);

	printf("Logical entropy encoder, writen by Polar, invented by Duda\n");
	printf("Encoding started\n");
	timer.start();
	EncodeData(source2, SIZE, 0, 255, rcode, rcode_size);
	timer.stop();
	printf("Encoding ended,   time %2.3f sec., size %d bytes\n", timer.duration(), rcode_size);
	timer.start();
	DecodeData(rcode, rcode_size, test2, test2_size);
	timer.stop();
	printf("Decoding ended,   time %2.3f sec.\n", timer.duration());

	isCorrect = true;
	for (int i=0; i<SIZE; i++) {
		if (test2[i] != source[i]) {
			printf("%d %d %d\n", i, test2[i], source[i]);
			isCorrect = false;
			break;
		}
	}

	if (isCorrect == true)
		printf("Round trip OK\n\n");
	else
		printf("Data mismatch\n\n");


	printf("RunCoder15 adaptive order0, author Andrew Polar\n");
	printf("Encoding started\n");
	timer.start();
	int rsize = Encode15(source2, SIZE, 256, rcode);
	timer.stop();
	printf("Encoding ended,   time %2.3f sec., size %d bytes\n", timer.duration(), rsize);
	timer.start();
	Decode15(rcode, 256, test2);
	timer.stop();
	printf("Decoding ended,   time %2.3f sec.\n", timer.duration());

	isCorrect = true;
	for (int i=0; i<SIZE; i++) {
		if (test2[i] != source[i]) {
			printf("%d %d %d\n", i, test2[i], source[i]);
			isCorrect = false;
			break;
		}
	}

	if (isCorrect == true)
		printf("Round trip OK\n\n");
	else
		printf("Data mismatch\n\n");

	printf("arithmetic_demo round trip ------------------------------\n");

	printf("Encoding started\n");
	timer.start();
    CEncode encode;
    encode.encode(source, source + SIZE);
	timer.stop();
	printf("Encoding ended,   time %2.3f sec., size %d bytes\n", timer.duration(), encode.m_buffer.size());

	timer.start();
    CDecode decode;
    decode.decode(&encode.m_buffer[0], &encode.m_buffer[0] + encode.m_buffer.size());
	timer.stop();
	printf("Decoding ended,   time %2.3f sec.\n", timer.duration());

	isCorrect = true;
	for (i=0; i<SIZE; i++) {
		if (decode.m_buffer[i] != source[i]) {
			printf("%d %d %d\n", i, decode.m_buffer[i], source[i]);
			isCorrect = false;
			break;
		}
	}

	if (isCorrect == true)
		printf("Round trip OK\n\n");
	else
		printf("Data mismatch\n\n");



	if (test2)
		free(test2);

	if (source)
		free(source);

	if (source2)
		free(source2);

	if (result)
		free(result);

	if (test)
		free(test);

	if (rcode)
		free(rcode);
}

void Error(const char * s)
{
  fprintf(stderr, "\n Error: %s.\n\n", s);
  exit(1);
}

unsigned Buffer_CRC(unsigned bytes,
                    unsigned char * buffer)
{
  static const unsigned CRC_Gen[8] = {        // data for generating CRC table
    0xEC1A5A3EU, 0x5975F5D7U, 0xB2EBEBAEU, 0xE49696F7U,
    0x486C6C45U, 0x90D8D88AU, 0xA0F0F0BFU, 0xC0A0A0D5U };

  static unsigned CRC_Table[256];            // table for fast CRC computation

  if (CRC_Table[1] == 0)                                      // compute table
    for (unsigned k = CRC_Table[0] = 0; k < 8; k++) {
      unsigned s = 1 << k, g = CRC_Gen[k];
      for (unsigned n = 0; n < s; n++) CRC_Table[n+s] = CRC_Table[n] ^ g;
    }

                                  // computes buffer's cyclic redundancy check
  unsigned crc = 0;
  if (bytes)
    do {
      crc = (crc >> 8) ^ CRC_Table[(crc&0xFFU)^unsigned(*buffer++)];
    } while (--bytes);
  return crc;
}

FILE * Open_Input_File(char * file_name)
{
  FILE * new_file = fopen(file_name, "rb");
  if (new_file == 0) Error("cannot open input file");
  return new_file;
}

FILE * Open_Output_File(char * file_name)
{
  FILE * new_file = fopen(file_name, "rb");
  if (new_file != 0) {
    fclose(new_file);
    printf("\n Overwrite file %s? (y = yes, otherwise quit) ", file_name);
    char line[128];
    gets_s(line);
    if (line[0] != 'y') exit(0);
  }
  new_file = fopen(file_name, "wb");
  if (new_file == 0) Error("cannot open output file");
  return new_file;
}

void Save_Number(unsigned n, unsigned char * b)
{                                                   // decompose 4-byte number
  b[0] = (unsigned char)( n        & 0xFFU);
  b[1] = (unsigned char)((n >>  8) & 0xFFU);
  b[2] = (unsigned char)((n >> 16) & 0xFFU);
  b[3] = (unsigned char)( n >> 24         );
}

unsigned Recover_Number(unsigned char * b)
{                                                    // recover 4-byte integer
  return unsigned(b[0]) + (unsigned(b[1]) << 8) + 
        (unsigned(b[2]) << 16) + (unsigned(b[3]) << 24);
}

void Encode_File(char * data_file_name,
                 char * code_file_name)
{
                                                                 // open files
  FILE * data_file = Open_Input_File(data_file_name);
  FILE * code_file = Open_Output_File(code_file_name);

                                                  // buffer for data file data
  unsigned char * data = new unsigned char[BufferSize];

  unsigned nb, bytes = 0, crc = 0;       // compute CRC (cyclic check) of file
  do {
    nb = (unsigned)fread(data, 1, BufferSize, data_file);
    bytes += nb;
    crc ^= Buffer_CRC(nb, data);
  } while (nb == BufferSize);

                                                      // define 12-byte header
  unsigned char header[12];
  Save_Number(FILE_ID, header);
  Save_Number(crc,      header + 4);
  Save_Number(bytes,    header + 8);
  if (fwrite(header, 1, 12, code_file) != 12) Error(W_MSG);
                                                            // set data models
  Adaptive_Data_Model dm[NumModels];
  for (unsigned m = 0; m < NumModels; m++) dm[m].set_alphabet(256);

  Arithmetic_Codec encoder(BufferSize);                  // set encoder buffer

  rewind(data_file);                               // second pass to code file

  unsigned context = 0;
  do {

    nb = (bytes < BufferSize ? bytes : BufferSize);
    if (fread(data, 1, nb, data_file) != nb) Error(R_MSG);   // read file data

    encoder.start_encoder();
    for (unsigned p = 0; p < nb; p++) {                       // compress data
      encoder.encode(data[p], dm[context]);
      context = unsigned(data[p]) & (NumModels - 1);
    }

    encoder.write_to_file(code_file);  // stop encoder & write compressed data

  } while (bytes -= nb);

                                                          // done: close files
  fflush(code_file);
  unsigned data_bytes = ftell(data_file), code_bytes = ftell(code_file);
  printf(" Compressed file size = %d bytes (%6.2f:1 compression)\n",
    code_bytes, double(data_bytes) / double(code_bytes));
  fclose(data_file);
  fclose(code_file);

  delete [] data;
}

void Decode_File(char * code_file_name,
                 char * data_file_name)
{
                                                                 // open files
  FILE * code_file = Open_Input_File(code_file_name);
  FILE * data_file  = Open_Output_File(data_file_name);

                                   // read file information from 12-byte header
  unsigned char header[12];
  if (fread(header, 1, 12, code_file) != 12) Error(R_MSG);
  unsigned fid   = Recover_Number(header);
  unsigned crc   = Recover_Number(header + 4);
  unsigned bytes = Recover_Number(header + 8);

  if (fid != FILE_ID) Error("invalid compressed file");

                                                  // buffer for data file data
  unsigned char * data = new unsigned char[BufferSize];
                                                            // set data models
  Adaptive_Data_Model dm[NumModels];
  for (unsigned m = 0; m < NumModels; m++) dm[m].set_alphabet(256);

  Arithmetic_Codec decoder(BufferSize);                  // set encoder buffer

  unsigned nb, new_crc = 0, context = 0;                    // decompress file
  do {

    decoder.read_from_file(code_file); // read compressed data & start decoder

    nb = (bytes < BufferSize ? bytes : BufferSize);
                                                            // decompress data
    for (unsigned p = 0; p < nb; p++) {
      data[p] = (unsigned char) decoder.decode(dm[context]);
      context = unsigned(data[p]) & (NumModels - 1);
    }
    decoder.stop_decoder();

    new_crc ^= Buffer_CRC(nb, data);                // compute CRC of new file
    if (fwrite(data, 1, nb, data_file) != nb) Error(W_MSG);

  } while (bytes -= nb);


  fclose(data_file);                                     // done: close files
  fclose(code_file);

  delete [] data;
                                                   // check if file is correct
  if (crc != new_crc) Error("incorrect file CRC");
}


void Encode_Data(unsigned char* source, int size, unsigned char* result, int& data_size)
{
	int alphabet = 256;
	unsigned nb, bytes; 

	unsigned char* data = new unsigned char[BufferSize];
	Adaptive_Data_Model dm[NumModels];
	for (unsigned m = 0; m < NumModels; m++) dm[m].set_alphabet(alphabet);

	Arithmetic_Codec encoder(BufferSize*2);                  
 
	unsigned context = 0;
	unsigned counter = 0;
	bytes = size;
	data_size = 0;
	do {
		nb = (bytes < BufferSize ? bytes : BufferSize);
		for (unsigned p = 0; p < nb; p++) {
			data[p] = source[counter++];
		}
		encoder.start_encoder();
		for (unsigned p = 0; p < nb; p++) {                       
			encoder.encode(data[p], dm[context]);
			context = unsigned(data[p]) & (NumModels - 1);
		}
		data_size = encoder.write_to_output_buffer(result, data_size);
	} while (bytes -= nb);

	delete[] data;
}

void Decode_Data(unsigned char* code, int size, unsigned char* result, int res_size)
{
	unsigned bytes = res_size;
	int alphabet = 256;
                   
	unsigned char* data = new unsigned char[BufferSize*2];

	Adaptive_Data_Model dm[NumModels];
	for (unsigned m = 0; m < NumModels; m++) dm[m].set_alphabet(alphabet);

	Arithmetic_Codec decoder(BufferSize*2); 

	unsigned nb, new_crc = 0, context = 0, counter = 0;
	int offset = 0;
	do {
		offset = decoder.read_from_input_buffer(code, offset);
		nb = (bytes < BufferSize ? bytes : BufferSize);
                                                            
		for (unsigned p = 0; p < nb; p++) {
			data[p] = decoder.decode(dm[context]);
			context = unsigned(data[p]) & (NumModels - 1);
		}
		decoder.stop_decoder();

		for (unsigned p = 0; p < nb; p++) {
			result[counter++] = data[p];
		}

	} while (bytes -= nb);

	delete[] data;
}

