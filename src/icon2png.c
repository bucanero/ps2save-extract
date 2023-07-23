/*
* icon2png 0.1.0 - Simple tool to extract a PS2 icon texture as .png
*
* Copyright (c) 2023 Bucanero <github.com/bucanero>
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy  of  this   software  and  associated   documentation  files  (the
* "Software"),  to deal  in the Software  without  restriction, including
* without  limitation  the rights to  use, copy,  modify, merge, publish,
* distribute,  sublicense, and/or  sell  copies of the  Software, and  to
* permit persons to  whom the Software is furnished  to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR   IMPLIED,  INCLUDING   BUT  NOT  LIMITED  TO   THE  WARRANTIES   OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR  PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE  AUTHORS OR COPYRIGHT HOLDERS  BE LIABLE FOR  ANY
* CLAIM,  DAMAGES  OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT  OR OTHERWISE,  ARISING FROM,  OUT OF  OR IN  CONNECTION  WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ps2icon.h"
#include "svpng.h"


static uint32_t TIM2RGBA(uint16_t iData)
{
	unsigned char	lRGBA[4];
	uint16_t	lRGB;
	uint32_t	*lReturn = (uint32_t *) &lRGBA;

	lRGB = iData;
	lRGBA[0] = 8 * (lRGB & 0x1F);
	lRGBA[1] = 8 * ((lRGB >> 5) & 0x1F);
	lRGBA[2] = 8 * (lRGB >> 10);
	lRGBA[3] = 0xFF;

	return *lReturn;
}

static void* ps2iconTexture(const char* icon_file)
{
	FILE* fp;
	Icon_Header header;
	Animation_Header anim_header;
	Frame_Data animation;
	uint16_t j, iData;
	uint32_t *lTexturePtr, *lRGBA;

	fp = fopen(icon_file, "rb");
	if (!fp)
	{
		printf("[!] error: can't open file\n");
		return NULL;
	}

	//read header:
	fread(&header, sizeof(Icon_Header), 1, fp);

	/* Check magic number to see that it is a icn file
	 */
	if (header.file_id != 0x010000)
	{
		printf("[!] error: Not an .icn file\n");
		return NULL;
	}

	//n_vertices has to be divisible by three, that's for sure:
	if(header.n_vertices % 3 != 0) {
		printf("[!] error: invalid number of vertices\n");
		return NULL;
	}

	printf("    - Shapes  : %d\n", header.animation_shapes);
	printf("    - Vertices: %d\n", header.n_vertices);

	//read icon data from file:
	///Vertex data
	// each vertex consists of animation_shapes tuples for vertex coordinates,
	// followed by one vertex coordinate tuple for normal coordinates
	// followed by one texture data tuple for texture coordinates and color
	for(unsigned int i=0; i<header.n_vertices; i++) {
		fseek(fp, sizeof(Vertex_Coord) * header.animation_shapes, SEEK_CUR);
		fseek(fp, sizeof(Vertex_Coord), SEEK_CUR);
		fseek(fp, sizeof(Texture_Data), SEEK_CUR);
	}

	//animation data
	// preceeded by an animation header, there is a frame data/key set for every frame:
	fread(&anim_header, sizeof(Animation_Header), 1, fp);

	//read animation data:
	for(unsigned int i=0; i<anim_header.n_frames; i++) {
		fread(&animation, sizeof(Frame_Data), 1, fp);
		if(animation.n_keys > 0) {
			fseek(fp, sizeof(Frame_Key) * animation.n_keys, SEEK_CUR);
		}
	}

	printf("[i] Texture data start: 0x%08lX\n", ftell(fp));

	lTexturePtr = (uint32_t *) malloc(128 * 128 * 4);
	lRGBA = lTexturePtr;

	if (header.texture_type <= 7)
	{	// Uncompressed texture
		for (int i = 0; i < (128 * 128); i++)
		{
			fread(&iData, sizeof(uint16_t), 1, fp);
			*lRGBA = TIM2RGBA(iData);
			lRGBA++;
		}
	}
	else
	{	//Compressed texture
		// compressed size
//		iData += 4;
		fseek(fp, 4, SEEK_CUR);
		do
		{
			fread(&iData, sizeof(uint16_t), 1, fp);
			j = iData;
			if (0xFF00 == (j & 0xFF00))
			{
				for (j = (0x0000 - j) & 0xFFFF; j > 0; j--)
				{
					fread(&iData, sizeof(uint16_t), 1, fp);
					*lRGBA = TIM2RGBA(iData);
					lRGBA++;
				}
			}
			else
			{
				fread(&iData, sizeof(uint16_t), 1, fp);
				for (; j > 0; j--)
				{
					*lRGBA = TIM2RGBA(iData);
					lRGBA++;
				}
			}
		} while ((lRGBA - lTexturePtr) < 0x4000);
	}

	printf("[i] Texture data end  : 0x%08lX\n", ftell(fp));
	fclose(fp);

	return (lTexturePtr);
}


int main(int argc, char **argv)
{
	FILE* fout;
	char pngName[256];
	uint8_t *ip_texture;

	printf("\n --- PS2 icon to .png - by Bucanero ---\n\n");
	if (argc != 2)
	{
		printf("usage: %s <file>\n\n", *argv);
		return 1;
	}

	/*
	 * Open an .icn icon file for testing, use fstat() to get filesize and malloc()
	 * a buffer for it.  Read the whole file into the buffer and close the file.
	 */
	argv++;
	printf("[i] Opening file: %s ...\n", *argv);
	
	ip_texture = ps2iconTexture(*argv);

	sprintf(pngName, "%s.png", *argv);
	printf("[+] Saving %s ...", pngName);

	/*
	 * Create a new file and write the raw texture buffer as PNG
	 */
	fout = fopen(pngName, "wb");
	if (!fout)
	{
		printf(" Error: Can't create .png file!\n\n");
		return 1;
	}
	svpng(fout, 128, 128, ip_texture, 1);
	fclose(fout);
	printf(" Done!\n\n");

	return 0;
}
