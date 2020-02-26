#pragma once

#include <vector>

using std::vector;

class CEncode
: public CArithmeticModel<CEncode>
{
public:
    vector<unsigned char> m_buffer;

	void PutByte(int byte)
	{
        m_buffer.push_back((unsigned char) byte);			/* now full.                */
	}
	void encode(const unsigned char* pBegin, const unsigned char* pEnd)
	{
        m_buffer.clear();
        m_buffer.reserve(1000000);
		start_model();				/* Set up other modules.    */
		start_outputing_bits();
		start_encoding();
		for (; pBegin != pEnd; ++pBegin) 
        {					/* Loop through characters. */
            int symbol = *pBegin;       /* Read the next character. */
			encode_symbol(symbol);		/* Encode that symbol.      */
		}
		encode_symbol(EOF_symbol);		/* Encode the EOF symbol.   */
		done_encoding();				/* Send the last few bits.  */
		done_outputing_bits();
	}
};

class CDecode
: public CArithmeticModel<CDecode>
{
    const unsigned char* m_pBegin;
    const unsigned char* m_pEnd;
public:

    vector<unsigned char> m_buffer;

    int GetByte()
	{
        return (m_pBegin != m_pEnd)? *m_pBegin++ : 0;
	}

	void decode(const unsigned char* pBegin, const unsigned char* pEnd)
	{
        m_buffer.clear();
        m_buffer.reserve(1000000);
        m_pBegin = pBegin;
        m_pEnd = pEnd;
		start_model();				/* Set up other modules.    */
		start_inputing_bits();
		start_decoding();
		for (;;) 
        {					/* Loop through characters. */
            int symbol = decode_symbol();	/* Decode next symbol.      */
			if (symbol==EOF_symbol) 
                break;		/* Exit loop if EOF symbol. */
            m_buffer.push_back((unsigned char) symbol);
		}
	}
};
