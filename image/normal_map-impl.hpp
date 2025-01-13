
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

//RGBから法線に変換
namespace anon{ template<class T, bool z01 = true> inline std::enable_if_t<std::is_floating_point<T>::value> conv_rgb_to_normal(const unsigned char *rgb, T *normal)
{
	if constexpr(z01){
		normal[2] = rgb[2] * (1 / T(255));
	}else{
		normal[2] = rgb[2] * (2 / T(255)) - 1;
	}
	normal[0] = rgb[0] * (2 / T(255)) - 1;
	normal[1] = rgb[1] * (2 / T(255)) - 1;
}}

///////////////////////////////////////////////////////////////////////////////////////////////////

//法線からRGBに変換
namespace anon{ template<class T, bool z01 = true> inline std::enable_if_t<std::is_floating_point<T>::value> conv_normal_to_rgb(const T *normal, unsigned char *rgb)
{
	if constexpr(z01){
		rgb[2] = static_cast<unsigned char>(normal[2] * 255 + T(0.5));
	}else{
		rgb[2] = static_cast<unsigned char>((normal[2] + 1) * (255 / T(2)) + T(0.5));
	}
	rgb[0] = static_cast<unsigned char>((normal[0] + 1) * (255 / T(2)) + T(0.5));
	rgb[1] = static_cast<unsigned char>((normal[1] + 1) * (255 / T(2)) + T(0.5));
}}

///////////////////////////////////////////////////////////////////////////////////////////////////
//normal_map
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline normal_map::normal_map() : image()
{
}

inline normal_map::normal_map(const std::string &filename, const double a) : image(filename)
{
	//高さマップの場合
	auto is_height_map = [&]()
	{
		for(int y = 0; y < m_height; y++){
			for(int x = 0; x < m_width; x++){

				//R/G/Bの値が異なる場合は高さマップではない
				const unsigned char R = (*this)(x, y)[0];
				const unsigned char G = (*this)(x, y)[1];
				const unsigned char B = (*this)(x, y)[2];
				if((R != G) || (R != B) || (G != B)){
					return false;
				}
			}
		}
		return true;
	};
	if(is_height_map()){

		std::vector<unsigned char> pixels(
			m_width * m_height * 3
		);
		for(int y = 0; y < m_height; y++){
		
			const int ym = std::max(y - 1, 0);
			const int yp = std::min(y + 1, m_height - 1);
			const unsigned char *row  = (*this)(0, y);
			const unsigned char *rowm = (*this)(0, ym);
			const unsigned char *rowp = (*this)(0, yp);

			for(int x = 0; x < m_width; x++){

				const int xm = std::max(x - 1, 0);
				const int xp = std::min(x + 1, m_width - 1);
				const unsigned char height_xm = row[m_channels * xm];
				const unsigned char height_xp = row[m_channels * xp];
				const unsigned char height_ym = rowm[m_channels * x];
				const unsigned char height_yp = rowp[m_channels * x];

				//法線計算
				double n[3] = {
					-a * (height_xp - height_xm), 
					-a * (height_yp - height_ym), 
					+1
				};
				const double inv_len = 1 / sqrt(
					n[0] * n[0] + n[1] * n[1] + n[2] * n[2]
				);
				n[0] *= inv_len;
				n[1] *= inv_len;
				n[2] *= inv_len;

				//RGBに変換
				anon::conv_normal_to_rgb(n, &pixels[3 * (x + m_width * y)]);
			}
		}
		(*this) = normal_map(m_width, m_height, std::move(pixels)); return;
	}

	//zの変換方法は2つある(1:z=[-1,1], 2:z=[0,1])
	//変換方法1の場合
	auto is_type1 = [&]()
	{
		for(int y = 0; y < m_height; y++){
			for(int x = 0; x < m_width; x++){
				
				const unsigned char R = (*this)(x, y)[0];
				const unsigned char G = (*this)(x, y)[1];
				const unsigned char B = (*this)(x, y)[2];

				//白黒のピクセルは無視
				if((R + G + B == 0) || (R + G + B == 3 * 255)){
					continue;
				}

				//方法1で変換したときにzが負になる場合は方法1ではない
				if(B <= 127){
					return false;
				}
			}
		}
		return true;
	};
	if(is_type1()){

		//変換方法2の形式に
		for(int y = 0; y < m_height; y++){
			for(int x = 0; x < m_width; x++){

				double n[3]; 
				anon::conv_rgb_to_normal<double, false>((*this)(x, y), n); 
				anon::conv_normal_to_rgb<double, true >(n, (*this)(x, y));
			}
		}
	}
	//変換方法2の場合
	else{

		//正規化
		for(int y = 0; y < m_height; y++){
			for(int x = 0; x < m_width; x++){

				double n[3]; 
				anon::conv_rgb_to_normal<double, true>((*this)(x, y), n); 

				const double inv_len = 1 / sqrt(
					n[0] * n[0] + n[1] * n[1] + n[2] * n[2]
				);
				n[0] *= inv_len;
				n[1] *= inv_len;
				n[2] *= inv_len;
				anon::conv_normal_to_rgb<double, true>(n, (*this)(x, y));
			}
		}
	}
}

inline normal_map::normal_map(const int width, const int height) : image(width, height, 3)
{
}

inline normal_map::normal_map(const int width, const int height, std::vector<unsigned char> &&data) : image(width, height, 3, std::move(data))
{
}

inline normal_map::normal_map(const int width, const int height, const std::vector<unsigned char> &data) : image(width, height, 3, data)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
