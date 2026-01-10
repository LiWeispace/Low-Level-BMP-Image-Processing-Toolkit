#include <iostream>
#include <fstream>
#include <vector>

// Enforce 1-byte structure alignment to match the standard BMP binary layout.
#pragma pack(1)

using namespace std;

// Standard BMP File Header
struct BMPFileHeader {
    uint16_t bfType;      // Magic number 'BM' (0x4D42).
    uint32_t bfSize;      // File size in bytes.
    uint16_t bfReserved1; // Reserved.
    uint16_t bfReserved2; // Reserved.
    uint32_t bfOffBits;   // Offset to the start of pixel data.
};

// Standard BMP Information Header (DIB Header)
struct BMPInfoHeader {
    uint32_t biSize;          // Header size.
    int32_t biWidth;          // Image width.
    int32_t biHeight;         // Image height.
    uint16_t biPlanes;        // Number of color planes (must be 1).
    uint16_t biBitCount;      // Bits per pixel (e.g., 24, 32).
    uint32_t biCompression;   // Compression type (0 = uncompressed).
    uint32_t biSizeImage;     // Image size in bytes.
    int32_t biXPelsPerMeter;  // Horizontal resolution.
    int32_t biYPelsPerMeter;  // Vertical resolution.
    uint32_t biClrUsed;       // Number of colors in palette.
    uint32_t biClrImportant;  // Important colors.
};

/**
 * Quantization Function: Reduces the color depth of RGB channels.
 * * This function maps the continuous [0, 255] pixel intensity range to a smaller set of 
 * discrete levels determined by the target bit depth.
 */
void quantizePixelData(vector<uint8_t>& pixelData, int bytesPerPixel, int width, int height, int rowSize, int quantizationBits) {
    // Calculate the number of discrete levels based on the target bit depth (2^n)
    int levels = 1 << quantizationBits;
    
    // Define the maximum pixel intensity for 8-bit channels
    int maxValue = 255;
    
    // Calculate the quantization step factor (scaling factor)
    // This maps the 0-255 range to the new reduced range.
    int factor = maxValue / (levels - 1); 

    for (int y = 0; y < height; y++) {
        // Get pointer to the start of the current row
        uint8_t* row = &pixelData[y * rowSize];
        
        for (int x = 0; x < width; ++x) {
            // Iterate through RGB channels (assuming BGR/BGRA order)
            // Limit loop to 3 to process only color channels and preserve Alpha if present.
            for (int byte = 0; byte < 3; ++byte) {
                uint8_t& colorValue = row[x * bytesPerPixel + byte];
                
                // Apply uniform quantization:
                // 1. Divide by factor to find the nearest bin index.
                // 2. Multiply back by factor to reconstruct the quantized value.
                colorValue = (colorValue / factor) * factor;
            }
        }
    }
}

int main() {
    const char* inputFileName = "images/input2.bmp";
    const char* outputFileName_6bit = "output2_1.bmp";
    const char* outputFileName_4bit = "output2_2.bmp";
    const char* outputFileName_2bit = "output2_3.bmp";

    // Open the input BMP file in binary mode
    ifstream inputFile(inputFileName, ios::binary);
    if(!inputFile) {
        cerr << "Can't open file." << endl;
        return 1;
    }

    // Read the BMP File Header
    BMPFileHeader fileHeader;
    inputFile.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

    // Validate the BMP file signature (Magic Number 0x4D42)
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

    // Ensure support only for 24-bit (RGB) and 32-bit (RGBA) uncompressed formats
    if (bitCount != 24 && bitCount != 32 || infoHeader.biCompression != 0) {
        cerr << "Only supports 24-bit or 32-bit uncompressed BMP." << endl;
        return 1;
    }

    // Calculate row stride with 4-byte alignment (padding)
    int rowSize = ((width * bytesPerPixel + 3) & (~3));

    // Allocate buffer and read raw pixel data
    vector<uint8_t> pixelData(rowSize * height);
    inputFile.seekg(fileHeader.bfOffBits, ios::beg);
    inputFile.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
    inputFile.close();

    // Process: 6-bit Quantization
    vector<uint8_t> pixelData_6bit = pixelData;
    quantizePixelData(pixelData_6bit, bytesPerPixel, width, height, rowSize, 6);
    
    ofstream outputFile_6bit(outputFileName_6bit, ios::binary);
    outputFile_6bit.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    outputFile_6bit.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    outputFile_6bit.write(reinterpret_cast<char*>(pixelData_6bit.data()), pixelData_6bit.size());
    outputFile_6bit.close();

    // Process: 4-bit Quantization
    vector<uint8_t> pixelData_4bit = pixelData;
    quantizePixelData(pixelData_4bit, bytesPerPixel, width, height, rowSize, 4);
    
    ofstream outputFile_4bit(outputFileName_4bit, ios::binary);
    outputFile_4bit.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    outputFile_4bit.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    outputFile_4bit.write(reinterpret_cast<char*>(pixelData_4bit.data()), pixelData_4bit.size());
    outputFile_4bit.close();

    // Process: 2-bit Quantization
    vector<uint8_t> pixelData_2bit = pixelData;
    quantizePixelData(pixelData_2bit, bytesPerPixel, width, height, rowSize, 2);
    
    ofstream outputFile_2bit(outputFileName_2bit, ios::binary);
    outputFile_2bit.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    outputFile_2bit.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    outputFile_2bit.write(reinterpret_cast<char*>(pixelData_2bit.data()), pixelData_2bit.size());
    outputFile_2bit.close();

    cout << "Quantization successful!" << endl;
    return 0;
}