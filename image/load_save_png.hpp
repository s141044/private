
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
//�֐���`
///////////////////////////////////////////////////////////////////////////////////////////////////

//png�摜�̓ǂݍ���
inline void load_image(const std::string &filename, int *p_width, int *p_height, int *p_channels, std::vector<unsigned char> *p_data)
{
	assert(p_width    != nullptr);
	assert(p_height   != nullptr);
	assert(p_channels != nullptr);
	assert(p_data     != nullptr);

	//�t�@�C���I�[�v��
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

	//�w�b�_�ǂݍ���
	png_byte header[8];
	if((fread(header, 1, 8, fp) != 8) || png_sig_cmp(header, 0, 8)){
		goto HANDLE_EXCEPTION;
	}

	//png�ǂݍ��ݍ\���̂𐶐�
	if((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)) == nullptr){
		goto HANDLE_EXCEPTION;
	}

	//png���\���̂𐶐�
	if((info_ptr = png_create_info_struct(png_ptr)) == nullptr){
		goto HANDLE_EXCEPTION;
	}
	
	//libpng�̃G���[��������longjmp�̖߂�ʒu
	if(setjmp(png_jmpbuf(png_ptr)) == 0){

		//libpng�t�@�C�����o�͂̏�����
		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);

		//png�摜���ǂݍ���
		png_read_info(png_ptr, info_ptr);

		//�摜���擾
		png_uint_32 width, height;
		png_int_32 bit_depth, color_type;
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

		//�摜�f�[�^��8bitRGB,8bitRGBA�ɐݒ�
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

		//�摜�����X�V
		png_read_update_info(png_ptr, info_ptr);

		//�摜���Ď擾
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

		png_size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
		png_byte channels = png_get_channels(png_ptr, info_ptr);

		//�������m��
		data.resize(row_bytes * height * sizeof(png_byte));
		rows.resize(height * sizeof(png_bytep));
		
		//�摜�ǂݍ���
		for(png_uint_32 i = 0, j = height - 1; i < height; i++, j--){
			rows[j] = &data[i * row_bytes];
		}
		png_read_image(png_ptr, rows.data());

		//�ǂݍ��ݏI��
		png_read_end(png_ptr, nullptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		fclose(fp);

		*p_width = width;
		*p_height = height;
		*p_channels = channels;
		*p_data = std::move(data);
	}
	//libpng�֐��̗�O����
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

//png�t�H�[�}�b�g�ŕۑ�
inline void save_image(const std::string &filename, const int width, const int height, const int channels, const std::vector<unsigned char> &data)
{
	assert(width    >= 1);
	assert(height   >= 1);
	assert(channels >= 3);
	assert(channels <= 4);
	assert(data.size() == size_t(width * height * channels));

	//�t�@�C���I�[�v��
	FILE *fp = nullptr;
	if((fp = fopen(filename.c_str(), "wb")) == nullptr){
		throw std::ios::failure("png::save_image: " + filename + " can't open.");
	}

	//
	std::vector<png_const_bytep> rows;
	
	//
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;

	//png�������ݍ\���̂𐶐�
	if((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)) == nullptr){
		goto HANDLE_EXCEPTION;
	}

	//png���\���̂𐶐�
	if((info_ptr = png_create_info_struct(png_ptr)) == nullptr){
		goto HANDLE_EXCEPTION;
	}

	//libpng�̃G���[��������longjmp�̖߂�ʒu
	if(setjmp(png_jmpbuf(png_ptr)) == 0){

		//libpng�t�@�C�����o�͂̏�����
		png_init_io(png_ptr, fp);

		//�摜���ݒ�
		const int color_type = (channels == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA;
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

		//�摜���w�b�_�̏�������
		png_write_info(png_ptr, info_ptr);

		//�������m��
		const int row_bytes = width * channels;
		rows.resize(height * row_bytes);

		//�摜��������
		for(int i = 0, j = height - 1; i < height; i++, j--){
			rows[j] = &data[i * row_bytes];
		}
		png_write_image(png_ptr, const_cast<png_bytepp>(rows.data()));

		//�������ݏI��
		png_write_end(png_ptr, info_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
	}
	//libpng�֐��̗�O����
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
