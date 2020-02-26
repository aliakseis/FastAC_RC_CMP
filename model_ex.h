#pragma once

#include <iostream>

#include <emmintrin.h>
#include <assert.h>


/*
This is based on the adaptive arithmetic coding software by R. M. Neal that 
is contained in the following reference:

  Witten, I. H., Neal, R. M., and Cleary, J. G. (1987) 
    "Arithmetic coding for data compression", Communications 
    of the ACM, vol. 30, no. 6 (June).
*/

/* INTERFACE TO THE MODEL. */

/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */


typedef long code_value;		/* Type of an arithmetic code value */

enum {
	Code_value_bits = 32,		/* Number of bits in a code value   */

/* THE SET OF SYMBOLS THAT MAY BE ENCODED. */
	No_of_chars = 256,					/* Number of character symbols      */
	EOF_symbol = (No_of_chars),		/* Index of EOF symbol              */
	No_of_symbols = (No_of_chars+1),	/* Total number of symbols          */

	Max_frequency = 0x3FFFFFFF,		/* Maximum allowed frequency count  */
};


/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
const unsigned long First_qtr = 1 << (Code_value_bits - 2); /* Point after first quarter        */
const unsigned long Half	= (2*First_qtr);				/* Point after first half           */

//warning C4127: conditional expression is constant
//warning C4710: function '...' not inlined
#pragma warning(disable:4710 4127)

const char Table256[] = {
	8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

inline int num_clear_bits(unsigned long v)
{
	unsigned int t, tt; // temporaries

	if (tt = v >> 16)
	{
		return (t = tt >> 8) ? Table256[t] : 8 + Table256[tt];
	}
	else 
	{
		return (t = v >> 8) ? 16 + Table256[t] : 24 + Table256[v];
	}
}

#pragma warning(disable:4035)

__declspec(naked) unsigned long __fastcall mul_div_(unsigned long, unsigned long)
{
	__asm   mul edx
	__asm   div ecx
	__asm	ret 
}

inline unsigned long mul_div(unsigned long nNumber,
                            unsigned long nNumerator,
                            unsigned long nDenominator)
{
    __asm    mov eax, nNumber
	return mul_div_(nDenominator, nNumerator);
}

__declspec(naked) unsigned long __fastcall mul_dec_div_(unsigned long, unsigned long)
{
	__asm   mul edx
	__asm   sub eax, 1
	__asm   sbb edx, 0
	__asm   div ecx
	__asm	ret 
}

inline unsigned long mul_dec_div(unsigned long nNumber,
								 unsigned long nNumerator,
								 unsigned long nDenominator)
{
	__asm    mov eax, nNumber
	return mul_dec_div_(nDenominator, nNumerator);
}

inline unsigned long mul_dec_hidword(unsigned long nNumber,
                            unsigned long nNumerator)
{
    __asm    mov eax, nNumber
    __asm    mul nNumerator
    __asm    sub eax, 1
    __asm    sbb edx, 0
    __asm    mov eax, edx
}

inline unsigned long hidword_div(unsigned long nNumber,
                            unsigned long nDenominator)
{
    __asm    xor eax, eax
    __asm    mov edx, nNumber
    __asm    div nDenominator
}

int __fastcall FastBSF(int n) { __asm bsf eax, ecx }

#pragma warning(default:4035)


template<typename T>
class CArithmeticModel
{
public:
	void start_model()
	{
        cum_freq_tree[0] = 0;
        for (int i = 1; i < sizeof(cum_freq_tree) / sizeof(cum_freq_tree[0]); ++i)
            cum_freq_tree[i] = 1 << FastBSF(i);
	}
	void update_model(int symbol_)
	{   
        int symbol = symbol_ + 1;
		if (cum_freq_tree[No_of_chars]==Max_frequency-1) {		/* TODO See if frequency counts  */
        }

        while (symbol <= No_of_chars){
		    cum_freq_tree[symbol]++;
		    symbol += (symbol & -symbol);
	    }
	}
    template <int symbol>
    void update_model()
    {
		if (cum_freq_tree[No_of_chars]==Max_frequency-1) {		/* TODO See if frequency counts  */
        }
        return do_update_model<symbol + 1>();
    }
    template <int symbol>
    __forceinline
    void do_update_model()
    {
        if (symbol > No_of_chars)
            return;
        
	    cum_freq_tree[symbol]++;
        do_update_model<(symbol <= No_of_chars)? symbol + (symbol & -symbol) : symbol>();
    }

	void start_encoding()
	{   
		low = 0;					/* Full code range.         */
		not_high = 0;
		bits_to_follow = 0;			/* No bits to follow next.  */
	}

	void encode_symbol(int symbol)	/* Symbol to encode                         */
	{   
        unsigned long range = -not_high - low; /* Size of the current code region          */
        if (range != 0)
        {
			UpdateBounds(symbol, range);
        }
        else
        {
		    not_high = -low -							/* Narrow the code region   */
			    (code_value)hidword_div(cum_freq(symbol+1), cum_freq_tree[No_of_chars]+1);	/* to that allotted to this */
		    low = low + 								/* symbol.                  */
			    (code_value)hidword_div(cum_freq(symbol), cum_freq_tree[No_of_chars]+1);

            if (symbol != EOF_symbol)
                update_model(symbol);
        }

		long half_flags = ~(low | not_high);
        if (half_flags >= 0)
        {
            int shift = num_clear_bits(half_flags);
            if (bits_to_follow)
            {
                //*
                if (bits_to_follow + shift > 32)
                {
                    bit_plus_follow(low & Half);
                    if (shift > 1)
                        output_bits(low << 1, shift - 1);
                }
                else
                //*/
                {
                    long data = (long(low ^ 0x80000000) >> bits_to_follow) ^ 0x80000000;
                    output_bits(data, bits_to_follow + shift);
                    bits_to_follow = 0;
                }
            }
            else
                output_bits(low, shift);

			low <<= shift;
			not_high <<= shift;
		}

		if (First_qtr & low & not_high)
		{
			do
			{
				bits_to_follow++;
				low <<= 1;
				not_high <<= 1; 
			}
			while (First_qtr & low & not_high);
			low += Half;
			not_high -= Half;
		}
	}
	void done_encoding()
	{   
		bits_to_follow += 1;					/* Output two bits that     */
		bit_plus_follow(low >= First_qtr);		/* select the quarter that  */
	}

	void start_decoding()
	{   
        value = input_bits(Code_value_bits);
		low = 0;								/* Full code range.         */
		not_high = 0;
	}
	int decode_symbol()
	{   
		int symbol;			/* Symbol decoded                           */
        unsigned long range = -not_high - low; /* Size of the current code region          */
        if (range != 0)
        {
            int cum = mul_dec_div(value-low+1, cum_freq_tree[No_of_chars]+1, range);
            symbol = find(cum);

			UpdateBounds(symbol, range);
        }
        else
        {
            unsigned long rel_value = value-low+1;
            if (0 == rel_value)
                symbol = 0;
            else
            {
                int cum = mul_dec_hidword(rel_value, cum_freq_tree[No_of_chars]+1);
                symbol = find(cum);
            }
		    not_high = -low -							/* Narrow the code region   */
			    (code_value)hidword_div(cum_freq(symbol+1), cum_freq_tree[No_of_chars]+1);	/* to that allotted to this */
		    low = low + 								/* symbol.                  */
			    (code_value)hidword_div(cum_freq(symbol), cum_freq_tree[No_of_chars]+1);

            if (symbol != EOF_symbol)
                update_model(symbol);
        }

		long half_flags = ~(low | not_high);
        int shift = 0;
        if (half_flags >= 0)
        {
    		shift = num_clear_bits(half_flags);
		}
        long qtr_flags = (~(low & not_high)) << (shift + 1);
        if (qtr_flags >= 0)
        {
    		shift += num_clear_bits(qtr_flags);
            value = (value << shift) | input_bits(shift);
			low <<= shift;
			not_high <<= shift;

			low += Half;
			not_high -= Half;
			value -= Half;
        }
        else
        {
            value = (value << shift) | input_bits(shift);
			low <<= shift;
			not_high <<= shift;
        }

		return symbol;
	}

	void start_inputing_bits()
	{   
        unsigned long buffer = GetByte_() << 24;
        buffer |= GetByte_() << 16;
        buffer |= GetByte_() << 8;
        buffer |= GetByte_();

        ullBuffer = buffer;
		bits_to_go = 32;					/* Buffer starts out with   */
    }									/* no bits in it.           */

    unsigned long input_bits(int count)
    {
        if (bits_to_go <= 32)
        {
            unsigned long buffer = GetByte_() << 24;
            buffer |= GetByte_() << 16;
            buffer |= GetByte_() << 8;
            buffer |= GetByte_();

            ullBuffer = ((unsigned long long)(unsigned long) ullBuffer) << 32 | buffer;

            bits_to_go += 32;
        }

        bits_to_go -= count;

        unsigned long result = unsigned long(ullBuffer >> (bits_to_go));

        return result;
    }

	void start_outputing_bits()
	{   
		buffer = 0;					/* Buffer is empty to start */
		bits_to_go= 8;				/* with.                    */
	}
	void done_outputing_bits()
	{   
		PutByte_(buffer << bits_to_go);
	}

	int GetByte_()
	{
		return static_cast<T*>(this)->GetByte();
	}
	void PutByte_(int byte)
	{
		static_cast<T*>(this)->PutByte(byte);
	}

    int cum_freq_tree[No_of_symbols+1];		/* Cumulative symbol frequencies    */

protected:
	void bit_plus_follow(long bit)
	{   
		output_bit(bit);				/* Output the bit.          */
		while (bits_to_follow>0) {
			/* Output bits_to_follow    */
			buffer <<= 1; 
			if (!bit)
				buffer |= 1;			/* Put bit in top of buffer.*/
			if (--bits_to_go==0) {		/* Output buffer if it is   */
				flush_bits_to_follow(bit? 0 : 255);
				return;
			}
			bits_to_follow--;			/* opposite bits. Set       */
		}								/* bits_to_follow to zero.  */
	}

	void output_bit(long bit)
	{   
		buffer <<= 1; 
		if (bit)
			buffer |= 1;				/* Put bit in top of buffer.*/
		if (--bits_to_go==0) {			/* Output buffer if it is   */
			PutByte_(buffer);			/* now full.                */
			bits_to_go = 8;
		}
	}

	void output_bits(unsigned long bits, int count)
    {
        if (count < bits_to_go)
        {
    		buffer <<= count; 
            buffer |= bits >> (32 - count);
            bits_to_go -= count;
        }
        else
        {
    		buffer <<= bits_to_go; 
            buffer |= bits >> (32 - bits_to_go);

            bits <<= bits_to_go;
            count -= bits_to_go;

            for (;;)
            {
    			PutByte_(buffer);
			    if (count < 8)
				    break;

                buffer = bits >> 24;
                bits <<= 8;

			    count -= 8;
            }

            buffer = bits >> (32 - count);
    		bits_to_go = 8 - count;
        }
    }

private:
	void flush_bits_to_follow(int buffer_)
	{
		for (;;)
		{
			PutByte_(buffer);			/* now full.                */
			buffer = buffer_;
			if (bits_to_follow <= 8)
				break;
			bits_to_follow -= 8;
		}
		bits_to_go = 9 - bits_to_follow;
		bits_to_follow = 0;
	}

    int cum_freq(int idx)
    {
	    int sum = 0;
	    while (idx > 0){
		    sum += cum_freq_tree[idx];
            idx &= idx - 1;
	    }
	    return sum;
    }

    template <int idx>
    __forceinline
    int cum_freq()
    {
        if (0 == idx)
            return 0;

        return cum_freq_tree[idx] + cum_freq<idx & (idx - 1)>();
    }


    template<int idx, int bitMask>
    __forceinline
    int DoFind(int cumFre)
    {
        if (0 == bitMask)
            return idx;

        enum { tIdx = idx + bitMask };
        if (cumFre == cum_freq_tree[tIdx]) 
			return tIdx; 
	    else if (cumFre > cum_freq_tree[tIdx])
            return DoFind<tIdx, bitMask/2>(cumFre - cum_freq_tree[tIdx]);
        else
            return DoFind<idx, bitMask/2>(cumFre);
    }

    int find(int cumFre) 
    { 
        if (cumFre >= cum_freq_tree[No_of_chars])
            return EOF_symbol;

        return DoFind<0, 128>(cumFre); 
    }

	code_value low, not_high;	/* Ends of the current code region          */
	long bits_to_follow;		/* Number of opposite bits to output after  */
								/* the next bit.                            */
	unsigned long value;		/* Currently-seen code value                */

	int buffer;					/* Bits waiting to be input                 */
	int bits_to_go;				/* Number of bits still in buffer           */

    unsigned long long ullBuffer;

#define HANDLE_GROUPS_3(a, b, c) \
HANDLE_GROUPS_4(a, b, c, 0) \
HANDLE_GROUPS_4(a, b, c, 1) \
HANDLE_GROUPS_4(a, b, c, 2) \
HANDLE_GROUPS_4(a, b, c, 3)

#define HANDLE_GROUPS_2(a, b) \
HANDLE_GROUPS_3(a, b, 0) \
HANDLE_GROUPS_3(a, b, 1) \
HANDLE_GROUPS_3(a, b, 2) \
HANDLE_GROUPS_3(a, b, 3)


#define HANDLE_GROUPS_1(a) \
HANDLE_GROUPS_2(a, 0) \
HANDLE_GROUPS_2(a, 1) \
HANDLE_GROUPS_2(a, 2) \
HANDLE_GROUPS_2(a, 3)


#define HANDLE_GROUPS_4(a, b, c, d) \
    case (a * 64 + b * 16 + c * 4 + d): \
    { \
        enum { value = (a * 64 + b * 16 + c * 4 + d) }; \
        const int cf_all = cum_freq_tree[No_of_chars]+1; \
        not_high = -low - (code_value)mul_div(range, cum_freq<value+1>(), cf_all); \
	    low = low + (code_value)mul_div(range, cum_freq<value>(), cf_all); \
        update_model<value>(); \
        break; \
    }


	void UpdateBounds(int symbol, unsigned long range)
	{
        switch (symbol)
        {
            HANDLE_GROUPS_1(0)
            HANDLE_GROUPS_1(1)
            HANDLE_GROUPS_1(2)
            HANDLE_GROUPS_1(3)
        case EOF_symbol:
            {
                not_high = -low - (code_value)mul_div(range, cum_freq<EOF_symbol+1>(), cum_freq_tree[No_of_chars]+1); 
	            low = low + (code_value)mul_div(range, cum_freq<EOF_symbol>(), cum_freq_tree[No_of_chars]+1); 
                break;
            }
        }
    }
};
