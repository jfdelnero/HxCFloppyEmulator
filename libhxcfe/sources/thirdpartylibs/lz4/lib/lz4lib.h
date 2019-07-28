// Minimalist LZ4 lib header.

int LZ4_decompress_safe (const char* src, char* dst, int compressedSize, int dstCapacity);

int LZ4_compress_default(const char* src, char* dst, int srcSize, int dstCapacity);
int LZ4_compressBound(int inputSize);
