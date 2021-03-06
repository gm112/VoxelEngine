// ===================================================================
//	Copyright (C) 2013 Tim Leonard
// ===================================================================
#ifndef _GENERIC_MATRIX4_
#define _GENERIC_MATRIX4_

#include "Vector3.h"

// Matrix's are alwyas in column-major order.
// Matrix's should be considered immutable (though you can use SetColumn etc to initialize)
struct Matrix4
{
public:
	float Elements[16];

	// Constructors.
	Matrix4();
	Matrix4(float c11, float c12, float c13, float c14,
			float c21, float c22, float c23, float c24,
			float c31, float c32, float c33, float c34,
			float c41, float c42, float c43, float c44);
	
	// Operator overloads.
	Matrix4 operator* (const Matrix4 a) const;

	// Column stuff.
	void SetColumn(int column, float e1, float e2, float e3, float e4);
	void SetRow(int column, float e1, float e2, float e3, float e4);

	// Perspective functions.
	static Matrix4 Identity	  ();
	static Matrix4 Frustum	  (float l, float r, float b, float t, float n, float f);
	static Matrix4 Perspective(float fov, float aspect_ratio, float z_near, float z_far);
	static Matrix4 LookAt	  (Vector3 eye, Vector3 center, Vector3 up);

};

#endif