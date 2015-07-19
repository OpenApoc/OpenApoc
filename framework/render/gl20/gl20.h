#pragma once

/* A cut down version of the GL2.0 api that we actually use, all dynamically loaded using
 * GetProcAddress*/
#include <functional>
#include <cstdint>
#include <memory>
#include <map>
#include <set>

#include "library/strings.h"

namespace OpenApoc
{

class GLLib;
class GL20
{
private:
	std::unique_ptr<GLLib> lib;
public:
	std::vector<int> version;

	UString vendorString;
	UString rendererString;
	UString versionString;
	UString extensionsString;

	typedef float GLfloat;
	typedef unsigned int GLbitfield;
	typedef unsigned char GLubyte;
	typedef unsigned int GLenum;
	typedef int GLint;
	typedef unsigned int GLuint;
	typedef char GLchar;
	typedef int GLsizei;
	typedef unsigned char GLboolean;

	enum GLEnumValues
	{
		FALSE = 0x0000,
		TRUE = 0x0001,
		UNPACK_ALIGNMENT = 0x0CF5,
		TEXTURE_1D = 0x0DE0,
		TEXTURE_2D = 0x0DE1,
		TEXTURE_2D_ARRAY = 0x8C1A, /* GL_EXT_texture_array */
		TEXTURE_BINDING_1D = 0x8068,
		TEXTURE_BINDING_2D = 0x8069,
		TEXTURE_BINDING_2D_ARRAY = 0x8C1D, /* GL_EXT_texture_array */
		VENDOR = 0x1f00,
		RENDERER = 0x1f01,
		VERSION = 0x1f02,
		EXTENSIONS = 0x1f03,
		TEXTURE0 = 0x84C0,
		TEXTURE1 = 0x84C1,
		TEXTURE2 = 0x84C2,
		TEXTURE3 = 0x84C3,
		TEXTURE4 = 0x84C4,
		TEXTURE5 = 0x84C5,
		TEXTURE6 = 0x84C6,
		TEXTURE7 = 0x84C7,
		TEXTURE8 = 0x84C8,
		TEXTURE9 = 0x84C9,
		TEXTURE10 = 0x84CA,
		TEXTURE11 = 0x84CB,
		TEXTURE12 = 0x84CC,
		TEXTURE13 = 0x84CD,
		TEXTURE14 = 0x84CE,
		TEXTURE15 = 0x84CF,
		TEXTURE16 = 0x84D0,
		TEXTURE17 = 0x84D1,
		TEXTURE18 = 0x84D2,
		TEXTURE19 = 0x84D3,
		TEXTURE20 = 0x84D4,
		TEXTURE21 = 0x84D5,
		TEXTURE22 = 0x84D6,
		TEXTURE23 = 0x84D7,
		TEXTURE24 = 0x84D8,
		TEXTURE25 = 0x84D9,
		TEXTURE26 = 0x84DA,
		TEXTURE27 = 0x84DB,
		TEXTURE28 = 0x84DC,
		TEXTURE29 = 0x84DD,
		TEXTURE30 = 0x84DE,
		TEXTURE31 = 0x84DF,
		ACTIVE_TEXTURE = 0x84E0,
		FRAMEBUFFER = 0x8D40,
		DRAW_FRAMEBUFFER = 0x8CA9, /* GL_ARB_framebuffer_object */
		DRAW_FRAMEBUFFER_BINDING = 0x8CA6, /* GL_ARB_framebuffer_object */
		FRAGMENT_SHADER = 0x8B30,
		VERTEX_SHADER = 0x8B31,
		COMPILE_STATUS = 0x8B81,
		LINK_STATUS = 0x8B82,
		INFO_LOG_LENGTH = 0x8B84,
		RGBA8 = 0x8058,
		RGBA = 0x1908,
		UNSIGNED_BYTE = 0x1401,
		TEXTURE_MAG_FILTER = 0x2800,
		TEXTURE_MIN_FILTER = 0x2801,
		NEAREST = 0x2600,
		LINEAR = 0x2601,
		COLOR_ATTACHMENT0 = 0x8CE0,
		FRAMEBUFFER_COMPLETE = 0x8CD5,
		VIEWPORT = 0x0BA2,
		COLOR_BUFFER_BIT = 0x00004000,
		FLOAT = 0x1406,
		TRIANGLE_STRIP = 0x0005,
		BLEND = 0x0BE2,
		SRC_ALPHA = 0x0302,
		ONE_MINUS_SRC_ALPHA = 0x0303,
	};

	GL20();
	~GL20();
	bool loadedSuccessfully;


	std::map<UString, bool> loadedExtensions;

	std::set<UString> driverExtensions;

	std::function<void(GLuint, GLuint)>AttachShader;
	std::function<void(GLenum)>ActiveTexture;
	std::function<void(GLenum, GLuint)>BindFramebuffer;
	std::function<void(GLenum, GLuint)>BindTexture;
	std::function<void(GLenum, GLenum)>BlendFunc;
	std::function<GLenum(GLenum)>CheckFramebufferStatus;
	std::function<void(GLenum)>Clear;
	std::function<void(GLfloat, GLfloat, GLfloat, GLfloat)> ClearColor;
	std::function<void(GLuint)> CompileShader;
	std::function<GLuint(void)> CreateProgram;
	std::function<GLuint(GLenum)> CreateShader;
	std::function<void(GLsizei, const GLuint*)> DeleteFramebuffers;
	std::function<void(GLuint)> DeleteProgram;
	std::function<void(GLuint)> DeleteShader;
	std::function<void(GLsizei, const GLuint*)> DeleteTextures;
	std::function<void(GLenum, GLint, GLsizei)> DrawArrays;
	std::function<void(GLenum)> Enable;
	std::function<void(GLint)> EnableVertexAttribArray;
	std::function<void(GLenum, GLenum, GLenum, GLuint, GLint)> FramebufferTexture2D;
	std::function<void(GLsizei, GLuint*)> GenFramebuffers;
	std::function<void(GLsizei, GLuint*)> GenTextures;
	std::function<GLint(GLuint, const GLchar*)> GetAttribLocation;
	std::function<void(GLenum, GLint*)> GetIntegerv;
	std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> GetProgramInfoLog;
	std::function<void(GLuint, GLenum, GLint*)> GetProgramiv;
	std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> GetShaderInfoLog;
	std::function<void(GLuint, GLenum, GLint*)> GetShaderiv;
	std::function<const GLubyte*(GLenum)> GetString;
	std::function<void(GLenum, GLenum, GLint*)> GetTexParameteriv;
	std::function<GLint(GLuint, const GLchar*)> GetUniformLocation;
	std::function<void(GLuint)> LinkProgram;
	std::function<void(GLenum, GLint)> PixelStorei;
	std::function<void(GLuint, GLsizei, const GLchar *const*, const GLint*)> ShaderSource;
	std::function<void(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)> TexImage2D;
	std::function<void(GLenum, GLenum, GLint)> TexParameteri;
	std::function<void(GLint, GLint)> Uniform1i;
	std::function<void(GLint, GLfloat)> Uniform1f;
	std::function<void(GLint, GLfloat, GLfloat)> Uniform2f;
	std::function<void(GLuint)> UseProgram;
	std::function<void(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)> VertexAttribPointer;
	std::function<void(GLint, GLint, GLsizei, GLsizei)> Viewport;

};

}; //namespace OpenApoc
