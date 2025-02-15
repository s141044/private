
#pragma once

#ifndef NS_IMAGE_HPP
#define NS_IMAGE_HPP

#include<array>
#include<vector>
#include<string>
#include<limits>
#include<locale>
#include<cassert>
#include<algorithm>
#include"utility.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////
//前方宣言
///////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> class Image;

class hdr_image;
using imagef = Image<float>;
using imaged = Image<double>;
using image  = Image<unsigned char>;

///////////////////////////////////////////////////////////////////////////////////////////////////
//Image
///////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> class Image
{
public:

	//コンストラクタ
	Image();
	Image(const int width, const int height, const int channels);
	Image(const int width, const int height, const int channels, std::vector<T> &&data);
	Image(const int width, const int height, const int channels, const std::vector<T> &data);

	//画像情報を返す
	int width() const;
	int height() const;
	int channels() const;

	//ピクセルを返す
	T *operator()(const int x, const int y);
	const T *operator()(const int x, const int y) const;

protected:
	
	int m_width;
	int m_height;
	int m_channels;
	std::vector<T> m_data;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//Image<unsigned char>
///////////////////////////////////////////////////////////////////////////////////////////////////

template<> class Image<unsigned char>
{
public:

	//コンストラクタ
	Image();
	Image(const std::string &filename);
	Image(const int width, const int height, const int channels);
	Image(const int width, const int height, const int channels, std::vector<unsigned char> &&data);
	Image(const int width, const int height, const int channels, const std::vector<unsigned char> &data);

	//保存
	void save(const std::string &filename) const;

	//画像情報を返す
	int width() const;
	int height() const;
	int channels() const;

	//ピクセルを返す
	unsigned char *operator()(const int x, const int y);
	const unsigned char *operator()(const int x, const int y) const;

protected:
	
	int m_width;
	int m_height;
	int m_channels;
	std::vector<unsigned char> m_data;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//normal_map
/*/////////////////////////////////////////////////////////////////////////////////////////////////
zは[0,1]に変換
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class normal_map : public image
{
public:

	//コンストラクタ
	normal_map();
	normal_map(const int width, const int height);
	normal_map(const int width, const int height, std::vector<unsigned char> &&data);
	normal_map(const int width, const int height, const std::vector<unsigned char> &data);
	normal_map(const std::string &filename, const double a = 0.05);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//hdr_image
/*/////////////////////////////////////////////////////////////////////////////////////////////////
RGBE形式でデータを保持
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class hdr_image : public image
{
public:

	//コンストラクタ
	hdr_image();
	hdr_image(const imagef &image);
	hdr_image(const imaged &image);
	hdr_image(const std::string &filename);
	hdr_image(const int width, const int height);
	hdr_image(const int width, const int height, std::vector<unsigned char> &&data);
	hdr_image(const int width, const int height, const std::vector<unsigned char> &data);

	//保存
	void save(const std::string &filename) const;

	//キャスト演算子
	operator imagef() const;
	operator imaged() const;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#include"image/image-impl.hpp"
#include"image/hdr_image-impl.hpp"
#include"image/normal_map-impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
