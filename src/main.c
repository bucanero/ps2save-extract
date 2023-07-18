/*
*
*	PS2 Save Extractor - (c) 2023 by Bucanero - www.bucanero.com.ar
*
* PS2 Save format code from:
*	- https://github.com/PMStanley/PSV-Exporter
*	- https://github.com/root670/CheatDevicePS2
*
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <sys/stat.h>

#include "aes.h"
#include "sha1.h"
#include "ps2mc.h"

#define SEED_OFFSET 0x8
#define HASH_OFFSET 0x1c
#define TYPE_OFFSET 0x3C

#define PSV_MAGIC 0x50535600

const uint8_t key[2][0x10] = {
							{0xFA, 0x72, 0xCE, 0xEF, 0x59, 0xB4, 0xD2, 0x98, 0x9F, 0x11, 0x19, 0x13, 0x28, 0x7F, 0x51, 0xC7}, 
							{0xAB, 0x5A, 0xBC, 0x9F, 0xC1, 0xF4, 0x9D, 0xE6, 0xA0, 0x51, 0xDB, 0xAE, 0xFA, 0x51, 0x88, 0x59}
						};
const uint8_t iv[0x10] = {0xB3, 0x0F, 0xFE, 0xED, 0xB7, 0xDC, 0x5E, 0xB7, 0x13, 0x3D, 0xA6, 0x0D, 0x1B, 0x6B, 0x2C, 0xDC};


int extractPSU(const char *save);
int extractMAX(const char *save);
int extractMCS(const char *save);
int extractPSX(const char *save);
int extractCBS(const char *save);
int extractXPS(const char *save);

char* endsWith(const char * a, const char * b)
{
	int al = strlen(a), bl = strlen(b);
    
	if (al < bl)
		return NULL;

	a += (al - bl);
	while (*a)
		if (toupper(*a++) != toupper(*b++)) return NULL;

	return (char*) (a - bl);
}

static void usage(char *argv[])
{
	printf("This tool extracts data files from PS1/PS2 saves.\n\n");
	printf("USAGE: %s <filename>\n\n", argv[0]);
	printf("INPUT FORMATS\n");
	printf(" .mcs            PS1 MCS File\n");
	printf(" .psx            PS1 AR/GS/XP PSX File\n");
	printf(" .cbs            PS2 CodeBreaker File\n");
	printf(" .max            PS2 ActionReplay Max File\n");
	printf(" .xps            PS2 Xploder File\n");
	printf(" .sps            PS2 SharkPort File\n");
	printf(" .psu            PS2 EMS File (uLaunchELF)\n");
	printf(" .psv            PS3 PSV File (PlayStation 3)\n\n");
	return;
}

int ps1_psv2mcs(const char* psvfile)
{
	char dstName[256];
	uint8_t mcshdr[128];
	size_t sz;
	uint8_t *input;
	FILE *pf;
	ps1_header_t *ps1h;

    pf = fopen(psvfile, "rb");
    if(!pf)
        return 0;

    fseek(pf, 0, SEEK_END);
    sz = ftell(pf);
    fseek(pf, 0, SEEK_SET);
    input = malloc(sz);
    fread(input, 1, sz, pf);
    fclose(pf);

	snprintf(dstName, sizeof(dstName), "%s", psvfile);
	strcpy(strrchr(dstName, '.'), ".mcs");
	pf = fopen(dstName, "wb");
	if (!pf) {
		perror("Failed to open output file");
		free(input);
		return 0;
	}
	
	ps1h = (ps1_header_t*)(input + 0x40);

	memset(mcshdr, 0, sizeof(mcshdr));
	memcpy(mcshdr + 4, &ps1h->saveSize, 4);
	memcpy(mcshdr + 56, &ps1h->saveSize, 4);
	memcpy(mcshdr + 10, ps1h->prodCode, sizeof(ps1h->prodCode));
	mcshdr[0] = 0x51;
	mcshdr[8] = 0xFF;
	mcshdr[9] = 0xFF;

	for (int x=0; x<127; x++)
		mcshdr[127] ^= mcshdr[x];

	fwrite(mcshdr, sizeof(mcshdr), 1, pf);
	fwrite(input + 0x84, sz - 0x84, 1, pf);
	fclose(pf);
	free(input);

	printf("MCS generated successfully: %s\n", dstName);
	return 1;
}

int ps2_psv2psu(const char *save)
{
    u32 dataPos = 0;
    FILE *psuFile, *psvFile;
    int numFiles, displaySize=0;
    char dstName[256];
    u8 *data;
    ps2_McFsEntry entry;
    ps2_MainDirInfo_t ps2md;
    ps2_FileInfo_t ps2fi;
    
    psvFile = fopen(save, "rb");
    if(!psvFile)
        return 0;

    // Read main directory entry
    fseek(psvFile, 0x68, SEEK_SET);
    fread(&ps2md, sizeof(ps2_MainDirInfo_t), 1, psvFile);
    numFiles = (ps2md.numberOfFilesInDir) - 2;
    mkdir(ps2md.filename, 0777);

	printf("\nSave contents:\n");

    for (; numFiles > 0; numFiles--)
    {
        fread(&ps2fi, sizeof(ps2_FileInfo_t), 1, psvFile);
        dataPos = ftell(psvFile);
        data = malloc(ps2fi.filesize);
        fseek(psvFile, ps2fi.positionInFile, SEEK_SET);
        fread(data, 1, ps2fi.filesize, psvFile);

		displaySize += ps2fi.filesize;
	    printf(" %8d bytes  : %s\n", ps2fi.filesize, ps2fi.filename);

	    snprintf(dstName, sizeof(dstName), "%s/%s", ps2md.filename, ps2fi.filename);
	    if((psuFile = fopen(dstName, "wb")) != NULL)
	    {
	        fwrite(data, 1, ps2fi.filesize, psuFile);
	        fclose(psuFile);
	    }
        else
        {
            printf("[!] Error writing %s\n", dstName);
        }

        free(data);
        fseek(psvFile, dataPos, SEEK_SET);
    }
    printf(" %8d Total bytes\n", displaySize);

    fclose(psvFile);

    return 1;
}

int extractPSV(const char* psvfile)
{
	uint8_t input[0x40];
	FILE *pf;

    pf = fopen(psvfile, "rb");
    if(!pf)
        return 0;

    fread(input, 1, sizeof(input), pf);
    fclose(pf);

	if (*(uint32_t*) input != PSV_MAGIC) {
		printf("Not a .psv file\n");
		return 0;
	}

	printf("Exporting %s file...\n", psvfile);
	
	switch (input[TYPE_OFFSET])
	{
		case 1:
			ps1_psv2mcs(psvfile);
			break;

		case 2:
			ps2_psv2psu(psvfile);
			break;
			
		default:
			printf("Unsupported .psv type\n");
			return 0;
	}

	return 1;
}

int main(int argc, char **argv)
{
	printf("\n PS2 Save Extractor v1.0.0 - (c) 2023 by Bucanero\n\n");

	if (argc != 2) {
		usage(argv);
		return 1;
	}

	if (endsWith(argv[1], ".max"))
		extractMAX(argv[1]);

	else if (endsWith(argv[1], ".psu"))
		extractPSU(argv[1]);

	else if (endsWith(argv[1], ".mcs"))
		extractMCS(argv[1]);

	else if (endsWith(argv[1], ".psx"))
		extractPSX(argv[1]);

	else if (endsWith(argv[1], ".cbs"))
		extractCBS(argv[1]);

	else if (endsWith(argv[1], ".xps") || endsWith(argv[1], ".sps"))
		extractXPS(argv[1]);

	else if (endsWith(argv[1], ".psv"))
		extractPSV(argv[1]);

	else
		usage(argv);

	return 0;
}
