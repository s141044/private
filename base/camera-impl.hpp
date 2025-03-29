
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//�֐���`
///////////////////////////////////////////////////////////////////////////////////////////////////

//�C�ӎ���]
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

//�ʒu&�����_��ݒ�
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

//�ʒu��Ԃ�
inline const float3 &camera::position() const
{
	return m_pos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�ʒu��ݒ�
inline void camera::set_position(const float3& position)
{
	m_pos = position;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�����_��Ԃ�
inline float3 camera::center() const
{
	return m_pos - m_axis_z * m_dist;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�A�X�y�N�g���ݒ�
inline void camera::set_aspect(const float aspect)
{
	assert(aspect > 0);
	m_aspect = aspect;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�A�X�y�N�g���Ԃ�
inline float camera::aspect() const
{
	return m_aspect;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//��p��ݒ�
inline void camera::set_fovy(const float fovy)
{
	assert(fovy > 0);
	m_fovy = fovy;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//��p��Ԃ�
inline float camera::fovy() const
{
	return m_fovy;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�N���b�v������ݒ�
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

//�N���b�v������Ԃ�
inline float camera::near_clip() const
{
	return m_near_clip;
}

inline float camera::far_clip() const
{
	return m_far_clip;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�r���[�s���Ԃ�
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

//�ˉe�s���Ԃ�
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
	throw; //�K�v�ɂȂ�����Ή�����
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//����Ԃ�
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

//�����_�܂ł̋�����Ԃ�
inline float camera::distance() const
{
	return m_dist;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//���s�ړ�
inline void camera::translate(const float3 &t)
{
	m_pos += t;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//������U��
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

//������U��
inline void camera::tilt(const float radian)
{
	const float3x3 m = anon::rotate(m_axis_x, radian);
	m_axis_y = m * m_axis_y;
	m_axis_z = m * m_axis_z;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�����_����̉�]
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

//�����_�ɐڋ�
inline void camera::dolly_in(const float dist)
{
	const float3 center = m_pos - m_axis_z * m_dist;
	m_dist = std::max(m_dist - dist, 0.01f);
	m_pos = center + m_axis_z * m_dist;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//��p�����߂�
inline void camera::zoom_in(const float degree)
{
	m_fovy = std::max(m_fovy - degree, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
