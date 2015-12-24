////////////////////////////////////
//      AnaDisk File format

/*
 AnaDisk Dump Operation

 The Dump operation writes a specified area of a diskette to a DOS file.
 After selecting the Dump option from the Main  Menu, the diskette drive
 containing the diskette to be read, the range of cylinders and sides to be
 written to a specified DOS file are selected.
 Each  sector written  to  the file is optionally preceded by an 8-byte header
 record of the following form:

 +------+------+------+------+------+------+----------+
 | ACYL | ASID | LCYL | LSID | LSEC | LLEN |  COUNT   |
 +------+------+------+------+------+------+----------+

 ACYL  Actual cylinder, 1 byte
 ASID  Actual side, 1 byte
 LCYL  Logical cylinder; cylinder as read, 1 byte
 LSID  Logical side; or side as read, 1 byte
 LSEC  Sector number as read, 1 byte
 LLEN  Length code as read, 1 byte
 COUNT Byte count of data to follow,  2 bytes.If zero,  no data is contained in this sector.

 All sectors occurring on a side will be grouped together; however,
 they will appear in the same order as they occurred on the diskette.
 Therefore, if an 8 sector-per-track diskette were scanned which had a physical
 interleave of 2:1, the sectors might appear in the order 1,5,2,6,3,7,4,8 in the DOS dump file.
 After the last specified cylinder has been  written  to  the  DOS file, AnaDisk returns to the Main Menu.
*/

#pragma pack(1)
 
typedef struct AnaDisk_sectorheader_
{
	uint8_t  cylinder;               // ACYL  Actual cylinder, 1 byte
	uint8_t  side;                   // ASID  Actual side, 1 byte
	uint8_t  logical_cylinder;       // Logical cylinder; cylinder as read, 1 byte
	uint8_t  logical_side;           // Logical side; or side as read, 1 byte
	uint8_t  logical_sector;         // Sector number as read, 1 byte
	uint8_t  sector_len_code;        // Length code as read, 1 byte
	uint16_t data_len;               // Byte count of data to follow,  2 bytes.If zero,  no data is contained in this sector.
}AnaDisk_sectorheader;

#pragma pack()
