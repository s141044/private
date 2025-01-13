
#pragma once

#ifndef NS_LOAD_SAVE_HDR_HPP
#define NS_LOAD_SAVE_HDR_HPP

#include<vector>
#include<string>
#include<fstream>
#include<sstream>
#include<algorithm>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace hdr{

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

//hdr画像の読み込み
inline void load_image(const std::string &filename, int *p_width, int *p_height, std::vector<unsigned char> *p_data)
{
	assert(p_width  != nullptr);
	assert(p_height != nullptr);
	assert(p_data   != nullptr);

	//ファイルオープン
	std::ifstream ifs(
		filename, std::ios::binary
	);
	if(not(ifs.is_open())){
		throw std::ios::failure("hdr::load_image: " + filename + " can't open.");
	}
	ifs.exceptions(std::ios::badbit | std::ios::failbit);

	//ヘッダ読み込み
	std::string line;
	while(std::getline(ifs, line), !line.empty()){

		std::stringstream ss(line);
		std::string buf; ss >> buf;

		if(buf.compare(0, 2, "#?") == 0){
			if((buf.compare(2, 8, "RADIANCE") != 0) && (buf.compare(2, 4, "RGBE") != 0)){
				throw std::invalid_argument("hdr::load_image: " + filename + " is not a hdr image.");
			}
		}
		if(buf.compare(0, 5, "GAMMA") == 0){
		}
	}

	//画像情報取得
	std::getline(ifs, line);

	std::stringstream ss(line);
	std::string buf1, buf2;
	int val1, val2;
	
	ss >> buf1 >> val1;
	ss >> buf2 >> val2;
	
	const int width  = (buf1[1] == 'X') ? val1 : val2;
	const int height = (buf1[1] == 'Y') ? val1 : val2;
	
	const int n = val1;
	const int m = val2;
	
	const int step1 = (buf1[0] == '+') ? 1 : -1;
	const int step2 = (buf2[0] == '+') ? 1 : -1;
	
	const int init_idx1 = (buf1[0] == '+') ? 0 : n - 1;
	const int init_idx2 = (buf2[0] == '+') ? 0 : m - 1;

	//画像読み込み
	std::vector<unsigned char> data(
		width * height * 4
	);
	std::vector<unsigned char> rgbe[] = {
		std::vector<unsigned char>(m), 
		std::vector<unsigned char>(m), 
		std::vector<unsigned char>(m), 
		std::vector<unsigned char>(m), 
	};

	for(int i = 0, idx1 = init_idx1; i < n; i++, idx1 += step1){

		//マジックナンバー読み込み
		char magic[4];
		ifs.read(magic, 4);

		if((magic[0] != 0x02) || (magic[1] != 0x02)){
			throw std::invalid_argument("hdr::load_image: " + filename + " is invalid.");
		}

		//RGBEデータをバッファに読み込み
		for(int j = 0; j < 4; j++){
			for(int k = 0; k < m;){

				//後続のデータが連続データか不連続データかを表すフラグを読み込む
				const int flag = ifs.get();

				//連続データの場合
				if(flag > 128){

					//連続するデータを読み込む
					const int col = ifs.get();
					
					//連続する分だけコピー
					const int count = flag - 128;
					memset((char*)&rgbe[j][k], col, count);
					k += count;
				}
				//不連続データの場合
				else{
					const int count = flag;
					ifs.read((char*)&rgbe[j][k], count);
					k += count;
				}
			}
		}

		//ピクセルに格納
		for(int j = 0, idx2 = init_idx2; j < m; j++, idx2 += step2){
			data[4 * (idx2 + m * idx1) + 0] = rgbe[0][j];
			data[4 * (idx2 + m * idx1) + 1] = rgbe[1][j];
			data[4 * (idx2 + m * idx1) + 2] = rgbe[2][j];
			data[4 * (idx2 + m * idx1) + 3] = rgbe[3][j];
		}
	}

	//読み込み終了
	*p_width = width;
	*p_height = height;
	*p_data = std::move(data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//hdrフォーマットで保存
inline void save_image(const std::string &filename, const int width, const int height, const std::vector<unsigned char> &data)
{
	assert(width  >= 1);
	assert(height >= 1);
	assert(data.size() == size_t(width * height * 4));

	//ファイルオープン
	std::ofstream ofs(
		filename, std::ios::binary
	);
	if(not(ofs.is_open())){
		throw std::ios::failure("hdr::save_image: " + filename + " can't open.");
	}
	ofs.exceptions(std::ios::badbit | std::ios::failbit);

	//ヘッダ書き込み
	ofs << "#?RADIANCE" << std::endl << std::endl;

	//画像情報書き込み
	ofs << "-Y " << height;
	ofs << " ";
	ofs << "+X " << width; 
	ofs << std::endl;
		
	//画像書き込み
	std::vector<unsigned char> rgbe[] = {
		std::vector<unsigned char>(width),
		std::vector<unsigned char>(width),
		std::vector<unsigned char>(width),
		std::vector<unsigned char>(width),
	};

	for(int i = 0, y = height - 1; i < height; i++, y--){

		//バッファに書き込み
		for(int x = 0; x < width; x++){
			const int idx = 4 * (x + width * y);
			rgbe[0][x] = data[idx + 0];
			rgbe[1][x] = data[idx + 1];
			rgbe[2][x] = data[idx + 2];
			rgbe[3][x] = data[idx + 3];
		}

		//マジックナンバー書き込み
		char magic[4] = { 0x02, 0x02, static_cast<char>((width >> 8) & 0xff), static_cast<char>(width & 0xff) };
		ofs.write(magic, 4);

		//RGBE書き込み
		for(int c = 0; c < 4; c++){
			for(int j = 0, k = 1; j < width; j += k, k = 1){
				//行の間の場合
				if(j + 1 < width){
					//連続データの場合
					if(rgbe[c][j] == rgbe[c][j + 1]){
						//連続データが続く間ループ
						const int n = std::min(127, width - j);
						for (k++; k < n; k++){
							if(rgbe[c][j] != rgbe[c][j + k]){
								break;
							}
						}
						//データ書き込み
						const unsigned char tmp = static_cast<unsigned char>(k + 128);
						ofs.write((char*)&tmp, 1);
						ofs.write((char*)&rgbe[c][j], 1);
					}
					//不連続データの場合
					else{
						//不連続データが続く間ループ
						const int n = std::min(128, width - j);
						for (k++; k < n; k++){
							if(rgbe[c][j + k - 1] == rgbe[c][j + k]){
								k--;
								break;
							}
						}
						//データ書き込み
						const unsigned char tmp = static_cast<unsigned char>(k);
						ofs.write((char*)&tmp, 1);
						ofs.write((char*)&rgbe[c][j], tmp);
					}
				}
				//行の最後の場合
				else{
					//不連続データとして書き込み
					const unsigned char tmp = 1;
					ofs.write((char*)&tmp, 1);
					ofs.write((char*)&rgbe[c][j], tmp);
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace hdr

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
