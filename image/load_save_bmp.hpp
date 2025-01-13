
#pragma once

#ifndef NS_LOAD_SAVE_BMP_HPP
#define NS_LOAD_SAVE_BMP_HPP

#include<string>
#include<vector>
#include<fstream>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace bmp{

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

//エンディアン変換
inline int swap_endian16(const unsigned char *values)
{
	return (values[0]) | (values[1] << 8);
}
inline int swap_endian32(const unsigned char *values)
{
	return (values[0]) | (values[1] << 8) | (values[2] << 16) | (values[3] << 24);
}
inline int swap_endian16(const char *values)
{
	return swap_endian16((unsigned char*)values);
}
inline int swap_endian32(const char *values)
{
	return swap_endian32((unsigned char*)values);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//bmp画像の読み込み
inline void load_image(const std::string &filename, int *p_width, int *p_height, int *p_channels, std::vector<unsigned char> *p_data)
{
	assert(p_width    != nullptr);
	assert(p_height   != nullptr);
	assert(p_channels != nullptr);
	assert(p_data     != nullptr);

	//ファイルオープン
	std::ifstream ifs(
		filename, std::ios::binary
	);
	if(not(ifs.is_open())){
		throw std::ios::failure("bmp::load_image: " + filename + " can't open.");
	}
	ifs.exceptions(std::ios::badbit | std::ios::failbit);

	//ファイルヘッダ読み込み
	char file_header[14];
	ifs.read((char*)file_header, 14);

	if(file_header[0] != 'B' || file_header[1] != 'M'){
		throw std::invalid_argument("bmp::load_image: " + filename + " is not a bmp file.");
	}

	//情報ヘッダ読み込み
	char info_header[40];
	ifs.read(info_header, 4);

	const int info_size = swap_endian32(info_header);
	ifs.read(&info_header[4], info_size - 4);

	if(info_size < 40){
		throw std::invalid_argument("bmp::load_image: " + filename + " is not a windows bitmap.");
	}

	//画像情報取得
	const int width = swap_endian32(&info_header[4]);
	const int tmp_h = swap_endian32(&info_header[8]);
	const int height = (tmp_h >= 0) ? tmp_h : -tmp_h;
	const int bit_depth = swap_endian16(&info_header[14]);
	const int channels = 3;

	if(swap_endian32(&info_header[16]) != 0){
		throw std::invalid_argument("bmp::load_image: " + filename + " (run length encoded image) can't read.");
	}

	switch(bit_depth){
	case 1: throw std::invalid_argument("bmp::load_image: " + filename + " (1 bit color image) can't read.");
	case 4: throw std::invalid_argument("bmp::load_image: " + filename + " (16 bits color image) can't read.");
	}

	//カラーパレットの読み込み
	auto read_palette = [&ifs](const int bit_depth, const int used_colors) -> std::vector<unsigned char>
	{
		std::vector<unsigned char> palette;
		if(bit_depth <= 8 || used_colors > 0){
			palette.resize(4 * ((used_colors > 0) ? used_colors : (1 << bit_depth)));
			ifs.read((char*)palette.data(), palette.size());
		}
		return palette;
	};
	const auto palette = read_palette(bit_depth, swap_endian32(&info_header[32]));
	
	//画像読み込み
	const int row_bytes = ((((bit_depth * width + 7) >> 3) + 3) >> 2) << 2;

	std::vector<unsigned char> buffer(row_bytes);
	std::vector<unsigned char> data(channels * width * height);

	int y = (tmp_h >= 0) ? 0 : height - 1;
	const int dy = (tmp_h >= 0) ? 1 : -1;
	for(int i = 0; i < height; i++, y += dy){

		//１行読み込み
		ifs.read((char*)buffer.data(), row_bytes);

		//カラーパレットがある場合
		if(!(palette.empty())){
			for(int x = 0; x < width; x++){
				const int idx = buffer[x];
				data[channels * (x + width * y) + 0] = palette[4 * idx + 2]; //R
				data[channels * (x + width * y) + 1] = palette[4 * idx + 1]; //G
				data[channels * (x + width * y) + 2] = palette[4 * idx + 0]; //B
			}
		}
		//トゥルーカラーの場合
		else{
			for(int x = 0; x < width; x++){
				data[channels * (x + width * y) + 0] = buffer[channels * x + 2]; //R
				data[channels * (x + width * y) + 1] = buffer[channels * x + 1]; //G
				data[channels * (x + width * y) + 2] = buffer[channels * x + 0]; //B
			}
		}
	}

	//読み込み終了
	*p_width = width;
	*p_height = height;
	*p_channels = channels;
	*p_data = std::move(data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//bmpフォーマットで保存
inline void save_image(const std::string &filename, const int width, const int height, const int channels, const std::vector<unsigned char> &data)
{
	assert(width    >= 1);
	assert(height   >= 1);
	assert(channels >= 3);
	assert(channels <= 4);
	assert(data.size() == size_t(width * height * channels));

	//ファイルオープン
	std::ofstream ofs(
		filename, std::ios::binary
	);
	if(not(ofs.is_open())){
		throw std::invalid_argument("bmp::save_image: " + filename + " can't open.");
	}
	ofs.exceptions(std::ios::badbit | std::ios::failbit);

	//画像の横幅のサイズを計算
	//4の倍数になるように調整が必要
	const int row_bytes = ((width * 3 + 3) >> 2) << 2;

	//ファイルヘッダ書き込み
	const short bfType = 0x4d42;
	ofs.write((char*)&bfType, 2);
	const int bfSize = 14 + 40 + row_bytes * height;
	ofs.write((char*)&bfSize, 4);
	const short bfReserved1 = 0;
	ofs.write((char*)&bfReserved1, 2);
	const short bfReserved2 = 0;
	ofs.write((char*)&bfReserved2, 2);
	const int bfOffBits = 14 + 40;
	ofs.write((char*)&bfOffBits, 4);

	//情報ヘッダ書き込み
	const int biSize = 40;
	ofs.write((char*)&biSize, 4);
	const int biWidth = width;
	ofs.write((char*)&biWidth, 4);
	const int biHeight = height;
	ofs.write((char*)&biHeight, 4);
	const short biPlanes = 1;
	ofs.write((char*)&biPlanes, 2);
	const short biBitCount = 24;
	ofs.write((char*)&biBitCount, 2);
	const int biCompression = 0;
	ofs.write((char*)&biCompression, 4);
	const int biSizeImage = 0;
	ofs.write((char*)&biSizeImage, 4);
	const int biXPelsPerMeter = 0;
	ofs.write((char*)&biXPelsPerMeter, 4);
	const int biYPelsPerMeter = 0;
	ofs.write((char*)&biYPelsPerMeter, 4);
	const int biClrUsed = 0;
	ofs.write((char*)&biClrUsed, 4);
	const int biClrImportant = 0;
	ofs.write((char*)&biClrImportant, 4);

	//画像書き込み
	std::vector<unsigned char> scanline(
		row_bytes
	);
	for(int y = 0; y < height; y++){
		for(int x = 0; x < width; x++){
			scanline[3 * x + 0] = data[channels * (x + width * y) + 2]; //r
			scanline[3 * x + 1] = data[channels * (x + width * y) + 1]; //g
			scanline[3 * x + 2] = data[channels * (x + width * y) + 0]; //b
		}
		ofs.write((char*)scanline.data(), row_bytes);
	}

	//書き込み終了
	ofs.close();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace bmp

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
