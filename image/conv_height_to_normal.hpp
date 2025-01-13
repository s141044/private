
#pragma once

#ifndef NS_CONV_HEIGHT_TO_NORMAL_HPP
#define NS_CONV_HEIGHT_TO_NORMAL_HPP

#include<cmath>
#include<vector>
#include<algorithm>

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace ns{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace anon{

///////////////////////////////////////////////////////////////////////////////////////////////////

//高さマップから法線マップに変換
inline void conv_height_to_normal(const int width, const int height, const int src_channels, const std::vector<unsigned char> &src_pixels, const int dst_channels, std::vector<unsigned char> *p_dst_pixels, const double a)
{
	//高さマップか確認
	bool is_height_map = false;
	for(int i = 0, n = width * height * src_channels; i < n; i++){

		//ピクセル値が黒か白は背景とみなす
		if((src_pixels[i + 0] ==   0) && (src_pixels[i + 1] ==   0) && (src_pixels[i + 2] ==   0)) continue;
		if((src_pixels[i + 0] == 255) && (src_pixels[i + 1] == 255) && (src_pixels[i + 2] == 255)) continue;
		
		//ピクセル値を法線に変換
		const double nx = src_pixels[i + 0] / 255.0 * 2.0 - 1.0;
		const double ny = src_pixels[i + 1] / 255.0 * 2.0 - 1.0;
		const double nz = src_pixels[i + 2] / 255.0 * 2.0 - 1.0;

		//法線の長さがおおよそ1でない場合は高さマップとみなす
		if(fabs(1.0 - (nx * nx + ny * ny + nz * nz)) > 0.1){ 
			is_height_map = true; break;
		}
	}
	if(is_height_map){

		std::vector<unsigned char> pixels(
			width * height * dst_channels
		);
		for(int y = 0; y < height; y++){

			const int ym = std::max(y - 1, 0);
			const int yp = std::min(y + 1, height - 1);
			const unsigned char *row = &src_pixels[src_channels * width * y];
			const unsigned char *rowm = &src_pixels[src_channels * width * ym];
			const unsigned char *rowp = &src_pixels[src_channels * width * yp];

			for(int x = 0; x < width; x++){

				const int xm = std::max(x - 1, 0);
				const int xp = std::min(x + 1, width - 1);
				const unsigned char height_xm = row[src_channels * xm];
				const unsigned char height_xp = row[src_channels * xp];
				const unsigned char height_ym = rowm[src_channels * x];
				const unsigned char height_yp = rowp[src_channels * x];

				//傾き
				const double dx = a * (height_xp - height_xm);
				const double dy = a * (height_yp - height_ym);

				//法線
				const double tmp_nx = -dx;
				const double tmp_ny = -dy;
				const double tmp_nz = 1.0;

				const double inv_norm = 1.0 / std::sqrt(tmp_nx * tmp_nx + tmp_ny * tmp_ny + tmp_nz * tmp_nz);
			
				const double nx = tmp_nx * inv_norm;
				const double ny = tmp_ny * inv_norm;
				const double nz = tmp_nz * inv_norm;

				//RGBに変換
				pixels[3 * (x + width * y) + 0] = static_cast<unsigned char>(std::max(0.0, std::min(nx * 0.5 + 0.5, 1.0)) * 255.0);
				pixels[3 * (x + width * y) + 1] = static_cast<unsigned char>(std::max(0.0, std::min(ny * 0.5 + 0.5, 1.0)) * 255.0);
				pixels[3 * (x + width * y) + 2] = static_cast<unsigned char>(std::max(0.0, std::min(nz * 0.5 + 0.5, 1.0)) * 255.0);
			}
		}
		if(dst_channels == 4){
			for(int i = 3, n = width * height * 4; i < n; i += 4){
				pixels[i] = 127;
			}
		}
		*p_dst_pixels = std::move(pixels);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace anon

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace ns

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
