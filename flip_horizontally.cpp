#include <iostream>
#include <fstream>
#include <vector>

// Enforce 1-byte structure alignment to ensure the struct layout matches the binary file format.
#pragma pack(1)

using namespace std;

// Standard BMP File Header structure
struct BMPFileHeader {
    uint16_t bfType;      // Magic number for file type, must be 0x4D42 ('BM').
    uint32_t bfSize;      // Total size of the file in bytes.
    uint16_t bfReserved1; // Reserved field, must be 0.
    uint16_t bfReserved2; // Reserved field, must be 0.
    uint32_t bfOffBits;   // Offset to the beginning of the pixel data (bitmap bits).
};

// Standard BMP Information Header structure (DIB Header)
struct BMPInfoHeader {
    uint32_t biSize;          // Size of this header structure in bytes.
    int32_t biWidth;          // Width of the image in pixels.
    int32_t biHeight;         // Height of the image in pixels.
    uint16_t biPlanes;        // Number of color planes, must be 1.
    uint16_t biBitCount;      // Number of bits per pixel (BPP), e.g., 24 or 32.
    uint32_t biCompression;   // Compression method (0 indicates uncompressed).
    uint32_t biSizeImage;     // Size of the raw bitmap data in bytes.
    int32_t biXPelsPerMeter;  // Horizontal resolution (pixels per meter).
    int32_t biYPelsPerMeter;  // Vertical resolution (pixels per meter).
    uint32_t biClrUsed;       // Number of color indexes in the color table (0 for max).
    uint32_t biClrImportant;  // Number of important color indexes (0 for all).
};

// Function to perform an in-place horizontal flip of the image data
void flipHorizontally(vector<uint8_t>& pixelData, int width, int height, int rowSize, int bytesPerPixel) {
    for (int y = 0; y < height; ++y) {
        // Get the pointer to the beginning of the current row
        uint8_t* row = &pixelData[y * rowSize];
        
        // Iterate through half of the row width to swap pixels
        for (int x = 0; x < width / 2; ++x) {
            for (int byte = 0; byte < bytesPerPixel; ++byte) {
                // Swap the pixel components (B, G, R) between the left and right sides
                swap(row[x * bytesPerPixel + byte], row[(width - x - 1) * bytesPerPixel + byte]);
            }
        }
    }
}

int main() {
    const char* inputFileName = "images/input1.bmp";
    const char* outputFileName = "output1_filp.bmp";

    // Open the input BMP file in binary mode
    ifstream inputFile(inputFileName, ios::binary);
    if (!inputFile) {
        cerr << "Can't open file." << endl;
        return 1;
    }

    // Read the BMP File Header
    BMPFileHeader fileHeader;
    inputFile.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

    // Validate the file signature (Magic Number 0x4D42)
    if (fileHeader.bfType != 0x4D42) {
        cerr << "Input file is not a BMP file." << endl;
        return 1;
    }

    // Read the BMP Information Header
    BMPInfoHeader infoHeader;
    inputFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int bitCount = infoHeader.biBitCount;

    int bytesPerPixel = bitCount / 8;

    // Validate support for 24-bit and 32-bit uncompressed formats only
    if ((bitCount != 24 && bitCount != 32) || infoHeader.biCompression != 0) {
        cerr << "Only supports 24-bit or 32-bit uncompressed BMP." << endl;
        return 1;
    }

    // Calculate the row stride (row size in bytes), ensuring 4-byte alignment (padding).
    // The formula ((width * bytesPerPixel + 3) & (~3)) aligns the size to the next multiple of 4.
    int rowSize = ((width * bytesPerPixel + 3) & (~3));

    // Allocate buffer and read pixel data
    vector<uint8_t> pixelData(rowSize * height);
    
    // Move file pointer to the start of pixel data
    inputFile.seekg(fileHeader.bfOffBits, ios::beg);
    inputFile.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
    inputFile.close();

    // Perform horizontal flip
    flipHorizontally(pixelData, width, height, rowSize, bytesPerPixel);

    // Open the output file in binary mode
    ofstream outputFile(outputFileName, ios::binary);
    if (!outputFile) {
        cerr << "Can't open file." << endl;
        return 1;
    }

    // Write the headers to the new file
    outputFile.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    outputFile.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    // Write the modified pixel data
    outputFile.write(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
    outputFile.close();

    cout << "The file is successful!" << endl;
    return 0;
}