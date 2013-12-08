// fdrawcmd.sys 1.0.1.10
//
// Low-level floppy filter, by Simon Owen
//
// http://simonowen.com/fdrawcmd/

#ifndef FDRAWCMD_H
#define FDRAWCMD_H

#ifndef CTL_CODE
#include <winioctl.h>
#endif

#define FDRAWCMD_VERSION                0x0100010a      // Compile-time version, for structures and definitions below
                                                        // Must be checked with run-time driver for compatibility

#define FD_CTL_CODE(i,m)                CTL_CODE(FILE_DEVICE_UNKNOWN, i, m, FILE_READ_DATA|FILE_WRITE_DATA)

                                                                                // If you're not using C/C++, use the IOCTL values below
#define IOCTL_FDRAWCMD_GET_VERSION      FD_CTL_CODE(0x888, METHOD_BUFFERED)     // 0x0022e220

#define IOCTL_FDCMD_READ_TRACK          FD_CTL_CODE(0x802, METHOD_OUT_DIRECT)   // 0x0022e00a
#define IOCTL_FDCMD_SPECIFY             FD_CTL_CODE(0x803, METHOD_BUFFERED)     // 0x0022e00c
#define IOCTL_FDCMD_SENSE_DRIVE_STATUS  FD_CTL_CODE(0x804, METHOD_BUFFERED)     // 0x0022e010
#define IOCTL_FDCMD_WRITE_DATA          FD_CTL_CODE(0x805, METHOD_IN_DIRECT)    // 0x0022e015
#define IOCTL_FDCMD_READ_DATA           FD_CTL_CODE(0x806, METHOD_OUT_DIRECT)   // 0x0022e01a
#define IOCTL_FDCMD_RECALIBRATE         FD_CTL_CODE(0x807, METHOD_BUFFERED)     // 0x0022e01c
#define IOCTL_FDCMD_SENSE_INT_STATUS    FD_CTL_CODE(0x808, METHOD_BUFFERED)     // 0x0022e020   // added in 1.0.0.22
#define IOCTL_FDCMD_WRITE_DELETED_DATA  FD_CTL_CODE(0x809, METHOD_IN_DIRECT)    // 0x0022e025
#define IOCTL_FDCMD_READ_ID             FD_CTL_CODE(0x80a, METHOD_BUFFERED)     // 0x0022e028
#define IOCTL_FDCMD_READ_DELETED_DATA   FD_CTL_CODE(0x80c, METHOD_OUT_DIRECT)   // 0x0022e032
#define IOCTL_FDCMD_FORMAT_TRACK        FD_CTL_CODE(0x80d, METHOD_BUFFERED)     // 0x0022e034
#define IOCTL_FDCMD_DUMPREG             FD_CTL_CODE(0x80e, METHOD_BUFFERED)     // 0x0022e038
#define IOCTL_FDCMD_SEEK                FD_CTL_CODE(0x80f, METHOD_BUFFERED)     // 0x0022e03c
#define IOCTL_FDCMD_VERSION             FD_CTL_CODE(0x810, METHOD_BUFFERED)     // 0x0022e040
#define IOCTL_FDCMD_SCAN_EQUAL          FD_CTL_CODE(0x811, METHOD_IN_DIRECT)    // 0x0022e045   (not implemented yet)
#define IOCTL_FDCMD_PERPENDICULAR_MODE  FD_CTL_CODE(0x812, METHOD_BUFFERED)     // 0x0022e048
#define IOCTL_FDCMD_CONFIGURE           FD_CTL_CODE(0x813, METHOD_BUFFERED)     // 0x0022e04c
#define IOCTL_FDCMD_LOCK                FD_CTL_CODE(0x814, METHOD_BUFFERED)     // 0x0022e050
#define IOCTL_FDCMD_VERIFY              FD_CTL_CODE(0x816, METHOD_BUFFERED)     // 0x0022e058
#define IOCTL_FDCMD_POWERDOWN_MODE      FD_CTL_CODE(0x817, METHOD_BUFFERED)     // 0x0022e05c   (not implemented yet)
#define IOCTL_FDCMD_PART_ID             FD_CTL_CODE(0x818, METHOD_BUFFERED)     // 0x0022e060
#define IOCTL_FDCMD_SCAN_LOW_OR_EQUAL   FD_CTL_CODE(0x819, METHOD_IN_DIRECT)    // 0x0022e065   (not implemented yet)
#define IOCTL_FDCMD_SCAN_HIGH_OR_EQUAL  FD_CTL_CODE(0x81d, METHOD_IN_DIRECT)    // 0x0022e075   (not implemented yet)
#define IOCTL_FDCMD_SAVE                FD_CTL_CODE(0x82e, METHOD_BUFFERED)     // 0x0022e0b8   (not implemented yet)
#define IOCTL_FDCMD_OPTION              FD_CTL_CODE(0x833, METHOD_BUFFERED)     // 0x0022e0cc   (not implemented yet)
#define IOCTL_FDCMD_RESTORE             FD_CTL_CODE(0x84e, METHOD_BUFFERED)     // 0x0022e138   (not implemented yet)
#define IOCTL_FDCMD_DRIVE_SPEC_CMD      FD_CTL_CODE(0x88e, METHOD_BUFFERED)     // 0x0022e238   (not implemented yet)
#define IOCTL_FDCMD_RELATIVE_SEEK       FD_CTL_CODE(0x88f, METHOD_BUFFERED)     // 0x0022e23c
#define IOCTL_FDCMD_FORMAT_AND_WRITE    FD_CTL_CODE(0x8ef, METHOD_BUFFERED)     // 0x0022e3bc   // added in 1.0.1.10

#define IOCTL_FD_SCAN_TRACK             FD_CTL_CODE(0x900, METHOD_BUFFERED)     // 0x0022e400
#define IOCTL_FD_GET_RESULT             FD_CTL_CODE(0x901, METHOD_BUFFERED)     // 0x0022e404
#define IOCTL_FD_RESET                  FD_CTL_CODE(0x902, METHOD_BUFFERED)     // 0x0022e408
#define IOCTL_FD_SET_MOTOR_TIMEOUT      FD_CTL_CODE(0x903, METHOD_BUFFERED)     // 0x0022e40c
#define IOCTL_FD_SET_DATA_RATE          FD_CTL_CODE(0x904, METHOD_BUFFERED)     // 0x0022e410
#define IOCTL_FD_GET_FDC_INFO           FD_CTL_CODE(0x905, METHOD_BUFFERED)     // 0x0022e414
#define IOCTL_FD_GET_REMAIN_COUNT       FD_CTL_CODE(0x906, METHOD_BUFFERED)     // 0x0022e418   // added in 1.0.0.22
#define IOCTL_FD_SET_DISK_CHECK         FD_CTL_CODE(0x908, METHOD_BUFFERED)     // 0x0022e420
#define IOCTL_FD_SET_SHORT_WRITE        FD_CTL_CODE(0x909, METHOD_BUFFERED)     // 0x0022e424   // added in 1.0.0.22
#define IOCTL_FD_SET_SECTOR_OFFSET      FD_CTL_CODE(0x90a, METHOD_BUFFERED)     // 0x0022e428   // added in 1.0.0.22
#define IOCTL_FD_SET_HEAD_SETTLE_TIME   FD_CTL_CODE(0x90b, METHOD_BUFFERED)     // 0x0022e42c   // added in 1.0.0.22
#define IOCTL_FD_LOCK_FDC               FD_CTL_CODE(0x910, METHOD_BUFFERED)     // 0x0022e440   // obsolete from 1.0.1.0
#define IOCTL_FD_UNLOCK_FDC             FD_CTL_CODE(0x911, METHOD_BUFFERED)     // 0x0022e444   // obsolete from 1.0.1.0
#define IOCTL_FD_MOTOR_ON               FD_CTL_CODE(0x912, METHOD_BUFFERED)     // 0x0022e448
#define IOCTL_FD_MOTOR_OFF              FD_CTL_CODE(0x913, METHOD_BUFFERED)     // 0x0022e44c
#define IOCTL_FD_WAIT_INDEX             FD_CTL_CODE(0x914, METHOD_BUFFERED)     // 0x0022e450   // added in 1.0.0.22
#define IOCTL_FD_TIMED_SCAN_TRACK       FD_CTL_CODE(0x915, METHOD_BUFFERED)     // 0x0022e454   // added in 1.0.0.22
#define IOCTL_FD_RAW_READ_TRACK         FD_CTL_CODE(0x916, METHOD_OUT_DIRECT)   // 0x0022e45a   // added in 1.0.1.4
#define IOCTL_FD_CHECK_DISK             FD_CTL_CODE(0x917, METHOD_BUFFERED)     // 0x0022e45c   // added in 1.0.1.10
#define IOCTL_FD_GET_TRACK_TIME         FD_CTL_CODE(0x918, METHOD_BUFFERED)     // 0x0022e460   // added in 1.0.1.10

///////////////////////////////////////////////////////////////////////////////

// Command flags: multi-track, MFM, sector skip, relative seek direction, verify enable count
#define FD_OPTION_MT        0x80
#define FD_OPTION_MFM       0x40
#define FD_OPTION_SK        0x20
#define FD_OPTION_DIR       0x40
#define FD_OPTION_EC        0x01
#define FD_OPTION_FM        0x00
#define FD_ENCODING_MASK    FD_OPTION_MFM

// Controller data rates, for use with IOCTL_FD_SET_DATA_RATE
#define FD_RATE_MASK        3
#define FD_RATE_500K        0
#define FD_RATE_300K        1
#define FD_RATE_250K        2
#define FD_RATE_1M          3

// FD_FDC_INFO controller types
#define FDC_TYPE_UNKNOWN    0
#define FDC_TYPE_UNKNOWN2   1
#define FDC_TYPE_NORMAL     2
#define FDC_TYPE_ENHANCED   3
#define FDC_TYPE_82077      4
#define FDC_TYPE_82077AA    5
#define FDC_TYPE_82078_44   6
#define FDC_TYPE_82078_64   7
#define FDC_TYPE_NATIONAL   8

// Bits representing supported data rates, for the FD_FDC_INFO structure below
#define FDC_SPEED_250K      0x01
#define FDC_SPEED_300K      0x02
#define FDC_SPEED_500K      0x04
#define FDC_SPEED_1M        0x08
#define FDC_SPEED_2M        0x10


#pragma pack(push,1)
#pragma warning(push)
#pragma warning(disable:4200)           // allow zero-sized arrays


typedef struct tagFD_ID_HEADER
{
    BYTE cyl, head, sector, size;
}
FD_ID_HEADER, *PFD_ID_HEADER;

typedef struct tagFD_SEEK_PARAMS
{
    BYTE cyl;
    BYTE head;
}
FD_SEEK_PARAMS, *PFD_SEEK_PARAMS;

typedef struct tagFD_RELATIVE_SEEK_PARAMS
{
    BYTE flags;                         // DIR
    BYTE head;
    BYTE offset;
}
FD_RELATIVE_SEEK_PARAMS, *PFD_RELATIVE_SEEK_PARAMS;

typedef struct tagFD_READ_WRITE_PARAMS
{
    BYTE flags;                         // MT MFM SK
    BYTE phead;
    BYTE cyl, head, sector, size;
    BYTE eot, gap, datalen;
}
FD_READ_WRITE_PARAMS, *PFD_READ_WRITE_PARAMS;

typedef struct tagFD_CMD_RESULT
{
    BYTE st0, st1, st2;
    BYTE cyl, head, sector, size;
}
FD_CMD_RESULT, *PFD_CMD_RESULT;

typedef struct tagFD_FORMAT_PARAMS
{
    BYTE flags;                         // MFM
    BYTE phead;
    BYTE size, sectors, gap, fill;

    FD_ID_HEADER Header[];
}
FD_FORMAT_PARAMS, *PFD_FORMAT_PARAMS;

typedef struct tagFD_READ_ID_PARAMS
{
    BYTE flags;                         // MFM
    BYTE head;
}
FD_READ_ID_PARAMS, *PFD_READ_ID_PARAMS;

typedef struct tagFD_CONFIGURE_PARAMS
{
    BYTE eis_efifo_poll_fifothr;        // b6 = enable implied seek, b5 = enable fifo, b4 = poll disable, b3-b0 = fifo threshold
    BYTE pretrk;                        // precompensation start track
}
FD_CONFIGURE_PARAMS, *PFD_CONFIGURE_PARAMS;

typedef struct tagFD_SPECIFY_PARAMS
{
    BYTE srt_hut;                       // b7-b4 = step rate, b3-b0 = head unload time
    BYTE hlt_nd;                        // b7-b1 = head load time, b0 = non-DMA flag (unsupported)
}
FD_SPECIFY_PARAMS, *PFD_SPECIFY_PARAMS;

typedef struct tagFD_SENSE_PARAMS
{
    BYTE head;
}
FD_SENSE_PARAMS, *PFD_SENSE_PARAMS;

typedef struct tagFD_DRIVE_STATUS
{
    BYTE st3;
}
FD_DRIVE_STATUS, *PFD_DRIVE_STATUS;

typedef struct tagFD_INTERRUPT_STATUS
{
    BYTE st0;                           // status register 0
    BYTE pcn;                           // present cylinder number
}
FD_INTERRUPT_STATUS, *PFD_INTERRUPT_STATUS;

typedef struct tagFD_PERPENDICULAR_PARAMS
{
    BYTE ow_ds_gap_wgate;               // b7 = OW, b6 = 0, b5-b2 = drive select, b1 = gap2, b0 = write gate pre-erase loads
}
FD_PERPENDICULAR_PARAMS, *PFD_PERPENDICULAR_PARAMS;

typedef struct tagFD_LOCK_PARAMS
{
    BYTE lock;                          // b7 = lock
}
FD_LOCK_PARAMS, *PFD_LOCK_PARAMS;

typedef struct tagFD_LOCK_RESULT
{
    BYTE lock;                          // b4 = lock
}
FD_LOCK_RESULT, *PFD_LOCK_RESULT;

typedef struct tagFD_DUMPREG_RESULT
{
    BYTE pcn0, pcn1, pcn2, pcn3;        // present cylinder numbers
    BYTE srt_hut;                       // b7-4 = step rate, b3-0 = head unload time
    BYTE hlt_nd;                        // b7-1 = head load time, b0 = non-dma mode
    BYTE sceot;                         // sector count / end of track
    BYTE lock_d0123_gap_wgate;          // b7 = setting lock, b5-2 = drive selects, b1 = gap 2 (perpendicular), b0 = write gate
    BYTE eis_efifo_poll_fifothr;        // b6 = implied seeks, b5 = fifo enable, b4 = poll disable, b3-0 = fifo threshold
    BYTE pretrk;                        // pre-comp start track
}
FD_DUMPREG_RESULT, *PFD_DUMPREG_RESULT;

typedef struct tagFD_SECTOR_OFFSET_PARAMS
{
    BYTE sectors;                       // number of sectors to skip after index
}
FD_SECTOR_OFFSET_PARAMS, *PFD_SECTOR_OFFSET_PARAMS;

typedef struct tagFD_SHORT_WRITE_PARAMS
{
    DWORD length;                       // length to write before interrupting
    DWORD finetune;                     // finetune delay in microseconds
}
FD_SHORT_WRITE_PARAMS, *PFD_SHORT_WRITE_PARAMS;

typedef struct tagFD_SCAN_PARAMS
{
    BYTE flags;                         // MFM
    BYTE head;
}
FD_SCAN_PARAMS, *PFD_SCAN_PARAMS;

typedef struct tagFD_SCAN_RESULT
{
    BYTE count;                         // count of returned headers
    FD_ID_HEADER Header[];              // array of 'count' id fields
}
FD_SCAN_RESULT, *PFD_SCAN_RESULT;


typedef struct tagFD_TIMED_ID_HEADER
{
    DWORD reltime;                      // time relative to index (in microseconds)
    BYTE cyl, head, sector, size;
}
FD_TIMED_ID_HEADER, *PFD_TIMED_ID_HEADER;

typedef struct tagFD_TIMED_SCAN_RESULT
{
    BYTE count;                         // count of returned headers
    BYTE firstseen;                     // offset of first sector detected
    DWORD tracktime;                    // total time for track (in microseconds)
    FD_TIMED_ID_HEADER Header[];        // array of 'count' id fields
}
FD_TIMED_SCAN_RESULT, *PFD_TIMED_SCAN_RESULT;

typedef struct tagFD_FDC_INFO
{
    BYTE ControllerType;                // FDC_TYPE_*
    BYTE SpeedsAvailable;               // FDC_SPEED_* values ORed together

    BYTE BusType;
    DWORD BusNumber;
    DWORD ControllerNumber;
    DWORD PeripheralNumber;
}
FD_FDC_INFO, *PFD_FDC_INFO;

typedef struct tagFD_RAW_READ_PARAMS
{
    BYTE flags;                         // MFM
    BYTE head, size;
}
FD_RAW_READ_PARAMS, *PFD_RAW_READ_PARAMS;

#pragma warning(pop)
#pragma pack(pop)

#endif
