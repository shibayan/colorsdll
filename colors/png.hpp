/*
    png.hpp
    COLORS PNG Loader Library
*/

#pragma once

#include <png.h>

#include "saori.h"
#include "image.hpp"

bool png_load_image(const string_t &file, image &src)
{
	FILE *fp;
    if (tfopen_s(&fp, file.c_str(), _T("rb")) != 0)
	{
		return false;
	}

	png_uint_32 width, height;
    int depth, colortype;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL)
	{
		fclose(fp);
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return false;
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &depth, &colortype, NULL, NULL, NULL);

	// バッファ確保
    src.resize(width, height);

	// デコード
    color *buffer = src.buffer();
	png_bytepp pp = new png_bytep[height];
	for (png_uint_32 i = 0;i < height; ++i)
	{
        pp[i] = reinterpret_cast<png_bytep>(&buffer[width * i]);
	}

    // 読み込み設定
	if (colortype == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(png_ptr);
	}
	if (colortype == PNG_COLOR_TYPE_GRAY && depth < 8)
	{
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	}
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(png_ptr);
	}
    if (colortype != PNG_COLOR_TYPE_RGBA)
    {
	    png_set_add_alpha(png_ptr, 255, PNG_FILLER_AFTER);
    }
	if (depth == 16)
	{
		png_set_strip_16(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);

    // ファイルを読み込み
	png_read_image(png_ptr, pp);

    // 終了処理
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	fclose(fp);
	delete [] pp;

	return true;
}

bool png_save_image(const string_t &file, image &src)
{
	FILE *fp;
    if (tfopen_s(&fp, file.c_str(), _T("wb")) != 0)
	{
		return false;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL)
	{
		fclose(fp);
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}

    png_uint_32 width = src.width();
    png_uint_32 height = src.height();

    // バッファを準備
    color *buffer = src.buffer();
	png_bytepp pp = new png_bytep[height];
	for (png_uint_32 i = 0; i < height; ++i)
	{
        pp[i] = reinterpret_cast<png_bytep>(&buffer[width * i]);
	}

    // 書き込み準備
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_filter(png_ptr, 0, PNG_NO_FILTERS);
	png_set_compression_level(png_ptr, Z_BEST_SPEED);
	png_write_info(png_ptr, info_ptr);

    // ファイルに書き込み
	png_write_image(png_ptr, pp);

    // 終了処理
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
	delete [] pp;

	return true;
}