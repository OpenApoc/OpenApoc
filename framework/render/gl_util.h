#pragma once

#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>
#include <string>
#include <memory>
#include <iostream>
#include "library/vec.h"

namespace OpenApoc
{
namespace GL
{

class IdentityQuad
{
public:
	static GLuint vbo;
	static GLuint vao;

	static void setup_vbo()
	{
		static const float vertices[] =
		{
			0.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
		};
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenVertexArrays(1, &vao);

	}

	static void draw(GLint positionAttribute)
	{
		if (!vbo)
			setup_vbo();

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(positionAttribute);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(positionAttribute);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
};

GLuint IdentityQuad::vbo = 0;
GLuint IdentityQuad::vao = 0;

static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	std::cerr << "DEBUG MESSAGE: \"" << std::string(message, length) << "\"\n";
	assert(0);
}

class RGBSpriteProgram
{
	const std::string RGBSprite_vertexSource{
		"#version 120\n"
		"attribute vec2 position;\n"
		"varying vec2 texcoord;\n"
		"uniform vec2 size;\n"
		"uniform vec2 offset;\n"
		"uniform vec2 screenSize;\n"
		"void main() {\n"
		"  texcoord = position;\n"
		"  vec2 tmpPos = position;\n"
		"  tmpPos *= size;\n"
		"  tmpPos += offset;\n"
		"  tmpPos /= screenSize;\n"
		"  tmpPos -= vec2(0.5,0.5);\n"
		"  gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
		"}\n"
	};

	const std::string RGBSprite_fragmentSource{
		"#version 120\n"
		"varying vec2 texcoord;\n"
		"uniform sampler2D tex;\n"
		"void main() {\n"
		"  gl_FragColor = texture2D(tex, texcoord);\n"
		"}\n"
	};
	GLuint programID;
	GLint offsetLoc;
	GLint sizeLoc;
	GLint screenSizeLoc;
	GLint texLoc;
	RGBSpriteProgram()
	{
		GLuint vShader, fShader;
		GLint status;

		std::cerr << "Setting up debug message\n";
		glDebugMessageCallback((GLDEBUGPROC)debugCallback, NULL);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glEnable(GL_DEBUG_OUTPUT);

		vShader = glCreateShader(GL_VERTEX_SHADER);
		const char *shaderSource = RGBSprite_vertexSource.c_str();
		GLint shaderLength  = RGBSprite_vertexSource.length();
		glShaderSource(vShader, 1, &shaderSource, &shaderLength);
		glCompileShader(vShader);
		glGetShaderiv(vShader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			std::cerr << "vShader compile failed:\n";
			glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &status);
			char *log = new char[status];
			glGetShaderInfoLog(vShader, status, NULL, log);
			std::cerr << log << "\n";
			delete[] log;
			assert(0);
		}
		else
			std::cerr << "vShader compile OK\n";

		fShader = glCreateShader(GL_FRAGMENT_SHADER);
		shaderSource = RGBSprite_fragmentSource.c_str();
		shaderLength  = RGBSprite_fragmentSource.length();
		glShaderSource(fShader, 1, &shaderSource, &shaderLength);
		glCompileShader(fShader);
		glGetShaderiv(fShader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			std::cerr << "fShader compile failed:\n";
			glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &status);
			char *log = new char[status];
			glGetShaderInfoLog(fShader, status, NULL, log);
			std::cerr << log << "\n";
			delete[] log;
			assert(0);
		}
		else
			std::cerr << "fShader compile OK\n";

		programID = glCreateProgram();
		glAttachShader(programID, vShader);
		glAttachShader(programID, fShader);
		glLinkProgram(programID);

		glDeleteShader(vShader);
		glDeleteShader(fShader);

		this->positionLoc = glGetAttribLocation(programID, "position");
		this->offsetLoc = glGetUniformLocation(programID, "offset");
		this->sizeLoc = glGetUniformLocation(programID, "size");
		this->screenSizeLoc = glGetUniformLocation(programID, "screenSize");
		this->texLoc = glGetUniformLocation(programID, "tex");

//		glBindFragDataLocation(programID, 0, "colour");

	}
	static std::weak_ptr<RGBSpriteProgram> p;
public:
	GLint positionLoc;
	static std::shared_ptr<RGBSpriteProgram> get()
	{
		std::shared_ptr<RGBSpriteProgram> ptr = p.lock();
		if (ptr)
			return ptr;
		ptr.reset(new RGBSpriteProgram());
		p = ptr;
		return ptr;
	}
	void enable(Vec2<int> offset, Vec2<int> size, Vec2<int> screenSize)
	{
		glUseProgram(this->programID);
		glUniform2f(this->offsetLoc, (float)offset.x, (float)offset.y);
		glUniform2f(this->sizeLoc, (float)size.x, (float)size.y);
		glUniform2f(this->screenSizeLoc, (float)screenSize.x, (float)screenSize.y);
		glUniform1i(this->texLoc, 0);
	}
};

class PaletteSpriteProgram
{
	const std::string PaletteSprite_vertexSource{
		"#version 120\n"
		"attribute vec2 position;\n"
		"varying vec2 texcoord;\n"
		"uniform vec2 size;\n"
		"uniform vec2 offset;\n"
		"uniform vec2 screenSize;\n"
		"void main() {\n"
		"  texcoord = position;\n"
		"  vec2 tmpPos = position;\n"
		"  tmpPos *= size;\n"
		"  tmpPos += offset;\n"
		"  tmpPos /= screenSize;\n"
		"  tmpPos -= vec2(0.5,0.5);\n"
		"  gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
		"}\n"
	};

	const std::string PaletteSprite_fragmentSource{
		"#version 120\n"
		"varying vec2 texcoord;\n"
		"uniform sampler2D tex;\n"
		"uniform sampler2DRect pal;\n"
		"void main() {\n"
		"  gl_FragColor = texture2DRect(pal, vec2(texture2D(tex, texcoord).r * 255, 0));\n"
		"}\n"
	};
	GLuint programID;
	GLint offsetLoc;
	GLint sizeLoc;
	GLint screenSizeLoc;
	GLint texLoc;
	GLint paletteLoc;
	PaletteSpriteProgram()
	{
		GLuint vShader, fShader;
		GLint status;

		std::cerr << "Setting up debug message\n";
		glDebugMessageCallback((GLDEBUGPROC)debugCallback, NULL);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glEnable(GL_DEBUG_OUTPUT);

		vShader = glCreateShader(GL_VERTEX_SHADER);
		const char *shaderSource = PaletteSprite_vertexSource.c_str();
		GLint shaderLength  = PaletteSprite_vertexSource.length();
		glShaderSource(vShader, 1, &shaderSource, &shaderLength);
		glCompileShader(vShader);
		glGetShaderiv(vShader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			std::cerr << "vShader compile failed:\n";
			glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &status);
			char *log = new char[status];
			glGetShaderInfoLog(vShader, status, NULL, log);
			std::cerr << log << "\n";
			delete[] log;
			assert(0);
		}
		else
			std::cerr << "vShader compile OK\n";

		fShader = glCreateShader(GL_FRAGMENT_SHADER);
		shaderSource = PaletteSprite_fragmentSource.c_str();
		shaderLength  = PaletteSprite_fragmentSource.length();
		glShaderSource(fShader, 1, &shaderSource, &shaderLength);
		glCompileShader(fShader);
		glGetShaderiv(fShader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			std::cerr << "fShader compile failed:\n";
			glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &status);
			char *log = new char[status];
			glGetShaderInfoLog(fShader, status, NULL, log);
			std::cerr << log << "\n";
			delete[] log;
			assert(0);
		}
		else
			std::cerr << "fShader compile OK\n";

		programID = glCreateProgram();
		glAttachShader(programID, vShader);
		glAttachShader(programID, fShader);
		glLinkProgram(programID);

		glDeleteShader(vShader);
		glDeleteShader(fShader);

		this->positionLoc = glGetAttribLocation(programID, "position");
		this->offsetLoc = glGetUniformLocation(programID, "offset");
		this->sizeLoc = glGetUniformLocation(programID, "size");
		this->screenSizeLoc = glGetUniformLocation(programID, "screenSize");
		this->texLoc = glGetUniformLocation(programID, "tex");
		this->paletteLoc = glGetUniformLocation(programID, "pal");

	//		glBindFragDataLocation(programID, 0, "colour");

	}
	static std::weak_ptr<PaletteSpriteProgram> p;
	public:
	GLint positionLoc;
	static std::shared_ptr<PaletteSpriteProgram> get()
	{
		std::shared_ptr<PaletteSpriteProgram> ptr = p.lock();
		if (ptr)
			return ptr;
		ptr.reset(new PaletteSpriteProgram());
		p = ptr;
		return ptr;
	}
	void enable(Vec2<int> offset, Vec2<int> size, Vec2<int> screenSize)
	{
		glUseProgram(this->programID);
		glUniform2f(this->offsetLoc, (float)offset.x, (float)offset.y);
		glUniform2f(this->sizeLoc, (float)size.x, (float)size.y);
		glUniform2f(this->screenSizeLoc, (float)screenSize.x, (float)screenSize.y);
		glUniform1i(this->texLoc, 0);
		glUniform1i(this->paletteLoc, 1);
	}
};

std::weak_ptr<RGBSpriteProgram> RGBSpriteProgram::p;
std::weak_ptr<PaletteSpriteProgram> PaletteSpriteProgram::p;

class ActiveTexture
{
private:
	GLint prevTexUnit;
	static GLenum getTexUnit(int idx)
	{
		return GL_TEXTURE0 + idx;
	}
public:
	ActiveTexture(GLuint unit)
	{
		glGetIntegerv(GL_ACTIVE_TEXTURE, &prevTexUnit);
		glActiveTexture(getTexUnit(unit));
	}
	~ActiveTexture()
	{
		glActiveTexture(prevTexUnit);
	}
};

class BindTexture
{
private:
	GLint prevTexId;
	GLuint unit;
public:
	BindTexture(GLuint texID, GLuint unit = 0)
		:unit(unit)
	{
		ActiveTexture a(unit);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexId);
		glBindTexture(GL_TEXTURE_2D, texID);
	}
	~BindTexture()
	{
		ActiveTexture a(unit);
		glBindTexture(GL_TEXTURE_2D, prevTexId);
	}
};

class BindTextureRect
{
private:
	GLint prevTexId;
	GLuint unit;
public:
	BindTextureRect(GLuint texID, GLuint unit = 0)
		:unit(unit)
	{
		ActiveTexture a(unit);
		glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE, &prevTexId);
		glBindTexture(GL_TEXTURE_RECTANGLE, texID);
	}
	~BindTextureRect()
	{
		ActiveTexture a(unit);
		glBindTexture(GL_TEXTURE_RECTANGLE, prevTexId);
	}
};

class BindFramebuffer
{
private:
	GLint prevFboId;
public:
	BindFramebuffer(GLuint fboId)
	{
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFboId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboId);
	}
	~BindFramebuffer()
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFboId);
	}
};

}; //namespace OpenApoc::GL
}; //namespace OpenApoc
