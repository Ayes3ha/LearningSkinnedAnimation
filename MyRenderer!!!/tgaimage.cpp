#include <iostream>

#include "tgaimage.h"


TGAImage::TGAImage() : data(NULL), width(0), height(0), bytespp(0) {
}

TGAImage::TGAImage(int w, int h, int bpp) : data(NULL), width(w), height(h), bytespp(bpp) {
	unsigned long nbytes = width * height * bytespp;
	data = new unsigned char[nbytes];
	memset(data, 0, nbytes);
}

TGAImage::~TGAImage() {
	if (data) delete[] data;
}

// 读取tga文件
bool TGAImage::read_tga_file(const char* filename) {
	// 释放data的内存（因为是读取文件，现有的data可能不够大）
	if (data) delete[] data;	
	data = NULL;

	std::ifstream in;
	in.open(filename, std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "can't open file" << filename << "\n";
		in.close();
		return false;
	}

	// 读取TGA File Header
	TGA_Header header;
	in.read((char*)&header, sizeof(header));
	if (!in.good()) {
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	// 修改TGAImage的属性
	width = header.width;
	height = header.height;
	bytespp = header.bitsPerPixel >> 3;		// (>>3) == (/8) 右移3位相当于除以8
	if (width <= 0 || height <= 0 || (bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA)) {
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}
	
	// 开辟新data！
	unsigned long nbytes = bytespp * width * height;
	data = new unsigned char[nbytes];

	if (3 == header.dataTypeCode || 2 == header.dataTypeCode) {			// 不压缩
		in.read((char*)data, nbytes);
		if (!in.good()) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else if (10 == header.dataTypeCode || 11 == header.dataTypeCode) {
		if (!load_rle_data(in)) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else {
		in.close();
		std::cerr << "unknown file format " << (int)header.dataTypeCode << "\n";
		return false;
	}

	std::cout << "读取tga文件成功！！！参数为:" << width << " x " << height << " / " << bytespp * 8 << std::endl;;

	in.close();
	return true;
}

bool TGAImage::load_rle_data(std::ifstream& in) {
	unsigned long pixelcount = width * height;
	unsigned long currentpixel = 0;		// 当前的像素，比如1024*1024
	unsigned long currentbyte = 0;		// 当前的比特，比如1024*1024*24

	Color colorbuffer;

	do {
		unsigned char chunkheader = 0;
		// 读取数据块的第一位
		chunkheader = in.get();
		if (!in.good()) {
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		// 数据块的最高位若是0，则代表其后面存放的是非重复数据，次数由低7位决定（最高127）
		if (chunkheader < 128) {
			chunkheader++;
			for (int i = 0; i < chunkheader; i++) {
				in.read((char*)colorbuffer.raw, bytespp); // 读取颜色块
				if (!in.good()) {
					std::cerr << "an error occured while reading the header\n";
					return false;
				}

				for (int t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.raw[t];

				currentpixel++;		// 当前的像素位置+1

				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
		// 数据块的最高位若是1， 则代表其后面存放的是重复数据，次数也由低7位决定（最高127）
		else {
			chunkheader -= 127;
			in.read((char*)colorbuffer.raw, bytespp);	// 读取颜色块
			if (!in.good()) {
				std::cerr << "an error occured while reading the header\n";
				return false;
			}

			for (int i = 0; i < chunkheader; i++) {
				for (int t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.raw[t];
				
				currentpixel++;		// 当前的像素位置+1

				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount);

	return true;
}

Color TGAImage::get(int x, int y) {
	// 边界判断
	//std::cout << "x: " << x << " y: " << y << std::endl;
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		std::cout << "图像索引输入不合法，无法获取正确的颜色！\n" << std::endl;
		return Color();
	}

	Color c;
	unsigned char* p = data + (y * width + x) * bytespp;

	for (int i = 0; i < bytespp; i++)
		c.raw[i] = p[i];

	return c;
}

int TGAImage::get_width()
{
	return width;
}
int TGAImage::get_height()
{
	return height;
}





