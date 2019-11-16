//////////////////////////////
//
//
// Created by Binary2Header V0.6
// (c) HxC2001
// (c) PowerOfAsm
//
// File: init.script  Size: 29  (30) 
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
0x00,0x23,0x20,0x0a,0x23,0x20,0x6c,0x69,0x62,0x68,
0x78,0x63,0x66,0x65,0x20,0x69,0x6e,0x69,0x74,0x20,
0x73,0x63,0x72,0x69,0x70,0x74,0x0a,0x23,0x0a,0x0a
};


static datatype data_init_script[]=
{
	{ 255, 29, 30, data__init_script, 0 }
};
