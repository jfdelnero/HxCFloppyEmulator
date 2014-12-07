
/*OOB Header (4 bytes)

The data header indicates the start of a new OOB information block. E.g. index signal, transfer status, stream information. A repeated OOB code makes it possible to detect the end of the read stream while reading from the device with a simple check regardless of the current stream alignment.

Offset	Bytes	Value	Name 	Description
0 	1 	0x0D 	Sign 	Constant value indicating start of OOB header
1 	1 		Type 	OOB block data type, see below for possible values
2 	2 		Size 	The number of bytes of the OOB data that follow the header

Type Values

The type value indicates the following OOB data section, or no following data section if it is type "End".
Value	Name 	Meaning
0×00 	Invalid 	?
0×01 	Stream Read 	Start of flux transition timing data block (multiple per track)
0×02 	Index 	Index signal data
0×03 	Stream End 	Signifies there are no more stream read blocks (one per track)
0x0D 	End 	End of data (no more data to process)
*/

#pragma pack(1)

#define OOB_SIGN 0x0D

typedef struct s_oob_header_
{
	uint8_t 	Sign;  // 0x0D
	uint8_t 	Type;
	uint16_t 	Size;
}s_oob_header;


typedef struct s_oob_StreamRead_
{
	uint32_t 	StreamPosition;
	uint32_t 	TrTime;
}s_oob_StreamRead;

typedef struct s_oob_StreamEnd_
{
	uint32_t 	StreamPosition;
	uint32_t 	Result;
}s_oob_StreamEnd;

typedef struct s_oob_DiskIndex_
{
	uint32_t 	StreamPosition;
	uint32_t 	Timer;
	uint32_t 	SysClk;
}s_oob_DiskIndex;

#define OOBTYPE_Stream_Read 0x01  // Start of flux transition timing data block (multiple per track)
#define OOBTYPE_Index       0x02  // Index Index signal data
#define OOBTYPE_Stream_End  0x03  // Stream End Signifies there are no more stream read blocks (one per track)
#define OOBTYPE_End         0x0D  // End of data (no more data to process) 

#pragma pack()
