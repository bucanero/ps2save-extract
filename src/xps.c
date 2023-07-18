#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "ps2mc.h"

#define XPS_HEADER_MAGIC "SharkPortSave\0\0\0"
#define mode_swap(M)     ((M & 0x00FF) << 8) + ((M & 0xFF00) >> 8)

typedef struct __attribute__((__packed__)) xpsEntry
{
    uint16_t entry_sz;
    char name[64];
    uint32_t length;
    uint32_t start;
    uint32_t end;
    uint32_t mode;
    sceMcStDateTime created;
    sceMcStDateTime modified;
    char unk1[4];
    char padding[12];
    char title_ascii[64];
    char title_sjis[64];
    char unk2[8];
} xpsEntry_t;


int extractXPS(const char *save)
{
    u32 displaySize = 0;
    FILE *xpsFile, *psvFile;
    int numFiles, i;
    char dstName[128];
    char tmp[100];
    u32 len;
    u8 *data;
    xpsEntry_t entry;
    
    xpsFile = fopen(save, "rb");
    if(!xpsFile)
        return 0;

    fread(&tmp, 1, 0x15, xpsFile);

    if (memcmp(&tmp[4], XPS_HEADER_MAGIC, 16) != 0)
    {
        fclose(xpsFile);
        return 0;
    }

    // Skip the variable size header
    fread(&len, 1, sizeof(u32), xpsFile);
    fread(&tmp, 1, len, xpsFile);
    fread(&len, 1, sizeof(u32), xpsFile);
    fread(&tmp, 1, len, xpsFile);
    fread(&len, 1, sizeof(u32), xpsFile);
    fread(&len, 1, sizeof(u32), xpsFile);
    
    // Read main directory entry
    fread(&entry, 1, sizeof(xpsEntry_t), xpsFile);
    numFiles = entry.length - 2;

    // Keep the file position (start of file entries)
    len = ftell(xpsFile);
    
    mkdir(entry.name, 0777);
    strcpy(tmp, entry.name);

    // Copy each file entry
    for(i = 0; i < numFiles; i++)
    {
        fread(&entry, 1, sizeof(xpsEntry_t), xpsFile);
        data = malloc(entry.length);
        fread(data, 1, entry.length, xpsFile);

		displaySize += entry.length;
	    printf(" %8d bytes  : %s\n", entry.length, entry.name);

        snprintf(dstName, sizeof(dstName), "%s/%s", tmp, entry.name);
        if((psvFile = fopen(dstName, "wb")) != NULL)
        {
            fwrite(data, 1, entry.length, psvFile);
            fclose(psvFile);
        }
        else
        {
            printf("[!] Error writing %s\n", dstName);
        }

        free(data);
    }
    printf(" %8d Total bytes\n", displaySize);

    fclose(xpsFile);
    
    return 1;
}
