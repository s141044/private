
#pragma once

#ifndef NS_LOAD_SAVE_IMAGE_HPP
#define NS_LOAD_SAVE_IMAGE_HPP

#include<string>
#include<vector>
#include<fstream>

#include"load_save_jpg.hpp"
#include"load_save_png.hpp"
#include"load_save_bmp.hpp"
#include"load_save_tga.hpp"
#include"load_save_hdr.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace ns{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace anon{

///////////////////////////////////////////////////////////////////////////////////////////////////

//画像読み込み
inline void load_image(const std::string &filename, int *p_width, int *p_height, int *p_channels, std::vector<unsigned char> *p_pixels)
{
	//ファイルオープン
	std::ifstream ifs(filename, std::ios::binary);

	if(!ifs){
		throw std::ios::failure("load_image: " + filename + " can't open.");
	}
	ifs.exceptions(std::ios::badbit | std::ios::failbit);

	//フォーマット識別子に従い読み込み
	const unsigned char png_id[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
	const unsigned char jpg_id[] = { 0xff, 0xd8, 0xff };
	const unsigned char bmp_id[] = { 0x42, 0x4d };
	const unsigned char tga_id[] = { 0x54, 0x52, 0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4f, 0x4e, 0x2d, 0x58, 0x46, 0x49, 0x4c, 0x45, 0x2e };

	char id[18]; ifs.read(id, 10);

	//png
	if(strncmp(id, (char*)png_id, sizeof(png_id)) == 0){
		return load_png(filename, p_width, p_height, p_channels, p_pixels);
	}
	//jpg
	if(strncmp(id, (char*)jpg_id, sizeof(jpg_id)) == 0){
		return load_jpg(filename, p_width, p_height, p_channels, p_pixels);
	}
	//bmp
	if(strncmp(id, (char*)bmp_id, sizeof(bmp_id)) == 0){
		return load_bmp(filename, p_width, p_height, p_channels, p_pixels);
	}
	//tga
	//ifs.seekg(-18, std::ios::end);
	//ifs.read(id, 17);
	//if(strncmp(id, (char*)tga_id, sizeof(tga_id)) == 0){
	if(filename.substr(filename.find_last_of('.')) == ".tga"){
		return load_tga(filename, p_width, p_height, p_channels, p_pixels);
	}
	//その他
	throw std::invalid_argument("load_image: " + filename + " can't read.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//画像保存
inline void save_image(const std::string &filename, const int width, const int height, const int channels, const std::vector<unsigned char> &pixels)
{
	//拡張子に従い画像保存
	std::string ext = filename.substr(filename.find_last_of('.') + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if(ext == "png"){ return save_png(filename, width, height, channels, pixels); }
	if(ext == "jpg"){ return save_jpg(filename, width, height, channels, pixels); }
	if(ext == "bmp"){ return save_bmp(filename, width, height, channels, pixels); }
	if(ext == "tga"){ return save_tga(filename, width, height, channels, pixels); }
	if(ext == "hdr"){ return save_hdr(filename, width, height, channels, pixels); }

	throw std::invalid_argument("save_image: " + filename + " can't save.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace anon

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace ns

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
