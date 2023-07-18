#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

#include "ps2mc.h"

time_t getMcDateTime(sceMcStDateTime* mc)
{
    struct tm ftm;

    ftm.tm_sec = mc->Sec;
    ftm.tm_min = mc->Min;
    ftm.tm_hour = mc->Hour;
    ftm.tm_mday = mc->Day;
    ftm.tm_mon = mc->Month - 1;
    ftm.tm_year = mc->Year - 1900;

    return (timegm(&ftm));
}

int extractPSU(const char *save)
{
    u32 displaySize = 0;
    FILE *psuFile, *psvFile;
    int numFiles, next;
    char dstName[128], dirName[64];
    u8 *data;
    ps2_McFsEntry entry;
    struct utimbuf new_times;
    
    psuFile = fopen(save, "rb");
    if(!psuFile)
        return 0;
    
    // Read main directory entry
    fread(&entry, 1, sizeof(ps2_McFsEntry), psuFile);
    numFiles = entry.length - 2;
    
    strcpy(dirName, entry.name);
    mkdir(entry.name, 0777);

    // Skip "." and ".."
    fseek(psuFile, sizeof(ps2_McFsEntry)*2, SEEK_CUR);
    
    // Copy each file entry
    for(int i = 0; i < numFiles; i++)
    {
        fread(&entry, 1, sizeof(ps2_McFsEntry), psuFile);
        data = malloc(entry.length);
        fread(data, 1, entry.length, psuFile);

		displaySize += entry.length;
	    printf(" %8d bytes  : %s\n", entry.length, entry.name);

        snprintf(dstName, sizeof(dstName), "%s/%s", dirName, entry.name);
        if((psvFile = fopen(dstName, "wb")) != NULL)
        {
            fwrite(data, 1, entry.length, psvFile);
            fclose(psvFile);
        }
        else
        {
            printf("[!] Error writing %s\n", dstName);
        }

        new_times.actime = getMcDateTime(&entry.created);
        new_times.modtime = getMcDateTime(&entry.modified);
        utime(dstName, &new_times);

        free(data);
        
        next = 1024 - (entry.length % 1024);
        if(next < 1024)
            fseek(psuFile, next, SEEK_CUR);
    }
    printf(" %8d Total bytes\n", displaySize);

    fclose(psuFile);
    
    return 1;
}
