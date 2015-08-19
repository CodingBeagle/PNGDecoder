#pragma once
#include <cstdint>
#include <stdlib.h>
#include "zlib.h"

class Inflator
{
public:
	Inflator(unsigned char* inputStream, uint32_t inputStreamByteLength); // Default constructor
	~Inflator(); // Destructor

	unsigned char* inflateStream();
	unsigned char* getOutputStream();
	int getInflateStatus();
private:
	unsigned char* m_inputStream;
	unsigned char* m_outputStream;
	uint32_t m_inputStreamByteLength;
	int m_inflateStatus;
};
