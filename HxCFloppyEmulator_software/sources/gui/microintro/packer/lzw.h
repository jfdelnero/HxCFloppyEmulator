typedef unsigned char ubyte;
//typedef char byte;

ubyte *lzw_expand( ubyte *inputbuf, ubyte *outputbuf, int length );
ubyte *lzw_compress( ubyte *inputbuf, ubyte *outputbuf, int input_size, int *output_size );
