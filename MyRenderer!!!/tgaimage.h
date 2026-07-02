#pragma once

#include <fstream>
#include <windows.h>

#pragma pack(push, 1)



// 我的颜色结构！
union Color
{
	struct {
		unsigned char b, g, r, a;
	};
	COLORREF color;
	unsigned char raw[4];
};

struct TGA_Header {
	char idLength;
	char colorMapType;
	char dataTypeCode;
	short colorMapOrigin;
	short colorMapLength;
	char colorMapDepth;
	short x_origin;
	short y_origin;
	short width;
	short height;
	char bitsPerPixel;
	char imageDescriptor;
};
#pragma pack(pop)


class TGAImage {
protected:
	unsigned char* data;
	int width;
	int height;
	int bytespp;

	bool load_rle_data(std::ifstream& in);

public:
	enum Format {
		GRAYSCALE = 1, RGB = 3, RGBA = 4
	};

	TGAImage();
	TGAImage(int w, int h, int bpp);


	bool read_tga_file(const char* filename);
	bool write_tga_file(const char* filename, bool rle = true);

	Color get(int x, int y);

	~TGAImage();

	int get_width();
	int get_height();
};


