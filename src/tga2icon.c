/*
* tga2icon 0.1.0 - Simple tool to convert a .tga image to a PS2 icon
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
#include <string.h>
#include <math.h>

#include "cube.h"
#include "billboard.h"

/*
   TGA file decoding based on:
   http://www.paulbourke.net/dataformats/tga/

   The following is rather crude demonstration code to read 
   uncompressed and compressed TGA files of 16, 24, or 32 bit 
   TGA. 
   Hope it is helpful.
*/

typedef struct __attribute__((packed)) {
   char  idlength;
   char  colourmaptype;
   char  datatypecode;
   short int colourmaporigin;
   short int colourmaplength;
   char  colourmapdepth;
   short int x_origin;
   short int y_origin;
   short width;
   short height;
   char  bitsperpixel;
   char  imagedescriptor;
} HEADER;

typedef struct {
   unsigned char r,g,b,a;
} PIXEL;


void MergeBytes(PIXEL *pixel,unsigned char *p,int bytes)
{
   if (bytes == 4) {
      pixel->r = p[2];
      pixel->g = p[1];
      pixel->b = p[0];
      pixel->a = p[3];
   } else if (bytes == 3) {
      pixel->r = p[2];
      pixel->g = p[1];
      pixel->b = p[0];
      pixel->a = 255;
   } else if (bytes == 2) {
      pixel->r = (p[1] & 0x7c) << 1;
      pixel->g = ((p[1] & 0x03) << 6) | ((p[0] & 0xe0) >> 2);
      pixel->b = (p[0] & 0x1f) << 3;
      pixel->a = (p[1] & 0x80);
   }
}

void EncodeIcon(FILE* fout, const PIXEL* texture)
{
	//compressed textures:
	//allocate space for size info:
	int base = ftell(fout);
	unsigned int size = 0;
	fwrite(&base, 4, 1, fout);	//dummy data
	//rle main loop:
	int i=0;
	while(i<16384) {
		//rle step 1: count replication
		unsigned short rep_count = 1;
		while( memcmp(&texture[i], &texture[i+rep_count], sizeof(PIXEL)) == 0 ) { rep_count++; }
		if(rep_count > 1) {
			//pixels are replicated rep_count times
			unsigned short c = ((unsigned short)(texture[i].r >> 3) | (texture[i].g << 2) | (texture[i].b << 7) );
			fwrite(&rep_count, 2, 1, fout);
			fwrite(&c, 2, 1, fout);
			i += rep_count;
		} else {
			//no replication
			unsigned short pix_count = 0;	//number of non equal subsequent pixels
			while(memcmp(&texture[i+pix_count], &texture[i+pix_count+1], sizeof(PIXEL)) &&
			      (pix_count < 255) ) { pix_count++; }
			pix_count = 0xFFFF ^ (pix_count-1);
			fwrite(&pix_count, 2, 1, fout);
			pix_count ^= 0xFFFF;
			for(unsigned short j=0; j<=pix_count; j++) {
				//insert pixels:
				unsigned short c = ((unsigned short)(texture[i].r >> 3) | (texture[i].g << 2) | (texture[i].b << 7) );
				fwrite(&c, 2, 1, fout);
				i++;
			}
		}
	}
	//finalize: write size of the encoded segment to base:
	size = ftell(fout);
	size -= (base + 4);

	fseek(fout, base, SEEK_SET);
	fwrite(&size, 4, 1, fout);
}

int main(int argc,char **argv)
{
   int n=0,i,j;
   int bytes2read,skipover = 0;
   unsigned char p[5];
   FILE *fptr;
   HEADER header;
   PIXEL *pixels;

	printf("\n --- .TGA to PS2 icon - by Bucanero ---\n\n");
   if (argc < 2) {
      fprintf(stderr,"Usage: %s file.tga\n\n",argv[0]);
      exit(-1);
   }

   /* Open the file */
   if ((fptr = fopen(argv[1],"rb")) == NULL) {
      fprintf(stderr,"File open failed\n");
      exit(-1);
   }

   /* Display the header fields */
   fread(&header, sizeof(HEADER), 1, fptr);
   fprintf(stderr,"[ .TGA HEADER INFO ]\n");
   fprintf(stderr,"ID length:         %d\n",header.idlength);
   fprintf(stderr,"Colourmap type:    %d\n",header.colourmaptype);
   fprintf(stderr,"Image type:        %d\n",header.datatypecode);
   fprintf(stderr,"Colour map offset: %d\n",header.colourmaporigin);
   fprintf(stderr,"Colour map length: %d\n",header.colourmaplength); 
   fprintf(stderr,"Colour map depth:  %d\n",header.colourmapdepth);
   fprintf(stderr,"X origin:          %d\n",header.x_origin);
   fprintf(stderr,"Y origin:          %d\n",header.y_origin);
   fprintf(stderr,"Width:             %d\n",header.width);
   fprintf(stderr,"Height:            %d\n",header.height);
   fprintf(stderr,"Bits per pixel:    %d\n",header.bitsperpixel);
   fprintf(stderr,"Descriptor:        %d\n",header.imagedescriptor);

   /* Allocate space for the image */
   if ((pixels = malloc(header.width*header.height*sizeof(PIXEL))) == NULL) {
      fprintf(stderr,"malloc of image failed\n");
      exit(-1);
   }
   memset(pixels, 0, header.width*header.height*sizeof(PIXEL));

   /* What can we handle */
   if (header.datatypecode != 2 && header.datatypecode != 10) {
      fprintf(stderr,"Can only handle image type 2 and 10\n");
      exit(-1);
   }
   if (header.bitsperpixel != 16 && 
       header.bitsperpixel != 24 && header.bitsperpixel != 32) {
      fprintf(stderr,"Can only handle pixel depths of 16, 24, and 32\n");
      exit(-1);
   }
   if (header.colourmaptype != 0 && header.colourmaptype != 1) {
      fprintf(stderr,"Can only handle colour map types of 0 and 1\n");
      exit(-1);
   }
   if (header.width != 128 || header.height != 128) {
      fprintf(stderr,"Can only handle 128x128 images\n");
      exit(-1);
   }

   /* Skip over unnecessary stuff */
   skipover += header.idlength;
   skipover += header.colourmaptype * header.colourmaplength;
   fprintf(stderr,"Skip over %d bytes\n",skipover);
   fseek(fptr,skipover,SEEK_CUR);

   /* Read the image */
   bytes2read = header.bitsperpixel / 8;
   while (n < header.width * header.height) {
      if (header.datatypecode == 2) {                     /* Uncompressed */
         if (fread(p,1,bytes2read,fptr) != bytes2read) {
            fprintf(stderr,"Unexpected end of file at pixel %d\n",i);
            exit(-1);
         }
         MergeBytes(&(pixels[n]),p,bytes2read);
         n++;
      } else if (header.datatypecode == 10) {             /* Compressed */
         if (fread(p,1,bytes2read+1,fptr) != bytes2read+1) {
            fprintf(stderr,"Unexpected end of file at pixel %d\n",i);
            exit(-1);
         }
         j = p[0] & 0x7f;
         MergeBytes(&(pixels[n]),&(p[1]),bytes2read);
         n++;
         if (p[0] & 0x80) {         /* RLE chunk */
            for (i=0;i<j;i++) {
               MergeBytes(&(pixels[n]),&(p[1]),bytes2read);
               n++;
            }
         } else {                   /* Normal chunk */
            for (i=0;i<j;i++) {
               if (fread(p,1,bytes2read,fptr) != bytes2read) {
                  fprintf(stderr,"Unexpected end of file at pixel %d\n",i);
                  exit(-1);
               }
               MergeBytes(&(pixels[n]),p,bytes2read);
               n++;
            }
         }
      }
   }
   fclose(fptr);

   /* Write the result as a compressed PS2 icon */
   if ((fptr = fopen("tgatest.ico","wb")) == NULL) {
      fprintf(stderr,"Failed to open output file\n");
      exit(-1);
   }

   if (argc == 3 && argv[2][0] == 'c')
      fwrite(cube, sizeof(cube), 1, fptr);
   else
      fwrite(billboard, sizeof(billboard), 1, fptr);

   EncodeIcon(fptr, pixels);
   fclose(fptr);
   
   free(pixels);
   fprintf(stderr,"\n PS2 icon created OK!\n\n");
}
