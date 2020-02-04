//////////////////////////////
//
//
// Created by Binary2Header V0.6
// (c) HxC2001
// (c) PowerOfAsm
//
// File: init.script  Size: 2141  (1217) 
//
//


#ifndef DATATYPEDEF
#define DATATYPEDEF

typedef  struct _datatype
{
   int type;
   int size;
   int csize;
   unsigned char * data;
   unsigned char * unpacked_data;
}datatype;

#endif


unsigned char data__init_script[]={
0x01,0x11,0x82,0x84,0x62,0x03,0x61,0xa4,0xc4,0x68,
0x3c,0x18,0xcc,0xc6,0x51,0x01,0xa4,0xdc,0x69,0x3a,
0x08,0x0e,0x66,0x33,0x91,0xa4,0xe0,0x74,0x82,0x02,
0x81,0x47,0x33,0x2c,0x4c,0x8c,0x4c,0x2a,0x96,0x0a,
0x65,0x42,0x91,0x14,0x82,0x4d,0x2f,0x90,0x89,0x32,
0x62,0x09,0x50,0x8a,0x5f,0x23,0x12,0x49,0x93,0x02,
0x91,0x7c,0xa0,0x41,0x29,0x94,0xc8,0xa5,0x31,0x00,
0xca,0x3b,0x1f,0x10,0x48,0x64,0x72,0x59,0x3c,0xa6,
0x57,0x2d,0x29,0x4b,0xe6,0x33,0x39,0xa9,0x16,0x6e,
0x57,0x24,0x93,0x88,0x84,0xf2,0xbc,0xfc,0x69,0x41,
0x90,0x48,0xa4,0x92,0x69,0x44,0xa8,0xa0,0x48,0x9d,
0x4c,0x48,0x64,0xf2,0x94,0x9c,0x86,0x54,0x24,0x93,
0xc9,0xc5,0xf2,0x21,0x24,0xac,0x49,0x29,0xd9,0x84,
0x03,0x8a,0xd5,0x0e,0xb9,0x46,0xaf,0x97,0xca,0x45,
0x02,0x6c,0xcc,0xb0,0x20,0x27,0x5b,0x08,0xa2,0x0c,
0x36,0x1f,0x11,0x89,0xc4,0x41,0x70,0x64,0xec,0x28,
0xbc,0x40,0x33,0x1b,0x0c,0x0a,0x84,0xf1,0x98,0xc0,
0x61,0x7d,0x26,0x88,0x32,0x19,0x7c,0xa6,0x5b,0x27,
0x9a,0xbb,0xd1,0x2b,0xb4,0x79,0x51,0x4c,0x96,0x49,
0x28,0x10,0x89,0x84,0xf2,0x19,0x2c,0xa6,0x44,0x22,
0xcc,0x2d,0x36,0xb2,0x70,0x80,0x61,0xa3,0xbc,0xd7,
0xa9,0x04,0xf2,0xb5,0x40,0x98,0x41,0x28,0x4f,0x08,
0x25,0x22,0x19,0x23,0x64,0x50,0x2a,0x12,0x37,0x02,
0xe1,0x80,0xc8,0x6b,0x8a,0xc3,0x41,0x48,0xa7,0x83,
0x09,0x8e,0x26,0x74,0x39,0x75,0xcd,0x62,0x03,0x91,
0x94,0xec,0x6f,0x36,0x1d,0x4e,0x86,0x93,0x79,0xb8,
0x40,0x6f,0x3b,0x19,0x4e,0x46,0xc3,0x09,0xc2,0x28,
0x65,0x30,0x9c,0x8c,0x66,0x81,0x01,0x90,0xcb,0x19,
0xfb,0x0a,0x22,0x1f,0x83,0xc4,0x04,0x20,0x1d,0x06,
0xf4,0x3c,0x6e,0x7f,0x90,0x61,0x94,0x6e,0x19,0xc6,
0x84,0x4c,0x70,0x1b,0xc7,0x57,0xd2,0x07,0x44,0xc7,
0x67,0xcc,0x69,0x18,0x5e,0x57,0x9c,0x29,0x6e,0x94,
0x56,0xf1,0x2a,0x54,0xd2,0xd1,0x24,0x41,0x13,0x14,
0x94,0xb9,0x30,0x6e,0x21,0x96,0x95,0x7b,0x60,0xc5,
0x01,0x30,0x4c,0x14,0x12,0x71,0x4c,0x59,0x13,0x84,
0x38,0x95,0x77,0x10,0x84,0xd1,0x40,0x45,0x16,0x05,
0x05,0x98,0x54,0x5b,0x84,0x51,0x18,0x41,0x15,0x53,
0x51,0x7d,0x24,0x12,0x45,0xa6,0x14,0x31,0x73,0xd5,
0x94,0x79,0x13,0x8d,0x63,0x78,0xe6,0x3b,0x8f,0x63,
0xf9,0x06,0x3c,0x16,0x45,0x39,0x15,0x85,0x0d,0x03,
0x86,0xe6,0x4b,0x08,0x24,0xd8,0xe2,0x3a,0x14,0xa3,
0xc6,0xca,0x52,0x90,0x85,0x86,0x54,0x54,0x88,0x16,
0xa1,0x35,0x85,0x0c,0x83,0x46,0x61,0x98,0x74,0xd5,
0x80,0xc0,0x6d,0x1c,0xe3,0x48,0xda,0x5f,0x94,0x26,
0x39,0x02,0x42,0x16,0x66,0x79,0xa4,0x49,0x9a,0xc2,
0x00,0xc4,0x36,0x62,0x90,0x5a,0x0c,0x75,0x9d,0x67,
0x61,0x41,0x6f,0x6a,0x27,0x89,0x86,0x51,0x9e,0xe3,
0xc9,0x12,0x46,0xa0,0x83,0x90,0xca,0x5b,0x50,0xa4,
0xda,0x30,0x4b,0xa3,0xa6,0x28,0xfa,0x91,0x17,0xe5,
0x59,0x5c,0x20,0x0e,0x66,0xe4,0x71,0x03,0x41,0x45,
0x37,0x69,0xf2,0x1b,0x65,0xd8,0xd8,0x20,0x75,0x60,
0xc1,0xc9,0x13,0x77,0xc7,0x37,0x89,0xe4,0x79,0x9e,
0x81,0x86,0x04,0x7c,0x47,0x47,0x96,0x08,0x9d,0x50,
0x50,0xa0,0x44,0x79,0xd0,0xe1,0x88,0x65,0x19,0x86,
0xf7,0x7c,0x20,0xae,0xc7,0x90,0x80,0x70,0x1c,0x86,
0xf1,0x8c,0x65,0x1c,0xc7,0x34,0x40,0x67,0x43,0xc6,
0x60,0x82,0x07,0x18,0x46,0x21,0xb0,0x65,0x19,0x21,
0x84,0x0e,0x8a,0xa7,0x45,0xf5,0xe9,0x48,0x9e,0xa5,
0x39,0x0e,0x56,0xa5,0x03,0x80,0xc6,0x96,0xb9,0x24,
0xfa,0x3e,0xe7,0x4a,0xae,0x99,0xf2,0xec,0x96,0x25,
0xab,0xc6,0x60,0x8f,0x2f,0x4a,0x42,0xea,0x99,0x84,
0xf9,0xa1,0x35,0xa0,0x26,0xc9,0xbe,0x6f,0x08,0x10,
0x5a,0x5c,0x30,0x0a,0xa4,0x86,0x63,0x0d,0x0c,0x02,
0x00,0xb4,0x3e,0x4f,0xd9,0x89,0xd0,0x20,0x0a,0xa8,
0x2c,0x47,0x13,0xc6,0x9d,0xe7,0x82,0xb7,0x85,0x86,
0xeb,0xee,0x50,0xbf,0xaf,0x69,0x52,0x7e,0xc1,0x28,
0x1a,0x0e,0x85,0xa0,0x83,0x6a,0x22,0xa7,0x41,0x1f,
0x17,0x62,0xca,0x0b,0xec,0x91,0xc8,0x6d,0x85,0x47,
0x37,0xdc,0x65,0x18,0xc6,0xf7,0xe0,0x72,0xb6,0xc6,
0xeb,0x76,0xdf,0x19,0x1f,0x78,0x3a,0xd7,0x80,0x06,
0x84,0x38,0x70,0x1a,0x5d,0x88,0x39,0x0e,0x19,0x47,
0x8a,0xc8,0x74,0x0b,0x91,0xbc,0x8e,0x8f,0x11,0x44,
0xe1,0x05,0xac,0x4c,0x57,0x21,0x3c,0x49,0x8d,0x45,
0xf5,0xfd,0x2a,0xd6,0x56,0x55,0xbc,0x4e,0x11,0xe8,
0x2d,0x5e,0x3c,0xd6,0x75,0xb1,0x33,0x5d,0x5c,0xf6,
0x04,0xa8,0x46,0xd9,0x23,0x11,0x3f,0x67,0xda,0x43,
0x1d,0xac,0x5f,0xdb,0x75,0xc1,0x7d,0x29,0x12,0x44,
0x71,0x07,0x62,0xdd,0x37,0xcd,0xdb,0x78,0xda,0xa5,
0xc9,0x7a,0xf2,0xdb,0x35,0xad,0xf8,0x45,0x13,0x45,
0x54,0xcb,0x75,0xd9,0x95,0x3d,0xe7,0x7b,0xdf,0x76,
0xfd,0x8b,0x90,0x10,0x94,0xb5,0x4f,0x84,0xe5,0x37,
0x7e,0x5b,0x89,0xa6,0x67,0x7e,0x33,0x86,0xdb,0x93,
0x11,0x50,0x59,0x59,0x77,0x3e,0x87,0x88,0xde,0xb8,
0xae,0x9a,0xfc,0xea,0x37,0xe7,0x0a,0x2a,0x11,0x44,
0x91,0x24,0x5f,0x11,0xc4,0x31,0x48,0x31,0xe1,0xb9,
0x5d,0xa3,0xa4,0x93,0x3b,0x39,0x43,0x99,0x4c,0x7b,
0x7d,0xbf,0xba,0xef,0x3b,0xe0,0xcb,0xc1,0xe8,0xbc,
0x3e,0xc7,0xa5,0x93,0xbb,0x4f,0x23,0x7f,0x14,0x84,
0x21,0x54,0x52,0x11,0xc4,0x44,0xbf,0xd0,0xe2,0x29,
0x8f,0x17,0xd5,0xf1,0xf8,0xee,0x69,0xc5,0xf6,0xbd,
0xc8,0xbc,0x53,0xf8,0x3a,0x36,0xe5,0x77,0x12,0x05,
0x81,0x0e,0xf4,0x6b,0x44,0x16,0xc9,0x37,0x55,0x45,
0x5d,0xf9,0x25,0x11,0x45,0x08,0x95,0x01,0x4b,0x8f,
0xc5,0xf9,0xa1,0xb7,0xea,0xfd,0xd7,0x33,0x96,0x73,
0x49,0x58,0xd9,0x3f,0xf2,0x85,0x00,0x5f,0xa0,0x4f,
0x7e,0xc5,0x41,0x11,0x14,0xb4,0x48,0xf8,0x81,0x04,
0x0d,0x80,0x70,0x3e,0x02,0xb8,0xb5,0xf8,0x8c,0xd2,
0xe0,0x53,0x08,0x6f,0x91,0x47,0x84,0xe0,0xaa,0x13,
0x42,0x14,0x11,0x09,0xe1,0x18,0xbe,0x04,0x50,0xac,
0x13,0xc9,0x11,0x6a,0x2d,0x84,0xf8,0x19,0x9d,0x23,
0x18,0x1d,0x43,0x6a,0xc7,0x68,0x01,0xbd,0x6d,0x1d,
0xf3,0xc2,0x78,0xd9,0x0b,0x3a,0x0c,0xe8,0x1c,0xf6,
0x21,0x55,0xc0,0x80,0x10,0x10,0x74,0x69,0x4b,0x6d,
0xa9,0x2c,0xa0,0xe9,0x11,0x20,0xfb,0xfe,0x0c,0xc1,
0xa5,0x6f,0x91,0xc4,0xb8,0x12,0xc2,0x31,0x4b,0x0a,
0xeb,0x96,0x11,0xc2,0x58,0x4f,0x0a,0x49,0x3c,0x2c,
0x85,0xc6,0xda,0x18,0x98,0x98,0x69,0x0d,0x8f,0x61,
0xe9,0x87,0x4c,0x7e,0x1e,0xab,0x98,0x7f,0x10,0x4e,
0xdc,0x4c,0x68,0xa8,0x05,0xa4,0xb5,0x08,0x94,0xac,
0xe2,0x24,0x55,0x3b,0xc1,0x84,0x3b,0x82,0x08,0xa0,
0xb7,0xd4,0x4c,0x54,0x8a,0xc1,0x04,0x2b,0xc0,0x48,
0x22,0xfe,0x5f,0xd9,0x30,0x7f,0xd0,0x56,0x2a,0xc5,
0x79,0x06,0x4d,0xd2,0xb3,0x68,0x81,0x01,0x26,0x05,
0x48,0x99,0x01,0x20,0xa0,0xcc,0x11,0x25,0x88,0x8c,
0xc2,0xc9,0x39,0x17,0x25,0x89,0xbc,0x1b,0x47,0x70,
0x74,0xa1,0x44,0xe9,0x18,0x5b,0x82,0x7b,0xfa,0x81,
0x12,0x1e,0x05,0x91,0x39,0x47,0x27,0x60,0x34,0x8f,
0x26,0x30,0x26,0x4d,0x17,0x79,0x59,0x04,0x24,0xf1,
0x4a,0x29,0x92,0xa8,0x10,0x4b,0x48,0x34,0xf1,0x93,
0x0c,0xa1,0x95,0x68,0x6c,0x24,0x04,0x60,0x8b,0x29,
0x24,0xc4,0x13,0x96,0x50,0x7a,0x60,0xcc,0x39,0x8b,
0x2f,0x42,0xa2,0x25,0x00,0x04,0x00};


static datatype data_init_script[]=
{
	{ 255, 2141, 1217, data__init_script, 0 }
};