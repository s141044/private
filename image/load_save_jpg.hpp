
#pragma once

#ifndef NS_LOAD_SAVE_JPG_HPP
#define NS_LOAD_SAVE_JPG_HPP

#include<string>
#include<cstdio>
#include<vector>
#include<fstream>
#include<cassert>
#include<csetjmp>

#include<jerror.h>
#include<jpeglib.h>
#include<jconfig.h>
#include<jmorecfg.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace jpg{

///////////////////////////////////////////////////////////////////////////////////////////////////
//my_error_mgr
/*/////////////////////////////////////////////////////////////////////////////////////////////////
libjpegのエラー処理関数に渡される構造体
/////////////////////////////////////////////////////////////////////////////////////////////////*/

struct my_error_mgr : jpeg_error_mgr
{
	static void error_exit_func(j_common_ptr cinfo)
	{
		//エラーメッセージ出力
		(*cinfo->err->output_message)(cinfo);

		//setjmp位置に戻る
		longjmp(static_cast<my_error_mgr*>(cinfo->err)->setjmp_buffer, 1);
	}
	jmp_buf setjmp_buffer;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

//jpg画像の読み込み
inline void load_image(const std::string &filename, int *p_width, int *p_height, int *p_channels, std::vector<unsigned char> *p_data)
{
	assert(p_width    != nullptr);
	assert(p_height   != nullptr);
	assert(p_channels != nullptr);
	assert(p_data     != nullptr);

	//ファイルオープン
	FILE *fp = nullptr;
	if((fp = fopen(filename.c_str(), "rb")) == nullptr){
		throw std::ios::failure("jpg::load_image: " + filename + " can't oepn.");
	}

	//
	std::vector<unsigned char> data;
	
	//
	jpeg_decompress_struct cinfo = {};

	//libjpegのエラー処理の設定
	my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jerr.error_exit = my_error_mgr::error_exit_func;
	
	//libjpegのエラー発生時のlongjmpの戻り位置
	if(setjmp(jerr.setjmp_buffer) == 0){

		//jpeg読み込み構造体の初期化
		jpeg_create_decompress(&cinfo);

		//libjpegファイル入出力の初期化
		jpeg_stdio_src(&cinfo, fp);

		//jpegファイルヘッダの読み込み
		jpeg_read_header(&cinfo, true);

		//jpegファイルの展開
		jpeg_start_decompress(&cinfo);

		//画像情報取得
		const int width = cinfo.output_width;
		const int height = cinfo.output_height;
		const int channels = cinfo.output_components;
		const int scanline_size = width * channels;

		//画像のメモリ確保
		data.resize(scanline_size * height);

		//画像読み込み
		for(int y = height - 1; y >= 0; y--){
			JSAMPROW row = &data[scanline_size * y];
			jpeg_read_scanlines(&cinfo, &row, 1);
		}

		//読み込み終了
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		fclose(fp);
		
		*p_width = width;
		*p_height = height;
		*p_channels = channels;
		*p_data = std::move(data);
	}
	//libjpegの例外処理
	else{

		jpeg_destroy_decompress(&cinfo);
		fclose(fp);

		throw std::runtime_error("jpg::load_image: libjpeg exception occurs while loading " + filename + ".");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//jpgフォーマットで保存
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
		throw std::ios::failure("jpg::save_image: " + filename + " can't open.");
	}

	//
	std::vector<unsigned char> buffer;

	//
	jpeg_compress_struct cinfo = {};

	//libjpegのエラー処理の設定
	my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jerr.error_exit = my_error_mgr::error_exit_func;

	//libjpegのエラー発生時のlongjmpの戻り位置
	if(setjmp(jerr.setjmp_buffer) == 0){

		//libjpeg書き込み構造体の初期化
		jpeg_create_compress(&cinfo);

		//libjpegファイル入出力の初期化
		jpeg_stdio_dest(&cinfo, fp);

		//画像情報の設定
		cinfo.image_width = static_cast<JDIMENSION>(width);
		cinfo.image_height = static_cast<JDIMENSION>(height);
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;

		//デフォルトの圧縮設定
		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, 75, TRUE);

		//画像書き込み
		jpeg_start_compress(&cinfo, TRUE);

		//RGBの場合
		if(channels == 3){
			const int scanline_size = width * channels;
			for(int i = 0, y = height - 1; i < height; i++, y--){
				JSAMPROW row = const_cast<JSAMPROW>(&data[y * scanline_size]);
				jpeg_write_scanlines(&cinfo, &row, 1);
			}
		}
		//RGBAの場合
		else{
			buffer.resize(width * 3);
			for(int i = 0, y = height - 1; i < height; i++, y--){
				for(int x = 0; x < width; x++){
					buffer[3 * x + 0] = data[4 * (x + width * y) + 0];
					buffer[3 * x + 1] = data[4 * (x + width * y) + 1];
					buffer[3 * x + 2] = data[4 * (x + width * y) + 2];
				}
				JSAMPROW row = buffer.data();
				jpeg_write_scanlines(&cinfo, &row, 1);
			}
		}

		//書き込み終了
		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);
		fclose(fp);
	}
	//libjpeg関数の例外処理
	else{

		jpeg_destroy_compress(&cinfo);
		fclose(fp);

		throw std::runtime_error("save_jpg: libjpeg exception occur while saving " + filename + ".");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace jpg

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
