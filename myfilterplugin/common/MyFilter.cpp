#include "Image.h"
#include "ImageView.h"
#include "PIDefines.h"
#include "PIFilter.h"
#include "PITypes.h"
#include "antialias.h"
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

static uint8 antialias_value_8(uint8 value)
{
	return static_cast<uint8>(255 - value);
}

static uint16 antialias_value_16(uint16 value)
{
	return static_cast<uint16>(65535U - value);
}

static float antialias_value_32(float value)
{
	return 1.0f - value;
}

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

euclase::Image convertToGrayscale8(ImageView const &source)
{
	euclase::Image ret(source.width(), source.height(), euclase::Image::Format_U8_Grayscale);
	auto const to8From16 = [](uint16 value) -> uint8_t {
		return static_cast<uint8_t>((value + 128U) / 257U);
	};
	auto const to8FromFloat = [](float value) -> uint8_t {
		return static_cast<uint8_t>(euclase::clamp_f32(value) * 255.0f + 0.5f);
	};

	for (int y = 0; y < source.height(); ++y) {
		uint8_t *outLine = ret.scanLine(y);
		switch (source.format()) {
		case ImageView::Format_Grayscale8: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				outLine[x] = inLine[x];
			}
			break;
		}
		case ImageView::Format_Grayscale16: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				outLine[x] = to8From16(inLine[x]);
			}
			break;
		}
		case ImageView::Format_Grayscale32: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				outLine[x] = to8FromFloat(inLine[x]);
			}
			break;
		}
		case ImageView::Format_GrayscaleA8: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				outLine[x] = inLine[x * 2];
			}
			break;
		}
		case ImageView::Format_GrayscaleA16: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				outLine[x] = to8From16(inLine[x * 2]);
			}
			break;
		}
		case ImageView::Format_GrayscaleA32: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				outLine[x] = to8FromFloat(inLine[x * 2]);
			}
			break;
		}
		case ImageView::Format_RGB24: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 3;
				outLine[x] = static_cast<uint8_t>(euclase::gray(inLine[base], inLine[base + 1], inLine[base + 2]));
			}
			break;
		}
		case ImageView::Format_RGB48: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 3;
				outLine[x] = static_cast<uint8_t>(euclase::gray(to8From16(inLine[base]), to8From16(inLine[base + 1]), to8From16(inLine[base + 2])));
			}
			break;
		}
		case ImageView::Format_RGB96: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 3;
				outLine[x] = to8FromFloat(euclase::grayf(inLine[base], inLine[base + 1], inLine[base + 2]));
			}
			break;
		}
		case ImageView::Format_RGBA32: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 4;
				outLine[x] = static_cast<uint8_t>(euclase::gray(inLine[base], inLine[base + 1], inLine[base + 2]));
			}
			break;
		}
		case ImageView::Format_RGBA64: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 4;
				outLine[x] = static_cast<uint8_t>(euclase::gray(to8From16(inLine[base]), to8From16(inLine[base + 1]), to8From16(inLine[base + 2])));
			}
			break;
		}
		case ImageView::Format_RGBA128: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 4;
				outLine[x] = to8FromFloat(euclase::grayf(inLine[base], inLine[base + 1], inLine[base + 2]));
			}
			break;
		}}
	}

	return ret;
}

euclase::Image convertToGrayscale8A(ImageView const &source)
{
	euclase::Image ret(source.width(), source.height(), euclase::Image::Format_U8_GrayscaleA);
	auto const to8From16 = [](uint16 value) -> uint8_t {
		return static_cast<uint8_t>((value + 128U) / 257U);
	};
	auto const to8FromFloat = [](float value) -> uint8_t {
		return static_cast<uint8_t>(euclase::clamp_f32(value) * 255.0f + 0.5f);
	};

	for (int y = 0; y < source.height(); ++y) {
		uint8_t *outLine = ret.scanLine(y);
		switch (source.format()) {
		case ImageView::Format_Grayscale8: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 2;
				uint8_t const v = inLine[x];
				outLine[base] = v;
				outLine[base + 1] = 255;
			}
			break;
		}
		case ImageView::Format_Grayscale16: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 2;
				outLine[base] = to8From16(inLine[x]);
				outLine[base + 1] = 255;
			}
			break;
		}
		case ImageView::Format_Grayscale32: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 2;
				outLine[base] = to8FromFloat(inLine[x]);
				outLine[base + 1] = 255;
			}
			break;
		}
		case ImageView::Format_GrayscaleA8: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 2;
				int const outBase = x * 2;
				outLine[outBase] = inLine[inBase];
				outLine[outBase + 1] = inLine[inBase + 1];
			}
			break;
		}
		case ImageView::Format_GrayscaleA16: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 2;
				int const outBase = x * 2;
				outLine[outBase] = to8From16(inLine[inBase]);
				outLine[outBase + 1] = to8From16(inLine[inBase + 1]);
			}
			break;
		}
		case ImageView::Format_GrayscaleA32: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 2;
				int const outBase = x * 2;
				outLine[outBase] = to8FromFloat(inLine[inBase]);
				outLine[outBase + 1] = to8FromFloat(inLine[inBase + 1]);
			}
			break;
		}
		case ImageView::Format_RGB24: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 3;
				int const outBase = x * 2;
				outLine[outBase] = static_cast<uint8_t>(euclase::gray(inLine[inBase], inLine[inBase + 1], inLine[inBase + 2]));
				outLine[outBase + 1] = 255;
			}
			break;
		}
		case ImageView::Format_RGB48: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 3;
				int const outBase = x * 2;
				outLine[outBase] = static_cast<uint8_t>(euclase::gray(to8From16(inLine[inBase]), to8From16(inLine[inBase + 1]), to8From16(inLine[inBase + 2])));
				outLine[outBase + 1] = 255;
			}
			break;
		}
		case ImageView::Format_RGB96: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 3;
				int const outBase = x * 2;
				outLine[outBase] = to8FromFloat(euclase::grayf(inLine[inBase], inLine[inBase + 1], inLine[inBase + 2]));
				outLine[outBase + 1] = 255;
			}
			break;
		}
		case ImageView::Format_RGBA32: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 4;
				int const outBase = x * 2;
				outLine[outBase] = static_cast<uint8_t>(euclase::gray(inLine[inBase], inLine[inBase + 1], inLine[inBase + 2]));
				outLine[outBase + 1] = inLine[inBase + 3];
			}
			break;
		}
		case ImageView::Format_RGBA64: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 4;
				int const outBase = x * 2;
				outLine[outBase] = static_cast<uint8_t>(euclase::gray(to8From16(inLine[inBase]), to8From16(inLine[inBase + 1]), to8From16(inLine[inBase + 2])));
				outLine[outBase + 1] = to8From16(inLine[inBase + 3]);
			}
			break;
		}
		case ImageView::Format_RGBA128: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 4;
				int const outBase = x * 2;
				outLine[outBase] = to8FromFloat(euclase::grayf(inLine[inBase], inLine[inBase + 1], inLine[inBase + 2]));
				outLine[outBase + 1] = to8FromFloat(inLine[inBase + 3]);
			}
			break;
		}
		}
	}

	return ret;
}

euclase::Image convertToRGBA32(ImageView const &source)
{
	euclase::Image ret(source.width(), source.height(), euclase::Image::Format_U8_RGBA);
	auto const to8From16 = [](uint16 value) -> uint8_t {
		return static_cast<uint8_t>((value + 128U) / 257U);
	};
	auto const to8FromFloat = [](float value) -> uint8_t {
		return static_cast<uint8_t>(euclase::clamp_f32(value) * 255.0f + 0.5f);
	};

	for (int y = 0; y < source.height(); ++y) {
		uint8_t *outLine = ret.scanLine(y);
		switch (source.format()) {
		case ImageView::Format_Grayscale8: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				uint8_t const v = inLine[x];
				int const base = x * 4;
				outLine[base] = v;
				outLine[base + 1] = v;
				outLine[base + 2] = v;
				outLine[base + 3] = 255;
			}
			break;
		}
		case ImageView::Format_Grayscale16: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				uint8_t const v = to8From16(inLine[x]);
				int const base = x * 4;
				outLine[base] = v;
				outLine[base + 1] = v;
				outLine[base + 2] = v;
				outLine[base + 3] = 255;
			}
			break;
		}
		case ImageView::Format_Grayscale32: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				uint8_t const v = to8FromFloat(inLine[x]);
				int const base = x * 4;
				outLine[base] = v;
				outLine[base + 1] = v;
				outLine[base + 2] = v;
				outLine[base + 3] = 255;
			}
			break;
		}
		case ImageView::Format_GrayscaleA8: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 2;
				int const outBase = x * 4;
				uint8_t const v = inLine[base];
				outLine[outBase] = v;
				outLine[outBase + 1] = v;
				outLine[outBase + 2] = v;
				outLine[outBase + 3] = inLine[base + 1];
			}
			break;
		}
		case ImageView::Format_GrayscaleA16: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 2;
				int const outBase = x * 4;
				uint8_t const v = to8From16(inLine[base]);
				outLine[outBase] = v;
				outLine[outBase + 1] = v;
				outLine[outBase + 2] = v;
				outLine[outBase + 3] = to8From16(inLine[base + 1]);
			}
			break;
		}
		case ImageView::Format_GrayscaleA32: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 2;
				int const outBase = x * 4;
				uint8_t const v = to8FromFloat(inLine[base]);
				outLine[outBase] = v;
				outLine[outBase + 1] = v;
				outLine[outBase + 2] = v;
				outLine[outBase + 3] = to8FromFloat(inLine[base + 1]);
			}
			break;
		}
		case ImageView::Format_RGB24: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 3;
				int const outBase = x * 4;
				outLine[outBase] = inLine[inBase];
				outLine[outBase + 1] = inLine[inBase + 1];
				outLine[outBase + 2] = inLine[inBase + 2];
				outLine[outBase + 3] = 255;
			}
			break;
		}
		case ImageView::Format_RGB48: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 3;
				int const outBase = x * 4;
				outLine[outBase] = to8From16(inLine[inBase]);
				outLine[outBase + 1] = to8From16(inLine[inBase + 1]);
				outLine[outBase + 2] = to8From16(inLine[inBase + 2]);
				outLine[outBase + 3] = 255;
			}
			break;
		}
		case ImageView::Format_RGB96: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const inBase = x * 3;
				int const outBase = x * 4;
				outLine[outBase] = to8FromFloat(inLine[inBase]);
				outLine[outBase + 1] = to8FromFloat(inLine[inBase + 1]);
				outLine[outBase + 2] = to8FromFloat(inLine[inBase + 2]);
				outLine[outBase + 3] = 255;
			}
			break;
		}
		case ImageView::Format_RGBA32: {
			uint8_t const *inLine = source.scanLine(y);
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 4;
				outLine[base] = inLine[base];
				outLine[base + 1] = inLine[base + 1];
				outLine[base + 2] = inLine[base + 2];
				outLine[base + 3] = inLine[base + 3];
			}
			break;
		}
		case ImageView::Format_RGBA64: {
			uint16_t const *inLine = reinterpret_cast<uint16_t const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 4;
				outLine[base] = to8From16(inLine[base]);
				outLine[base + 1] = to8From16(inLine[base + 1]);
				outLine[base + 2] = to8From16(inLine[base + 2]);
				outLine[base + 3] = to8From16(inLine[base + 3]);
			}
			break;
		}
		case ImageView::Format_RGBA128: {
			float const *inLine = reinterpret_cast<float const *>(source.scanLine(y));
			for (int x = 0; x < source.width(); ++x) {
				int const base = x * 4;
				outLine[base] = to8FromFloat(inLine[base]);
				outLine[base + 1] = to8FromFloat(inLine[base + 1]);
				outLine[base + 2] = to8FromFloat(inLine[base + 2]);
				outLine[base + 3] = to8FromFloat(inLine[base + 3]);
			}
			break;
		}}
	}

	return ret;
}

static void antialias_grayscale_plane(
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

	euclase::Image intermediate = convertToGrayscale8(inputImage);
	filter_antialias(&intermediate);

	switch (inputImage.format()) {
	case ImageView::Format_Grayscale8:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = inLine[x];
			}
		}
		break;
	case ImageView::Format_Grayscale16:
		intermediate = intermediate.convertToFormat(euclase::Image::Format_U8_Grayscale);
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = static_cast<uint16>(inLine[x]) * 257U;
			}
		}
		break;
	case ImageView::Format_Grayscale32:
		intermediate = intermediate.convertToFormat(euclase::Image::Format_F16_Grayscale);
		for (int32 y = 0; y < height; ++y) {
			float const *inLine = reinterpret_cast<float const *>(intermediate.scanLine(y));
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

static void antialias_grayscale_alpha_packed(
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

	euclase::Image intermediate = convertToGrayscale8A(inputImage);
	filter_antialias(&intermediate);

	switch (inputImage.format()) {
	case ImageView::Format_GrayscaleA8:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 2;
				outLine[base] = inLine[base];
				outLine[base + 1] = inLine[base + 1];
			}
		}
		break;

	case ImageView::Format_GrayscaleA16:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 2;
				outLine[base] = static_cast<uint16>(inLine[base]) * 257U;
				outLine[base + 1] = static_cast<uint16>(inLine[base + 1]) * 257U;
			}
		}
		break;

	case ImageView::Format_GrayscaleA32:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 2;
				outLine[base] = inLine[base] / 255.0f;
				outLine[base + 1] = inLine[base + 1] / 255.0f;
			}
		}
		break;

	default:
		break;
	}
}

static void antialias_rgb_packed(
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

	euclase::Image intermediate = convertToRGBA32(inputImage);
	filter_antialias(&intermediate);

	switch (inputImage.format()) {
	case ImageView::Format_RGB24:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[x * 3] = inLine[base];
				outLine[x * 3 + 1] = inLine[base + 1];
				outLine[x * 3 + 2] = inLine[base + 2];
			}
		}
		break;

	case ImageView::Format_RGB48:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[x * 3] = static_cast<uint16>(inLine[base]) * 257U;
				outLine[x * 3 + 1] = static_cast<uint16>(inLine[base + 1]) * 257U;
				outLine[x * 3 + 2] = static_cast<uint16>(inLine[base + 2]) * 257U;
			}
		}
		break;

	case ImageView::Format_RGB96:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[x * 3] = inLine[base] / 255.0f;
				outLine[x * 3 + 1] = inLine[base + 1] / 255.0f;
				outLine[x * 3 + 2] = inLine[base + 2] / 255.0f;
			}
		}
		break;

	default:
		break;
	}
}

static void antialias_rgba_packed(
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

	euclase::Image intermediate = convertToRGBA32(inputImage);
	filter_antialias(&intermediate);

	switch (inputImage.format()) {
	case ImageView::Format_RGBA32:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[base] = inLine[base];
				outLine[base + 1] = inLine[base + 1];
				outLine[base + 2] = inLine[base + 2];
				outLine[base + 3] = inLine[base + 3];
			}
		}
		break;

	case ImageView::Format_RGBA64:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[base] = static_cast<uint16>(inLine[base]) * 257U;
				outLine[base + 1] = static_cast<uint16>(inLine[base + 1]) * 257U;
				outLine[base + 2] = static_cast<uint16>(inLine[base + 2]) * 257U;
				outLine[base + 3] = static_cast<uint16>(inLine[base + 3]) * 257U;
			}
		}
		break;

	case ImageView::Format_RGBA128:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				int32 const base = x * 4;
				outLine[base] = inLine[base] / 255.0f;
				outLine[base + 1] = inLine[base + 1] / 255.0f;
				outLine[base + 2] = inLine[base + 2] / 255.0f;
				outLine[base + 3] = inLine[base + 3] / 255.0f;
			}
		}
		break;

	default:
		break;
	}
}

static void antialias_rgb_plane_fallback(
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

	euclase::Image intermediate = convertToGrayscale8(inputImage);
	filter_antialias(&intermediate);

	switch (inputImage.format()) {
	case ImageView::Format_Grayscale8:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = inLine[x];
			}
		}
		break;

	case ImageView::Format_Grayscale16:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = static_cast<uint16>(inLine[x]) * 257U;
			}
		}
		break;

	case ImageView::Format_Grayscale32:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = inLine[x] / 255.0f;
			}
		}
		break;

	default:
		break;
	}
}

static void antialias_copy_alpha_plane_fallback(
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

	euclase::Image intermediate = convertToGrayscale8(inputImage);
	filter_antialias(&intermediate);

	switch (inputImage.format()) {
	case ImageView::Format_Grayscale8:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint8_t *outLine = outputImage.scanLine(y);
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = inLine[x];
			}
		}
		break;

	case ImageView::Format_Grayscale16:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			uint16 *outLine = reinterpret_cast<uint16 *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = static_cast<uint16>(inLine[x]) * 257U;
			}
		}
		break;

	case ImageView::Format_Grayscale32:
		for (int32 y = 0; y < height; ++y) {
			uint8_t const *inLine = intermediate.scanLine(y);
			float *outLine = reinterpret_cast<float *>(outputImage.scanLine(y));
			for (int32 x = 0; x < width; ++x) {
				outLine[x] = inLine[x] / 255.0f;
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

		antialias_grayscale_alpha_packed(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);

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
				antialias_grayscale_plane(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		} else {
             antialias_copy_alpha_plane_fallback(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
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
            antialias_rgba_packed(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		} else {
         antialias_rgb_packed(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
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
         antialias_rgb_plane_fallback(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
		} else {
         antialias_copy_alpha_plane_fallback(filterRecord->inData, filterRecord->outData, filterRecord->outRowBytes, filterRect, filterRecord->depth);
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

	if (filterRecord->depth == 32) {
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