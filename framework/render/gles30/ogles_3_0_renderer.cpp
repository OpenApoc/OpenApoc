#include "library/sp.h"
#include "framework/renderer_interface.h"
#include "framework/logger.h"
#include "framework/image.h"
#include "framework/palette.h"
#include "framework/trace.h"
#include <memory>
#include <array>

#include <string>

namespace
{
#include <GLES3/gl3.h>
} // anonymous namespace

namespace
{

using namespace OpenApoc;

class Program
{
  public:
	GLuint prog;
	static GLuint CreateShader(GLenum type, const UString source)
	{
		GLuint shader = glCreateShader(type);
		auto sourceString = source.str();
		const GLchar *string = sourceString.c_str();
		GLint stringLength = sourceString.length();
		glShaderSource(shader, 1, &string, &stringLength);
		glCompileShader(shader);
		GLint compileStatus;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
		if (compileStatus == GL_TRUE)
			return shader;

		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		glGetShaderInfoLog(shader, logLength, NULL, log.get());

		LogError("Shader compile error: %s", log.get());

		glDeleteShader(shader);
		return 0;
	}
	Program(const UString vertexSource, const UString fragmentSource) : prog(0)
	{
		GLuint vShader = CreateShader(GL_VERTEX_SHADER, vertexSource);
		if (!vShader)
		{
			LogError("Failed to compile vertex shader");
			return;
		}
		GLuint fShader = CreateShader(GL_FRAGMENT_SHADER, fragmentSource);
		if (!fShader)
		{
			LogError("Failed to compile fragment shader");
			glDeleteShader(vShader);
			return;
		}

		prog = glCreateProgram();
		glAttachShader(prog, vShader);
		glAttachShader(prog, fShader);

		glDeleteShader(vShader);
		glDeleteShader(fShader);

		glLinkProgram(prog);

		GLint linkStatus;
		glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == GL_TRUE)
			return;

		GLint logLength;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);

		std::unique_ptr<char[]> log(new char[logLength]);
		glGetProgramInfoLog(prog, logLength, NULL, log.get());

		LogError("Program link error: %s", log.get());

		glDeleteProgram(prog);
		prog = 0;
		return;
	}

	void Uniform(GLuint loc, Colour c)
	{
		glUniform4f(loc, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}

	void Uniform(GLuint loc, Vec2<float> v) { glUniform2f(loc, v.x, v.y); }
	void Uniform(GLuint loc, Vec2<int> v)
	{
		// FIXME: Float conversion
		glUniform2f(loc, v.x, v.y);
	}
	void Uniform(GLuint loc, float v) { glUniform1f(loc, v); }
	void Uniform(GLuint loc, int v) { glUniform1i(loc, v); }

	void Uniform(GLuint loc, bool v) { glUniform1f(loc, (v ? 1.0f : 0.0f)); }

	virtual ~Program()
	{
		if (prog)
			glDeleteProgram(prog);
	}
};

class SpriteProgram : public Program
{
  protected:
	SpriteProgram(const UString vertexSource, const UString fragmentSource)
	    : Program(vertexSource, fragmentSource)
	{
	}

  public:
	GLint posLoc;
	GLint texcoordLoc;
	GLint screenSizeLoc;
	GLint texLoc;
	GLint flipYLoc;
};
const char *RGBProgram_vertexSource = {
    "#version 300 es\n"
    "in vec2 position;\n"
    "in vec2 texcoord_in;\n"
    "out vec2 texcoord;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  texcoord = texcoord_in;\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2.0), -(tmpPos.y*2.0),0.0,1.0);\n"
    "  else gl_Position = vec4((tmpPos.x*2.0), (tmpPos.y*2.0),0.0,1.0);\n"
    "}\n"};
const char *RGBProgram_fragmentSource = {"#version 300 es\n"
                                         "precision mediump float;\n"
                                         "in vec2 texcoord;\n"
                                         "uniform sampler2D tex;\n"
                                         "out vec4 out_colour;\n"
                                         "void main() {\n"
                                         " out_colour = texture(tex, texcoord);\n"
                                         "}\n"};
class RGBProgram : public SpriteProgram
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	GLint currentTexUnit;

  public:
	RGBProgram()
	    : SpriteProgram(RGBProgram_vertexSource, RGBProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(0), currentTexUnit(0)
	{
		this->posLoc = glGetAttribLocation(this->prog, "position");
		if (this->posLoc < 0)
			LogError("\"position\" attribute not found in shader");
		this->texcoordLoc = glGetAttribLocation(this->prog, "texcoord_in");
		if (this->texcoordLoc < 0)
			LogError("\"texcoord_in\" attribute not found in shader");
		this->screenSizeLoc = glGetUniformLocation(this->prog, "screenSize");
		if (this->screenSizeLoc < 0)
			LogError("\"screenSize\" uniform not found in shader");
		this->texLoc = glGetUniformLocation(this->prog, "tex");
		if (this->texLoc < 0)
			LogError("\"tex\" uniform not found in shader");
		this->flipYLoc = glGetUniformLocation(this->prog, "flipY");
		if (this->flipYLoc < 0)
			LogError("\"flipY\" uniform not found in shader");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0)
	{
		if (screenSize != currentScreenSize)
		{
			currentScreenSize = screenSize;
			this->Uniform(this->screenSizeLoc, screenSize);
		}
		if (texUnit != currentTexUnit)
		{
			currentTexUnit = texUnit;
			this->Uniform(this->texLoc, texUnit);
		}
		if (flipY != currentFlipY)
		{
			currentFlipY = flipY;
			this->Uniform(this->flipYLoc, flipY);
		}
	}
};
const char *PaletteProgram_vertexSource = {
    "#version 300 es\n"
    "in vec2 position;\n"
    "in vec2 texcoord_in;\n"
    "out vec2 texcoord;\n"
    "flat out int sprite;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  texcoord = texcoord_in;\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2.0), -(tmpPos.y*2.0),0.0,1.0);\n"
    "  else gl_Position = vec4((tmpPos.x*2.0), (tmpPos.y*2.0),0.0,1.0);\n"
    "}\n"};
const char *PaletteProgram_fragmentSource = {
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 texcoord;\n"
    "uniform isampler2D tex;\n"
    "uniform sampler2D pal;\n"
    "out vec4 out_colour;\n"
    "void main() {\n"
    " int idx = texelFetch(tex, ivec2(texcoord.x, texcoord.y),0).r;\n"
    " if (idx == 0) discard;\n"
    " out_colour = texelFetch(pal, ivec2(idx,0),0);\n"
    "}\n"};
class PaletteProgram : public SpriteProgram
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	GLint currentTexUnit;
	GLint currentPalUnit;

  public:
	GLint palLoc;
	PaletteProgram()
	    : SpriteProgram(PaletteProgram_vertexSource, PaletteProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(false), currentTexUnit(0), currentPalUnit(0)
	{
		this->posLoc = glGetAttribLocation(this->prog, "position");
		this->texcoordLoc = glGetAttribLocation(this->prog, "texcoord_in");
		this->screenSizeLoc = glGetUniformLocation(this->prog, "screenSize");
		this->texLoc = glGetUniformLocation(this->prog, "tex");
		this->palLoc = glGetUniformLocation(this->prog, "pal");
		this->flipYLoc = glGetUniformLocation(this->prog, "flipY");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0, GLint palUnit = 1)
	{
		if (screenSize != currentScreenSize)
		{
			currentScreenSize = screenSize;
			this->Uniform(this->screenSizeLoc, screenSize);
		}
		if (texUnit != currentTexUnit)
		{
			currentTexUnit = texUnit;
			this->Uniform(this->texLoc, texUnit);
		}
		if (palUnit != currentPalUnit)
		{
			currentPalUnit = palUnit;
			this->Uniform(this->palLoc, palUnit);
		}
		if (currentFlipY != flipY)
		{
			currentFlipY = flipY;
			this->Uniform(this->flipYLoc, flipY);
		}
	}
};

const char *PaletteSetProgram_vertexSource = {
    "#version 300 es\n"
    "in vec2 position;\n"
    "in vec2 texcoord_in;\n"
    "in int sprite_in;\n"
    "out vec2 texcoord;\n"
    "flat out int sprite;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  texcoord = texcoord_in;\n"
    "  sprite = sprite_in;\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2.0), -(tmpPos.y*2.0),0.0,1.0);\n"
    "  else gl_Position = vec4((tmpPos.x*2.0), (tmpPos.y*2.0),0.0,1.0);\n"
    "}\n"};
const char *PaletteSetProgram_fragmentSource = {
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 texcoord;\n"
    "flat in int sprite;\n"
    "uniform isampler2DArray tex;\n"
    "uniform sampler2D pal;\n"
    "out vec4 out_colour;\n"
    "void main() {\n"
    " int idx = texelFetch(tex, ivec3(texcoord.x, texcoord.y, sprite), 0).r;\n"
    " if (idx == 0) discard;\n"
    " out_colour = texelFetch(pal, ivec2(idx,0), 0);\n"
    "}\n"};
class PaletteSetProgram : public Program
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	GLint currentTexUnit;
	GLint currentPalUnit;

  public:
	GLuint posLoc;
	GLuint texcoordLoc;
	GLuint spriteLoc;
	GLuint screenSizeLoc;
	GLuint texLoc;
	GLuint palLoc;
	GLuint flipYLoc;
	PaletteSetProgram()
	    : Program(PaletteSetProgram_vertexSource, PaletteSetProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(false), currentTexUnit(0), currentPalUnit(0)
	{
		this->posLoc = glGetAttribLocation(this->prog, "position");
		this->texcoordLoc = glGetAttribLocation(this->prog, "texcoord_in");
		this->spriteLoc = glGetAttribLocation(this->prog, "sprite_in");

		this->screenSizeLoc = glGetUniformLocation(this->prog, "screenSize");
		this->texLoc = glGetUniformLocation(this->prog, "tex");
		this->palLoc = glGetUniformLocation(this->prog, "pal");
		this->flipYLoc = glGetUniformLocation(this->prog, "flipY");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, GLint texUnit = 0, GLint palUnit = 1)
	{
		if (screenSize != currentScreenSize)
		{
			currentScreenSize = screenSize;
			this->Uniform(this->screenSizeLoc, screenSize);
		}
		if (texUnit != currentTexUnit)
		{
			currentTexUnit = texUnit;
			this->Uniform(this->texLoc, texUnit);
		}
		if (palUnit != currentPalUnit)
		{
			currentPalUnit = palUnit;
			this->Uniform(this->palLoc, palUnit);
		}
		if (currentFlipY != flipY)
		{
			currentFlipY = flipY;
			this->Uniform(this->flipYLoc, flipY);
		}
	}
	class VertexDef
	{
	  public:
		VertexDef(Vec2<float> pos, Vec2<float> texcoords, int sprite)
		    : pos(pos), texcoords(texcoords), sprite(sprite)
		{
		}
		VertexDef() {}

		Vec2<float> pos;
		Vec2<float> texcoords;
		int sprite;
	};
	static_assert(sizeof(VertexDef) == 20, "VertexDef should be tightly packed");
	class SpriteDef
	{
	  public:
		SpriteDef(Vec2<float> offset, Vec2<float> size, int sprite)
		{
			v[0] = {offset, Vec2<float>{0, 0}, sprite};
			v[1] = {offset + size * Vec2<float>{0, 1}, Vec2<float>{0, 1}, sprite};
			v[2] = {offset + size * Vec2<float>{1, 0}, Vec2<float>{1, 0}, sprite};
			v[3] = {offset + size * Vec2<float>{1, 0}, Vec2<float>{1, 0}, sprite};
		}

		VertexDef v[4];
	};
	static_assert(sizeof(SpriteDef) == sizeof(VertexDef) * 4, "SpriteDef should be tightly packed");
};

const char *SolidColourProgram_vertexSource = {
    "#version 300 es\n"
    "in vec2 position;\n"
    "uniform vec2 screenSize;\n"
    "uniform bool flipY;\n"
    "void main() {\n"
    "  vec2 tmpPos = position;\n"
    "  tmpPos /= screenSize;\n"
    "  tmpPos -= vec2(0.5,0.5);\n"
    "  if (flipY) gl_Position = vec4((tmpPos.x*2.0), -(tmpPos.y*2.0),0.0,1.0);\n"
    "  else gl_Position = vec4((tmpPos.x*2.0), (tmpPos.y*2.0),0.0,1.0);\n"
    "}\n"};
const char *SolidColourProgram_fragmentSource = {"#version 300 es\n"
                                                 "precision mediump float;\n"
                                                 "uniform vec4 colour;\n"
                                                 "out vec4 out_colour;\n"
                                                 "void main() {\n"
                                                 " out_colour = colour;\n"
                                                 "}\n"};
class SolidColourProgram : public Program
{
  private:
	Vec2<int> currentScreenSize;
	bool currentFlipY;
	Colour currentColour;

  public:
	GLuint posLoc;
	GLuint screenSizeLoc;
	GLuint colourLoc;
	GLuint flipYLoc;
	SolidColourProgram()
	    : Program(SolidColourProgram_vertexSource, SolidColourProgram_fragmentSource),
	      currentScreenSize(0, 0), currentFlipY(false), currentColour(0, 0, 0, 0)
	{
		this->posLoc = glGetAttribLocation(this->prog, "position");
		this->screenSizeLoc = glGetUniformLocation(this->prog, "screenSize");
		this->colourLoc = glGetUniformLocation(this->prog, "colour");
		this->flipYLoc = glGetUniformLocation(this->prog, "flipY");
	}
	void setUniforms(Vec2<int> screenSize, bool flipY, Colour colour)
	{
		if (currentScreenSize != screenSize)
		{
			currentScreenSize = screenSize;
			this->Uniform(this->screenSizeLoc, screenSize);
		}
		if (currentColour != colour)
		{
			currentColour = colour;
			this->Uniform(this->colourLoc, colour);
		}
		if (currentFlipY != flipY)
		{
			currentFlipY = flipY;
			this->Uniform(this->flipYLoc, flipY);
		}
	}
};

class Quad
{
  public:
	std::array<Vec2<float>, 4> vertices;
	std::array<Vec2<float>, 4> texcoords;
	Quad(const Rect<float> &position, const Rect<float> texCoords,
	     const Vec2<float> &rotationCenter = {0.0f, 0.0f}, float rotationAngleRadians = 0.0f)
	{
		texcoords = {{
		    Vec2<float>{texCoords.p0}, Vec2<float>{texCoords.p1.x, texCoords.p0.y},
		    Vec2<float>{texCoords.p0.x, texCoords.p1.y}, Vec2<float>{texCoords.p1},
		}};

		if (rotationAngleRadians != 0.0f)
		{
			auto rotMatrix = glm::rotate(rotationAngleRadians, Vec3<float>{0.0f, 0.0f, 1.0f});
			Vec2<float> size = position.p1 - position.p0;
			vertices = {{
			    Vec2<float>{0.0f, 0.0f}, Vec2<float>{size.x, 0.0f}, Vec2<float>{0.0f, size.y},
			    Vec2<float>{size},
			}};
			for (auto &p : vertices)
			{
				p -= rotationCenter;
				glm::vec4 transformed = rotMatrix * glm::vec4{p.x, p.y, 0.0f, 1.0f};
				p.x = transformed.x;
				p.y = transformed.y;
				p += rotationCenter;
				p += position.p0;
			}
		}
		else
		{
			vertices = {{
			    Vec2<float>{position.p0}, Vec2<float>{position.p1.x, position.p0.y},
			    Vec2<float>{position.p0.x, position.p1.y}, Vec2<float>{position.p1},
			}};
		}
	}
	void draw(GLuint vertexAttribPos, GLuint texcoordAttribPos)
	{
		glEnableVertexAttribArray(vertexAttribPos);
		glVertexAttribPointer(vertexAttribPos, 2, GL_FLOAT, GL_FALSE, 0, &vertices);
		glEnableVertexAttribArray(texcoordAttribPos);
		glVertexAttribPointer(texcoordAttribPos, 2, GL_FLOAT, GL_FALSE, 0, &texcoords);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	void draw(GLuint vertexAttribPos)
	{
		glEnableVertexAttribArray(vertexAttribPos);
		glVertexAttribPointer(vertexAttribPos, 2, GL_FLOAT, GL_FALSE, 0, &vertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
};
class Line
{
  public:
	std::array<Vec2<float>, 2> vertices;
	float thickness;
	Line(Vec2<float> p0, Vec2<float> p1, float thickness) : thickness(thickness)
	{
		vertices = {{p0, p1}};
	}
	void draw(GLuint vertexAttribPos)
	{
		glLineWidth(thickness);
		glEnableVertexAttribArray(vertexAttribPos);
		glVertexAttribPointer(vertexAttribPos, 2, GL_FLOAT, GL_FALSE, 0, &vertices);
		glDrawArrays(GL_LINES, 0, 2);
	}
};
class ActiveTexture
{
	ActiveTexture(const ActiveTexture &) = delete;

  public:
	static GLenum getUnitEnum(int unit) { return GL_TEXTURE0 + unit; }

	ActiveTexture(int unit)
	{
		GLenum prevUnit;
		glGetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint *>(&prevUnit));
		if (prevUnit == getUnitEnum(unit))
		{
			return;
		}
		glActiveTexture(getUnitEnum(unit));
	}
};

class UnpackAlignment
{
	UnpackAlignment(const UnpackAlignment &) = delete;

  public:
	UnpackAlignment(int align)
	{
		GLint prevAlign;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlign);
		if (prevAlign == align)
		{
			return;
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, align);
	}
};

class BindTexture
{
	BindTexture(const BindTexture &) = delete;

  public:
	GLenum bind;
	int unit;
	bool nop;
	static GLenum getBindEnum(GLenum e)
	{
		switch (e)
		{
			// FIXME: Not supported?
			/*case GL_TEXTURE_1D:
			    return GL_TEXTURE_BINDING_1D; */
			case GL_TEXTURE_2D:
				return GL_TEXTURE_BINDING_2D;
			case GL_TEXTURE_3D:
				return GL_TEXTURE_BINDING_3D;
			case GL_TEXTURE_2D_ARRAY:
				return GL_TEXTURE_BINDING_2D_ARRAY;
			default:
				LogError("Unknown texture enum %d", static_cast<int>(e));
				return GL_TEXTURE_BINDING_2D;
		}
	}
	BindTexture(GLuint id, GLint unit = 0, GLenum bind = GL_TEXTURE_2D) : bind(bind), unit(unit)
	{
		ActiveTexture a(unit);
		GLuint prevID;
		glGetIntegerv(getBindEnum(bind), reinterpret_cast<GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		glBindTexture(bind, id);
	}
};

template <GLenum param> class TexParam
{
	TexParam(const TexParam &) = delete;

  public:
	GLuint id;
	GLenum type;
	bool nop;

	TexParam(GLuint id, GLint value, GLenum type = GL_TEXTURE_2D) : id(id), type(type)
	{
		GLint prevValue;
		BindTexture b(id, 0, type);
		glGetTexParameteriv(type, param, &prevValue);
		if (prevValue == value)
		{
			return;
		}
		glTexParameteri(type, param, value);
	}
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;

  public:
	BindFramebuffer(GLuint id)
	{
		GLuint prevID;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, reinterpret_cast<GLint *>(&prevID));
		if (prevID == id)
		{
			return;
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	}
};

class FBOData : public RendererImageData
{
  public:
	GLuint fbo;
	GLuint tex;
	GLuint depthBuffer;
	Vec2<float> size;
	// Constructor /only/ to be used for default surface (FBO ID == 0)
	FBOData(GLuint fbo)
	    // FIXME: Check FBO == 0
	    // FIXME: Warn if trying to texture from FBO 0
	    : fbo(fbo),
	      tex(-1),
	      size(0, 0)
	{
	}

	FBOData(Vec2<int> size) : size(size.x, size.y)
	{
		TRACE_FN;
		glGenTextures(1, &this->tex);
		BindTexture b(this->tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		             NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.x, size.y);

		glGenFramebuffers(1, &this->fbo);
		BindFramebuffer f(this->fbo);

		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->tex,
		                       0);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
		                          depthBuffer);
		assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	}
	virtual ~FBOData()
	{
		if (tex)
			glDeleteTextures(1, &tex);
		if (depthBuffer)
			glDeleteRenderbuffers(1, &depthBuffer);
		if (fbo)
			glDeleteFramebuffers(1, &fbo);
	}
};

class GLRGBImage : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<RGBImage> parent;
	GLRGBImage(sp<RGBImage> parent) : size(parent->size), parent(parent)
	{
		TRACE_FN;
		RGBImageLock l(parent, ImageLockUse::Read);
		glGenTextures(1, &this->texID);
		BindTexture b(this->texID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, parent->size.x, parent->size.y, 0, GL_RGBA,
		             GL_UNSIGNED_BYTE, l.getData());
	}
	virtual ~GLRGBImage() { glDeleteTextures(1, &this->texID); }
};

class GLPalette : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<Palette> parent;
	GLPalette(sp<Palette> parent) : size(Vec2<float>(parent->colours.size(), 1)), parent(parent)
	{
		TRACE_FN;
		glGenTextures(1, &this->texID);
		BindTexture b(this->texID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, parent->colours.size(), 1, 0, GL_RGBA,
		             GL_UNSIGNED_BYTE, parent->colours.data());
	}
	virtual ~GLPalette() { glDeleteTextures(1, &this->texID); }
};

class GLPaletteImage : public RendererImageData
{
  public:
	GLuint texID;
	Vec2<float> size;
	std::weak_ptr<PaletteImage> parent;
	GLPaletteImage(sp<PaletteImage> parent) : size(parent->size), parent(parent)
	{
		TRACE_FN;
		PaletteImageLock l(parent, ImageLockUse::Read);
		glGenTextures(1, &this->texID);
		BindTexture b(this->texID);
		UnpackAlignment align(1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, parent->size.x, parent->size.y, 0, GL_RED_INTEGER,
		             GL_UNSIGNED_BYTE, l.getData());
	}
	virtual ~GLPaletteImage() { glDeleteTextures(1, &this->texID); }
};

class GLPaletteSpritesheet : public RendererImageData
{
  public:
	std::weak_ptr<ImageSet> parent;
	Vec2<int> maxSize;
	unsigned numSprites;
	GLuint texID;
	GLPaletteSpritesheet(sp<ImageSet> parent)
	    : parent(parent), maxSize(parent->maxSize), numSprites(parent->images.size())
	{
		TRACE_FN;
		glGenTextures(1, &this->texID);
		BindTexture b(this->texID, 0, GL_TEXTURE_2D_ARRAY);
		UnpackAlignment align(1);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8UI, maxSize.x, maxSize.y, numSprites, 0,
		             GL_RED_INTEGER, GL_UNSIGNED_BYTE, NULL);

		std::unique_ptr<char[]> zeros(new char[maxSize.x * maxSize.y]);
		memset(zeros.get(), 1, maxSize.x * maxSize.y);

		LogInfo("Uploading %d sprites in {%d,%d} spritesheet", numSprites, maxSize.x, maxSize.y);

		for (unsigned int i = 0; i < numSprites; i++)
		{
			sp<PaletteImage> img = std::dynamic_pointer_cast<PaletteImage>(parent->images[i]);
			// FIXME: HACK - better way of clearing undefined portions to '0'?
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, maxSize.x, maxSize.y, 1,
			                GL_RED_INTEGER, GL_UNSIGNED_BYTE, zeros.get());

			PaletteImageLock l(img, ImageLockUse::Read);

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, img->size.x, img->size.y, 1,
			                GL_RED_INTEGER, GL_UNSIGNED_BYTE, l.getData());
		}
		LogInfo("Uploading spritesheet complete", numSprites);
	}
	virtual ~GLPaletteSpritesheet() { glDeleteTextures(1, &this->texID); }
};

class OGL30Renderer : public Renderer
{
  private:
	enum class RendererState
	{
		Idle,
		BatchingSpritesheet,
	};
	RendererState state;
	sp<RGBProgram> rgbProgram;
	sp<SolidColourProgram> colourProgram;
	sp<PaletteProgram> paletteProgram;
	sp<PaletteSetProgram> paletteSetProgram;
	GLuint currentBoundProgram;
	GLuint currentBoundFBO;

	sp<Surface> currentSurface;
	sp<Palette> currentPalette;

	friend class RendererSurfaceBinding;
	virtual void setSurface(sp<Surface> s) override
	{
		if (this->currentSurface == s)
		{
			return;
		}
		this->flush();
		this->currentSurface = s;
		if (!s->rendererPrivateData)
			s->rendererPrivateData.reset(new FBOData(s->size));

		FBOData *fbo = static_cast<FBOData *>(s->rendererPrivateData.get());
		glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
		this->currentBoundFBO = fbo->fbo;
		glViewport(0, 0, s->size.x, s->size.y);
	}
	virtual sp<Surface> getSurface() override { return currentSurface; }
	sp<Surface> defaultSurface;

  public:
	OGL30Renderer();
	virtual ~OGL30Renderer();
	virtual void clear(Colour c = Colour{0, 0, 0, 0}) override;
	virtual void setPalette(sp<Palette> p) override
	{
		if (p == this->currentPalette)
			return;
		this->flush();
		if (!p->rendererPrivateData)
			p->rendererPrivateData.reset(new GLPalette(p));
		this->currentPalette = p;
	}
	virtual sp<Palette> getPalette() override { return this->currentPalette; }

	virtual void draw(sp<Image> i, Vec2<float> position) override;
	virtual void drawRotated(sp<Image> image, Vec2<float> center, Vec2<float> position,
	                         float angle) override
	{
		auto size = image->size;
		if (this->state != RendererState::Idle)
			this->flush();
		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage);
				image->rendererPrivateData.reset(img);
			}
			DrawRGB(*img, position, size, Scaler::Linear, center, angle);
			return;
		}

		sp<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
		LogError("Unsupported image type");
	}
	virtual void drawScaled(sp<Image> image, Vec2<float> position, Vec2<float> size,
	                        Scaler scaler = Scaler::Linear) override
	{

		if (this->state != RendererState::Idle)
			this->flush();
		sp<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
		if (rgbImage)
		{
			GLRGBImage *img = dynamic_cast<GLRGBImage *>(rgbImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLRGBImage(rgbImage);
				image->rendererPrivateData.reset(img);
			}
			DrawRGB(*img, position, size, scaler);
			return;
		}

		sp<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
		if (paletteImage)
		{
			GLPaletteImage *img =
			    dynamic_cast<GLPaletteImage *>(paletteImage->rendererPrivateData.get());
			if (!img)
			{
				img = new GLPaletteImage(paletteImage);
				image->rendererPrivateData.reset(img);
			}
			if (scaler != Scaler::Nearest)
			{
				// blending indices doesn't make sense. You'll have to render
				// it to an RGB surface then scale that
				LogError("Only nearest scaler is supported on paletted images");
			}
			DrawPalette(*img, position, size);
			return;
		}

		sp<Surface> surface = std::dynamic_pointer_cast<Surface>(image);
		if (surface)
		{
			FBOData *fbo = dynamic_cast<FBOData *>(surface->rendererPrivateData.get());
			if (!fbo)
			{
				fbo = new FBOData(image->size);
				image->rendererPrivateData.reset(fbo);
			}
			DrawSurface(*fbo, position, size, scaler);
			return;
		}
		LogError("Unsupported image type");
	}
	virtual void drawTinted(sp<Image> i, Vec2<float> position, Colour tint) override
	{
		LogError("Unimplemented function");
		std::ignore = i;
		std::ignore = position;
		std::ignore = tint;
	}
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c) override;
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c,
	                      float thickness = 1.0) override
	{
		this->drawLine(position, Vec2<float>{position.x + size.x, position.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x + size.x, position.y},
		               Vec2<float>{position.x + size.x, position.y + size.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x + size.x, position.y + size.y},
		               Vec2<float>{position.x, position.y + size.y}, c, thickness);
		this->drawLine(Vec2<float>{position.x, position.y + size.y}, position, c, thickness);
	}
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0) override
	{
		if (this->state != RendererState::Idle)
			this->flush();
		this->DrawLine(p1, p2, c, thickness);
	}
	virtual void flush() override;
	virtual UString getName() override;
	virtual sp<Surface> getDefaultSurface() override { return this->defaultSurface; }

	void BindProgram(sp<Program> p)
	{
		if (this->currentBoundProgram == p->prog)
			return;
		glUseProgram(p->prog);
		this->currentBoundProgram = p->prog;
	}

	void DrawRGB(GLRGBImage &img, Vec2<float> offset, Vec2<float> size, Scaler scaler,
	             Vec2<float> rotationCenter = {0, 0}, float rotationAngleRadians = 0)
	{
		GLenum filter;
		Rect<float> pos(offset, offset + size);
		switch (scaler)
		{
			case Scaler::Linear:
				filter = GL_LINEAR;
				break;
			case Scaler::Nearest:
				filter = GL_NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = GL_NEAREST;
				break;
		}
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(img.texID);
		TexParam<GL_TEXTURE_MAG_FILTER> mag(img.texID, filter);
		TexParam<GL_TEXTURE_MIN_FILTER> min(img.texID, filter);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}}, rotationCenter, rotationAngleRadians);
		q.draw(rgbProgram->posLoc, rgbProgram->texcoordLoc);
	}

	void DrawPalette(GLPaletteImage &img, Vec2<float> offset, Vec2<float> size)
	{
		BindProgram(paletteProgram);
		Rect<float> pos(offset, offset + size);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(img.texID, 0);

		BindTexture p(
		    static_cast<GLPalette *>(this->currentPalette->rendererPrivateData.get())->texID, 1);
		Quad q(pos, Rect<float>{{0, 0}, {img.size}});
		q.draw(paletteProgram->posLoc, paletteProgram->texcoordLoc);
	}

	void DrawSurface(FBOData &fbo, Vec2<float> offset, Vec2<float> size, Scaler scaler)
	{
		GLenum filter;
		Rect<float> pos(offset, offset + size);
		switch (scaler)
		{
			case Scaler::Linear:
				filter = GL_LINEAR;
				break;
			case Scaler::Nearest:
				filter = GL_NEAREST;
				break;
			default:
				LogError("Unknown scaler requested");
				filter = GL_NEAREST;
				break;
		}
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(fbo.tex);
		TexParam<GL_TEXTURE_MAG_FILTER> mag(fbo.tex, filter);
		TexParam<GL_TEXTURE_MIN_FILTER> min(fbo.tex, filter);
		Quad q(pos, Rect<float>{{0, 0}, {1, 1}});
		q.draw(rgbProgram->posLoc, rgbProgram->texcoordLoc);
	}

	void DrawRect(Vec2<float> offset, Vec2<float> size, Colour c)
	{
		BindProgram(colourProgram);
		Rect<float> pos(offset, offset + size);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(this->currentSurface->size, flipY, c);
		Quad q(pos, Rect<float>{{0, 0}, {0, 0}});
		q.draw(colourProgram->posLoc);
	}

	void DrawLine(Vec2<float> p0, Vec2<float> p1, Colour c, float thickness)
	{
		BindProgram(colourProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(this->currentSurface->size, flipY, c);
		Line l(p0, p1, thickness);
		l.draw(colourProgram->posLoc);
	}

	class BatchedVertex
	{
	  public:
		Vec2<float> position;
		Vec2<float> texCoord;
		int spriteIdx;
		BatchedVertex() {}
		BatchedVertex(Vec2<float> p, Vec2<float> tc, int i)
		    : position(p), texCoord(tc), spriteIdx(i)
		{
		}
	};
	static_assert(sizeof(BatchedVertex) == 20, "BatchedVertex unexpected size");

	class BatchedSprite
	{
	  public:
		std::array<BatchedVertex, 4> vertices;
		BatchedSprite(Vec2<float> screenPosition, Vec2<float> spriteSize, int spriteIdx)
		{
			Vec2<float> maxTexCoords = spriteSize;
			Vec2<float> maxPosition = screenPosition + spriteSize;
			vertices[0] = BatchedVertex{screenPosition, Vec2<float>{0, 0}, spriteIdx};
			vertices[1] = BatchedVertex{Vec2<float>{screenPosition.x, maxPosition.y},
			                            Vec2<float>{0, maxTexCoords.y}, spriteIdx};
			vertices[2] = BatchedVertex{Vec2<float>{maxPosition.x, screenPosition.y},
			                            Vec2<float>{maxTexCoords.x, 0}, spriteIdx};
			vertices[3] = BatchedVertex{maxPosition, maxTexCoords, spriteIdx};
		}
	};
	static_assert(sizeof(BatchedSprite) == sizeof(BatchedVertex) * 4,
	              "BatchedSprite unexpected size");

	std::vector<BatchedSprite> batchedSprites;
	unsigned maxBatchedSprites;
	unsigned maxSpritesheetSize;
	sp<GLPaletteSpritesheet> boundSpritesheet;

	std::unique_ptr<GLint[]> firstList;
	std::unique_ptr<GLsizei[]> countList;

	void DrawBatchedSpritesheet()
	{
		BindProgram(paletteSetProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteSetProgram->setUniforms(this->currentSurface->size, flipY);
		BindTexture t(this->boundSpritesheet->texID, 0, GL_TEXTURE_2D_ARRAY);
		BindTexture p(
		    static_cast<GLPalette *>(this->currentPalette->rendererPrivateData.get())->texID, 1);

		glEnableVertexAttribArray(paletteSetProgram->posLoc);
		glEnableVertexAttribArray(paletteSetProgram->texcoordLoc);
		glEnableVertexAttribArray(paletteSetProgram->spriteLoc);

		const char *vertexPtr = reinterpret_cast<const char *>(this->batchedSprites.data());

		glVertexAttribPointer(paletteSetProgram->posLoc, 2, GL_FLOAT, GL_FALSE,
		                      sizeof(BatchedVertex), vertexPtr + offsetof(BatchedVertex, position));
		glVertexAttribPointer(paletteSetProgram->texcoordLoc, 2, GL_FLOAT, GL_FALSE,
		                      sizeof(BatchedVertex), vertexPtr + offsetof(BatchedVertex, texCoord));
		glVertexAttribIPointer(paletteSetProgram->spriteLoc, 1, GL_INT, sizeof(BatchedVertex),
		                       vertexPtr + offsetof(BatchedVertex, spriteIdx));
		// FIXME: glMultiDrawArrays is not supported, so I'm throwing in this stupid loop
		for (int i = 0; i < batchedSprites.size(); ++i)
		{
			glDrawArrays(GL_TRIANGLE_STRIP, *(this->firstList.get() + i),
			             *(this->countList.get() + i));
		}
		/*glMultiDrawArrays(GL_TRIANGLE_STRIP, this->firstList.get(), this->countList.get(),
		                    this->batchedSprites.size());*/

		this->batchedSprites.clear();
		this->state = RendererState::Idle;
	}
};

OGL30Renderer::OGL30Renderer()
    : state(RendererState::Idle), rgbProgram(new RGBProgram()),
      colourProgram(new SolidColourProgram()), paletteProgram(new PaletteProgram()),
      paletteSetProgram(new PaletteSetProgram()), currentBoundProgram(0), currentBoundFBO(0)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	LogInfo("Viewport {%d,%d,%d,%d}", viewport[0], viewport[1], viewport[2], viewport[3]);
	assert(viewport[0] == 0 && viewport[1] == 0);
	this->defaultSurface = std::make_shared<Surface>(Vec2<int>{viewport[2], viewport[3]});
	this->defaultSurface->rendererPrivateData.reset(new FBOData(0));
	this->currentSurface = this->defaultSurface;

	GLint maxTexArrayLayers;
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxTexArrayLayers);
	LogInfo("MAX_ARRAY_TEXTURE_LAYERS: %d", maxTexArrayLayers);
	this->maxBatchedSprites = 2048;
	this->maxSpritesheetSize = maxTexArrayLayers;

	this->firstList.reset(new GLint[this->maxBatchedSprites]);
	this->countList.reset(new GLsizei[this->maxBatchedSprites]);

	for (unsigned int i = 0; i < this->maxBatchedSprites; i++)
	{
		this->firstList[i] = 4 * i;
		this->countList[i] = 4;
	}

	GLint maxTexUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
	LogInfo("MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d", maxTexUnits);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

OGL30Renderer::~OGL30Renderer() {}

void OGL30Renderer::clear(Colour c)
{
	this->flush();
	glClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void OGL30Renderer::draw(sp<Image> image, Vec2<float> position)
{
	sp<ImageSet> owningSet = image->owningSet.lock();
	if (owningSet)
	{
		if (owningSet->images.size() > maxSpritesheetSize)
		{
			static bool warnonce = false;
			if (!warnonce)
			{
				warnonce = true;
				LogWarning("Spritesheet size %d would be over max array size %d - falling back to "
				           "'slow' path",
				           owningSet->images.size(), maxSpritesheetSize);
			}
		}
		else
		{
			sp<GLPaletteSpritesheet> ss =
			    std::dynamic_pointer_cast<GLPaletteSpritesheet>(owningSet->rendererPrivateData);
			if (!ss)
			{
				ss = std::make_shared<GLPaletteSpritesheet>(owningSet);
				owningSet->rendererPrivateData = ss;
			}
			switch (this->state)
			{
				default:
					this->flush();
				case RendererState::BatchingSpritesheet:
					if (ss != this->boundSpritesheet ||
					    this->batchedSprites.size() >= this->maxBatchedSprites)
					{
						this->flush();
					}
				case RendererState::Idle:
					break;
			}
			this->boundSpritesheet = ss;
			this->state = RendererState::BatchingSpritesheet;
			this->batchedSprites.emplace_back(position, Vec2<float>(image->size.x, image->size.y),
			                                  image->indexInSet);
			return;
		}
	}
	drawScaled(image, position, image->size, Scaler::Nearest);
}
void OGL30Renderer::drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c)
{
	this->flush();
	DrawRect(position, size, c);
}

void OGL30Renderer::flush()
{
	switch (this->state)
	{
		case RendererState::Idle:
			break;
		case RendererState::BatchingSpritesheet:
			this->DrawBatchedSpritesheet();
			break;
	}
	this->state = RendererState::Idle;
}

UString OGL30Renderer::getName() { return "Highly Experimental OGLES3.0 Renderer"; }

class OGL30RendererFactory : public OpenApoc::RendererFactory
{
	bool alreadyInitialised;
	bool functionLoadSuccess;

  public:
	OGL30RendererFactory() : alreadyInitialised(false), functionLoadSuccess(false) {}
	virtual OpenApoc::Renderer *create() override
	{
		if (!alreadyInitialised)
		{
			alreadyInitialised = true;
			// check version string for "3.0 es"
			const char *version_gl = (const char *)glGetString(GL_VERSION);
			std::string version = std::string(version_gl);
			LogInfo("[GLES3Renderer] Version: %s", version_gl);
			bool success = (version.find("ES 3.0") != std::string::npos);
			if (!success)
			{
				LogWarning("GLES3.0 not available; disabling GLES3 renderer...");
				return nullptr;
			}

			/*
			if (success.GetNumMissing())
			    return nullptr;*/
			functionLoadSuccess = true;
		}
		if (functionLoadSuccess)
			return new OGL30Renderer();
		return nullptr;
	}
};

OpenApoc::RendererRegister<OGL30RendererFactory> register_at_load_gl_3_0_renderer("GLES_3_0");

}; // anonymous namespace
