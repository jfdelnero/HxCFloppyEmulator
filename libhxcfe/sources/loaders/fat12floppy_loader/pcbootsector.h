/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
//
// This file is part of the HxCFloppyEmulator library
//
// HxCFloppyEmulator may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/
//; C:\Documents and Settings\USER\Bureau\pcbootsector.bin is 512 bytes long


unsigned char msdos_bootsector[] = {
  0xeb, 0x3c, 0x90, 0x4d, 0x53, 0x44, 0x4f, 0x53,
  0x35, 0x2e, 0x30, 0x00, 0x02, 0x01, 0x01, 0x00,
  0x02, 0xe0, 0x00, 0x40, 0x0b, 0xf0, 0x09, 0x00,
  0x12, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x48,
  0x58, 0x43, 0x46, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41,
  0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0xfa, 0x33,
  0xc0, 0x8e, 0xd0, 0xbc, 0x00, 0x7c, 0x16, 0x07,
  0xbb, 0x78, 0x00, 0x36, 0xc5, 0x37, 0x1e, 0x56,
  0x16, 0x53, 0xbf, 0x3e, 0x7c, 0xb9, 0x0b, 0x00,
  0xfc, 0xf3, 0xa4, 0x06, 0x1f, 0xc6, 0x45, 0xfe,
  0x0f, 0x8b, 0x0e, 0x18, 0x7c, 0x88, 0x4d, 0xf9,
  0x89, 0x47, 0x02, 0xc7, 0x07, 0x3e, 0x7c, 0xfb,
  0xcd, 0x13, 0x72, 0x79, 0x33, 0xc0, 0x39, 0x06,
  0x13, 0x7c, 0x74, 0x08, 0x8b, 0x0e, 0x13, 0x7c,
  0x89, 0x0e, 0x20, 0x7c, 0xa0, 0x10, 0x7c, 0xf7,
  0x26, 0x16, 0x7c, 0x03, 0x06, 0x1c, 0x7c, 0x13,
  0x16, 0x1e, 0x7c, 0x03, 0x06, 0x0e, 0x7c, 0x83,
  0xd2, 0x00, 0xa3, 0x50, 0x7c, 0x89, 0x16, 0x52,
  0x7c, 0xa3, 0x49, 0x7c, 0x89, 0x16, 0x4b, 0x7c,
  0xb8, 0x20, 0x00, 0xf7, 0x26, 0x11, 0x7c, 0x8b,
  0x1e, 0x0b, 0x7c, 0x03, 0xc3, 0x48, 0xf7, 0xf3,
  0x01, 0x06, 0x49, 0x7c, 0x83, 0x16, 0x4b, 0x7c,
  0x00, 0xbb, 0x00, 0x05, 0x8b, 0x16, 0x52, 0x7c,
  0xa1, 0x50, 0x7c, 0xe8, 0x92, 0x00, 0x72, 0x1d,
  0xb0, 0x01, 0xe8, 0xac, 0x00, 0x72, 0x16, 0x8b,
  0xfb, 0xb9, 0x0b, 0x00, 0xbe, 0xe6, 0x7d, 0xf3,
  0xa6, 0x75, 0x0a, 0x8d, 0x7f, 0x20, 0xb9, 0x0b,
  0x00, 0xf3, 0xa6, 0x74, 0x18, 0xbe, 0x9e, 0x7d,
  0xe8, 0x5f, 0x00, 0x33, 0xc0, 0xcd, 0x16, 0x5e,
  0x1f, 0x8f, 0x04, 0x8f, 0x44, 0x02, 0xcd, 0x19,
  0x58, 0x58, 0x58, 0xeb, 0xe8, 0x8b, 0x47, 0x1a,
  0x48, 0x48, 0x8a, 0x1e, 0x0d, 0x7c, 0x32, 0xff,
  0xf7, 0xe3, 0x03, 0x06, 0x49, 0x7c, 0x13, 0x16,
  0x4b, 0x7c, 0xbb, 0x00, 0x07, 0xb9, 0x03, 0x00,
  0x50, 0x52, 0x51, 0xe8, 0x3a, 0x00, 0x72, 0xd8,
  0xb0, 0x01, 0xe8, 0x54, 0x00, 0x59, 0x5a, 0x58,
  0x72, 0xbb, 0x05, 0x01, 0x00, 0x83, 0xd2, 0x00,
  0x03, 0x1e, 0x0b, 0x7c, 0xe2, 0xe2, 0x8a, 0x2e,
  0x15, 0x7c, 0x8a, 0x16, 0x24, 0x7c, 0x8b, 0x1e,
  0x49, 0x7c, 0xa1, 0x4b, 0x7c, 0xea, 0x00, 0x00,
  0x70, 0x00, 0xac, 0x0a, 0xc0, 0x74, 0x29, 0xb4,
  0x0e, 0xbb, 0x07, 0x00, 0xcd, 0x10, 0xeb, 0xf2,
  0x3b, 0x16, 0x18, 0x7c, 0x73, 0x19, 0xf7, 0x36,
  0x18, 0x7c, 0xfe, 0xc2, 0x88, 0x16, 0x4f, 0x7c,
  0x33, 0xd2, 0xf7, 0x36, 0x1a, 0x7c, 0x88, 0x16,
  0x25, 0x7c, 0xa3, 0x4d, 0x7c, 0xf8, 0xc3, 0xf9,
  0xc3, 0xb4, 0x02, 0x8b, 0x16, 0x4d, 0x7c, 0xb1,
  0x06, 0xd2, 0xe6, 0x0a, 0x36, 0x4f, 0x7c, 0x8b,
  0xca, 0x86, 0xe9, 0x8a, 0x16, 0x24, 0x7c, 0x8a,
  0x36, 0x25, 0x7c, 0xcd, 0x13, 0xc3, 0x0d, 0x0a,
  0x4e, 0x6f, 0x6e, 0x2d, 0x53, 0x79, 0x73, 0x74,
  0x65, 0x6d, 0x20, 0x64, 0x69, 0x73, 0x6b, 0x20,
  0x6f, 0x72, 0x20, 0x64, 0x69, 0x73, 0x6b, 0x20,
  0x65, 0x72, 0x72, 0x6f, 0x72, 0x0d, 0x0a, 0x52,
  0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x20, 0x61,
  0x6e, 0x64, 0x20, 0x70, 0x72, 0x65, 0x73, 0x73,
  0x20, 0x61, 0x6e, 0x79, 0x20, 0x6b, 0x65, 0x79,
  0x20, 0x77, 0x68, 0x65, 0x6e, 0x20, 0x72, 0x65,
  0x61, 0x64, 0x79, 0x0d, 0x0a, 0x00, 0x49, 0x4f,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x53, 0x59,
  0x53, 0x4d, 0x53, 0x44, 0x4f, 0x53, 0x20, 0x20,
  0x20, 0x53, 0x59, 0x53, 0x00, 0x00, 0x55, 0xaa
};


unsigned char win95_bootsector[] = {
  0xeb, 0x3c, 0x90, 0x4d, 0x53, 0x57, 0x49, 0x4e,
  0x34, 0x2e, 0x31, 0x00, 0x02, 0x01, 0x01, 0x00,
  0x02, 0xe0, 0x00, 0x40, 0x0b, 0xf0, 0x09, 0x00,
  0x12, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x48,
  0x58, 0x43, 0x46, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41,
  0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0x33, 0xc9,
  0x8e, 0xd1, 0xbc, 0xfc, 0x7b, 0x16, 0x07, 0xbd,
  0x78, 0x00, 0xc5, 0x76, 0x00, 0x1e, 0x56, 0x16,
  0x55, 0xbf, 0x22, 0x05, 0x89, 0x7e, 0x00, 0x89,
  0x4e, 0x02, 0xb1, 0x0b, 0xfc, 0xf3, 0xa4, 0x06,
  0x1f, 0xbd, 0x00, 0x7c, 0xc6, 0x45, 0xfe, 0x0f,
  0x38, 0x4e, 0x24, 0x7d, 0x20, 0x8b, 0xc1, 0x99,
  0xe8, 0x7e, 0x01, 0x83, 0xeb, 0x3a, 0x66, 0xa1,
  0x1c, 0x7c, 0x66, 0x3b, 0x07, 0x8a, 0x57, 0xfc,
  0x75, 0x06, 0x80, 0xca, 0x02, 0x88, 0x56, 0x02,
  0x80, 0xc3, 0x10, 0x73, 0xed, 0x33, 0xc9, 0xfe,
  0x06, 0xd8, 0x7d, 0x8a, 0x46, 0x10, 0x98, 0xf7,
  0x66, 0x16, 0x03, 0x46, 0x1c, 0x13, 0x56, 0x1e,
  0x03, 0x46, 0x0e, 0x13, 0xd1, 0x8b, 0x76, 0x11,
  0x60, 0x89, 0x46, 0xfc, 0x89, 0x56, 0xfe, 0xb8,
  0x20, 0x00, 0xf7, 0xe6, 0x8b, 0x5e, 0x0b, 0x03,
  0xc3, 0x48, 0xf7, 0xf3, 0x01, 0x46, 0xfc, 0x11,
  0x4e, 0xfe, 0x61, 0xbf, 0x00, 0x07, 0xe8, 0x28,
  0x01, 0x72, 0x3e, 0x38, 0x2d, 0x74, 0x17, 0x60,
  0xb1, 0x0b, 0xbe, 0xd8, 0x7d, 0xf3, 0xa6, 0x61,
  0x74, 0x3d, 0x4e, 0x74, 0x09, 0x83, 0xc7, 0x20,
  0x3b, 0xfb, 0x72, 0xe7, 0xeb, 0xdd, 0xfe, 0x0e,
  0xd8, 0x7d, 0x7b, 0xa7, 0xbe, 0x7f, 0x7d, 0xac,
  0x98, 0x03, 0xf0, 0xac, 0x98, 0x40, 0x74, 0x0c,
  0x48, 0x74, 0x13, 0xb4, 0x0e, 0xbb, 0x07, 0x00,
  0xcd, 0x10, 0xeb, 0xef, 0xbe, 0x82, 0x7d, 0xeb,
  0xe6, 0xbe, 0x80, 0x7d, 0xeb, 0xe1, 0xcd, 0x16,
  0x5e, 0x1f, 0x66, 0x8f, 0x04, 0xcd, 0x19, 0xbe,
  0x81, 0x7d, 0x8b, 0x7d, 0x1a, 0x8d, 0x45, 0xfe,
  0x8a, 0x4e, 0x0d, 0xf7, 0xe1, 0x03, 0x46, 0xfc,
  0x13, 0x56, 0xfe, 0xb1, 0x04, 0xe8, 0xc2, 0x00,
  0x72, 0xd7, 0xea, 0x00, 0x02, 0x70, 0x00, 0x52,
  0x50, 0x06, 0x53, 0x6a, 0x01, 0x6a, 0x10, 0x91,
  0x8b, 0x46, 0x18, 0xa2, 0x26, 0x05, 0x96, 0x92,
  0x33, 0xd2, 0xf7, 0xf6, 0x91, 0xf7, 0xf6, 0x42,
  0x87, 0xca, 0xf7, 0x76, 0x1a, 0x8a, 0xf2, 0x8a,
  0xe8, 0xc0, 0xcc, 0x02, 0x0a, 0xcc, 0xb8, 0x01,
  0x02, 0x80, 0x7e, 0x02, 0x0e, 0x75, 0x04, 0xb4,
  0x42, 0x8b, 0xf4, 0x8a, 0x56, 0x24, 0xcd, 0x13,
  0x61, 0x61, 0x72, 0x0a, 0x40, 0x75, 0x01, 0x42,
  0x03, 0x5e, 0x0b, 0x49, 0x75, 0x77, 0xc3, 0x03,
  0x18, 0x01, 0x27, 0x0d, 0x0a, 0x49, 0x6e, 0x76,
  0x61, 0x6c, 0x69, 0x64, 0x20, 0x73, 0x79, 0x73,
  0x74, 0x65, 0x6d, 0x20, 0x64, 0x69, 0x73, 0x6b,
  0xff, 0x0d, 0x0a, 0x44, 0x69, 0x73, 0x6b, 0x20,
  0x49, 0x2f, 0x4f, 0x20, 0x65, 0x72, 0x72, 0x6f,
  0x72, 0xff, 0x0d, 0x0a, 0x52, 0x65, 0x70, 0x6c,
  0x61, 0x63, 0x65, 0x20, 0x74, 0x68, 0x65, 0x20,
  0x64, 0x69, 0x73, 0x6b, 0x2c, 0x20, 0x61, 0x6e,
  0x64, 0x20, 0x74, 0x68, 0x65, 0x6e, 0x20, 0x70,
  0x72, 0x65, 0x73, 0x73, 0x20, 0x61, 0x6e, 0x79,
  0x20, 0x6b, 0x65, 0x79, 0x0d, 0x0a, 0x00, 0x00,
  0x49, 0x4f, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x53, 0x59, 0x53, 0x4d, 0x53, 0x44, 0x4f, 0x53,
  0x20, 0x20, 0x20, 0x53, 0x59, 0x53, 0x7f, 0x01,
  0x00, 0x41, 0xbb, 0x00, 0x07, 0x60, 0x66, 0x6a,
  0x00, 0xe9, 0x3b, 0xff, 0x00, 0x00, 0x55, 0xaa
};


unsigned char xp_bootsector[] = {
  0xeb, 0x3c, 0x90, 0x4d, 0x53, 0x44, 0x4f, 0x53,
  0x35, 0x2e, 0x30, 0x00, 0x02, 0x01, 0x01, 0x00,
  0x02, 0xe0, 0x00, 0x40, 0x0b, 0xf0, 0x09, 0x00,
  0x12, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x48,
  0x58, 0x43, 0x46, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41,
  0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0x33, 0xc9,
  0x8e, 0xd1, 0xbc, 0xf0, 0x7b, 0x8e, 0xd9, 0xb8,
  0x00, 0x20, 0x8e, 0xc0, 0xfc, 0xbd, 0x00, 0x7c,
  0x38, 0x4e, 0x24, 0x7d, 0x24, 0x8b, 0xc1, 0x99,
  0xe8, 0x3c, 0x01, 0x72, 0x1c, 0x83, 0xeb, 0x3a,
  0x66, 0xa1, 0x1c, 0x7c, 0x26, 0x66, 0x3b, 0x07,
  0x26, 0x8a, 0x57, 0xfc, 0x75, 0x06, 0x80, 0xca,
  0x02, 0x88, 0x56, 0x02, 0x80, 0xc3, 0x10, 0x73,
  0xeb, 0x33, 0xc9, 0x8a, 0x46, 0x10, 0x98, 0xf7,
  0x66, 0x16, 0x03, 0x46, 0x1c, 0x13, 0x56, 0x1e,
  0x03, 0x46, 0x0e, 0x13, 0xd1, 0x8b, 0x76, 0x11,
  0x60, 0x89, 0x46, 0xfc, 0x89, 0x56, 0xfe, 0xb8,
  0x20, 0x00, 0xf7, 0xe6, 0x8b, 0x5e, 0x0b, 0x03,
  0xc3, 0x48, 0xf7, 0xf3, 0x01, 0x46, 0xfc, 0x11,
  0x4e, 0xfe, 0x61, 0xbf, 0x00, 0x00, 0xe8, 0xe6,
  0x00, 0x72, 0x39, 0x26, 0x38, 0x2d, 0x74, 0x17,
  0x60, 0xb1, 0x0b, 0xbe, 0xa1, 0x7d, 0xf3, 0xa6,
  0x61, 0x74, 0x32, 0x4e, 0x74, 0x09, 0x83, 0xc7,
  0x20, 0x3b, 0xfb, 0x72, 0xe6, 0xeb, 0xdc, 0xa0,
  0xfb, 0x7d, 0xb4, 0x7d, 0x8b, 0xf0, 0xac, 0x98,
  0x40, 0x74, 0x0c, 0x48, 0x74, 0x13, 0xb4, 0x0e,
  0xbb, 0x07, 0x00, 0xcd, 0x10, 0xeb, 0xef, 0xa0,
  0xfd, 0x7d, 0xeb, 0xe6, 0xa0, 0xfc, 0x7d, 0xeb,
  0xe1, 0xcd, 0x16, 0xcd, 0x19, 0x26, 0x8b, 0x55,
  0x1a, 0x52, 0xb0, 0x01, 0xbb, 0x00, 0x00, 0xe8,
  0x3b, 0x00, 0x72, 0xe8, 0x5b, 0x8a, 0x56, 0x24,
  0xbe, 0x0b, 0x7c, 0x8b, 0xfc, 0xc7, 0x46, 0xf0,
  0x3d, 0x7d, 0xc7, 0x46, 0xf4, 0x29, 0x7d, 0x8c,
  0xd9, 0x89, 0x4e, 0xf2, 0x89, 0x4e, 0xf6, 0xc6,
  0x06, 0x96, 0x7d, 0xcb, 0xea, 0x03, 0x00, 0x00,
  0x20, 0x0f, 0xb6, 0xc8, 0x66, 0x8b, 0x46, 0xf8,
  0x66, 0x03, 0x46, 0x1c, 0x66, 0x8b, 0xd0, 0x66,
  0xc1, 0xea, 0x10, 0xeb, 0x5e, 0x0f, 0xb6, 0xc8,
  0x4a, 0x4a, 0x8a, 0x46, 0x0d, 0x32, 0xe4, 0xf7,
  0xe2, 0x03, 0x46, 0xfc, 0x13, 0x56, 0xfe, 0xeb,
  0x4a, 0x52, 0x50, 0x06, 0x53, 0x6a, 0x01, 0x6a,
  0x10, 0x91, 0x8b, 0x46, 0x18, 0x96, 0x92, 0x33,
  0xd2, 0xf7, 0xf6, 0x91, 0xf7, 0xf6, 0x42, 0x87,
  0xca, 0xf7, 0x76, 0x1a, 0x8a, 0xf2, 0x8a, 0xe8,
  0xc0, 0xcc, 0x02, 0x0a, 0xcc, 0xb8, 0x01, 0x02,
  0x80, 0x7e, 0x02, 0x0e, 0x75, 0x04, 0xb4, 0x42,
  0x8b, 0xf4, 0x8a, 0x56, 0x24, 0xcd, 0x13, 0x61,
  0x61, 0x72, 0x0b, 0x40, 0x75, 0x01, 0x42, 0x03,
  0x5e, 0x0b, 0x49, 0x75, 0x06, 0xf8, 0xc3, 0x41,
  0xbb, 0x00, 0x00, 0x60, 0x66, 0x6a, 0x00, 0xeb,
  0xb0, 0x4e, 0x54, 0x4c, 0x44, 0x52, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x0d, 0x0a, 0x52, 0x65,
  0x6d, 0x6f, 0x76, 0x65, 0x20, 0x64, 0x69, 0x73,
  0x6b, 0x73, 0x20, 0x6f, 0x72, 0x20, 0x6f, 0x74,
  0x68, 0x65, 0x72, 0x20, 0x6d, 0x65, 0x64, 0x69,
  0x61, 0x2e, 0xff, 0x0d, 0x0a, 0x44, 0x69, 0x73,
  0x6b, 0x20, 0x65, 0x72, 0x72, 0x6f, 0x72, 0xff,
  0x0d, 0x0a, 0x50, 0x72, 0x65, 0x73, 0x73, 0x20,
  0x61, 0x6e, 0x79, 0x20, 0x6b, 0x65, 0x79, 0x20,
  0x74, 0x6f, 0x20, 0x72, 0x65, 0x73, 0x74, 0x61,
  0x72, 0x74, 0x0d, 0x0a, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xac, 0xcb, 0xd8, 0x55, 0xaa
};
