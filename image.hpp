
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
//�O���錾
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

	//�R���X�g���N�^
	Image();
	Image(const int width, const int height, const int channels);
	Image(const int width, const int height, const int channels, std::vector<T> &&data);
	Image(const int width, const int height, const int channels, const std::vector<T> &data);

	//�摜����Ԃ�
	int width() const;
	int height() const;
	int channels() const;

	//�s�N�Z����Ԃ�
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

	//�R���X�g���N�^
	Image();
	Image(const std::string &filename);
	Image(const int width, const int height, const int channels);
	Image(const int width, const int height, const int channels, std::vector<unsigned char> &&data);
	Image(const int width, const int height, const int channels, const std::vector<unsigned char> &data);

	//�ۑ�
	void save(const std::string &filename) const;

	//�摜����Ԃ�
	int width() const;
	int height() const;
	int channels() const;

	//�s�N�Z����Ԃ�
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
z��[0,1]�ɕϊ�
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class normal_map : public image
{
public:

	//�R���X�g���N�^
	normal_map();
	normal_map(const int width, const int height);
	normal_map(const int width, const int height, std::vector<unsigned char> &&data);
	normal_map(const int width, const int height, const std::vector<unsigned char> &data);
	normal_map(const std::string &filename, const double a = 0.05);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//hdr_image
/*/////////////////////////////////////////////////////////////////////////////////////////////////
RGBE�`���Ńf�[�^��ێ�
/////////////////////////////////////////////////////////////////////////////////////////////////*/

class hdr_image : public image
{
public:

	//�R���X�g���N�^
	hdr_image();
	hdr_image(const imagef &image);
	hdr_image(const imaged &image);
	hdr_image(const std::string &filename);
	hdr_image(const int width, const int height);
	hdr_image(const int width, const int height, std::vector<unsigned char> &&data);
	hdr_image(const int width, const int height, const std::vector<unsigned char> &data);

	//�ۑ�
	void save(const std::string &filename) const;

	//�L���X�g���Z�q
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
