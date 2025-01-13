
#pragma once

#ifndef NS_LOAD_SAVE_PNG_HPP
#define NS_LOAD_SAVE_PNG_HPP

#include<cstdio>
#include<vector>
#include<string>
#include<fstream>
#include<cassert>
#include<csetjmp>

#include<png.h>
#include<pngconf.h>
#include<pnglibconf.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace png{

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

//png画像の読み込み
inline void load_image(const std::string &filename, int *p_width, int *p_height, int *p_channels, std::vector<unsigned char> *p_data)
{
	assert(p_width    != nullptr);
	assert(p_height   != nullptr);
	assert(p_channels != nullptr);
	assert(p_data     != nullptr);

	//ファイルオープン
	FILE *fp = nullptr;
	if((fp = fopen(filename.c_str(), "rb")) == nullptr){
		throw std::ios::failure("png::save_image: " + filename + " can't oepn.");
	}

	//
	std::vector<png_byte> data;
	std::vector<png_bytep> rows;
	
	//
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;

	//ヘッダ読み込み
	png_byte header[8];
	if((fread(header, 1, 8, fp) != 8) || png_sig_cmp(header, 0, 8)){
		goto HANDLE_EXCEPTION;
	}

	//png読み込み構造体を生成
	if((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)) == nullptr){
		goto HANDLE_EXCEPTION;
	}

	//png情報構造体を生成
	if((info_ptr = png_create_info_struct(png_ptr)) == nullptr){
		goto HANDLE_EXCEPTION;
	}
	
	//libpngのエラー発生時のlongjmpの戻り位置
	if(setjmp(png_jmpbuf(png_ptr)) == 0){

		//libpngファイル入出力の初期化
		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);

		//png画像情報読み込み
		png_read_info(png_ptr, info_ptr);

		//画像情報取得
		png_uint_32 width, height;
		png_int_32 bit_depth, color_type;
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

		//画像データを8bitRGB,8bitRGBAに設定
		if(bit_depth == 16){
			png_set_scale_16(png_ptr);
		}
		if(color_type == PNG_COLOR_TYPE_PALETTE){
			png_set_expand(png_ptr);
		}
		if(bit_depth < 8){
			png_set_expand(png_ptr);
		}
		if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)){
			png_set_expand(png_ptr);
		}
		if(color_type == PNG_COLOR_TYPE_GRAY){
			png_set_gray_to_rgb(png_ptr);
		}
		if(color_type == PNG_COLOR_TYPE_GRAY_ALPHA){
			png_set_gray_to_rgb(png_ptr);
		}

		//画像情報を更新
		png_read_update_info(png_ptr, info_ptr);

		//画像情報再取得
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

		png_size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
		png_byte channels = png_get_channels(png_ptr, info_ptr);

		//メモリ確保
		data.resize(row_bytes * height * sizeof(png_byte));
		rows.resize(height * sizeof(png_bytep));
		
		//画像読み込み
		for(png_uint_32 i = 0, j = height - 1; i < height; i++, j--){
			rows[j] = &data[i * row_bytes];
		}
		png_read_image(png_ptr, rows.data());

		//読み込み終了
		png_read_end(png_ptr, nullptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		fclose(fp);

		*p_width = width;
		*p_height = height;
		*p_channels = channels;
		*p_data = std::move(data);
	}
	//libpng関数の例外処理
	else{

HANDLE_EXCEPTION:	
		
		png_destroy_read_struct(
			(png_ptr != nullptr) ? &png_ptr : nullptr,
			(info_ptr != nullptr) ? &info_ptr : nullptr,
			nullptr
		);
		fclose(fp);
		
		throw std::runtime_error("png::load_image: libpng exception occurs while loading " + filename + ".");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//pngフォーマットで保存
inline void save_image(const std::string &filename, const int width, const int height, const int channels, const std::vector<unsigned char> &data)
{
	assert(width    >= 1);
	assert(height   >= 1);
	assert(channels >= 3);
	assert(channels <= 4);
	assert(data.size() == size_t(width * height * channels));

	//ファイルオープン
	FILE *fp = nullptr;
	if((fp = fopen(filename.c_str(), "wb")) == nullptr){
		throw std::ios::failure("png::save_image: " + filename + " can't open.");
	}

	//
	std::vector<png_const_bytep> rows;
	
	//
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;

	//png書き込み構造体を生成
	if((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)) == nullptr){
		goto HANDLE_EXCEPTION;
	}

	//png情報構造体を生成
	if((info_ptr = png_create_info_struct(png_ptr)) == nullptr){
		goto HANDLE_EXCEPTION;
	}

	//libpngのエラー発生時のlongjmpの戻り位置
	if(setjmp(png_jmpbuf(png_ptr)) == 0){

		//libpngファイル入出力の初期化
		png_init_io(png_ptr, fp);

		//画像情報設定
		const int color_type = (channels == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA;
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

		//画像情報ヘッダの書き込み
		png_write_info(png_ptr, info_ptr);

		//メモリ確保
		const int row_bytes = width * channels;
		rows.resize(height * row_bytes);

		//画像書き込み
		for(int i = 0, j = height - 1; i < height; i++, j--){
			rows[j] = &data[i * row_bytes];
		}
		png_write_image(png_ptr, const_cast<png_bytepp>(rows.data()));

		//書き込み終了
		png_write_end(png_ptr, info_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
	}
	//libpng関数の例外処理
	else{

HANDLE_EXCEPTION:
		
		png_destroy_write_struct(
			(png_ptr != nullptr) ? &png_ptr : nullptr,
			(info_ptr != nullptr) ? &info_ptr : nullptr
		);
		fclose(fp);
		
		throw std::runtime_error("png::save_image: libpng exception occurs while saving " + filename + ".");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace png

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
