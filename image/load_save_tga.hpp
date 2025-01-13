
#ifndef NS_LOAD_SAVE_TGA_HPP
#define NS_LOAD_SAVE_TGA_HPP

#include<vector>
#include<string>
#include<fstream>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace tga{

///////////////////////////////////////////////////////////////////////////////////////////////////
//�֐���`
///////////////////////////////////////////////////////////////////////////////////////////////////

//�G���f�B�A���ϊ�
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

inline std::vector<unsigned char> load_color_map_image(const std::string &filename, std::ifstream &ifs, const int width, const int height, const int channles, const char *header)
{
	//�J���[�}�b�v�������݈ʒu�Ɉړ�
	ifs.seekg(swap_endian16(&header[3]));

	//�J���[�}�b�v�ǂݍ���
	const int num_entries = swap_endian16(&header[3]);
	const int entry_size = header[7];

	std::vector<unsigned char> color_map(
		num_entries * entry_size
	);
	ifs.read((char*)color_map.data(), color_map.size());

	//�摜�ǂݍ���
	std::vector<unsigned char> data(width * height * channles);
	{
		//���k�f�[�^�̏ꍇ
		if(header[2] & 0x8){
			throw std::invalid_argument("tga::load_image: " + filename + " (compressed color map image) can't read.");
		}
		//�񈳏k�f�[�^�̏ꍇ
		else{
			throw std::invalid_argument("tga::load_image: " + filename + " (color map image) can't read.");
		}
	}

	//�ǂݍ��ݏI��
	return data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline std::vector<unsigned char> load_compressed_rgba_true_color_image(const std::string&, std::ifstream &ifs, const int width, const int height, const int channels,const char*)
{
	std::vector<unsigned char> data(
		width * height * channels
	);

	for(int i = 0, n = int(data.size()); i < n;){

		//�㑱�̃f�[�^���A���f�[�^���s�A���f�[�^����\���t���O��ǂݍ���
		const int flag = ifs.get();

		//�A���f�[�^�̏ꍇ
		if(flag & 0x80){

			//�A������f�[�^��ǂݍ���
			unsigned char col[4];
			for(int j = 0; j < channels; j++){
				col[j] = static_cast<unsigned char>(ifs.get());
			}

			//�A������f�[�^�������R�s�[
			const int m = (flag & 0x7f) + 1;
			for(int j = 0; j < m; j++){
				for(int k = 0; k < channels; k++){
					data[i++] = col[k];
				}
			}
		}
		//�s�A���f�[�^�̏ꍇ
		else{

			//�s�A���f�[�^���ǂݍ���
			const int count = channels * ((flag & 0x7f) + 1);
			ifs.read((char*)&data[i], count);
			i += count;
		}
	}

	//�ǂݍ��ݏI��
	return data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline std::vector<unsigned char> load_compressed_binary_true_color_image(const std::string &filename, std::ifstream&, const int, const int, const int,const char*)
{
	throw std::invalid_argument("tga::load_image: " + filename + " (compressed binary image) can't read.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline std::vector<unsigned char> load_rgba_true_color_image(const std::string&, std::ifstream &ifs, const int width, const int height, const int channels,const char*)
{
	//���ړǂݍ��ނ���
	std::vector<unsigned char> data(
		width * height * channels
	);
	return ifs.read((char*)data.data(), data.size()), std::move(data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline std::vector<unsigned char> load_binary_true_color_image(const std::string &filename, std::ifstream&, const int, const int, const int,const char*)
{
	throw std::invalid_argument("tga::load_image: " + filename + " (binary image) can't read.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline std::vector<unsigned char> load_true_color_image(const std::string &filename, std::ifstream &ifs, const int width, const int height, const int channels,const char *header)
{
	//�摜�ǂݍ���
	switch(header[2]){
	case 10: 
	case 11:
		return load_compressed_rgba_true_color_image(filename, ifs, width, height, channels, header);
	case 2:
	case 3:
		return load_rgba_true_color_image(filename, ifs, width, height, channels, header);
	default:
		throw std::invalid_argument("tga::load_image: " + filename + " can't read.");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline std::vector<unsigned char> load_data(const std::string &filename, std::ifstream &ifs, const int width, const int height, const int channels, const char *header)
{
	//�J���[�}�b�v������ꍇ
	if(header[1]){
		return load_color_map_image(filename, ifs, width, height, channels, header);
	}
	//�g�D���[�J���[�̏ꍇ
	else{
		return load_true_color_image(filename, ifs, width, height, channels, header);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline void convert_bgra_to_rgba(const int width, const int height, const int channels, std::vector<unsigned char> &data)
{
	if((channels != 3) && (channels != 4)){
		return;
	}

	for(int y = 0; y < height; y++){
		for(int x = 0; x < width; x++){
			const int idx = channels * (x + width * y);
			const unsigned char b = data[idx + 0];
			data[idx + 0] = data[idx + 2];
			data[idx + 2] = b;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline void flip_y_axis(const int width, const int height, const int channels, std::vector<unsigned char> &data)
{
	for(int y1 = 0, y2 = height - 1; y1 < y2; y1++, y2--){
		for(int x = 0; x < width; x++){
			
			const int idx1 = channels * (x + width * y1);
			const int idx2 = channels * (x + width * y2);

			//�s�N�Z���l������
			unsigned char col[4];
			memcpy(col, &data[idx1], channels);
			memcpy(&data[idx1], &data[idx2], channels);
			memcpy(&data[idx2], col, channels);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

inline void flip_x_axis(const int width, const int height, const int channels, std::vector<unsigned char> &data)
{
	for(int y = 0; y < height; y++){
		for(int x1 = 0, x2 = width - 1; x1 < x2; x1++, x2--){
			
			const int idx1 = channels * (x1 + width * y);
			const int idx2 = channels * (x2 + width * y);

			//�s�N�Z���l������
			unsigned char col[4];
			memcpy(col, &data[idx1], channels);
			memcpy(&data[idx1], &data[idx2], channels);
			memcpy(&data[idx2], col, channels);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//tga�摜�̓ǂݍ���
inline void load_image(const std::string &filename, int *p_width, int *p_height, int *p_channels, std::vector<unsigned char> *p_data)
{
	assert(p_width    != nullptr);
	assert(p_height   != nullptr);
	assert(p_channels != nullptr);
	assert(p_data     != nullptr);

	//�t�@�C���I�[�v��
	std::ifstream ifs(
		filename, std::ios::binary
	);
	if(not(ifs.is_open())){
		throw std::ios::failure("tga::load_image: " + filename + " can't open.");
	}
	ifs.exceptions(std::ios::badbit | std::ios::failbit);

	//�t�@�C���w�b�_�ǂݍ���
	char header[18];
	ifs.read(header, 18);

	if(header[2] == 0){
		throw std::invalid_argument("tga::load_image: " + filename + " doesn't have image data.");
	}

	//�摜���擾
	const int width = swap_endian16(&header[12]);
	const int height = swap_endian16(&header[14]);
	const int channels = header[16] >> 3;

	//�摜ID�ǂݔ�΂�
	ifs.ignore(header[0]);

	//�摜�ǂݍ���
	auto data = tga::load_data(filename, ifs, width, height, channels, header);

	//BGRA����RGBA�ɕϊ�
	tga::convert_bgra_to_rgba(width, height, channels, data);

	//x���𔽓]
	if(header[17] & 0x10){
		tga::flip_x_axis(width, height, channels, data);
	}

	//y���𔽓]
	if(header[17] & 0x20){
		tga::flip_y_axis(width, height, channels, data);
	}

	//�ǂݍ��ݏI��
	*p_width = width;
	*p_height = height;
	*p_channels = channels;
	*p_data = std::move(data);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//tga�t�H�[�}�b�g�ŕۑ�
inline void save_image(const std::string&, const int, const int, const int, const std::vector<unsigned char>&)
{
	throw std::logic_error("tga::save_image: save_image is not implemented.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace tga

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
