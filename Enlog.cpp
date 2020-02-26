// Enlog.cpp : Defines the entry point for the console application.
//
// This is DEMO of the concept of ANS encoder suggested by Jarek Duda at 
// http://demonstrations.wolfram.com/DataCompressionUsingAsymmetricNumeralSystems/
// and reduced to practice by Andrew Polar. Theoretical description can be found 
// on ezcodesample.com. Current version is experimental. It needs computational 
// stability test. You can use it on your own risk. No warranty but in case
// of wrong encoding author can give a favor and investigate the problem. 
// Any feedback is appreciated andrewpolar@bellsouth.net
//
// First  version May 24, 2008
// Second version June 2, 2008
// This version   Aug. 8, 2008
//
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <conio.h>
#include <time.h>
#include <math.h>

//this parameters is a trade off between compression ratio and speed.
//the higher the value the better compression.
const unsigned char PRECISION = 4; //must not be less than 2

int bitlen(unsigned D) {

	int len = 0;
	while (D > 0) {
		D >>= 1;
		++len;
	}
	return len;
}

void GetOrder(int* data, int* order, int size) {
	
	int min = data[0];
	int max = data[0];
	for (int i=0; i<size; ++i) {
		if (min > data[i])
			min = data[i];
		if (max < data[i])
			max = data[i];
	}
	if (min == max) {
		for (int i=0; i<size; ++i)
			order[i] = i;
		return;
	}
	++max;

	int* tmp = (int*)malloc(size*sizeof(int));
	memcpy(tmp, data, size*sizeof(int));
	
	for (int k=0; k<size; ++k) {
		int pos = 0;
		min = tmp[pos];
		for (int i=0; i<size; ++i) {
			if (min > tmp[i]) {
				min = tmp[i];
				pos = i;
			}
		}
		order[k] = pos;
		tmp[pos] = max;
	}

	if (tmp)
		free(tmp);

}

void CollectStatistics(int* data, int data_size, int* freq, int nSymbols, int PROBABILITY_PRECISION) {

	memset(freq, 0x00, nSymbols*sizeof(int));
	for (int i=0; i<data_size; ++i)
		++freq[data[i]];
	double coef = (double)(1<<PROBABILITY_PRECISION);
	coef /= double(data_size);
	for (int j=0; j<nSymbols; ++j) {
		freq[j] = (int)(double(freq[j])*coef);
	}
	int total = 0;
	for (int k=0; k<nSymbols; ++k) {
		total += freq[k];
	}
	int diff = (1<<PROBABILITY_PRECISION) - total;
	
	int maxPos = 0;
	int max = freq[maxPos];
	for (int k=0; k<nSymbols; ++k) {
		if (max < freq[k]) {
			max = freq[k];
			maxPos = k;
		}
	}
	freq[maxPos] += diff;

	for (int k=0; k<nSymbols; ++k) {
		if (freq[k] == 0) {
			freq[k] = 1;
			freq[maxPos]--;
		}
	}
}

void MakeChart(int* chart, int* freq, int* sizes, int nSymbols, int PROBABILITY_PRECISION,
			   int MAX_STATE) {

	for (int i=0; i<nSymbols; ++i) {
		sizes[i] = 1;  //this is because table starts from 1 not from 0
	}

	int* order = (int*)malloc(nSymbols*sizeof(int));
	GetOrder(freq, order, nSymbols);

	int denominator = 1<<PROBABILITY_PRECISION;
	for (int k=0; k<=MAX_STATE; ++k)
		chart[k] = denominator;

    int state;
	for (int j=1; j<=MAX_STATE; ++j) {
		for (int i=0; i<nSymbols; ++i) {
			state = (j << PROBABILITY_PRECISION)/freq[order[i]]; 
			if (state < 2)
				state = 2;
			if (state <= MAX_STATE) {
				if (chart[state] == denominator) {
					chart[state] = order[i];
					++sizes[order[i]];
				}
				else {
					do {
						++state;
						if (state > MAX_STATE)
							break;
						if (chart[state] == denominator) {
							chart[state] = order[i];
							++sizes[order[i]];
							break;
						}
					}while (true);
				}
			}
		}
	}

	if (order)
		free(order);
}

void MakeDESCENDTable(int** descend, int* chart, int* sizes, int nSymbols, int PROBABILITY_PRECISION,
					  int MAX_STATE) {

	int denominator = 1<<PROBABILITY_PRECISION;
	int* order = (int*)malloc(nSymbols*sizeof(int));
	for (int k=0; k<nSymbols; ++k)
		order[k] = 1;

	for (int j=2; j<=MAX_STATE; ++j) {
		if (chart[j] != denominator) {
			descend[chart[j]][order[chart[j]]++] = j;
		}
	}

	if (order)
		free(order);
}

void MakeASCENDTable(int** descend, int* ascendSymbol,
					 int* ascendState, int nSymbols, int* sizes) {

	ascendSymbol[0] = 0;
	ascendState [0] = 0;
	for (int i=0; i<nSymbols; ++i) {
		int counter = 1;
		int j = 1;
		for (int k=0; k<sizes[i]-1; ++k) {
			ascendSymbol[descend[i][j]] = i;
			ascendState [descend[i][j]] = counter++;
			++j;
		}
	}
}

bool EncodeData(int* source, int data_size, int min, int max, unsigned char* result, int& res_size) {
	
	int nSymbols = max - min + 1;
	int PROBABILITY_PRECISION = bitlen(nSymbols) + PRECISION;
	int STATE_PRECISION = PROBABILITY_PRECISION + 1;
	int MAX_STATE = (1<<STATE_PRECISION)-1;

	int* freq = (int*)malloc(nSymbols*sizeof(int));
	CollectStatistics(source, data_size, freq, nSymbols, PROBABILITY_PRECISION);
	//printf("Relative freqency distribution\n");
	//for (int i=0; i<nSymbols; i++) {
	//	printf("%10d", freq[i]);
	//}
	//printf("\n\n");

	//output normalized frequencies and sizes
	res_size = 0;
	memcpy(result + res_size, &nSymbols, 2);
	res_size += 2;

	for (int i=0; i<nSymbols; ++i) {
		memcpy(result + res_size, &freq[i], 2);
		res_size += 2;
	}

	memcpy(result + res_size, &data_size, 4);
	res_size += 4;

	//Make CHART
	int* chart = (int*)malloc((MAX_STATE+1) * sizeof(int));
	int* sizes = (int*)malloc(nSymbols*sizeof(int));
	MakeChart(chart, freq, sizes, nSymbols, PROBABILITY_PRECISION, MAX_STATE);
	//for (int i=0; i<MAX_STATE+1; ++i) {
	//	printf("%d %d\n", i, chart[i]);
	//}
	//printf("Sizes for DESCEND table\n");
	//for (int i=0; i<nSymbols; i++) {
	//	printf("%10d", sizes[i]-1);
	//}
	//printf("\n\n");

	//Make DESCEND table
	int** descend = (int**)malloc(nSymbols * sizeof(int*));
	for (int i=0; i<nSymbols; ++i) {
		descend[i] = (int*)malloc((sizes[i]) * sizeof(int));
		memset(descend[i], 0x00, (sizes[i]) * sizeof(int));
	}

	MakeDESCENDTable(descend, chart, sizes, nSymbols, PROBABILITY_PRECISION, MAX_STATE);
	//printf("DESCEND table\n");
	//bool isPrinted;
	//for (int i=1; i<=MAX_STATE; ++i) {
	//	isPrinted = false;
	//	for (int j=0; j<nSymbols; ++j) {
	//		if (i < sizes[j]) {
	//			printf("%10d", descend[j][i]);
	//			isPrinted = true;
	//		}
	//		else
	//			printf("          ");
	//	}
	//	printf("\n");
	//	if (isPrinted == false)
	//		break;
	//}

	//encoding
	bool encodingIsCorrect = true;
	int state = MAX_STATE;
	unsigned char byte = 0;
	unsigned char bit  = 0;
	int byte_size = 0;
	int control_MASK = 1 << (STATE_PRECISION-1);
	for (int i=0; i<data_size; ++i) {
		while (state > sizes[source[i]]-1) {
			++bit;
			byte |= state & 1;
			if (bit == 8) {
				result[res_size++] = byte;
				bit = 0;
				byte = 0;
			}
			else {
				byte <<=1;
			}
			state >>= 1;
		}
		state = descend[source[i]][state];
		if (state < control_MASK) {
			printf("problem with data symbol %d %d %d\n", i, source[i], state);
			encodingIsCorrect = false;
			break;
		}
	}

	int byte_offset; 
	if (bit == 0) {
		byte_offset = 0;
	}
	else {
		byte_offset = 8 - bit;
		result[res_size++] = byte << (7 - bit);
	}

	memcpy(result + res_size, &byte_offset, 1);
	res_size += 1;
	
	memcpy(result + res_size, &byte_size, 4);
	res_size += 4;

	memcpy(result + res_size, &state, 4);
	res_size += 4;
	//end encoding

	if (freq)
		free(freq);

	if (descend) {
		for (int i=0; i<nSymbols; ++i) {
			if (descend[i])
				free(descend[i]);
		}
		free(descend);
		descend = 0;
	}

	if (sizes)
		free(sizes);

	if (chart)
		free(chart);
	
	return encodingIsCorrect;
}

void DecodeData(unsigned char* data, int data_size, int* decoded, int& decoded_size) {

	int nSymbols = 0;
	int offset = 0;
	memcpy(&nSymbols, data + offset, 2);
	offset += 2;

	int* freq = (int*)malloc(nSymbols*sizeof(int));
	memset(freq, 0x00, nSymbols*sizeof(int));
	for (int i=0; i<nSymbols; ++i) {
		memcpy(&freq[i], data + offset, 2);
		offset += 2;
	}
	
	int PROBABILITY_PRECISION = bitlen(nSymbols) + PRECISION;
	int STATE_PRECISION = PROBABILITY_PRECISION + 1;
	int MAX_STATE = (1<<STATE_PRECISION)-1;

	decoded_size = 0;
	memcpy(&decoded_size, data + offset, 4);

	//Make CHART
	int* chart = (int*)malloc((MAX_STATE+1) * sizeof(int));
	int* sizes = (int*)malloc(nSymbols*sizeof(int));
	MakeChart(chart, freq, sizes, nSymbols, PROBABILITY_PRECISION, MAX_STATE);

	//Make DESCEND table
	int** descend = (int**)malloc(nSymbols * sizeof(int*));
	for (int i=0; i<nSymbols; ++i) {
		descend[i] = (int*)malloc((sizes[i]) * sizeof(int));
		memset(descend[i], 0x00, (sizes[i]) * sizeof(int));
	}

	MakeDESCENDTable(descend, chart, sizes, nSymbols, PROBABILITY_PRECISION, MAX_STATE);
	
	//Making of ASCEND table
	int* ascendSymbol = (int*)malloc((MAX_STATE+1)*sizeof(int));
	int* ascendState  = (int*)malloc((MAX_STATE+1)*sizeof(int));
	MakeASCENDTable(descend, ascendSymbol, ascendState, nSymbols, sizes);

	//decoding
	int state;
	memcpy(&state, data + data_size - 4, 4);
	int byte_size;
	memcpy(&byte_size, data + data_size - 8, 4);
	int byte_offset;
	memcpy(&byte_offset, data + data_size - 9, 1);

	int MASK = 1<<(STATE_PRECISION-1);
	unsigned char shift = byte_offset;
	int counter = data_size - 10;
	for (int i=decoded_size-1; i>=0; --i) {
		decoded[i] = ascendSymbol[state];
		state  = ascendState[state];
		while (state < MASK) {
			state <<= 1;
			state |= (data[counter] & (1<<shift))>>shift;
			++shift;
			if (shift == 8) {
				shift = 0;
				--counter;
			}
		}
	}
	//end decoding

	if (ascendSymbol)
		free(ascendSymbol);
	if (ascendState)
		free(ascendState);

	if (descend) {
		for (int i=0; i<nSymbols; ++i) {
			if (descend[i])
				free(descend[i]);
		}
		free(descend);
		descend = 0;
	}

	if (sizes)
		free(sizes);

	if (chart)
		free(chart);

	if (freq)
		free(freq);
}

