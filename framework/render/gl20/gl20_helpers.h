#pragma once

#include "gl20.h"
#include "framework/logger.h"

namespace OpenApoc
{

class ActiveTexture
{
	ActiveTexture(const ActiveTexture &) = delete;
	const GL20 &gl;

public:
	GL20::GLenum prevUnit;
	GL20::GLenum getUnitEnum(int unit)
	{
		return GL20::TEXTURE0 + unit;
	}

	ActiveTexture(const GL20 &gl, int unit)
		: gl(gl)
	{
		gl.GetIntegerv(GL20::ACTIVE_TEXTURE, (GL20::GLint*)&prevUnit);
		gl.ActiveTexture(getUnitEnum(unit));
	}
	~ActiveTexture()
	{
		gl.ActiveTexture(prevUnit);
	}
};

class UnpackAlignment
{
	UnpackAlignment(const UnpackAlignment &) = delete;
	const GL20 &gl;
public:
	GL20::GLint prevAlign;
	UnpackAlignment(const GL20 &gl, int align)
		: gl(gl)
	{
		gl.GetIntegerv(GL20::UNPACK_ALIGNMENT, &prevAlign);
		gl.PixelStorei(GL20::UNPACK_ALIGNMENT, align);
	}
	~UnpackAlignment()
	{
		gl.PixelStorei(GL20::UNPACK_ALIGNMENT, prevAlign);
	}
};

class BindTexture
{
	BindTexture(const BindTexture &) = delete;
	const GL20 &gl;
public:
	GL20::GLenum bind;
	GL20::GLuint prevID;
	int unit;
	static GL20::GLenum getBindEnum(GL20::GLenum e)
	{
		switch (e) {
			case GL20::TEXTURE_1D: return GL20::TEXTURE_BINDING_1D;
			case GL20::TEXTURE_2D: return GL20::TEXTURE_BINDING_2D;
			case GL20::TEXTURE_2D_ARRAY: return GL20::TEXTURE_BINDING_2D_ARRAY;
			default:
				LogError("Unknown texture enum %d", (int)e);
				return GL20::TEXTURE_BINDING_2D;
		}
	}
	BindTexture(const GL20 &gl, GL20::GLuint id, GL20::GLint unit = 0, GL20::GLenum bind = GL20::TEXTURE_2D)
		: gl(gl), bind(bind), unit(unit) 
	{
		ActiveTexture a(gl, unit);
		gl.GetIntegerv(getBindEnum(bind), (GL20::GLint*)&prevID);
		gl.BindTexture(bind, id);
	}
	~BindTexture()
	{
		ActiveTexture a(gl, unit);
		gl.BindTexture(bind, prevID);
	}
};

template <GL20::GLenum param>
class TexParam
{
	TexParam(const TexParam&) = delete;
	const GL20 &gl;
public:
	GL20::GLint prevValue;
	GL20::GLuint id;
	GL20::GLenum type;

	TexParam(const GL20 &gl, GL20::GLuint id, GL20::GLint value, GL20::GLenum type = GL20::TEXTURE_2D)
		: gl(gl), id(id), type(type)
	{
		BindTexture b(gl, id, 0, type);
		gl.GetTexParameteriv(type, param, &prevValue);
		gl.TexParameteri(type, param, value);
	}
	~TexParam()
	{
		BindTexture b(gl, id, 0, type);
		gl.TexParameteri(type, param, prevValue);
	}
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;
	const GL20 &gl;
public:
	GL20::GLuint prevID;
	BindFramebuffer(const GL20 &gl, GL20::GLuint id)
		: gl(gl)
	{
		gl.GetIntegerv(GL20::DRAW_FRAMEBUFFER_BINDING, (GL20::GLint*)&prevID);
		gl.BindFramebuffer(GL20::DRAW_FRAMEBUFFER, id);

	}
	~BindFramebuffer()
	{
		gl.BindFramebuffer(GL20::DRAW_FRAMEBUFFER, prevID);
	}
};

class Shader
{
private:
	Shader(const Shader&) = delete;
	const GL20 &gl;
public:
	GL20::GLuint id;
	UString name;

	Shader(const GL20 &gl, GL20::GLenum type, const UString &source)
		: gl(gl), id(0)
	{
		switch (type)
		{
			case GL20::VERTEX_SHADER:
				name = "vertex";
				break;
			case GL20::FRAGMENT_SHADER:
				name = "fragment";
				break;
			default:
				LogError("Invalid shader type 0x%04x", type);
				name = "invalid";
				return;
		}
		id = gl.CreateShader(type);
		if (!id)
		{
			LogError("Failed to create %s shader", name.str().c_str());
			return;
		}

		auto sourceString = source.str();
		const GL20::GLchar *string = sourceString.c_str();
		GL20::GLint stringLength = sourceString.length();
		gl.ShaderSource(id, 1, &string, &stringLength);
		gl.CompileShader(id);
		GL20::GLint status;
		gl.GetShaderiv(id, GL20::COMPILE_STATUS, &status);
		if (status == GL20::_TRUE)
		{
			//success
			return;
		}

		GL20::GLint logLength;
		gl.GetShaderiv(id, GL20::INFO_LOG_LENGTH, &logLength);
		std::unique_ptr<char[]> logBuf(new char[logLength]);
		gl.GetShaderInfoLog(id, logLength, NULL, logBuf.get());
		LogError("Shader compile error: \"%s\"", logBuf.get());
		gl.DeleteShader(id);
		id = 0;
		return;
	}

	explicit operator bool() const
	{
		return id != 0;
	}

	~Shader()
	{
		if (id)
			gl.DeleteShader(id);
	}
};

class Program
{
private:
	Program(const Program&) = delete;
	const GL20 &gl;
public:
	GL20::GLuint id;
	Program(const GL20 &gl, const UString &vSource, const UString &fSource)
		: gl(gl), id(0)
	{
		Shader vShader(gl, GL20::VERTEX_SHADER, vSource);
		if (!vShader)
			return;
		Shader fShader(gl, GL20::FRAGMENT_SHADER, fSource);
		if (!fShader)
			return;

		id = gl.CreateProgram();
		if (!id)
		{
			LogError("Failed to create program");
			return;
		}

		gl.AttachShader(id, vShader.id);
		gl.AttachShader(id, fShader.id);

		gl.LinkProgram(id);

		GL20::GLint status;
		gl.GetProgramiv(id, GL20::LINK_STATUS, &status);
		if (status == GL20::_TRUE)
		{
			//Success`
			return;
		}
		GL20::GLint logLength;
		gl.GetProgramiv(id, GL20::INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		gl.GetProgramInfoLog(id, logLength, NULL, log.get());

		LogError("Program link error: %s", log.get());

		gl.DeleteProgram(id);
		id = 0;

	}
	explicit operator bool() const
	{
		return id != 0;
	}
	~Program()
	{
		if (id)
			gl.DeleteProgram(id);
	}
	std::map<std::string, GL20::GLint> uniformLocations;
	GL20::GLint uniformLoc(const std::string &name)
	{
		auto it = uniformLocations.find(name);
		if (it != uniformLocations.end())
			return it->second;
		GL20::GLint loc = gl.GetUniformLocation(this->id, name.c_str());
		uniformLocations.emplace(name, loc);
		return loc;

	}
	std::map<std::string, GL20::GLint> attribLocations;
	GL20::GLint attribLoc(const std::string &name)
	{
		auto it = attribLocations.find(name);
		if (it != attribLocations.end())
			return it->second;
		GL20::GLint loc = gl.GetAttribLocation(this->id, name.c_str());
		attribLocations.emplace(name, loc);
		return loc;
	}
	void Uniform(const std::string &name, GL20::GLfloat val)
	{
		gl.Uniform1f(this->uniformLoc(name), val);
	}
	void Uniform(const std::string &name, const Vec2<float> &val)
	{
		gl.Uniform2f(this->uniformLoc(name), val.x, val.y);
	}
	void Uniform(const std::string &name, int val)
	{
		gl.Uniform1i(this->uniformLoc(name), val);
	}
};

};
