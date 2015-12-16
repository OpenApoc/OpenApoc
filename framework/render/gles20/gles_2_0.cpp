#include "gles_2_0.hpp"
#include <GLES2/gl2.h>

namespace gl
{
void ActiveTexture(GLenum texture) { glActiveTexture(texture); }
void AttachShader(GLuint program, GLuint shader) { glAttachShader(program, shader); }
void BindAttribLocation(GLuint program, GLuint index, const GLchar *name)
{
	glBindAttribLocation(program, index, name);
}
void BindBuffer(GLenum target, GLuint buffer) { glBindBuffer(target, buffer); }
void BindFramebuffer(GLenum target, GLuint framebuffer) { glBindFramebuffer(target, framebuffer); }
void BindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	glBindRenderbuffer(target, renderbuffer);
}
void BindTexture(GLenum target, GLuint texture) { glBindTexture(target, texture); }
void BlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	glBlendColor(red, green, blue, alpha);
}
void BlendEquation(GLenum mode) { glBlendEquation(mode); }
void BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	glBlendEquationSeparate(modeRGB, modeAlpha);
}
void BlendFunc(GLenum sfactor, GLenum dfactor) { glBlendFunc(sfactor, dfactor); }
void BlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}
void BufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
	glBufferData(target, size, data, usage);
}
void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data)
{
	glBufferSubData(target, offset, size, data);
}
GLenum CheckFramebufferStatus(GLenum target) { return glCheckFramebufferStatus(target); }
void Clear(GLbitfield mask) { glClear(mask); }
void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	glClearColor(red, green, blue, alpha);
}
void ClearDepthf(GLclampf depth) { glClearDepthf(depth); }
void ClearStencil(GLint s) { glClearStencil(s); }
void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	glColorMask(red, green, blue, alpha);
}
void CompileShader(GLuint shader) { glCompileShader(shader); }
void CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                          GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
{
	glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}
void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                             GLsizei width, GLsizei height, GLenum format, GLsizei imageSize,
                             const GLvoid *data)
{
	glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize,
	                          data);
}
void CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y,
                    GLsizei width, GLsizei height, GLint border)
{
	glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}
void CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y,
                       GLsizei width, GLsizei height)
{
	glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}
GLuint CreateProgram(void) { return glCreateProgram(); }
GLuint CreateShader(GLenum type) { return glCreateShader(type); }
void CullFace(GLenum mode) { glCullFace(mode); }
void DeleteBuffers(GLsizei n, const GLuint *buffers) { glDeleteBuffers(n, buffers); }
void DeleteFramebuffers(GLsizei n, const GLuint *framebuffers)
{
	glDeleteFramebuffers(n, framebuffers);
}
void DeleteProgram(GLuint program) { glDeleteProgram(program); }
void DeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers)
{
	glDeleteRenderbuffers(n, renderbuffers);
}
void DeleteShader(GLuint shader) { glDeleteShader(shader); }
void DeleteTextures(GLsizei n, const GLuint *textures) { glDeleteTextures(n, textures); }
void DepthFunc(GLenum func) { glDepthFunc(func); }
void DepthMask(GLboolean flag) { glDepthMask(flag); }
void DepthRangef(GLclampf zNear, GLclampf zFar) { glDepthRangef(zNear, zFar); }
void DetachShader(GLuint program, GLuint shader) { glDetachShader(program, shader); }
void Disable(GLenum cap) { glDisable(cap); }
void DisableVertexAttribArray(GLuint index) { glDisableVertexAttribArray(index); }
void DrawArrays(GLenum mode, GLint first, GLsizei count) { glDrawArrays(mode, first, count); }
void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	glDrawElements(mode, count, type, indices);
}
void Enable(GLenum cap) { glEnable(cap); }
void EnableVertexAttribArray(GLuint index) { glEnableVertexAttribArray(index); }
void Finish(void) { glFinish(); }
void Flush(void) { glFlush(); }
void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget,
                             GLuint renderbuffer)
{
	glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}
void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
                          GLint level)
{
	glFramebufferTexture2D(target, attachment, textarget, texture, level);
}
void FrontFace(GLenum mode) { glFrontFace(mode); }
void GenBuffers(GLsizei n, GLuint *buffers) { glGenBuffers(n, buffers); }
void GenerateMipmap(GLenum target) { glGenerateMipmap(target); }
void GenFramebuffers(GLsizei n, GLuint *framebuffers) { glGenFramebuffers(n, framebuffers); }
void GenRenderbuffers(GLsizei n, GLuint *renderbuffers) { glGenRenderbuffers(n, renderbuffers); }
void GenTextures(GLsizei n, GLuint *textures) { glGenTextures(n, textures); }
void GetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei *length, GLint *size,
                     GLenum *type, GLchar *name)
{
	glGetActiveAttrib(program, index, bufsize, length, size, type, name);
}
void GetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei *length, GLint *size,
                      GLenum *type, GLchar *name)
{
	glGetActiveUniform(program, index, bufsize, length, size, type, name);
}
void GetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei *count, GLuint *shaders)
{
	glGetAttachedShaders(program, maxcount, count, shaders);
}
GLint GetAttribLocation(GLuint program, const GLchar *name)
{
	return glGetAttribLocation(program, name);
}
void GetBooleanv(GLenum pname, GLboolean *params) { glGetBooleanv(pname, params); }
void GetBufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
	glGetBufferParameteriv(target, pname, params);
}
GLenum GetError(void) { return glGetError(); }
void GetFloatv(GLenum pname, GLfloat *params) { glGetFloatv(pname, params); }
void GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname,
                                         GLint *params)
{
	glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}
void GetIntegerv(GLenum pname, GLint *params) { glGetIntegerv(pname, params); }
void GetProgramiv(GLuint program, GLenum pname, GLint *params)
{
	glGetProgramiv(program, pname, params);
}
void GetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei *length, GLchar *infolog)
{
	glGetProgramInfoLog(program, bufsize, length, infolog);
}
void GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
	glGetRenderbufferParameteriv(target, pname, params);
}
void GetShaderiv(GLuint shader, GLenum pname, GLint *params)
{
	glGetShaderiv(shader, pname, params);
}
void GetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *infolog)
{
	glGetShaderInfoLog(shader, bufsize, length, infolog);
}
void GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range,
                              GLint *precision)
{
	glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}
void GetShaderSource(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *source)
{
	glGetShaderSource(shader, bufsize, length, source);
}
const GLubyte *GetString(GLenum name) { return glGetString(name); }
void GetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	glGetTexParameterfv(target, pname, params);
}
void GetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
	glGetTexParameteriv(target, pname, params);
}
void GetUniformfv(GLuint program, GLint location, GLfloat *params)
{
	glGetUniformfv(program, location, params);
}
void GetUniformiv(GLuint program, GLint location, GLint *params)
{
	glGetUniformiv(program, location, params);
}
GLint GetUniformLocation(GLuint program, const GLchar *name)
{
	return glGetUniformLocation(program, name);
}
void GetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params)
{
	glGetVertexAttribfv(index, pname, params);
}
void GetVertexAttribiv(GLuint index, GLenum pname, GLint *params)
{
	glGetVertexAttribiv(index, pname, params);
}
void GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid **pointer)
{
	glGetVertexAttribPointerv(index, pname, pointer);
}
void Hint(GLenum target, GLenum mode) { glHint(target, mode); }
GLboolean IsBuffer(GLuint buffer) { return glIsBuffer(buffer); }
GLboolean IsEnabled(GLenum cap) { return glIsEnabled(cap); }
GLboolean IsFramebuffer(GLuint framebuffer) { return glIsFramebuffer(framebuffer); }
GLboolean IsProgram(GLuint program) { return glIsProgram(program); }
GLboolean IsRenderbuffer(GLuint renderbuffer) { return glIsRenderbuffer(renderbuffer); }
GLboolean IsShader(GLuint shader) { return glIsShader(shader); }
GLboolean IsTexture(GLuint texture) { return glIsTexture(texture); }
void LineWidth(GLfloat width) { glLineWidth(width); }
void LinkProgram(GLuint program) { glLinkProgram(program); }
void PixelStorei(GLenum pname, GLint param) { glPixelStorei(pname, param); }
void PolygonOffset(GLfloat factor, GLfloat units) { glPolygonOffset(factor, units); }
void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                GLvoid *pixels)
{
	glReadPixels(x, y, width, height, format, type, pixels);
}
void ReleaseShaderCompiler(void) { glReleaseShaderCompiler(); }
void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	glRenderbufferStorage(target, internalformat, width, height);
}
void SampleCoverage(GLclampf value, GLboolean invert) { glSampleCoverage(value, invert); }
void Scissor(GLint x, GLint y, GLsizei width, GLsizei height) { glScissor(x, y, width, height); }
void ShaderBinary(GLsizei n, const GLuint *shaders, GLenum binaryformat, const GLvoid *binary,
                  GLsizei length)
{
	glShaderBinary(n, shaders, binaryformat, binary, length);
}
void ShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length)
{
	glShaderSource(shader, count, string, length);
}
void StencilFunc(GLenum func, GLint ref, GLuint mask) { glStencilFunc(func, ref, mask); }
void StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	glStencilFuncSeparate(face, func, ref, mask);
}
void StencilMask(GLuint mask) { glStencilMask(mask); }
void StencilMaskSeparate(GLenum face, GLuint mask) { glStencilMaskSeparate(face, mask); }
void StencilOp(GLenum fail, GLenum zfail, GLenum zpass) { glStencilOp(fail, zfail, zpass); }
void StencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
	glStencilOpSeparate(face, fail, zfail, zpass);
}
void TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
void TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	glTexParameterf(target, pname, param);
}
void TexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
	glTexParameterfv(target, pname, params);
}
void TexParameteri(GLenum target, GLenum pname, GLint param)
{
	glTexParameteri(target, pname, param);
}
void TexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
	glTexParameteriv(target, pname, params);
}
void TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                   GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}
void Uniform1f(GLint location, GLfloat x) { glUniform1f(location, x); }
void Uniform1fv(GLint location, GLsizei count, const GLfloat *v)
{
	glUniform1fv(location, count, v);
}
void Uniform1i(GLint location, GLint x) { glUniform1i(location, x); }
void Uniform1iv(GLint location, GLsizei count, const GLint *v) { glUniform1iv(location, count, v); }
void Uniform2f(GLint location, GLfloat x, GLfloat y) { glUniform2f(location, x, y); }
void Uniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
	glUniform2fv(location, count, v);
}
void Uniform2i(GLint location, GLint x, GLint y) { glUniform2i(location, x, y); }
void Uniform2iv(GLint location, GLsizei count, const GLint *v) { glUniform2iv(location, count, v); }
void Uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z) { glUniform3f(location, x, y, z); }
void Uniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
	glUniform3fv(location, count, v);
}
void Uniform3i(GLint location, GLint x, GLint y, GLint z) { glUniform3i(location, x, y, z); }
void Uniform3iv(GLint location, GLsizei count, const GLint *v) { glUniform3iv(location, count, v); }
void Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	glUniform4f(location, x, y, z, w);
}
void Uniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
	glUniform4fv(location, count, v);
}
void Uniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
	glUniform4i(location, x, y, z, w);
}
void Uniform4iv(GLint location, GLsizei count, const GLint *v) { glUniform4iv(location, count, v); }
void UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	glUniformMatrix2fv(location, count, transpose, value);
}
void UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	glUniformMatrix3fv(location, count, transpose, value);
}
void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	glUniformMatrix4fv(location, count, transpose, value);
}
void UseProgram(GLuint program) { glUseProgram(program); }
void ValidateProgram(GLuint program) { glValidateProgram(program); }
void VertexAttrib1f(GLuint indx, GLfloat x) { glVertexAttrib1f(indx, x); }
void VertexAttrib1fv(GLuint indx, const GLfloat *values) { glVertexAttrib1fv(indx, values); }
void VertexAttrib2f(GLuint indx, GLfloat x, GLfloat y) { glVertexAttrib2f(indx, x, y); }
void VertexAttrib2fv(GLuint indx, const GLfloat *values) { glVertexAttrib2fv(indx, values); }
void VertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
	glVertexAttrib3f(indx, x, y, z);
}
void VertexAttrib3fv(GLuint indx, const GLfloat *values) { glVertexAttrib3fv(indx, values); }
void VertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	glVertexAttrib4f(indx, x, y, z, w);
}
void VertexAttrib4fv(GLuint indx, const GLfloat *values) { glVertexAttrib4fv(indx, values); }
void VertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
                         const GLvoid *ptr)
{
	glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
}
void Viewport(GLint x, GLint y, GLsizei width, GLsizei height) { glViewport(x, y, width, height); }
namespace sys
{
exts::LoadTest LoadFunctions()
{
	int numFailed = 0;
	return exts::LoadTest(true, numFailed);
}

static int g_major_version = 0;
static int g_minor_version = 0;

static void GetGLVersion()
{
	// FIXME: GLES2.0 does not have GL_MAJOR_VERSION or GL_MINOR_VERSION,
	// but GLES3.0 does. Apparently you can only get GLES2 version by
	// parsing the GL_VERSION string.
	// For now, just return 2.0
	g_major_version = 2;
	g_minor_version = 0;
}

int GetMajorVersion()
{
	if (g_major_version == 0)
	{
		GetGLVersion();
	}
	return g_major_version;
}

int GetMinorVersion()
{
	if (g_major_version == 0)
	{
		GetGLVersion();
	}
	return g_minor_version;
}

bool IsVersionGEQ(int majorVersion, int minorVersion)
{
	if (g_major_version == 0)
		GetGLVersion();

	if (majorVersion > g_major_version)
		return true;
	if (majorVersion < g_major_version)
		return false;
	if (minorVersion >= g_minor_version)
		return true;
	return false;
}
}

} // namespace gl
