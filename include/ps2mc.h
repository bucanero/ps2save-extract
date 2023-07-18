#include <inttypes.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct
{
    uint32_t saveSize;          // e.g. 0x00200000
    uint32_t startOfSaveData;   // always 0x84000000 (132)
    uint32_t blockSize;         // always 0x00020000 (512). Block size?
    uint32_t padding1;          // always 0x00000000?
    uint32_t padding2;          // always 0x00000000?
    uint32_t padding3;          // always 0x00000000?
    uint32_t padding4;          // always 0x00000000?
    uint32_t dataSize;          // save size repeated?
    uint32_t unknown1;          // always 0x03900000 (36867)?
    char prodCode[20];          // 20 bytes, 0x00 filled & terminated
    uint32_t padding6;          // always 0x00000000?
    uint32_t padding7;          // always 0x00000000?
    uint32_t padding8;          // always 0x00000000?
} ps1_header_t;

typedef struct _sceMcStDateTime {
    u8  Resv2;
    u8  Sec;
    u8  Min;
    u8  Hour;
    u8  Day;
    u8  Month;
    u16 Year;
} sceMcStDateTime;

typedef struct {                // size = 512
    u16 mode;                   // 0
    u16 unused;                 // 2
    u32 length;                 // 4
    sceMcStDateTime created;    // 8
    u32 cluster;                // 16
    u32 dir_entry;              // 20
    sceMcStDateTime modified;   // 24
    u32 attr;                   // 32
    u32 unused2[7];             // 36
    char name[32];              // 64
    u8  unused3[416];           // 96
} ps2_McFsEntry;

typedef struct {
	char magic[4];
	u32 padding1;               //always 0x00000000
	u8 salt[20];
	u8 signature[20];           //digital sig
	u32 padding2;               //always 0x00000000
	u32 padding3;               //always 0x00000000
	u32 headerSize;             //always 0x0000002C in PS2, 0x00000014 in PS1. 
	u32 saveType;               //0x00000002 PS2, 0x00000001 PS1
} psv_header_t;

typedef struct {
	u32 displaySize;            //PS3 will just round this up to the neaest 1024 boundry so just make it as good as possible
	u32 sysPos;                 //location in file of icon.sys
	u32 sysSize;                //icon.sys size
	u32 icon1Pos;               //position of 1st icon
	u32 icon1Size;              //size of 1st icon
	u32 icon2Pos;               //position of 2nd icon
	u32 icon2Size;              //size of 2nd icon
	u32 icon3Pos;               //position of 3rd icon
	u32 icon3Size;              //size of 3rd icon
	u32 numberOfFiles;
} ps2_header_t;

typedef struct {
	sceMcStDateTime create;
	sceMcStDateTime modified;
	u32 numberOfFilesInDir;     // this is likely to be number of files in dir + 2 ("." and "..")
	u32 attribute;              // (0x00008427 dir)
	char filename[32];
} ps2_MainDirInfo_t;

typedef struct {
	sceMcStDateTime create;
	sceMcStDateTime modified;
	u32 filesize;
	u32 attribute;             // (0x00008497 file)
	char filename[32];         // 'Real' PSV files have junk in this after text.
	u32 positionInFile;
} ps2_FileInfo_t;

typedef struct {
    char magic[4];
    u16 padding1;             // 0000
    u16 secondLineOffset;
    u32 padding2;             // 00000000
    u32 transparencyVal;      // 0x00 (clear) to 0x80 (opaque)
    u8 bgColourUpperLeft[16];
    u8 bgColourUpperRight[16];
    u8 bgColourLowerLeft[16];
    u8 bgColourLowerRight[16];
    u8 light1Direction[16];
    u8 light2Direction[16];
    u8 light3Direction[16];
    u8 light1RGB[16];
    u8 light2RGB[16];
    u8 light3RGB[16];
    u8 ambientLightRGB[16];
    char title[68];            // null terminated, S-JIS
    char IconName[64];         // null terminated
    char copyIconName[64];     // null terminated
    char deleteIconName[64];   // null terminated
    u8 padding3[512];
} ps2_IconSys_t;
