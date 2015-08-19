#include "Inflator.h"

#define CHUNK_SIZE 280000

Inflator::Inflator(unsigned char* inputStream, uint32_t inputStreamByteLength)
{
	m_inputStream = inputStream;
	m_inputStreamByteLength = inputStreamByteLength;
	m_outputStream = nullptr;
	m_inflateStatus = 0;
}

Inflator::~Inflator()
{
}

unsigned char* Inflator::inflateStream()
{
	m_outputStream = (unsigned char*)malloc(sizeof(unsigned char) * CHUNK_SIZE);

	// Initialize a z_stream struct with all fields to null
	z_stream decompressionStream = {};

	// Initialize the z_stream structure
	int currentOutputBufferSize = CHUNK_SIZE;
	decompressionStream.avail_in = m_inputStreamByteLength; // Set the total amount of bytes available in the input stream
	decompressionStream.next_in = m_inputStream; // Set the pointer to the next available byte of the input stream
	decompressionStream.next_out = m_outputStream; // Set the pointer to the output stream
	decompressionStream.avail_out = currentOutputBufferSize; // Set the number of available bytes in the output stream

	// Initialize inflate process
	m_inflateStatus = inflateInit(&decompressionStream);
	if (m_inflateStatus != Z_OK)
	{
		m_outputStream = nullptr;
		return m_outputStream;
	}

	do
	{
		// Stop deflation if there is no more bytes in the input stream
		if (decompressionStream.avail_in == 0)
		{
			break; 
		}

		m_inflateStatus = inflate(&decompressionStream, Z_NO_FLUSH);

		// Check for inflate errors
		// We end inflation in case of any errors
		switch (m_inflateStatus)
		{
		case Z_NEED_DICT:
			m_inflateStatus = Z_DATA_ERROR;
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			inflateEnd(&decompressionStream);
			m_outputStream = nullptr;
			break;
		}

		// If there is no more bytes available in the output stream
		// we need to increase its size
		if (decompressionStream.avail_out == 0)
		{
			int tempOutputBufferSize = currentOutputBufferSize;
			currentOutputBufferSize += CHUNK_SIZE;
			m_outputStream = (unsigned char*)realloc(m_outputStream, currentOutputBufferSize);

			decompressionStream.next_out = (m_outputStream + tempOutputBufferSize);
			decompressionStream.avail_out = CHUNK_SIZE;
		}

	} while (m_inflateStatus != Z_STREAM_END);

	// Now we end the inflate, which will free dynamically allocated structures for this stream
	inflateEnd(&decompressionStream);

	return m_outputStream;
}

int Inflator::getInflateStatus()
{
	return m_inflateStatus;
}

unsigned char* Inflator::getOutputStream()
{
	return nullptr;
}
