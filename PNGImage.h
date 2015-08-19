#pragma once

#include <string>
#include <fstream>
#include <cstdint>
#include "Inflator.h"

using namespace std;

enum PNG_LOAD_STATUS {PNG_LOAD_OK, PNG_LOAD_FAILED, PNG_FILE_NOT_RECOGNIZED, PNG_FILE_FAILED_TO_OPEN};
enum ChunkType { IEND, IDAT, UNKNOWN };

class PNGImage
{
public:
	PNGImage(); // Our own default constructor
	~PNGImage(); // Destructor

	// Deletes the memory allocated to hold the image data
	void deleteImageData();

	// Getters
	uint32_t getWidth();
	uint32_t getHeight();
	int getBitDepth();
	string getColorType();
	string getFilename();
	unsigned char* getImageData();

	// Load the PNG image specified by the filename
	PNG_LOAD_STATUS loadFromFile(string filename);
private:
	// Gets the byte length of the data field from the next png chunk
	uint32_t getChunkDataFieldLength(ifstream& imageFile);

	// Gets the next chunk type of the PNG file
	ChunkType getChunkType(ifstream& imageFile);

	// Decode the PNG filtering, which leaves us with the actual image data (which can be used to render with OpenGL, for example)
	// Please see http://www.w3.org/TR/PNG/#9Filters to understand how I calculate the reconstructed byte values in this method
	void decodeInflatedData(unsigned char* inflationStream);

	// Get the inflated image data
	unsigned char* readInflatedImageData(ifstream& imageFile);

	// Read the image properties of PNG file (Width in pixels, height in pixels, bit depth, etc...)
	bool readImageProperties(ifstream& imageFile);

	// Check to see if the file is a PNG image
	bool isFilePNG(ifstream& imageFile);

	// Get the reversed value from a byte using the paeth filtering type
	int paethPredictor(int a, int b, int c);

	// The width of the PNG image in pixels
	uint32_t m_width;

	// The height of the PNG image in pixels
	uint32_t m_height;

	// The bit depth of the PNG image
	int m_bitDepth;

	// The color type of the PNG image
	string m_colorType;

	// The filename of the loaded PNG image
	string m_filename;

	// The raw image data (pixel array) of the loaded PNG image (use this to display the image)
	unsigned char *m_imageData;

	// The amount of bytes used to describe each pixel in the image
	const int m_bytesPerPixel = 4;
};
