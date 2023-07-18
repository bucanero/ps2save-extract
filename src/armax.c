#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "ps2mc.h"
#include "lzari.h"

#define  HEADER_MAGIC   "Ps2PowerSave"

typedef struct maxHeader
{
    char    magic[12];
    u32     crc;
    char    dirName[32];
    char    iconSysName[32];
    u32     compressedSize;
    u32     numFiles;

    // This is actually the start of the LZARI stream, but we need it to
    // allocate the buffer.
    u32     decompressedSize;
} maxHeader_t;

typedef struct maxEntry
{
    u32     length;
    char    name[32];
} maxEntry_t;



static void printMAXHeader(const maxHeader_t *header)
{
    if(!header)
        return;

    printf("Magic            : %.*s\n", (int)sizeof(header->magic), header->magic);
    printf("CRC              : %08X\n", header->crc);
    printf("dirName          : %.*s\n", (int)sizeof(header->dirName), header->dirName);
    printf("iconSysName      : %.*s\n", (int)sizeof(header->iconSysName), header->iconSysName);
    printf("compressedSize   : %u\n", header->compressedSize);
    printf("numFiles         : %u\n", header->numFiles);
    printf("decompressedSize : %u\n", header->decompressedSize);
}

static int roundUp(int i, int j)
{
    return (i + j - 1) / j * j;
}

int isMAXFile(const char *path)
{
    if(!path)
        return 0;

    FILE *f = fopen(path, "rb");
    if(!f)
        return 0;

    // Verify file size
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(len < (int)sizeof(maxHeader_t))
    {
        fclose(f);
        return 0;
    }

    // Verify header
    maxHeader_t header;
    fread(&header, 1, sizeof(maxHeader_t), f);
    fclose(f);

    printMAXHeader(&header);

    return (header.compressedSize > 0) &&
           (header.decompressedSize > 0) &&
           (header.numFiles > 0) &&
           strncmp(header.magic, HEADER_MAGIC, sizeof(header.magic)) == 0 &&
           strlen(header.dirName) > 0 &&
           strlen(header.iconSysName) > 0;
}

int extractMAX(const char *save)
{
    if (!isMAXFile(save))
    	return 0;
    
    FILE *f = fopen(save, "rb");
    if(!f)
        return 0;

    maxHeader_t header;
    fread(&header, 1, sizeof(maxHeader_t), f);

    char dirName[sizeof(header.dirName) + 1];
    char psvName[128];

    memcpy(dirName, header.dirName, sizeof(header.dirName));
    dirName[32] = '\0';
    mkdir(dirName, 0777);

    // Get compressed file entries
    u8 *compressed = malloc(header.compressedSize);

    fseek(f, sizeof(maxHeader_t) - 4, SEEK_SET); // Seek to beginning of LZARI stream.
    u32 ret = fread(compressed, 1, header.compressedSize, f);
    fclose(f);
    if(ret != header.compressedSize)
    {
        printf("Compressed size: actual=%d, expected=%d\n", ret, header.compressedSize);
        free(compressed);
        return 0;
    }

    u8 *decompressed = malloc(header.decompressedSize);

    ret = unlzari(compressed, header.compressedSize, decompressed, header.decompressedSize);
    // As with other save formats, decompressedSize isn't acccurate.
    if(ret == 0)
    {
        printf("Decompression failed.\n");
        free(decompressed);
        free(compressed);
        return 0;
    }

    free(compressed);

    int i;
    u32 offset = 0;
    u32 displaySize = 0;
    maxEntry_t *entry;
    FILE* psv;

	printf("\nSave contents:\n");

	// Write the file's data
    for(i = 0, offset = 0; i < (int)header.numFiles; i++)
    {
        entry = (maxEntry_t*) &decompressed[offset];
        offset += sizeof(maxEntry_t);
	    printf(" %8d bytes  : %s\n", entry->length, entry->name);

        snprintf(psvName, sizeof(psvName), "%s/%s", dirName, entry->name);
        if ((psv = fopen(psvName, "wb")) != NULL)
        {
            fwrite(&decompressed[offset], 1, entry->length, psv);
            fclose(psv);
        }
        else
        {
            printf("[!] Error writing %s\n", psvName);
        }
 
        offset = roundUp(offset + entry->length + 8, 16) - 8;
		displaySize += entry->length;
    }
    printf(" %8d Total bytes\n", displaySize);

    free(decompressed);
    
    return 1;
}
