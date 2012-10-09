//---------------------------------------------------------------------------
#ifndef targaH
#define targaH
//---------------------------------------------------------------------------

// this builds a 16 bit color value in 5.6.5 format (green dominate mode)
#define _RGB16BIT565(r,g,b) ((b & 31) + ((g & 63) << 5) + ((r & 31) << 11))
// get r, g , b from USHORT
#define GetRGB16Bit565(c, r, g, b) { r = c >> 11; \
	g = (c >> 5) & 63; \
	b = c & 31; \
	r <<= 3; \
	g <<= 2; \
	b <<= 3;	}

// TGA_LOAD_IMAGE RETURN CONSTANTS
#define TLI_OK              0
#define TLI_WRONGPARAMS     1
#define TLI_ERRORFOPEN      2
#define TLI_WRONGFORMAT     3

// basic  types
typedef unsigned short int USHORT;
typedef short    int   SHORT;
typedef unsigned int   UINT;
typedef unsigned char  UCHAR;

// targa file header struct, 18 bytes
typedef struct targa_file_header {
    UCHAR   IdLength;   // I will skip IdLength bytes after this header
    char    ColorMap;   // won't be used
    UCHAR   DataType;   // packed data? 2 - no, 10 - yes
    char    ColorMapInfo[5];    // won't be used
    SHORT   X_Origin;   // начало изобр. по оси Х
    SHORT   Y_Origin;   // -=- Y
    USHORT   Image_Width;
    USHORT   Image_Height;
    UCHAR   Bit_Per_Pixel;  // only 24 in this
    char    Description;    // won't be used
}   TGA_File_Header;

// targa image description, for convenience all in one. 8 bytes
typedef struct targa_image {
    USHORT       Width;     // image width
    USHORT       Height;    // image height
    USHORT      *buffer;   // contains pixels in 5.6.5 format
}   TGA_Image;

/*
загрузить изображение в переданный массив,
грузит 24 битную таргу в массив ргб 5.6.5
Прежде чем передавать TGA_Image лучше ее обнулить
*/
int TGA_Load_Image(TGA_Image *tga_image,const char *filename);
int TGA_Unload_Image(TGA_Image  *tga_image);

// function builds 16 bit rgb 5.6.5 value
inline USHORT RGB16Bit565(int r, int g, int b);

//-------------------------------------------------
#endif
