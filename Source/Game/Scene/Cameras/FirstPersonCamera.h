// ===================================================================
//	Copyright (C) 2013 Tim Leonard
// ===================================================================
#ifndef _ENGINE_CAMERA_FIRST_PERSON_CAMERA_
#define _ENGINE_CAMERA_FIRST_PERSON_CAMERA_

#include "Generic\Types\Rectangle.h"

#include "Engine\Scene\Camera.h"

class FirstPersonCamera : public Camera
{
private:
	float m_sensitivity_x;
	float m_sensitivity_y;
	float m_speed_x;
	float m_speed_z;
	bool  m_first_run;

public:
	FirstPersonCamera(float fov, Rect rect);
	
	// Base functions.
	void Tick(const FrameTime& time);

};

#endif

