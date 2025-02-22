
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//transform
///////////////////////////////////////////////////////////////////////////////////////////////////

//ïœä∑çsóÒÇï‘Ç∑
inline float3x4 transform::ltow_matrix() const
{
	const float sx = sin(to_radian(m_r.x));
	const float cx = cos(to_radian(m_r.x));
	const float sy = sin(to_radian(m_r.y));
	const float cy = cos(to_radian(m_r.y));
	const float sz = sin(to_radian(m_r.z));
	const float cz = cos(to_radian(m_r.z));
	const float r00 = cy * cz;
	const float r01 = sx * sy * cz - cx * sz;
	const float r02 = cx * sy * cz + sx * sz;
	const float r10 = cy * sz;
	const float r11 = sx * sy * sz + cx * cz;
	const float r12 = cx * sy * sz - sx * cz;
	const float r20 = -sy;
	const float r21 = sx * cy;
	const float r22 = cx * cy;

	return float3x4(
		m_s.x * r00, m_s.y * r01, m_s.z * r02, m_t.x, 
		m_s.x * r10, m_s.y * r11, m_s.z * r12, m_t.y, 
		m_s.x * r20, m_s.y * r21, m_s.z * r22, m_t.z);
}

inline float3x4 transform::wtol_matrix() const
{
	const float sx = sin(to_radian(m_r.x));
	const float cx = cos(to_radian(m_r.x));
	const float sy = sin(to_radian(m_r.y));
	const float cy = cos(to_radian(m_r.y));
	const float sz = sin(to_radian(m_r.z));
	const float cz = cos(to_radian(m_r.z));
	const float r00 = cy * cz;
	const float r01 = sx * sy * cz - cx * sz;
	const float r02 = cx * sy * cz + sx * sz;
	const float r10 = cy * sz;
	const float r11 = sx * sy * sz + cx * cz;
	const float r12 = cx * sy * sz - sx * cz;
	const float r20 = -sy;
	const float r21 = sx * cy;
	const float r22 = cx * cy;

	const float inv_sx = 1 / m_s.x;
	const float inv_sy = 1 / m_s.y;
	const float inv_sz = 1 / m_s.z;

	return float3x4(
		inv_sx * r00, inv_sx * r10, inv_sx * r20, -inv_sx * (r00 * m_t.x + r10 * m_t.y + r20 * m_t.z), 
		inv_sy * r01, inv_sy * r11, inv_sy * r21, -inv_sy * (r01 * m_t.x + r11 * m_t.y + r21 * m_t.z), 
		inv_sz * r02, inv_sz * r12, inv_sz * r22, -inv_sz * (r02 * m_t.x + r12 * m_t.y + r22 * m_t.z));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
