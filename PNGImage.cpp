#include "PNGImage.h"

PNGImage::PNGImage()
{
	m_width = 0;
	m_height = 0;
	m_bitDepth = 0;
	m_colorType = "";
	m_filename = "";
	m_imageData = nullptr;
}

PNGImage::~PNGImage()
{
	// Free allocated image data
	free(m_imageData);
	m_imageData = nullptr;
}

void PNGImage::deleteImageData()
{
	free(m_imageData);
	m_imageData = nullptr; // IMPORTANT, if our destructor calls a pointer already freed (without being nulled), behaviour is undefined
}

unsigned char* PNGImage::getImageData()
{
	return m_imageData;
}

PNG_LOAD_STATUS PNGImage::loadFromFile(string filename)
{
	ifstream imageFile;

	// We associate the stream with a file, mark the stream as binary and as used for input
	imageFile.open(filename, ios::binary | ios::in);
	if (imageFile.is_open() == false)
	{
		return PNG_FILE_FAILED_TO_OPEN;
	}

	// We initialize the stream by setting the position of the cursor to the very beginning of the stream (not sure if this is really needed, done to be sure)
	imageFile.seekg(0, ios_base::beg);

	// We check if the file is indeed of type PNG
	if (isFilePNG(imageFile) == false)
	{
		return PNG_FILE_NOT_RECOGNIZED;
	}

	// We read the general properties of the image (pixel width, pixel height, bit depth, etc...)
	if (readImageProperties(imageFile) == false)
	{
		return PNG_LOAD_FAILED;
	}

	// Now we read the inflated image data that we need to deflate
	unsigned char* inflatedImageData = readInflatedImageData(imageFile);
	if (inflatedImageData == nullptr)
	{
		return PNG_LOAD_FAILED;
	}

	// New we extract the actual image data (the pixel array we can use to display the image!)
	decodeInflatedData(inflatedImageData);

	return PNG_LOAD_OK;
}

// Read the image properties of PNG file (Width in pixels, height in pixels, bit depth, etc...)
bool PNGImage::readImageProperties(ifstream& imageFile)
{
	// The PNG specification states that all decoders must successfully understand "critical chunks".
	// The IHDR (Image Header) chunk is one of these.
	// The specification states the IHDR is the very first chunk that should appear in a PNG image file
	// The IHDR chunk contains general information about the image, such as pixel width, pixel height, bit depth, and so on

	// We skip the next 4 bytes, as these describe the length of the data field chunk, which is stated to be fixed by the specification
	imageFile.seekg(4, ios_base::cur);

	// We make sure that this is really the IHDR chunk by checking its type
	unsigned char chunkType[4] = {};
	imageFile.read(reinterpret_cast<char*>(chunkType), 4);

	if ( (chunkType[0] != 73) | (chunkType[1] != 72) | (chunkType[2] != 68) | (chunkType[3] != 82) )
	{
		return false;
	}

	// Read image width
	imageFile.read(reinterpret_cast<char*>(&m_width), 4);
	m_width = _byteswap_ulong(m_width);
	if (m_width == 0) // Zero is stated in the specification to be an invalid value
	{
		return false;
	}

	// Read image height
	imageFile.read(reinterpret_cast<char*>(&m_height), 4);
	m_height = _byteswap_ulong(m_height);
	if (m_height == 0) // Zero is stated in the specification to be an invalid value
	{
		return false;
	}

	// Read bit depth
	imageFile.read(reinterpret_cast<char*>(&m_bitDepth), 1);

	// Read color type
	unsigned char colorType = 0;
	imageFile.read(reinterpret_cast<char*>(&colorType), 1);
	if (colorType == 6)
	{
		m_colorType = "TrueColor";
	}
	else
	{
		// The decoder does not currently support any other color types than 6
		return false;
	}

	// We skip the last bytes of the IHDR data field (There was 3 bytes left) as we are not interested in those properties.
	// Additionally, we skip the 4 bytes of the CRC field (BAD decoder, bad! The CRC should really be calculated and checked against this value
	// to ensure data integrity!)
	imageFile.seekg(7, ios_base::cur);

	return true;
}

void PNGImage::decodeInflatedData(unsigned char* inflationStream)
{
	// Allocate all the bytes needed to describe all pixels in the image
	// There is 4 bytes per pixel, since you have 1 byte for each RGB value (red, green, blue)
	// Additionally, you also have 1 byte for the alpha channel.
	// Basically, we assume that the image will be in RGBA format with a bit depth of 8.
	// In this case, the bit depth meaning the number of bits used for each color component
	m_imageData = (unsigned char*)malloc(m_bytesPerPixel * (m_width * m_height));

	uint32_t imageArrayBytesPerColumn = (m_bytesPerPixel * m_width) + 1;
	uint32_t finalImageArrayBytesPerColumn = m_bytesPerPixel * m_width;

	// decode PNG filtering
	for (uint32_t currentRow = 0; currentRow < m_height; currentRow++)
	{
		uint32_t firstByteOfCurrentRow = currentRow * imageArrayBytesPerColumn;
		uint32_t firstByteOfPreviousRow = (currentRow - 1) * imageArrayBytesPerColumn;
		uint32_t filterMethodForCurrentRow = inflationStream[firstByteOfCurrentRow];

		// We start to decode not from column 0, but from column 1 (as column 0 is not image pixel data,
		// but data describing the PNG filtering method used for the current row
		for (uint32_t currentColumn = 1; currentColumn < imageArrayBytesPerColumn; currentColumn++)
		{
			uint32_t currentByteOfInflatedStream = firstByteOfCurrentRow + currentColumn;
			uint32_t currentByteInPreviousRowOfInflatedStream = firstByteOfPreviousRow + currentColumn;
			uint32_t currentBytelOfFinalImageData = (finalImageArrayBytesPerColumn * currentRow) + (currentColumn - 1);
			uint32_t currentColumnOfFinalImage = currentColumn - 1;
			uint32_t currentByteInPreviousRowOfFinalImageData = (finalImageArrayBytesPerColumn * (currentRow - 1)) + (currentColumn - 1);;

			int filteredCurrentByte = inflationStream[currentByteOfInflatedStream];
			int reconstructedByteA = 0;
			int reconstructedByteB = 0;
			int reconstructedByteC = 0;

			switch (filterMethodForCurrentRow)
			{
				case 0: // No filter method applied
				{
					// In this case, we don't need to do anything to the pixel data, we just pass it to the decoded image data
					m_imageData[currentBytelOfFinalImageData] = inflationStream[currentByteOfInflatedStream];
					break;
				}
				case 1: // Sub filter method
				{
					int reversedSubFilter = 0;

					if (currentColumnOfFinalImage > 3)
					{
						reconstructedByteA = m_imageData[currentBytelOfFinalImageData - 4];
					}

					reversedSubFilter = filteredCurrentByte + reconstructedByteA;

					m_imageData[currentBytelOfFinalImageData] = reversedSubFilter;
					break;
				}
				case 2: // Up filter method
				{
					int reversedUpFilter = 0;

					if (currentRow > 0)
					{
						reconstructedByteB = m_imageData[currentByteInPreviousRowOfFinalImageData];
					}

					reversedUpFilter = filteredCurrentByte + reconstructedByteB;

					m_imageData[currentBytelOfFinalImageData] = reversedUpFilter;
					break;
				}
				case 3: // Average filter method
				{
					int reversedAverageFilter = 0;

					if (currentColumnOfFinalImage > 3)
					{
						reconstructedByteA = m_imageData[currentBytelOfFinalImageData - 4];
					}

					if (currentRow > 0)
					{
						reconstructedByteB = m_imageData[currentByteInPreviousRowOfFinalImageData];
					}

					reversedAverageFilter = (int)(filteredCurrentByte + floor((reconstructedByteA + reconstructedByteB) / 2.0f));

					m_imageData[currentBytelOfFinalImageData] = reversedAverageFilter;

					break;
				}
				case 4: // Paeth filter method
				{
					int reversedPaethFilter = 0;

					if (currentColumnOfFinalImage > 3)
					{
						reconstructedByteA = m_imageData[currentBytelOfFinalImageData - 4];
					}

					if (currentRow > 0)
					{
						reconstructedByteB = m_imageData[currentByteInPreviousRowOfFinalImageData];
					}

					if (currentRow == 0 || currentColumnOfFinalImage <= 3)
					{
						reconstructedByteC = 0;
					}
					else
					{
						reconstructedByteC = m_imageData[currentByteInPreviousRowOfFinalImageData - 4];
					}

					reversedPaethFilter = filteredCurrentByte + paethPredictor(reconstructedByteA, reconstructedByteB, reconstructedByteC);

					m_imageData[currentBytelOfFinalImageData] = reversedPaethFilter;

					break;
				}
			}
		}
	}

	// Clean up memory
	free(inflationStream); // We have to clean up the inflated data stream given to us by our inflator
	inflationStream = nullptr;
}

unsigned char* PNGImage::readInflatedImageData(ifstream& imageFile)
{
	unsigned char* deflatedImageData = nullptr;
	bool didReachEndOfPNG = false;
	int deflatedDataLength = 0;

	while (didReachEndOfPNG == false && imageFile.eof() == false && imageFile.fail() == false)
	{
		uint32_t chunkDataFieldLength = getChunkDataFieldLength(imageFile);
		ChunkType chunkType = getChunkType(imageFile);

		// Check if we reach the PNG end chunk
		if (chunkType == ChunkType::IEND)
		{
			didReachEndOfPNG = true;
		}

		// Now we check if it's an IDAT chunk
		if (chunkType == ChunkType::IDAT)
		{
			deflatedDataLength += chunkDataFieldLength;
			deflatedImageData = (unsigned char*)realloc(deflatedImageData, sizeof(unsigned char) * deflatedDataLength);
			imageFile.read((char*)deflatedImageData, chunkDataFieldLength);
			chunkDataFieldLength = 0;
		}

		uint32_t byteLengthToNextChunk = 4 + chunkDataFieldLength;
		imageFile.seekg(byteLengthToNextChunk, ios_base::cur);
	}

	Inflator imageDataInflator(deflatedImageData, deflatedDataLength);
	unsigned char* inflatedImageData = imageDataInflator.inflateStream();

	// Clean up
	free(deflatedImageData);
	deflatedImageData = nullptr;

	return inflatedImageData;
}

uint32_t PNGImage::getChunkDataFieldLength(ifstream& imageFile)
{
	uint32_t chunkDataFieldByteLength = 0;
	imageFile.read(reinterpret_cast<char*>(&chunkDataFieldByteLength), 4);
	chunkDataFieldByteLength = _byteswap_ulong(chunkDataFieldByteLength);

	return chunkDataFieldByteLength;
}

ChunkType PNGImage::getChunkType(ifstream& imageFile)
{
	unsigned char chunkType[4] = {};
	imageFile.read(reinterpret_cast<char*>(chunkType), 4);

	// We check if the chunk type is what the user specified
	if (
		chunkType[0] == 73
		&&
		chunkType[1] == 69
		&&
		chunkType[2] == 78
		&&
		chunkType[3] == 68
		)
	{
		return ChunkType::IEND;
	}
	else if (
		chunkType[0] == 73
		&&
		chunkType[1] == 68
		&&
		chunkType[2] == 65
		&&
		chunkType[3] == 84
		)
	{
		return ChunkType::IDAT;
	}

	return ChunkType::UNKNOWN;
}

// isFilePNG checks to see if the given file is a PNG image or not
bool PNGImage::isFilePNG(ifstream& imageFile)
{
	// The PNG Specification states that the very first 8 bytes of a PNG file always contains the
	// following decimal sequence:
	// 137 80 78 71 13 10 26 10
	// You use this to make sure the decoder is actually examining a PNG file
	unsigned char pngSignature[8] = {};
	imageFile.read(reinterpret_cast<char*>(pngSignature), 8);

	if (
		pngSignature[0] == 137 
		&&
		pngSignature[1] == 80 
		&&
		pngSignature[2] == 78 
		&&
		pngSignature[3] == 71 
		&&
		pngSignature[4] == 13 
		&&
		pngSignature[5] == 10 
		&&
		pngSignature[6] == 26 
		&&
		pngSignature[7] == 10
		)
	{
		return true;
	}

	return false;
}

uint32_t PNGImage::getWidth()
{
	return m_width;
}

uint32_t PNGImage::getHeight()
{
	return m_height;
}

int PNGImage::getBitDepth()
{
	return m_bitDepth;
}

string PNGImage::getColorType()
{
	return m_colorType;
}

string PNGImage::getFilename()
{
	return m_filename;
}

int PNGImage::paethPredictor(int a, int b, int c)
{
	int Pr = 0;
	int p = a + b - c;
	int pa = abs(p - a);
	int pb = abs(p - b);
	int pc = abs(p - c);

	if (pa <= pb && pa <= pc)
	{
		Pr = a;
	}
	else if (pb <= pc)
	{
		Pr = b;
	}
	else
	{
		Pr = c;
	}

	return Pr;
}
