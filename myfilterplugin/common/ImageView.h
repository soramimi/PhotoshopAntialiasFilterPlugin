#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <cstdint>

class ImageView { // Inspired by QImage
public:
	enum Format {
		Format_Grayscale8,
		Format_Grayscale16,
		Format_Grayscale32,
		Format_GrayscaleA8,
		Format_GrayscaleA16,
		Format_GrayscaleA32,
		Format_RGB24,
		Format_RGB48,
		Format_RGB96,
		Format_RGBA32,
		Format_RGBA64,
		Format_RGBA128,
	};

private:
	uint8_t *data_;
	Format format_;
	int width_;
	int height_;
	int bytesPerLine_;

public:
	ImageView(uint8_t *data, int width, int height, Format format, int bytesPerLine = 0)
		: data_(data)
		, width_(width)
		, height_(height)
		, format_(format)
		, bytesPerLine_(bytesPerLine != 0 ? bytesPerLine : (width * bytesPerPixel()))
	{
	}
	int width() const
	{
		return width_;
	}
	int height() const
	{
		return height_;
	}
	Format format() const
	{
		return format_;
	}
	int bytesPerPixel() const
	{
		switch (format_) {
		case Format_Grayscale8:
			return 1;
		case Format_Grayscale16:
			return 2;
		case Format_Grayscale32:
			return 4;
		case Format_GrayscaleA8:
			return 2;
		case Format_GrayscaleA16:
			return 4;
		case Format_GrayscaleA32:
			return 8;
		case Format_RGB24:
			return 3;
		case Format_RGB48:
			return 6;
		case Format_RGB96:
			return 12;
		case Format_RGBA32:
			return 4;
		case Format_RGBA64:
			return 8;
		case Format_RGBA128:
			return 16;
		}
		return 0;
	}
	int bytesPerLine() const
	{
		return bytesPerLine_;
	}
	uint8_t *bits()
	{
		return data_;
	}
	uint8_t const *bits() const
	{
		return data_;
	}
	uint8_t *scanLine(int y)
	{
		return data_ + (y * bytesPerLine_);
	}
	uint8_t const *scanLine(int y) const
	{
		return data_ + (y * bytesPerLine_);
	}
};

#endif // IMAGEVIEW_H
