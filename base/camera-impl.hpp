
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//関数定義
///////////////////////////////////////////////////////////////////////////////////////////////////

//任意軸回転
namespace anon{ inline float3x3 rotate(const float3 &axis, const float radian)
{
	const float c = cos(radian);
	const float s = sin(radian);
	return float3x3(
		axis.x * axis.x * (1 - c) + c, axis.x * axis.y * (1 - c) - axis.z * s, axis.x * axis.z * (1 - c) + axis.y * s, 
		axis.x * axis.y * (1 - c) + axis.z * s, axis.y * axis.y * (1 - c) + c, axis.y * axis.z * (1 - c) - axis.x * s, 
		axis.x * axis.z * (1 - c) - axis.y * s, axis.y * axis.z * (1 - c) + axis.x * s, axis.z * axis.z * (1 - c) + c);
}}

///////////////////////////////////////////////////////////////////////////////////////////////////
//camera
///////////////////////////////////////////////////////////////////////////////////////////////////

//位置&注視点を設定
inline void camera::look_at(const float3 &pos, const float3 &center)
{
	const float3 dir = center - pos;
	const float dist = norm(dir);
	look_at(pos, dir / dist, dist);
}

inline void camera::look_at(const float3 &pos, const float3 &dir, const float dist)
{
	m_pos = pos;
	m_dist = dist;
	m_axis_z = -dir;

	const float theta = acos(std::clamp(m_axis_z.y, -1.0f, 1.0f));
	const float phi = atan2(m_axis_z.z, m_axis_z.x);
	const float ct = cos(theta);
	const float st = sin(theta);
	const float cp = cos(phi);
	const float sp = sin(phi);
	m_axis_y = float3(-ct * cp, st, -ct * sp);
	m_axis_x = float3(sp, 0, -cp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//位置を返す
inline const float3 &camera::position() const
{
	return m_pos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//位置を設定
inline void camera::set_position(const float3& position)
{
	m_pos = position;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//注視点を返す
inline float3 camera::center() const
{
	return m_pos - m_axis_z * m_dist;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//アスペクト比を設定
inline void camera::set_aspect(const float aspect)
{
	assert(aspect > 0);
	m_aspect = aspect;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//アスペクト比を返す
inline float camera::aspect() const
{
	return m_aspect;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//画角を設定
inline void camera::set_fovy(const float fovy)
{
	assert(fovy > 0);
	m_fovy = fovy;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//画角を返す
inline float camera::fovy() const
{
	return m_fovy;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//クリップ距離を設定
inline void camera::set_near_clip(const float dist)
{
	assert(dist > 0);
	m_near_clip = dist;
}

inline void camera::set_far_clip(const float dist)
{
	assert(dist > 0);
	m_far_clip = dist;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//クリップ距離を返す
inline float camera::near_clip() const
{
	return m_near_clip;
}

inline float camera::far_clip() const
{
	return m_far_clip;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ビュー行列を返す
inline float3x4 camera::view_mat() const
{
	return float3x4(
		m_axis_x.x, m_axis_x.y, m_axis_x.z, -dot(m_pos, m_axis_x), 
		m_axis_y.x, m_axis_y.y, m_axis_y.z, -dot(m_pos, m_axis_y), 
		m_axis_z.x, m_axis_z.y, m_axis_z.z, -dot(m_pos, m_axis_z));
}

inline float3x4 camera::inv_view_mat() const
{
	throw;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//射影行列を返す
inline float4x4 camera::proj_mat() const
{
	const float f = m_far_clip;
	const float n = m_near_clip;
	const float sy = 1 / tan(m_fovy * (PI() / 180) / 2);
	const float sx = sy / m_aspect;
	const float sz = n / (f - n);
	const float tz = f * sz;
	return float4x4(
		sx, 0, 0,  0, 
		0, sy, 0,  0,
		0, 0, sz, tz, 
		0, 0, -1,  0);
}

inline float4x4 camera::inv_proj_mat() const
{
	throw; //必要になったら対応する
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//軸を返す
inline const float3 &camera::axis_x() const
{
	return m_axis_x;
}

inline const float3 &camera::axis_y() const
{
	return m_axis_y;
}

inline const float3 &camera::axis_z() const
{
	return m_axis_z;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//注視点までの距離を返す
inline float camera::distance() const
{
	return m_dist;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//平行移動
inline void camera::translate(const float3 &t)
{
	m_pos += t;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//水平首振り
inline void camera::pan(const float radian)
{
#if 0
	const float3x3 m = anon::rotate(m_axis_y, radian);
	m_axis_x = m * m_axis_x;
	m_axis_z = m * m_axis_z;
#else
	const auto m = float3x3(nn::rotate_y(radian));
	m_axis_x = m * m_axis_x;
	m_axis_y = m * m_axis_y;
	m_axis_z = m * m_axis_z;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//垂直首振り
inline void camera::tilt(const float radian)
{
	const float3x3 m = anon::rotate(m_axis_x, radian);
	m_axis_y = m * m_axis_y;
	m_axis_z = m * m_axis_z;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//注視点周りの回転
inline void camera::rotate_x(const float radian)
{
#if 0
	const float3x3 m = anon::rotate(m_axis_y, radian);
	const float3 center = m_pos - m_axis_z * m_dist;
	m_pos = m * (m_pos - center) + center;
	m_axis_x = m * m_axis_x;
	m_axis_z = m * m_axis_z;
#else
	const auto m = float3x3(nn::rotate_y(radian));
	const float3 center = m_pos - m_axis_z * m_dist;
	m_pos = m * (m_pos - center) + center;
	m_axis_x = m * m_axis_x;
	m_axis_y = m * m_axis_y;
	m_axis_z = m * m_axis_z;
#endif
}

inline void camera::rotate_y(const float radian)
{
	const float3x3 m = anon::rotate(m_axis_x, radian);
	const float3 center = m_pos - m_axis_z * m_dist;
	m_pos = m * (m_pos - center) + center;
	m_axis_y = m * m_axis_y;
	m_axis_z = m * m_axis_z;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//注視点に接近
inline void camera::dolly_in(const float dist)
{
	const float3 center = m_pos - m_axis_z * m_dist;
	m_dist = std::max(m_dist - dist, 0.01f);
	m_pos = center + m_axis_z * m_dist;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//画角を狭める
inline void camera::zoom_in(const float degree)
{
	m_fovy = std::max(m_fovy - degree, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
