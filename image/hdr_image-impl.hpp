
#include"load_save_hdr.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

//RGBからRGBEに変換
namespace anon{ template<class T> inline std::enable_if_t<std::is_floating_point<T>::value> conv_rgb_to_rgbe(const T *rgb, unsigned char *rgbe)
{
	const auto r = rgb[0];
	const auto g = rgb[1];
	const auto b = rgb[2];

	if(auto v = std::max(r, std::max(g, b))){

		int e; 
		v = frexp(v, &e) * 256 / v;
		
		rgbe[0] = static_cast<unsigned char>(r * v);
		rgbe[1] = static_cast<unsigned char>(g * v);
		rgbe[2] = static_cast<unsigned char>(b * v);
		rgbe[3] = static_cast<unsigned char>(e + 128);
	}
	else{

		rgbe[0] = 0;
		rgbe[1] = 0;
		rgbe[2] = 0;
		rgbe[3] = 0;
	}
}}

///////////////////////////////////////////////////////////////////////////////////////////////////

//RGBEからRGBに変換
namespace anon{ template<class T> struct power_of_two
{
	static T eval(const int32_t e)
	{
		return scalbln(T(1), e);
	}
};}

namespace anon{ template<> struct power_of_two<float>
{
	static_assert(
		std::numeric_limits<float>::is_iec559, "float must be the IEC 559 (IEEE 754) format."
	);
	static float eval(const int32_t e)
	{
		return reinterpret<float>((e + 127) << 23);
	}
};}

namespace anon{ template<> struct power_of_two<double>
{
	static_assert(
		std::numeric_limits<double>::is_iec559, "double must be the IEC 559 (IEEE 754) format."
	);
	static double eval(const int64_t e)
	{
		return reinterpret<double>((e + 1023) << 52);
	}
};}

namespace anon{ template<class T> std::enable_if_t<std::is_floating_point<T>::value> conv_rgbe_to_rgb(const unsigned char *rgbe, T *rgb)
{
	const unsigned char r = rgbe[0];
	const unsigned char g = rgbe[1];
	const unsigned char b = rgbe[2];
	const unsigned char e = rgbe[3];

	if(e){
		
		const auto val = anon::power_of_two<T>::eval(
			e - (128 + 8)
		);
		rgb[0] = r * val;
		rgb[1] = g * val;
		rgb[2] = b * val;
	}
	else{

		rgb[0] = 0;
		rgb[1] = 0;
		rgb[2] = 0;
	}
}}

///////////////////////////////////////////////////////////////////////////////////////////////////
//hdr_image
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline hdr_image::hdr_image() : image()
{
}

namespace anon{ template<class T> inline hdr_image convert_to_hdri(const Image<T> &src)
{
	assert(src.channels() >= 3);
	assert(src.channels() <= 4);

	hdr_image dst(src.width(), src.height());
	{
		for(int y = 0; y < src.height(); y++){
			for(int x = 0; x < src.width(); x++){
				anon::conv_rgb_to_rgbe(src(x, y), dst(x, y));
			}
		}
	}
	return dst;
}}

inline hdr_image::hdr_image(const imagef &image) : hdr_image(anon::convert_to_hdri(image))
{
}

inline hdr_image::hdr_image(const imaged &image) : hdr_image(anon::convert_to_hdri(image))
{
}

inline hdr_image::hdr_image(const std::string &filename) : image()
{
	hdr::load_image(filename, &m_width, &m_height, &m_data); m_channels = 4;
}

inline hdr_image::hdr_image(const int width, const int height) : image(width, height, 4)
{
}

inline hdr_image::hdr_image(const int width, const int height, std::vector<unsigned char> &&data) : image(width, height, 4, std::move(data))
{
}

inline hdr_image::hdr_image(const int width, const int height, const std::vector<unsigned char> &data) : image(width, height, 4, data)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//保存
inline void hdr_image::save(const std::string &filename) const
{
	hdr::save_image(filename, m_width, m_height, m_data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//キャスト演算子
namespace anon{ template<class T> Image<T> convert_to_imageT(const hdr_image &src)
{
	Image<T> dst(src.width(), src.height(), 3);
	{
		for(int y = 0; y < src.height(); y++){
			for(int x = 0; x < src.width(); x++){
				conv_rgbe_to_rgb(src(x, y), dst(x, y));
			}
		}
	}
	return dst;
}}

inline hdr_image::operator imagef() const
{
	return anon::convert_to_imageT<float>(*this);
}

inline hdr_image::operator imaged() const
{
	return anon::convert_to_imageT<double>(*this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
