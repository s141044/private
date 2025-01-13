
#ifndef DISABLE_THRID_PARTY_IMAGE_LIBRARY
#include"load_save_jpg.hpp"
#include"load_save_png.hpp"
#endif
#include"load_save_bmp.hpp"
#include"load_save_tga.hpp"
#include"load_save_hdr.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////
//Image
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
template<class T> inline Image<T>::Image() : m_width(), m_height(), m_channels()
{
}

template<class T> inline Image<T>::Image(const int width, const int height, const int channels) : Image(width, height, channels, std::vector<T>(width * height * channels))
{
}

template<class T> inline Image<T>::Image(const int width, const int height, const int channels, std::vector<T> &&data) : m_width(width), m_height(height), m_channels(channels), m_data(std::move(data))
{
	assert((m_width > 0) && (m_height > 0) && (m_data.size() == size_t(m_width * m_height * m_channels)));
}

template<class T> inline Image<T>::Image(const int width, const int height, const int channels, const std::vector<T> &data) : Image(width, height, channels, std::vector<T>(data))
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//画像情報を返す
template<class T> inline int Image<T>::width() const
{
	return m_width;
}

template<class T> inline int Image<T>::height() const
{
	return m_height;
}

template<class T> inline int Image<T>::channels() const
{
	return m_channels;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ピクセルを返す
template<class T> inline T *Image<T>::operator()(const int x, const int y)
{
	assert((0 <= x) && (x < m_width));
	assert((0 <= y) && (y < m_height));
	return &m_data[m_channels * (x + m_width * y)];
}

template<class T> inline const T *Image<T>::operator()(const int x, const int y) const
{
	return const_cast<Image*>(this)->operator()(x, y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//Image<unsigned char>
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline Image<unsigned char>::Image() : m_width(), m_height(), m_channels()
{
}

inline Image<unsigned char>::Image(const std::string &filename)
{
	//ファイルオープン
	std::ifstream ifs(
		filename, std::ios::binary
	);
	if(not(ifs.is_open())){
		throw std::ios::failure("image: " + filename + " can't open.");
	}
	ifs.exceptions(std::ios::badbit | std::ios::failbit);

	//フォーマット識別子に従い読み込み
	const unsigned char png_id[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
	const unsigned char jpg_id[] = { 0xff, 0xd8, 0xff };
	const unsigned char bmp_id[] = { 0x42, 0x4d };
	const unsigned char tga_id[] = { 0x54, 0x52, 0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4f, 0x4e, 0x2d, 0x58, 0x46, 0x49, 0x4c, 0x45, 0x2e, 0x00 };
	{
		char id[18];
		ifs.read(id, 10);

#ifndef DISABLE_THRID_PARTY_IMAGE_LIBRARY
		//png
		if(strncmp(id, (char*)png_id, sizeof(png_id)) == 0){
			png::load_image(filename, &m_width, &m_height, &m_channels, &m_data); return;
		}
		//jpg
		if(strncmp(id, (char*)jpg_id, sizeof(jpg_id)) == 0){
			jpg::load_image(filename, &m_width, &m_height, &m_channels, &m_data); return;
		}
#endif
		//bmp
		if(strncmp(id, (char*)bmp_id, sizeof(bmp_id)) == 0){
			bmp::load_image(filename, &m_width, &m_height, &m_channels, &m_data); return;
		}

		ifs.seekg(-18, std::ios::end); 
		ifs.read(id, 18);
	
		//tga(ver1ではフォーマット識別子が存在しない)
		if((strncmp(id, (char*)tga_id, sizeof(tga_id)) == 0) || (filename.substr(filename.find_last_of('.') + 1) == "tga") || (filename.substr(filename.find_last_of('.') + 1) == "TGA")){
			tga::load_image(filename, &m_width, &m_height, &m_channels, &m_data); return;
		}
	}
	throw std::invalid_argument("image: " + filename + " can't load.");
}

inline Image<unsigned char>::Image(const int width, const int height, const int channels) : Image(width, height, channels, std::vector<unsigned char>(width * height * channels))
{
}

inline Image<unsigned char>::Image(const int width, const int height, const int channels, std::vector<unsigned char> &&data) : m_width(width), m_height(height), m_channels(channels), m_data(std::move(data))
{
	assert((m_width > 0) && (m_height > 0) && (m_data.size() == size_t(m_width * m_height * m_channels)));
}

inline Image<unsigned char>::Image(const int width, const int height, const int channels, const std::vector<unsigned char> &data) : Image(width, height, channels, std::vector<unsigned char>(data))
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//保存
inline void Image<unsigned char>::save(const std::string &filename) const
{
	const std::string ext = [&]()
	{
		std::string tmp_ext	= filename.substr(filename.find_last_of('.') + 1);
		{
			for(auto &c : tmp_ext){
				c = char(tolower(c));
			}
		}
		return tmp_ext;
	}();

#ifndef DISABLE_THRID_PARTY_IMAGE_LIBRARY
	if(ext == "png"){ return png::save_image(filename, m_width, m_height, m_channels, m_data); }
	if(ext == "jpg"){ return jpg::save_image(filename, m_width, m_height, m_channels, m_data); }
#endif
	if(ext == "bmp"){ return bmp::save_image(filename, m_width, m_height, m_channels, m_data); }
	if(ext == "tga"){ return tga::save_image(filename, m_width, m_height, m_channels, m_data); }

	throw std::invalid_argument("Image::save: " + filename + " can't save.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//画像情報を返す
inline int Image<unsigned char>::width() const
{
	return m_width;
}

inline int Image<unsigned char>::height() const
{
	return m_height;
}

inline int Image<unsigned char>::channels() const
{
	return m_channels;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ピクセルを返す
inline unsigned char *Image<unsigned char>::operator()(const int x, const int y)
{
	assert((0 <= x) && (x < m_width));
	assert((0 <= y) && (y < m_height));
	return &m_data[m_channels * (x + m_width * y)];
}

inline const unsigned char *Image<unsigned char>::operator()(const int x, const int y) const
{
	return const_cast<Image*>(this)->operator()(x, y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
