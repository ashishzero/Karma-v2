#pragma once
#include "kCommon.h"

static const u8 kToneMapPS[] = {
0x44,0x58,0x42,0x43,0x43,0x39,0x38,0xc7,0x7e,0x2f,0xb9,0xe1,0x9a,0x9d,0x10,
0x2e,0x62,0x63,0x13,0xd3,0x01,0x00,0x00,0x00,0x08,0x05,0x00,0x00,0x05,0x00,
0x00,0x00,0x34,0x00,0x00,0x00,0xbc,0x01,0x00,0x00,0x14,0x02,0x00,0x00,0x48,
0x02,0x00,0x00,0x6c,0x04,0x00,0x00,0x52,0x44,0x45,0x46,0x80,0x01,0x00,0x00,
0x01,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x3c,0x00,0x00,
0x00,0x00,0x05,0xff,0xff,0x00,0x01,0x00,0x00,0x58,0x01,0x00,0x00,0x52,0x44,
0x31,0x31,0x3c,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x28,
0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xbc,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,
0x00,0x00,0xc4,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x04,
0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
0x0d,0x00,0x00,0x00,0xcd,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x05,0x00,0x00,
0x00,0x04,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x01,0x00,0x00,0x00,0x01,0x00,
0x00,0x00,0x0d,0x00,0x00,0x00,0xd6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x53,0x61,0x6d,0x70,0x6c,0x65,0x72,
0x00,0x48,0x64,0x72,0x49,0x6d,0x61,0x67,0x65,0x00,0x48,0x75,0x64,0x49,0x6d,
0x61,0x67,0x65,0x00,0x63,0x6f,0x6e,0x73,0x74,0x61,0x6e,0x74,0x73,0x00,0xd6,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x10,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x01,0x00,0x00,0x00,0x00,0x00,
0x00,0x0c,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x34,0x01,0x00,0x00,0x00,0x00,
0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,
0x00,0x00,0x00,0x49,0x6e,0x74,0x65,0x6e,0x73,0x69,0x74,0x79,0x00,0x66,0x6c,
0x6f,0x61,0x74,0x33,0x00,0xab,0xab,0xab,0x01,0x00,0x03,0x00,0x01,0x00,0x03,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2a,0x01,0x00,0x00,0x4d,
0x69,0x63,0x72,0x6f,0x73,0x6f,0x66,0x74,0x20,0x28,0x52,0x29,0x20,0x48,0x4c,
0x53,0x4c,0x20,0x53,0x68,0x61,0x64,0x65,0x72,0x20,0x43,0x6f,0x6d,0x70,0x69,
0x6c,0x65,0x72,0x20,0x31,0x30,0x2e,0x31,0x00,0x49,0x53,0x47,0x4e,0x50,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0f,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x53,0x56,
0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x00,0x54,0x45,0x58,0x43,0x4f,
0x4f,0x52,0x44,0x00,0xab,0xab,0xab,0x4f,0x53,0x47,0x4e,0x2c,0x00,0x00,0x00,
0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x00,
0x00,0x00,0x53,0x56,0x5f,0x54,0x61,0x72,0x67,0x65,0x74,0x00,0xab,0xab,0x53,
0x48,0x45,0x58,0x1c,0x02,0x00,0x00,0x50,0x00,0x00,0x00,0x87,0x00,0x00,0x00,
0x6a,0x08,0x00,0x01,0x59,0x00,0x00,0x04,0x46,0x8e,0x20,0x00,0x00,0x00,0x00,
0x00,0x01,0x00,0x00,0x00,0x5a,0x00,0x00,0x03,0x00,0x60,0x10,0x00,0x00,0x00,
0x00,0x00,0x58,0x18,0x00,0x04,0x00,0x70,0x10,0x00,0x00,0x00,0x00,0x00,0x55,
0x55,0x00,0x00,0x58,0x18,0x00,0x04,0x00,0x70,0x10,0x00,0x01,0x00,0x00,0x00,
0x55,0x55,0x00,0x00,0x62,0x10,0x00,0x03,0x32,0x10,0x10,0x00,0x01,0x00,0x00,
0x00,0x65,0x00,0x00,0x03,0xf2,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x68,0x00,
0x00,0x02,0x03,0x00,0x00,0x00,0x45,0x00,0x00,0x8b,0xc2,0x00,0x00,0x80,0x43,
0x55,0x15,0x00,0x72,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x10,0x10,0x00,
0x01,0x00,0x00,0x00,0x46,0x7e,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x10,
0x00,0x00,0x00,0x00,0x00,0x38,0x00,0x00,0x08,0x72,0x00,0x10,0x00,0x00,0x00,
0x00,0x00,0x46,0x02,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x82,0x20,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,0x00,0x00,0x0f,0x72,0x00,0x10,0x00,
0x01,0x00,0x00,0x00,0x46,0x02,0x10,0x00,0x00,0x00,0x00,0x00,0x02,0x40,0x00,
0x00,0x9c,0xc4,0xc0,0x3f,0x9c,0xc4,0xc0,0x3f,0x9c,0xc4,0xc0,0x3f,0x00,0x00,
0x00,0x00,0x02,0x40,0x00,0x00,0x8f,0xc2,0xf5,0x3c,0x8f,0xc2,0xf5,0x3c,0x8f,
0xc2,0xf5,0x3c,0x00,0x00,0x00,0x00,0x38,0x00,0x00,0x0a,0x72,0x00,0x10,0x00,
0x02,0x00,0x00,0x00,0x46,0x02,0x10,0x00,0x00,0x00,0x00,0x00,0x02,0x40,0x00,
0x00,0x9a,0x99,0x19,0x3f,0x9a,0x99,0x19,0x3f,0x9a,0x99,0x19,0x3f,0x00,0x00,
0x00,0x00,0x32,0x00,0x00,0x0f,0x72,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x46,
0x02,0x10,0x00,0x00,0x00,0x00,0x00,0x02,0x40,0x00,0x00,0xbf,0x9f,0xba,0x3f,
0xbf,0x9f,0xba,0x3f,0xbf,0x9f,0xba,0x3f,0x00,0x00,0x00,0x00,0x02,0x40,0x00,
0x00,0x3d,0x0a,0x17,0x3f,0x3d,0x0a,0x17,0x3f,0x3d,0x0a,0x17,0x3f,0x00,0x00,
0x00,0x00,0x32,0x00,0x00,0x0c,0x72,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x46,
0x02,0x10,0x00,0x02,0x00,0x00,0x00,0x46,0x02,0x10,0x00,0x00,0x00,0x00,0x00,
0x02,0x40,0x00,0x00,0x29,0x5c,0x0f,0x3e,0x29,0x5c,0x0f,0x3e,0x29,0x5c,0x0f,
0x3e,0x00,0x00,0x00,0x00,0x38,0x00,0x00,0x07,0x72,0x00,0x10,0x00,0x01,0x00,
0x00,0x00,0x46,0x02,0x10,0x00,0x01,0x00,0x00,0x00,0x46,0x02,0x10,0x00,0x02,
0x00,0x00,0x00,0x0e,0x20,0x00,0x07,0x72,0x00,0x10,0x00,0x00,0x00,0x00,0x00,
0x46,0x02,0x10,0x00,0x01,0x00,0x00,0x00,0x46,0x02,0x10,0x00,0x00,0x00,0x00,
0x00,0x45,0x00,0x00,0x8b,0xc2,0x00,0x00,0x80,0x43,0x55,0x15,0x00,0x72,0x00,
0x10,0x00,0x01,0x00,0x00,0x00,0x46,0x10,0x10,0x00,0x01,0x00,0x00,0x00,0x46,
0x7e,0x10,0x00,0x01,0x00,0x00,0x00,0x00,0x60,0x10,0x00,0x00,0x00,0x00,0x00,
0x00,0x20,0x00,0x07,0x72,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x46,0x02,0x10,
0x00,0x00,0x00,0x00,0x00,0x46,0x02,0x10,0x00,0x01,0x00,0x00,0x00,0x36,0x00,
0x00,0x05,0x82,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x40,0x00,0x00,0x00,
0x00,0x80,0x3f,0x3e,0x00,0x00,0x01,0x53,0x54,0x41,0x54,0x94,0x00,0x00,0x00,
0x0c,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,
0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};

