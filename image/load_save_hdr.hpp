
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
//�֐���`
///////////////////////////////////////////////////////////////////////////////////////////////////

//hdr�摜�̓ǂݍ���
inline void load_image(const std::string &filename, int *p_width, int *p_height, std::vector<unsigned char> *p_data)
{
	assert(p_width  != nullptr);
	assert(p_height != nullptr);
	assert(p_data   != nullptr);

	//�t�@�C���I�[�v��
	std::ifstream ifs(
		filename, std::ios::binary
	);
	if(not(ifs.is_open())){
		throw std::ios::failure("hdr::load_image: " + filename + " can't open.");
	}
	ifs.exceptions(std::ios::badbit | std::ios::failbit);

	//�w�b�_�ǂݍ���
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

	//�摜���擾
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

	//�摜�ǂݍ���
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

		//�}�W�b�N�i���o�[�ǂݍ���
		char magic[4];
		ifs.read(magic, 4);

		if((magic[0] != 0x02) || (magic[1] != 0x02)){
			throw std::invalid_argument("hdr::load_image: " + filename + " is invalid.");
		}

		//RGBE�f�[�^���o�b�t�@�ɓǂݍ���
		for(int j = 0; j < 4; j++){
			for(int k = 0; k < m;){

				//�㑱�̃f�[�^���A���f�[�^���s�A���f�[�^����\���t���O��ǂݍ���
				const int flag = ifs.get();

				//�A���f�[�^�̏ꍇ
				if(flag > 128){

					//�A������f�[�^��ǂݍ���
					const int col = ifs.get();
					
					//�A�����镪�����R�s�[
					const int count = flag - 128;
					memset((char*)&rgbe[j][k], col, count);
					k += count;
				}
				//�s�A���f�[�^�̏ꍇ
				else{
					const int count = flag;
					ifs.read((char*)&rgbe[j][k], count);
					k += count;
				}
			}
		}

		//�s�N�Z���Ɋi�[
		for(int j = 0, idx2 = init_idx2; j < m; j++, idx2 += step2){
			data[4 * (idx2 + m * idx1) + 0] = rgbe[0][j];
			data[4 * (idx2 + m * idx1) + 1] = rgbe[1][j];
			data[4 * (idx2 + m * idx1) + 2] = rgbe[2][j];
			data[4 * (idx2 + m * idx1) + 3] = rgbe[3][j];
		}
	}

	//�ǂݍ��ݏI��
	*p_width = width;
	*p_height = height;
	*p_data = std::move(data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//hdr�t�H�[�}�b�g�ŕۑ�
inline void save_image(const std::string &filename, const int width, const int height, const std::vector<unsigned char> &data)
{
	assert(width  >= 1);
	assert(height >= 1);
	assert(data.size() == size_t(width * height * 4));

	//�t�@�C���I�[�v��
	std::ofstream ofs(
		filename, std::ios::binary
	);
	if(not(ofs.is_open())){
		throw std::ios::failure("hdr::save_image: " + filename + " can't open.");
	}
	ofs.exceptions(std::ios::badbit | std::ios::failbit);

	//�w�b�_��������
	ofs << "#?RADIANCE" << std::endl << std::endl;

	//�摜��񏑂�����
	ofs << "-Y " << height;
	ofs << " ";
	ofs << "+X " << width; 
	ofs << std::endl;
		
	//�摜��������
	std::vector<unsigned char> rgbe[] = {
		std::vector<unsigned char>(width),
		std::vector<unsigned char>(width),
		std::vector<unsigned char>(width),
		std::vector<unsigned char>(width),
	};

	for(int i = 0, y = height - 1; i < height; i++, y--){

		//�o�b�t�@�ɏ�������
		for(int x = 0; x < width; x++){
			const int idx = 4 * (x + width * y);
			rgbe[0][x] = data[idx + 0];
			rgbe[1][x] = data[idx + 1];
			rgbe[2][x] = data[idx + 2];
			rgbe[3][x] = data[idx + 3];
		}

		//�}�W�b�N�i���o�[��������
		char magic[4] = { 0x02, 0x02, static_cast<char>((width >> 8) & 0xff), static_cast<char>(width & 0xff) };
		ofs.write(magic, 4);

		//RGBE��������
		for(int c = 0; c < 4; c++){
			for(int j = 0, k = 1; j < width; j += k, k = 1){
				//�s�̊Ԃ̏ꍇ
				if(j + 1 < width){
					//�A���f�[�^�̏ꍇ
					if(rgbe[c][j] == rgbe[c][j + 1]){
						//�A���f�[�^�������ԃ��[�v
						const int n = std::min(127, width - j);
						for (k++; k < n; k++){
							if(rgbe[c][j] != rgbe[c][j + k]){
								break;
							}
						}
						//�f�[�^��������
						const unsigned char tmp = static_cast<unsigned char>(k + 128);
						ofs.write((char*)&tmp, 1);
						ofs.write((char*)&rgbe[c][j], 1);
					}
					//�s�A���f�[�^�̏ꍇ
					else{
						//�s�A���f�[�^�������ԃ��[�v
						const int n = std::min(128, width - j);
						for (k++; k < n; k++){
							if(rgbe[c][j + k - 1] == rgbe[c][j + k]){
								k--;
								break;
							}
						}
						//�f�[�^��������
						const unsigned char tmp = static_cast<unsigned char>(k);
						ofs.write((char*)&tmp, 1);
						ofs.write((char*)&rgbe[c][j], tmp);
					}
				}
				//�s�̍Ō�̏ꍇ
				else{
					//�s�A���f�[�^�Ƃ��ď�������
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
