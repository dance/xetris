//---------------------------------------------------------------------------

#ifndef TARGA_H
#define TARGA_H

#include <stdio.h>

// only for debug!
#ifdef TARGA_DEBUG
	#include <fstream>
	#include <time.h>
	using namespace std;

	static int files_num = 0;
#endif

#define SAFE_FREE(p)      { if (p) { free((p)); (p)=NULL; } }

#pragma warning( disable: 4018 )	// disable " '<' : signed/unsigned mismatch " warning.

#include <memory.h>
#include "targa.h"
#include "stdlib.h"
//---------------------------------------------------------------------------

// i got this function from Game programming of Andre Lamoth.
int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height);

// load targa image
int TGA_Load_Image(TGA_Image *tga_image,const char *filename)
{
    // check for correct parameters
    if (tga_image->buffer)
       free(tga_image->buffer);
    if (!filename)
        return TLI_WRONGPARAMS;

    // targa header
    TGA_File_Header tgafh;
    FILE *ftga;

    // read targa file header
    if (fopen_s(&ftga, filename, "rb") != 0)
        return TLI_ERRORFOPEN;

    fread(&tgafh,sizeof tgafh,1,ftga);

#ifdef TARGA_DEBUG

	ofstream dbg_out;	// writing log info to this file

	if (files_num == 0) {	// first file is being loading
		dbg_out.open("targa.log");
		char tmpbuf[128];

		_strdate_s( tmpbuf, 128 );	// get date
		dbg_out << "Current date/time: " << tmpbuf << " " ;
		_strtime_s( tmpbuf, 128 );	// get time
		dbg_out << tmpbuf << '\n' << endl;
	}
	else	// we already have some loaded files, lets open log file for appending info
		dbg_out.open("targa.log", ios::out | ios::app);

	dbg_out << "File: " << filename << endl;
	dbg_out << "IdLength: " << (int)tgafh.IdLength << endl;
	dbg_out << "Color Map: " << (int)tgafh.ColorMap << endl;
	dbg_out << "Data Type: " << (int)tgafh.DataType << endl;
	dbg_out << "Color map info: " << tgafh.ColorMapInfo << endl;
	dbg_out << "X_Origin: " << tgafh.X_Origin << endl;
	dbg_out << "Y_origin: " << tgafh.Y_Origin << endl;
	dbg_out << "Width: " << tgafh.Image_Width << endl;
	dbg_out << "Height: " << tgafh.Image_Height << endl;
	dbg_out << "Bit per pixel: " << (int)tgafh.Bit_Per_Pixel << endl;
	dbg_out << "Description: " << (int)tgafh.Description << endl << "\n";

#endif

    if (tgafh.Bit_Per_Pixel != 24) {
        fclose(ftga);
        return TLI_WRONGFORMAT;
    }

    tga_image->Width = tgafh.Image_Width;
    tga_image->Height = tgafh.Image_Height;

	fseek(ftga,tgafh.IdLength,SEEK_CUR);	// skip IdLength bytes

    // vars for reading image
    UCHAR   BlockInfo;
	UCHAR		R,G,B;
    char    WhatItIs;   // packed or unpacked data
    UINT    NumPixels;  // num of pixels in packed pixel

    // alloc mem
    tga_image->buffer = (USHORT *)malloc(sizeof(USHORT)*tga_image->Width * tga_image->Height);

	if (tgafh.DataType == 10)		// 10 - packed data
	{
    for (UINT i=0; i < tga_image->Width * tga_image->Height; )
    {
        fread(&BlockInfo,1,1,ftga);

        WhatItIs = BlockInfo & 128;
        NumPixels = BlockInfo & 127;

        if (WhatItIs)   // packed pixel
        {
            fread(&B,1,1,ftga);
            fread(&G,1,1,ftga);
            fread(&R,1,1,ftga);

            for (UINT k=0; k<NumPixels+1; k++)
            {
                tga_image->buffer[i] = RGB16Bit565(R,G,B);

                i++;   // inc pixel count
            }
        }
        else  // usual pixels
        {
            for (UINT k=0; k<NumPixels+1; k++)
            {
                fread(&B,1,1,ftga);
                fread(&G,1,1,ftga);
                fread(&R,1,1,ftga);

                tga_image->buffer[i] = RGB16Bit565(R,G,B);

                i++;   // inc pixel count
            }
        }
    }
	}	// 10 - packed data end
	else if (tgafh.DataType == 2)	// 2 - unpacked data
	{
		for (UINT i=0; i < tga_image->Height * tga_image->Width; i++) {
			fread(&B,1,1,ftga);
			fread(&G,1,1,ftga);
			fread(&R,1,1,ftga);

			tga_image->buffer[i] = RGB16Bit565(R,G,B);
		}
	}	// end unpacked data

	// check if we have normal picture or inverted vertically
	if (!(tgafh.Description & 32))	// flip neeeded
		Flip_Bitmap((UCHAR *)tga_image->buffer,2*tga_image->Width,tga_image->Height);

    fclose(ftga);   // close file

#ifdef TARGA_DEBUG
	files_num++;	// increase file counter
#endif

    return TLI_OK;
}  // end of TGA_Load_Image

// unload targa image
int TGA_Unload_Image(TGA_Image  *tga_image)
{
	SAFE_FREE(tga_image->buffer);
	tga_image->Height = 0;
	tga_image->Width = 0;

	return TLI_OK;
}

// rgb 5.6.5
inline USHORT RGB16Bit565(int r, int g, int b)
{
	// this function simply builds a 5.6.5 format 16 bit pixel
	// assumes input is RGB 0-255 each channel
	r>>=3; g>>=2; b>>=3;
	return (_RGB16BIT565((r),(g),(b)));

} // end RGB16Bit565

// function gets R G B components from color in 5.6.5 format
/*
void RGB16Bit565(USHORT c, int &r, int &g, int&b)
{
	r = c >> 11;
	g = (c >> 5) & 63;
	b = c & 31;
	r <<= 3;
	g <<= 2;
	b <<= 3;
	return;
}
*/

int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height)
{
	// this function is used to flip bottom-up .BMP images

	UCHAR *buffer; // used to perform the image processing
	int index;     // looping index

	// allocate the temporary buffer
	if (!(buffer = (UCHAR *)malloc(bytes_per_line*height)))
	   return(0);

	// copy image to work area
	memcpy(buffer,image,bytes_per_line*height);

	// flip vertically
	for (index=0; index < height; index++)
		memcpy(&image[((height-1) - index)*bytes_per_line],
			   &buffer[index*bytes_per_line], bytes_per_line);

	// release the memory
	free(buffer);

	// return success
	return 1;

} // end Flip_Bitmap

#endif	// ifndef TARGA_H