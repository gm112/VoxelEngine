// ===================================================================
//	Copyright (C) 2013 Tim Leonard
// ===================================================================
#ifndef _ENGINE_RENDERER_OPENGL_TEXTURE_
#define _ENGINE_RENDERER_OPENGL_TEXTURE_

#include "Engine\Renderer\Textures\Texture.h"

#include "Engine\Renderer\OpenGL\OpenGL_Renderer.h"

class OpenGL_Texture : public Texture
{
private:
	friend class OpenGL_Renderer;

	GLuint m_texture_id;

private:
	// Constructor!
	OpenGL_Texture(GLuint texture_id, char* data, int width, int height, int pitch, TextureFormat::Type format);

	GLuint Get_ID();

public:

	// Destructor!
	~OpenGL_Texture();

};

#endif

