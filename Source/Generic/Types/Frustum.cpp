// ===================================================================
//	Copyright (C) 2013 Tim Leonard
// ===================================================================
#include "Generic\Types\Frustum.h"
#include "Generic\Types\Plane.h"
#include "Generic\Math\Math.h"

Frustum::Frustum()
{

}

Frustum::Frustum(float angle,
		float aspect_ratio,
		float near,
		float far,
		const Vector3& pos,
		const Vector3& target,
		const Vector3& up)
{
	float angle_to_rad = 3.14159265358979323846f / 180.0f;
	Vector3 dir, nc, fc, x, y, z;

	// store the information
	m_aspect_ratio		= aspect_ratio;
	m_angle				= angle;
	m_near_distance		= near;
	m_far_distance		= far;

	// compute width and height of the near and far plane sections
	m_tangent		= (float)tan(angle_to_rad * angle * 0.5);
	m_near_height	= m_near_distance * m_tangent;
	m_near_width	= m_near_height * m_aspect_ratio; 
	m_far_height	= m_far_distance  * m_tangent;
	m_far_width		= m_far_height * m_aspect_ratio;

	// compute the Z axis of camera
	// this axis points in the opposite direction from 
	// the looking direction
	z = pos - target;
	z = z.Normalize();

	// X axis of camera with given "up" vector and Z axis
	x = up.Cross(z);
	x = x.Normalize();

	// the real "up" vector is the cross product of Z and X
	y = z.Cross(x);

	// compute the centers of the near and far planes
	nc = pos - z * m_near_distance;
	fc = pos - z * m_far_distance;

	// compute the 4 corners of the frustum on the near plane
	m_near_top_left		= nc + y * m_near_height - x * m_near_width;
	m_near_top_right	= nc + y * m_near_height + x * m_near_width;
	m_near_bottom_left	= nc - y * m_near_height - x * m_near_width;
	m_near_bottom_right = nc - y * m_near_height + x * m_near_width;

	// compute the 4 corners of the frustum on the far plane
	m_far_top_left		= fc + y * m_far_height - x * m_far_width;
	m_far_top_right		= fc + y * m_far_height + x * m_far_width;
	m_far_bottom_left	= fc - y * m_far_height - x * m_far_width;
	m_far_bottom_right	= fc - y * m_far_height + x * m_far_width;

	// compute the six planes
	// the function set3Points assumes that the points
	// are given in counter clockwise order
	m_planes[Planes::Top]		= Plane(m_near_top_right,		m_near_top_left,		m_far_top_left);
	m_planes[Planes::Bottom]	= Plane(m_near_bottom_left,		m_near_bottom_right,	m_far_bottom_right);
	m_planes[Planes::Left]		= Plane(m_near_top_left,		m_near_bottom_left,		m_far_bottom_left);
	m_planes[Planes::Right]		= Plane(m_near_bottom_right,	m_near_top_right,		m_far_bottom_right);
	m_planes[Planes::Near]		= Plane(m_near_top_left,		m_near_top_right,		m_near_bottom_right);
	m_planes[Planes::Far]		= Plane(m_far_top_right,		m_far_top_left,			m_far_bottom_left);
}

Frustum::IntersectionResult::Type Frustum::Intersects(const AABB& aabb)
{
    IntersectionResult::Type result = IntersectionResult::Inside;

	for (int i = 0; i < 6; i++)
	{
		int out = 0, in = 0;

		for (int k = 0; k < 8 && (in == 0 || out == 0); k++)
		{
			if (m_planes[i].Get_Distance_To_Point(aabb.Vertices[k]) < 0)
			{
				out++;
			}
			else
			{
				in++;
			}
		}

		if (!in)
		{
			return IntersectionResult::Outside;
		}
		else if (out)
		{
			result = IntersectionResult::Intersects;
		}
	}

	return result;
}

Frustum::IntersectionResult::Type Frustum::Intersects(const Sphere& sphere)
{
	Frustum::IntersectionResult::Type result = IntersectionResult::Inside;
	float distance = 0;

	for (int i = 0; i < 6; i++)
	{
		distance = m_planes[i].Get_Distance_To_Point(sphere.Center);

		if (distance < -sphere.Radius)
		{
			return IntersectionResult::Outside;
		}

		if (distance < sphere.Radius)
		{
			result = IntersectionResult::Intersects;
		}
	}

	return result;
}
