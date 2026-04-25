#include "PIDefines.h"
#include "PIFilter.h"
#include "PITypes.h"
#include <cstdint>

namespace {
struct Rect32 {
	int32 top;
	int32 left;
	int32 bottom;
	int32 right;
};

static Rect32 get_filter_rect(FilterRecordPtr filterRecord)
{
	if (filterRecord->bigDocumentData != NULL) {
		return {
			filterRecord->bigDocumentData->filterRect32.top,
			filterRecord->bigDocumentData->filterRect32.left,
			filterRecord->bigDocumentData->filterRect32.bottom,
			filterRecord->bigDocumentData->filterRect32.right,
		};
	}

	return {
		filterRecord->filterRect.top,
		filterRecord->filterRect.left,
		filterRecord->filterRect.bottom,
		filterRecord->filterRect.right,
	};
}

static void set_in_rect(FilterRecordPtr filterRecord, Rect32 const &rect)
{
	filterRecord->inRect.top = static_cast<int16>(rect.top);
	filterRecord->inRect.left = static_cast<int16>(rect.left);
	filterRecord->inRect.bottom = static_cast<int16>(rect.bottom);
	filterRecord->inRect.right = static_cast<int16>(rect.right);

	if (filterRecord->bigDocumentData != NULL) {
		filterRecord->bigDocumentData->inRect32.top = rect.top;
		filterRecord->bigDocumentData->inRect32.left = rect.left;
		filterRecord->bigDocumentData->inRect32.bottom = rect.bottom;
		filterRecord->bigDocumentData->inRect32.right = rect.right;
	}
}

static void set_out_rect(FilterRecordPtr filterRecord, Rect32 const &rect)
{
	filterRecord->outRect.top = static_cast<int16>(rect.top);
	filterRecord->outRect.left = static_cast<int16>(rect.left);
	filterRecord->outRect.bottom = static_cast<int16>(rect.bottom);
	filterRecord->outRect.right = static_cast<int16>(rect.right);

	if (filterRecord->bigDocumentData != NULL) {
		filterRecord->bigDocumentData->outRect32.top = rect.top;
		filterRecord->bigDocumentData->outRect32.left = rect.left;
		filterRecord->bigDocumentData->outRect32.bottom = rect.bottom;
		filterRecord->bigDocumentData->outRect32.right = rect.right;
	}
}

static void set_mask_rect(FilterRecordPtr filterRecord, Rect32 const &rect)
{
	filterRecord->maskRect.top = static_cast<int16>(rect.top);
	filterRecord->maskRect.left = static_cast<int16>(rect.left);
	filterRecord->maskRect.bottom = static_cast<int16>(rect.bottom);
	filterRecord->maskRect.right = static_cast<int16>(rect.right);

	if (filterRecord->bigDocumentData != NULL) {
		filterRecord->bigDocumentData->maskRect32.top = rect.top;
		filterRecord->bigDocumentData->maskRect32.left = rect.left;
		filterRecord->bigDocumentData->maskRect32.bottom = rect.bottom;
		filterRecord->bigDocumentData->maskRect32.right = rect.right;
	}
}

static void clear_rects(FilterRecordPtr filterRecord)
{
	Rect32 const zeroRect = { 0, 0, 0, 0 };
	set_in_rect(filterRecord, zeroRect);
	set_out_rect(filterRecord, zeroRect);
	set_mask_rect(filterRecord, zeroRect);
}

static int16 supported_color_planes(int16 imageMode)
{
	switch (imageMode) {
	case plugInModeGrayScale:
	case plugInModeGray16:
	case plugInModeGray32:
		return 1;

	case plugInModeRGBColor:
	case plugInModeRGB48:
	case plugInModeRGB96:
		return 3;

	default:
		return 0;
	}
}

static int16 transparency_plane_count(FilterRecordPtr const filterRecord)
{
	if (filterRecord->outTransparencyMask != 0) {
		return filterRecord->outTransparencyMask;
	}

	return filterRecord->inTransparencyMask;
}

static uint8 invert_value_8(uint8 value)
{
	return static_cast<uint8>(255 - value);
}

static uint16 invert_value_16(uint16 value)
{
	return static_cast<uint16>(65535U - value);
}

static float invert_value_32(float value)
{
	return 1.0f - value;
}

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

static ImageView::Format grayscale_format_for_depth(int32 depth)
{
	switch (depth) {
	case 8:
		return ImageView::Format_Grayscale8;
	case 16:
		return ImageView::Format_Grayscale16;
	default:
		return ImageView::Format_Grayscale32;
	}
}

static ImageView::Format grayscale_alpha_format_for_depth(int32 depth)
{
	switch (depth) {
	case 8:
		return ImageView::Format_GrayscaleA8;
	case 16:
		return ImageView::Format_GrayscaleA16;
	default:
		return ImageView::Format_GrayscaleA32;
	}
}

static ImageView::Format rgb_format_for_depth(int32 depth)
{
	switch (depth) {
	case 8:
		return ImageView::Format_RGB24;
	case 16:
		return ImageView::Format_RGB48;
	default:
		return ImageView::Format_RGB96;
	}
}

static ImageView::Format rgba_format_for_depth(int32 depth)
{
	switch (depth) {
	case 8:
		return ImageView::Format_RGBA32;
	case 16:
		return ImageView::Format_RGBA64;
	default:
		return ImageView::Format_RGBA128;
	}
}

static void invert_grayscale_plane(
	void const *inData,
	void *outData,
	int32 outRowBytes,
	Rect32 const &rect,
	int32 depth)
{
	int32 const width = rect.right - rect.left;
	int32 const height = rect.bottom - rect.top;
	ImageView const inputImage(
		const_cast<uint8_t *>(static_cast<uint8_t const *>(inData)),
		width,
		height,
		grayscale_format_for_depth(depth),
		outRowBytes);
	ImageView outputImage(
		static_cast<uint8_t *>(outData),
		width,
		height,
		grayscale_format_for_depth(depth),
		outRowBytes);

	switch (inputImage.format()) {
	case ImageView::Format_Grayscale8:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = inputImage.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = invert_value_8(inLine[x]);
			}
		}
		break;
	case ImageView::Format_Grayscale16:
		for (int32 y = 0; y < height; ++y) {
			uint16 const *inLine = reinterpret_cast<uint16 const *>(inputImage.scanLine(y));
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = invert_value_16(inLine[x]);
			}
		}
		break;
	case ImageView::Format_Grayscale32:
		for (int32 y = 0; y < height; ++y) {
			float const *inLine = reinterpret_cast<float const *>(inputImage.scanLine(y));
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = invert_value_32(inLine[x]);
			}
		}
		break;
	default:
		break;
	}
}

static void invert_grayscale_alpha_packed(
	void const *inData,
	void *outData,
	int32 outRowBytes,
	Rect32 const &rect,
	int32 depth)
{
	int32 const width = rect.right - rect.left;
	int32 const height = rect.bottom - rect.top;
	ImageView const inputImage(
		const_cast<uint8_t *>(static_cast<uint8_t const *>(inData)),
		width,
		height,
		grayscale_alpha_format_for_depth(depth),
		outRowBytes);
	ImageView outputImage(
		static_cast<uint8_t *>(outData),
		width,
		height,
		grayscale_alpha_format_for_depth(depth),
		outRowBytes);

	switch (inputImage.format()) {
	case ImageView::Format_GrayscaleA8:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = inputImage.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 2;
				outLine[base] = invert_value_8(inLine[base]);
				outLine[base + 1] = inLine[base + 1];
			}
		}
		break;

	case ImageView::Format_GrayscaleA16:
		for (int32 y = 0; y < height; ++y) {
			uint16 const *inLine = reinterpret_cast<uint16 const *>(inputImage.scanLine(y));
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 2;
				outLine[base] = invert_value_16(inLine[base]);
				outLine[base + 1] = inLine[base + 1];
			}
		}
		break;

	case ImageView::Format_GrayscaleA32:
		for (int32 y = 0; y < height; ++y) {
			float const *inLine = reinterpret_cast<float const *>(inputImage.scanLine(y));
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 2;
				outLine[base] = invert_value_32(inLine[base]);
				outLine[base + 1] = inLine[base + 1];
			}
		}
		break;

	default:
		break;
	}
}

static void invert_rgb_packed(
	void const *inData,
	void *outData,
	int32 outRowBytes,
	Rect32 const &rect,
	int32 depth)
{
	int32 const width = rect.right - rect.left;
	int32 const height = rect.bottom - rect.top;
	ImageView const inputImage(
		const_cast<uint8_t *>(static_cast<uint8_t const *>(inData)),
		width,
		height,
		rgb_format_for_depth(depth),
		outRowBytes);
	ImageView outputImage(
		static_cast<uint8_t *>(outData),
		width,
		height,
		rgb_format_for_depth(depth),
		outRowBytes);

	switch (inputImage.format()) {
	case ImageView::Format_RGB24:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = inputImage.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 3;
				outLine[base] = invert_value_8(inLine[base]);
				outLine[base + 1] = invert_value_8(inLine[base + 1]);
				outLine[base + 2] = invert_value_8(inLine[base + 2]);
			}
		}
		break;

	case ImageView::Format_RGB48:
		for (int32 y = 0; y < height; ++y) {
			uint16 const *inLine = reinterpret_cast<uint16 const *>(inputImage.scanLine(y));
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 3;
				outLine[base] = invert_value_16(inLine[base]);
				outLine[base + 1] = invert_value_16(inLine[base + 1]);
				outLine[base + 2] = invert_value_16(inLine[base + 2]);
			}
		}
		break;

	case ImageView::Format_RGB96:
		for (int32 y = 0; y < height; ++y) {
			float const *inLine = reinterpret_cast<float const *>(inputImage.scanLine(y));
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 3;
				outLine[base] = invert_value_32(inLine[base]);
				outLine[base + 1] = invert_value_32(inLine[base + 1]);
				outLine[base + 2] = invert_value_32(inLine[base + 2]);
			}
		}
		break;

	default:
		break;
	}
}

static void invert_rgba_packed(
	void const *inData,
	void *outData,
	int32 outRowBytes,
	Rect32 const &rect,
	int32 depth)
{
	int32 const width = rect.right - rect.left;
	int32 const height = rect.bottom - rect.top;
	ImageView const inputImage(
		const_cast<uint8_t *>(static_cast<uint8_t const *>(inData)),
		width,
		height,
		rgba_format_for_depth(depth),
		outRowBytes);
	ImageView outputImage(
		static_cast<uint8_t *>(outData),
		width,
		height,
		rgba_format_for_depth(depth),
		outRowBytes);

	switch (inputImage.format()) {
	case ImageView::Format_RGBA32:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = inputImage.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[base] = invert_value_8(inLine[base]);
				outLine[base + 1] = invert_value_8(inLine[base + 1]);
				outLine[base + 2] = invert_value_8(inLine[base + 2]);
				outLine[base + 3] = inLine[base + 3];
			}
		}
		break;

	case ImageView::Format_RGBA64:
		for (int32 y = 0; y < height; ++y) {
			uint16 const *inLine = reinterpret_cast<uint16 const *>(inputImage.scanLine(y));
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[base] = invert_value_16(inLine[base]);
				outLine[base + 1] = invert_value_16(inLine[base + 1]);
				outLine[base + 2] = invert_value_16(inLine[base + 2]);
				outLine[base + 3] = inLine[base + 3];
			}
		}
		break;

	case ImageView::Format_RGBA128:
		for (int32 y = 0; y < height; ++y) {
			float const *inLine = reinterpret_cast<float const *>(inputImage.scanLine(y));
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[base] = invert_value_32(inLine[base]);
				outLine[base + 1] = invert_value_32(inLine[base + 1]);
				outLine[base + 2] = invert_value_32(inLine[base + 2]);
				outLine[base + 3] = inLine[base + 3];
			}
		}
		break;

	default:
		break;
	}
}

static void invert_rgb_plane_fallback(
	void const *inData,
	void *outData,
	int32 outRowBytes,
	Rect32 const &rect,
	int32 depth)
{
	int32 const width = rect.right - rect.left;
	int32 const height = rect.bottom - rect.top;
	ImageView const inputImage(
		const_cast<uint8_t *>(static_cast<uint8_t const *>(inData)),
		width,
		height,
		grayscale_format_for_depth(depth),
		outRowBytes);
	ImageView outputImage(
		static_cast<uint8_t *>(outData),
		width,
		height,
		grayscale_format_for_depth(depth),
		outRowBytes);

	switch (inputImage.format()) {
	case ImageView::Format_Grayscale8:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = inputImage.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = invert_value_8(inLine[x]);
			}
		}
		break;

	case ImageView::Format_Grayscale16:
		for (int32 y = 0; y < height; ++y) {
			uint16 const *inLine = reinterpret_cast<uint16 const *>(inputImage.scanLine(y));
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = invert_value_16(inLine[x]);
			}
		}
		break;

	case ImageView::Format_Grayscale32:
		for (int32 y = 0; y < height; ++y) {
			float const *inLine = reinterpret_cast<float const *>(inputImage.scanLine(y));
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = invert_value_32(inLine[x]);
			}
		}
		break;

	default:
		break;
	}
}

static void copy_alpha_plane_fallback(
	void const *inData,
	void *outData,
	int32 outRowBytes,
	Rect32 const &rect,
	int32 depth)
{
	int32 const width = rect.right - rect.left;
	int32 const height = rect.bottom - rect.top;
	ImageView const inputImage(
		const_cast<uint8_t *>(static_cast<uint8_t const *>(inData)),
		width,
		height,
		grayscale_format_for_depth(depth),
		outRowBytes);
	ImageView outputImage(
		static_cast<uint8_t *>(outData),
		width,
		height,
		grayscale_format_for_depth(depth),
		outRowBytes);

	switch (inputImage.format()) {
	case ImageView::Format_Grayscale8:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = inputImage.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = inLine[x];
			}
		}
		break;

	case ImageView::Format_Grayscale16:
		for (int32 y = 0; y < height; ++y) {
			uint16 const *inLine = reinterpret_cast<uint16 const *>(inputImage.scanLine(y));
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = inLine[x];
			}
		}
		break;

	case ImageView::Format_Grayscale32:
		for (int32 y = 0; y < height; ++y) {
			float const *inLine = reinterpret_cast<float const *>(inputImage.scanLine(y));
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = inLine[x];
			}
		}
		break;

	default:
		break;
	}
}

static int16 process_grayscale_filter(FilterRecordPtr filterRecord, Rect32 const &filterRect)
{
	int16 const alphaPlanes = transparency_plane_count(filterRecord);

	if (alphaPlanes != 0 && filterRecord->supportsAlternateLayouts) {
		filterRecord->wantLayout = piLayoutRowsColumnsPlanes;
		filterRecord->inLoPlane = 0;
		filterRecord->inHiPlane = 1;
		filterRecord->outLoPlane = 0;
		filterRecord->outHiPlane = 1;
		filterRecord->inputPadding = plugInWantsErrorOnBoundsException;
		filterRecord->outputPadding = plugInWantsErrorOnBoundsException;
		filterRecord->maskPadding = plugInWantsErrorOnBoundsException;

		int16 const advanceResult = filterRecord->advanceState();
		if (advanceResult != noErr) {
			return advanceResult;
		}

		invert_grayscale_alpha_packed(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);

		if (filterRecord->progressProc != NULL) {
			filterRecord->progressProc(1, 1);
		}

		if (filterRecord->abortProc != NULL && filterRecord->abortProc()) {
			return userCanceledErr;
		}

		return noErr;
	}

	filterRecord->wantLayout = piLayoutTraditional;

	for (int16 plane = 0; plane < 1 + alphaPlanes; ++plane) {
		filterRecord->inLoPlane = plane;
		filterRecord->inHiPlane = plane;
		filterRecord->outLoPlane = plane;
		filterRecord->outHiPlane = plane;
		filterRecord->inputPadding = plugInWantsErrorOnBoundsException;
		filterRecord->outputPadding = plugInWantsErrorOnBoundsException;
		filterRecord->maskPadding = plugInWantsErrorOnBoundsException;

		int16 const advanceResult = filterRecord->advanceState();
		if (advanceResult != noErr) {
			return advanceResult;
		}

		if (plane == 0) {
			invert_grayscale_plane(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		} else {
			copy_alpha_plane_fallback(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		}

		if (filterRecord->progressProc != NULL) {
			filterRecord->progressProc(static_cast<int32>(plane + 1), static_cast<int32>(1 + alphaPlanes));
		}

		if (filterRecord->abortProc != NULL && filterRecord->abortProc()) {
			return userCanceledErr;
		}
	}

	return noErr;
}

static int16 process_rgb_filter(FilterRecordPtr filterRecord, Rect32 const &filterRect)
{
	int16 const alphaPlanes = transparency_plane_count(filterRecord);

	if (filterRecord->supportsAlternateLayouts) {
		filterRecord->wantLayout = piLayoutRowsColumnsPlanes;
		filterRecord->inLoPlane = 0;
		filterRecord->inHiPlane = static_cast<int16>(2 + alphaPlanes);
		filterRecord->outLoPlane = 0;
		filterRecord->outHiPlane = static_cast<int16>(2 + alphaPlanes);
		filterRecord->inputPadding = plugInWantsErrorOnBoundsException;
		filterRecord->outputPadding = plugInWantsErrorOnBoundsException;
		filterRecord->maskPadding = plugInWantsErrorOnBoundsException;

		int16 const advanceResult = filterRecord->advanceState();
		if (advanceResult != noErr) {
			return advanceResult;
		}

		if (alphaPlanes != 0) {
			invert_rgba_packed(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		} else {
			invert_rgb_packed(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		}

		if (filterRecord->progressProc != NULL) {
			filterRecord->progressProc(1, 1);
		}

		if (filterRecord->abortProc != NULL && filterRecord->abortProc()) {
			return userCanceledErr;
		}

		return noErr;
	}

	filterRecord->wantLayout = piLayoutTraditional;

	for (int16 plane = 0; plane < 3 + alphaPlanes; ++plane) {
		filterRecord->inLoPlane = plane;
		filterRecord->inHiPlane = plane;
		filterRecord->outLoPlane = plane;
		filterRecord->outHiPlane = plane;
		filterRecord->inputPadding = plugInWantsErrorOnBoundsException;
		filterRecord->outputPadding = plugInWantsErrorOnBoundsException;
		filterRecord->maskPadding = plugInWantsErrorOnBoundsException;

		int16 const advanceResult = filterRecord->advanceState();
		if (advanceResult != noErr) {
			return advanceResult;
		}

		if (plane < 3) {
			invert_rgb_plane_fallback(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		} else {
			copy_alpha_plane_fallback(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		}

		if (filterRecord->progressProc != NULL) {
			filterRecord->progressProc(static_cast<int32>(plane + 1), static_cast<int32>(3 + alphaPlanes));
		}

		if (filterRecord->abortProc != NULL && filterRecord->abortProc()) {
			return userCanceledErr;
		}
	}

	return noErr;
}

static int16 process_filter(FilterRecordPtr filterRecord)
{
	int16 const colorPlanes = supported_color_planes(filterRecord->imageMode);

	if (colorPlanes == 0) {
		return filterBadMode;
	}

	Rect32 const filterRect = get_filter_rect(filterRecord);
	set_in_rect(filterRecord, filterRect);
	set_out_rect(filterRecord, filterRect);
	set_mask_rect(filterRecord, { 0, 0, 0, 0 });
	filterRecord->autoMask = true;

	if (colorPlanes == 1) {
		int16 const result = process_grayscale_filter(filterRecord, filterRect);
		clear_rects(filterRecord);
		return result;
	}

	int16 const result = process_rgb_filter(filterRecord, filterRect);
	clear_rects(filterRecord);
	return result;
}
}

DLLExport MACPASCAL void PluginMain(
	int16 const selector,
	FilterRecordPtr filterRecord,
	intptr_t *data,
	int16 *result)
{
	(void)data;

	if (result == NULL) {
		return;
	}

	*result = noErr;

	if (selector == filterSelectorAbout) {
		return;
	}

	if (filterRecord == NULL) {
		*result = filterBadParameters;
		return;
	}

	if (filterRecord->bigDocumentData != NULL) {
		filterRecord->bigDocumentData->PluginUsing32BitCoordinates = true;
	}

	switch (selector) {
	case filterSelectorParameters:
		break;

	case filterSelectorPrepare:
		filterRecord->bufferSpace = 0;
		break;

	case filterSelectorStart:
		*result = process_filter(filterRecord);
		break;

	case filterSelectorContinue:
	case filterSelectorFinish:
		clear_rects(filterRecord);
		break;

	default:
		break;
	}
}