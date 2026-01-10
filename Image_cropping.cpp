#include <iostream>
#include <fstream>
#include <vector>
#include <cstring> // Required for memcpy

// Enforce 1-byte structure alignment to match BMP binary file format.
#pragma pack(1)

using namespace std;

// Standard BMP File Header
struct BMPFileHeader {
    uint16_t bfType;      // Magic number 'BM' (0x4D42).
    uint32_t bfSize;      // Total file size in bytes.
    uint16_t bfReserved1; // Reserved.
    uint16_t bfReserved2; // Reserved.
    uint32_t bfOffBits;   // Offset to the start of pixel data.
};

// Standard BMP Information Header (DIB Header)
struct BMPInfoHeader {
    uint32_t biSize;          // Header size.
    uint32_t biWidth;         // Image width.
    uint32_t biHeight;        // Image height.
    uint16_t biPlanes;        // Number of color planes (must be 1).
    uint16_t biBitCount;      // Bits per pixel.
    uint32_t biCompression;   // Compression type (0 = uncompressed).
    uint32_t biSizeImage;     // Image size in bytes.
    int32_t biXPelsPerMeter;  // Horizontal resolution.
    int32_t biYPelsPerMeter;  // Vertical resolution.
    uint32_t biClrUsed;       // Number of colors in palette.
    uint32_t biClrImportant;  // Important colors.
};

// Function to extract a Region of Interest (ROI) from the source image
void cropImage(const vector<uint8_t>& inputPixelData, vector<uint8_t>& croppedPixelData, int originalWidth, int originalHeight, int bytesPerPixel, int rowSize, int x, int y, int cropWidth, int cropHeight) {
    // Calculate the row stride (size in bytes) for the cropped image, ensuring 4-byte alignment (padding).
    int croppedRowSize = ((cropWidth * bytesPerPixel + 3) & (~3));
    
    // Resize the destination buffer to accommodate the cropped image data
    croppedPixelData.resize(croppedRowSize * cropHeight);

    for (int j = 0; j < cropHeight; ++j) {
        // Calculate the current row index in the source image
        int srcY = y + j;
        
        // Calculate the byte offset for the source row
        int srcOffset = srcY * rowSize + x * bytesPerPixel;
        
        // Calculate the byte offset for the destination row
        int destOffset = j * croppedRowSize;
        
        // Copy the pixel data for the current row from source to destination
        // Using memcpy for efficient memory block copying
        memcpy(&croppedPixelData[destOffset], &inputPixelData[srcOffset], cropWidth * bytesPerPixel);
    }
}

int main() {
    const char* inputFileName = "images/input2.bmp";
    const char* outputFileName = "output2_crop.bmp";

    // Open the input file in binary mode
    ifstream inputFile(inputFileName, ios::binary);
    if (!inputFile) {
        cerr << "Can't open file." << endl;
        return 1;
    }

    // Read BMP File Header
    BMPFileHeader fileHeader;
    inputFile.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

    // Validate BMP Signature
    if (fileHeader.bfType != 0x4D42) {
        cerr << "Input file is not a BMP file." << endl;
        return 1;
    }

    // Read BMP Info Header
    BMPInfoHeader infoHeader;
    inputFile.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int bitCount = infoHeader.biBitCount;
    int bytesPerPixel = bitCount / 8;

    // Support only 24-bit and 32-bit uncompressed formats
    if ((bitCount != 24 && bitCount != 32) || infoHeader.biCompression != 0) {
        cerr << "Only supports 24-bit or 32-bit uncompressed BMP." << endl;
        return 1;
    }

    // Calculate source image row stride with padding
    int rowSize = ((width * bytesPerPixel + 3) & (~3));

    // Allocate buffer and read pixel data
    vector<uint8_t> pixelData(rowSize * height);
    inputFile.seekg(fileHeader.bfOffBits, ios::beg);
    inputFile.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
    inputFile.close();

    // Define the Region of Interest (ROI) for cropping: (x, y, width, height)
    int cropX = 120;  // Top-left X coordinate
    int cropY = 150;  // Top-left Y coordinate
    int cropWidth = 100;  // Width of the ROI
    int cropHeight = 100; // Height of the ROI

    // Validate that the ROI is within the source image bounds
    if (cropX + cropWidth > width || cropY + cropHeight > height) {
        cerr << "Cropping area exceeds image bounds." << endl;
        return 1;
    }

    // Perform the cropping operation
    vector<uint8_t> croppedPixelData;
    cropImage(pixelData, croppedPixelData, width, height, bytesPerPixel, rowSize, cropX, cropY, cropWidth, cropHeight);

    // Update the BMP Information Header to reflect the new dimensions
    BMPInfoHeader croppedInfoHeader = infoHeader;
    croppedInfoHeader.biWidth = cropWidth;   // Update width
    croppedInfoHeader.biHeight = cropHeight; // Update height
    
    // Recalculate image size based on the new stride and height
    int croppedRowSize = ((cropWidth * bytesPerPixel + 3) & (~3));
    croppedInfoHeader.biSizeImage = croppedRowSize * cropHeight;

    // Update the BMP File Header
    BMPFileHeader croppedFileHeader = fileHeader;
    // Recalculate total file size: Header Offset + New Image Size
    croppedFileHeader.bfSize = croppedInfoHeader.biSizeImage + fileHeader.bfOffBits;

    // Write the cropped image to a new file
    ofstream outputFile(outputFileName, ios::binary);
    if (!outputFile) {
        cerr << "Cannot open output file." << endl;
        return 1;
    }

    // Write headers and pixel data
    outputFile.write(reinterpret_cast<char*>(&croppedFileHeader), sizeof(croppedFileHeader));
    outputFile.write(reinterpret_cast<char*>(&croppedInfoHeader), sizeof(croppedInfoHeader));
    outputFile.write(reinterpret_cast<char*>(croppedPixelData.data()), croppedPixelData.size());
    outputFile.close();

    cout << "Cropping completed. Cropped image file successfully created." << endl;
    return 0;
}