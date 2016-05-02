#include "framework/logger.h"

#define GLESWRAP_GLES3
#include "gleswrap.h"

#if defined(GLESWRAP_PLATFORM_WGL)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX 1
#endif // NOMINMAX

#include <Windows.h>
#include <gl/GL.h>

#elif defined(GLESWRAP_PLATFORM_GLX)

#include <GL/glx.h>

#elif defined(GLESWRAP_PLATFORM_EGL)

#include <EGL/egl.h>

#else
#error Unknown platform
#endif

#if defined(__linux__)
#define GLESWRAP_PLATFORM_DLFCN
#define GLESWRAP_SUPPORT_DIRECT_LOADING
#endif

#if defined(GLESWRAP_PLATFORM_DLFCN)
#include <dlfcn.h>
#endif

#include <cassert>
#include <cstring>
#include <string>

static const std::string FUNCTION_PREFIX = "gl";

namespace gles_wrap
{

class gles3::gles3_loader
{

  private:
#if defined(GLESWRAP_PLATFORM_DLFCN)
	void *dlfcn_handle;
#elif defined(GLESWRAP_PLATFORM_WGL)
	HMODULE win32_handle;
#endif

	bool desktop_extension;

  public:
	gles3_loader(bool desktop_extension, std::string lib_name)
	    : desktop_extension(desktop_extension)
	{
		if (desktop_extension)
		{
#if defined(GLESWRAP_PLATFORM_DLFCN)
			this->dlfcn_handle = nullptr;
#elif defined(GLESWRAP_PLATFORM_WGL)
			// Win32 may need to use LoadLibrary as wglGetProcAddress fails (on some systems?) for
			// stuff in gl1.1 and below, as they're directly exported by opengl32.dll.
			this->win32_handle = LoadLibraryA("opengl32.dll");
#endif
			return;
		}
		else
		{
#if defined(GLESWRAP_PLATFORM_DLFCN)
			this->dlfcn_handle = dlopen(lib_name.c_str(), RTLD_LOCAL);
#elif defined(GLESWRAP_PLATFORM_WGL)
			this->win32_handle = LoadLibraryA("opengl32.dll");
#endif
		}
	}
	~gles3_loader()
	{
#if defined(GLESWRAP_PLATFORM_DLFCN)
		if (this->dlfcn_handle)
			dlclose(this->dlfcn_handle);
#elif defined(GLESWRAP_PLATFORM_WGL)
		if (this->win32_handle)
			FreeLibrary(this->win32_handle);
#endif
	}
	template <typename T> bool Load(T &func_pointer, const std::string &proc_name)
	{
		auto full_proc_name = FUNCTION_PREFIX + proc_name;
		if (desktop_extension)
		{
#if defined(GLESWRAP_PLATFORM_GLX)
			func_pointer = reinterpret_cast<T>(
			    glXGetProcAddress(reinterpret_cast<const ::GLubyte *>(full_proc_name.c_str())));
			if (func_pointer)
				return true;
#elif defined(GLESWRAP_PLATFORM_WGL)
			// Win32 may need to use LoadLibrary as wglGetProcAddress fails (on some systems?) for
			// stuff in gl1.1 and below, as they're directly exported by opengl32.dll.
			if (this->win32_handle)
			{
				func_pointer = reinterpret_cast<T>(GetProcAddress(
				    this->win32_handle,
				    reinterpret_cast<LPCSTR>(this->win32_handle, full_proc_name.c_str())));
				if (func_pointer)
					return true;
			}
			func_pointer = reinterpret_cast<T>(
			    wglGetProcAddress(reinterpret_cast<LPCSTR>(full_proc_name.c_str())));
			if (func_pointer)
				return true;
#elif defined(GLESWRAP_PLATFORM_EGL)
			func_pointer = reinterpret_cast<T>(eglGetProcAddress(full_proc_name.c_str()));
			if (func_pointer)
				return true;
#endif
		}
		else
		{
#if defined(GLESWRAP_PLATFORM_DLFCN)
			if (this->dlfcn_handle)
			{
				func_pointer =
				    reinterpret_cast<T>(dlsym(this->dlfcn_handle, full_proc_name.c_str()));
				if (func_pointer)
					return true;
			}
#elif defined(GLESWRAP_PLATFORM_WGL)
			if (this->win32_handle)
			{
				func_pointer = reinterpret_cast<T>(GetProcAddress(
				    this->win32_handle,
				    reinterpret_cast<LPCSTR>(this->win32_handle, full_proc_name.c_str())));
				if (func_pointer)
					return true;
			}
#endif
		}
		return false;
	}
};

bool gles3::supported(bool desktop_extension, std::string lib_name)
{
	gles3_loader tmp_loader(desktop_extension, lib_name);

	const GLubyte *(GLESWRAP_APIENTRY * LocalGetString)(GLenum name) = nullptr;

	if (!tmp_loader.Load(LocalGetString, "GetString"))
	{
		LogInfo("No \"glGetString\" symbol found");
		return false;
	}

	if (desktop_extension)
	{
		std::string extension_list = reinterpret_cast<const char *>(LocalGetString(EXTENSIONS));
		LogInfo("GL_EXTENSIONS: \"%s\"", extension_list.c_str());
		if (extension_list.find("GL_ARB_ES3_compatibility ") != extension_list.npos)
		{
			return true;
		}
		return false;
	}
	else
	{
		std::string version_string = reinterpret_cast<const char *>(LocalGetString(VERSION));
		if (version_string.find("OpenGL ES ") != 0)
		{
			return false;
		}
		// Remove the 'OpenGL ES ' prefix - 10 chars
		version_string = version_string.substr(10, version_string.npos);
		// Then check the version in 'x.y' format
		auto major_version = version_string.substr(1, 1);
		int major_version_int = stoi(major_version);
		// We don't actually care about the minor version here
		if (major_version_int >= 3)
		{
			return true;
		}
		return false;
	}
}

gles3::gles3(bool desktop_extension, std::string lib_name)
    : loader(new gles3_loader(desktop_extension, lib_name))
{
	assert(this->supported(desktop_extension, lib_name));
	loader->Load(ActiveTexture, "ActiveTexture");
	loader->Load(AttachShader, "AttachShader");
	loader->Load(BindAttribLocation, "BindAttribLocation");
	loader->Load(BindBuffer, "BindBuffer");
	loader->Load(BindFramebuffer, "BindFramebuffer");
	loader->Load(BindRenderbuffer, "BindRenderbuffer");
	loader->Load(BindTexture, "BindTexture");
	loader->Load(BlendColor, "BlendColor");
	loader->Load(BlendEquation, "BlendEquation");
	loader->Load(BlendEquationSeparate, "BlendEquationSeparate");
	loader->Load(BlendFunc, "BlendFunc");
	loader->Load(BlendFuncSeparate, "BlendFuncSeparate");
	loader->Load(BufferData, "BufferData");
	loader->Load(BufferSubData, "BufferSubData");
	loader->Load(CheckFramebufferStatus, "CheckFramebufferStatus");
	loader->Load(Clear, "Clear");
	loader->Load(ClearColor, "ClearColor");
	loader->Load(ClearDepthf, "ClearDepthf");
	loader->Load(ClearStencil, "ClearStencil");
	loader->Load(ColorMask, "ColorMask");
	loader->Load(CompileShader, "CompileShader");
	loader->Load(CompressedTexImage2D, "CompressedTexImage2D");
	loader->Load(CompressedTexSubImage2D, "CompressedTexSubImage2D");
	loader->Load(CopyTexImage2D, "CopyTexImage2D");
	loader->Load(CopyTexSubImage2D, "CopyTexSubImage2D");
	loader->Load(CreateProgram, "CreateProgram");
	loader->Load(CreateShader, "CreateShader");
	loader->Load(CullFace, "CullFace");
	loader->Load(DeleteBuffers, "DeleteBuffers");
	loader->Load(DeleteFramebuffers, "DeleteFramebuffers");
	loader->Load(DeleteProgram, "DeleteProgram");
	loader->Load(DeleteRenderbuffers, "DeleteRenderbuffers");
	loader->Load(DeleteShader, "DeleteShader");
	loader->Load(DeleteTextures, "DeleteTextures");
	loader->Load(DepthFunc, "DepthFunc");
	loader->Load(DepthMask, "DepthMask");
	loader->Load(DepthRangef, "DepthRangef");
	loader->Load(DetachShader, "DetachShader");
	loader->Load(Disable, "Disable");
	loader->Load(DisableVertexAttribArray, "DisableVertexAttribArray");
	loader->Load(DrawArrays, "DrawArrays");
	loader->Load(DrawElements, "DrawElements");
	loader->Load(Enable, "Enable");
	loader->Load(EnableVertexAttribArray, "EnableVertexAttribArray");
	loader->Load(Finish, "Finish");
	loader->Load(Flush, "Flush");
	loader->Load(FramebufferRenderbuffer, "FramebufferRenderbuffer");
	loader->Load(FramebufferTexture2D, "FramebufferTexture2D");
	loader->Load(FrontFace, "FrontFace");
	loader->Load(GenBuffers, "GenBuffers");
	loader->Load(GenerateMipmap, "GenerateMipmap");
	loader->Load(GenFramebuffers, "GenFramebuffers");
	loader->Load(GenRenderbuffers, "GenRenderbuffers");
	loader->Load(GenTextures, "GenTextures");
	loader->Load(GetActiveAttrib, "GetActiveAttrib");
	loader->Load(GetActiveUniform, "GetActiveUniform");
	loader->Load(GetAttachedShaders, "GetAttachedShaders");
	loader->Load(GetAttribLocation, "GetAttribLocation");
	loader->Load(GetBooleanv, "GetBooleanv");
	loader->Load(GetBufferParameteriv, "GetBufferParameteriv");
	loader->Load(GetError, "GetError");
	loader->Load(GetFloatv, "GetFloatv");
	loader->Load(GetFramebufferAttachmentParameteriv, "GetFramebufferAttachmentParameteriv");
	loader->Load(GetIntegerv, "GetIntegerv");
	loader->Load(GetProgramiv, "GetProgramiv");
	loader->Load(GetProgramInfoLog, "GetProgramInfoLog");
	loader->Load(GetRenderbufferParameteriv, "GetRenderbufferParameteriv");
	loader->Load(GetShaderiv, "GetShaderiv");
	loader->Load(GetShaderInfoLog, "GetShaderInfoLog");
	loader->Load(GetShaderPrecisionFormat, "GetShaderPrecisionFormat");
	loader->Load(GetShaderSource, "GetShaderSource");
	loader->Load(GetString, "GetString");
	loader->Load(GetTexParameterfv, "GetTexParameterfv");
	loader->Load(GetTexParameteriv, "GetTexParameteriv");
	loader->Load(GetUniformfv, "GetUniformfv");
	loader->Load(GetUniformiv, "GetUniformiv");
	loader->Load(GetUniformLocation, "GetUniformLocation");
	loader->Load(GetVertexAttribfv, "GetVertexAttribfv");
	loader->Load(GetVertexAttribiv, "GetVertexAttribiv");
	loader->Load(GetVertexAttribPointerv, "GetVertexAttribPointerv");
	loader->Load(Hint, "Hint");
	loader->Load(IsBuffer, "IsBuffer");
	loader->Load(IsEnabled, "IsEnabled");
	loader->Load(IsFramebuffer, "IsFramebuffer");
	loader->Load(IsProgram, "IsProgram");
	loader->Load(IsRenderbuffer, "IsRenderbuffer");
	loader->Load(IsShader, "IsShader");
	loader->Load(IsTexture, "IsTexture");
	loader->Load(LineWidth, "LineWidth");
	loader->Load(LinkProgram, "LinkProgram");
	loader->Load(PixelStorei, "PixelStorei");
	loader->Load(PolygonOffset, "PolygonOffset");
	loader->Load(ReadPixels, "ReadPixels");
	loader->Load(ReleaseShaderCompiler, "ReleaseShaderCompiler");
	loader->Load(RenderbufferStorage, "RenderbufferStorage");
	loader->Load(SampleCoverage, "SampleCoverage");
	loader->Load(Scissor, "Scissor");
	loader->Load(ShaderBinary, "ShaderBinary");
	loader->Load(ShaderSource, "ShaderSource");
	loader->Load(StencilFunc, "StencilFunc");
	loader->Load(StencilFuncSeparate, "StencilFuncSeparate");
	loader->Load(StencilMask, "StencilMask");
	loader->Load(StencilMaskSeparate, "StencilMaskSeparate");
	loader->Load(StencilOp, "StencilOp");
	loader->Load(StencilOpSeparate, "StencilOpSeparate");
	loader->Load(TexImage2D, "TexImage2D");
	loader->Load(TexParameterf, "TexParameterf");
	loader->Load(TexParameterfv, "TexParameterfv");
	loader->Load(TexParameteri, "TexParameteri");
	loader->Load(TexParameteriv, "TexParameteriv");
	loader->Load(TexSubImage2D, "TexSubImage2D");
	loader->Load(Uniform1f, "Uniform1f");
	loader->Load(Uniform1fv, "Uniform1fv");
	loader->Load(Uniform1i, "Uniform1i");
	loader->Load(Uniform1iv, "Uniform1iv");
	loader->Load(Uniform2f, "Uniform2f");
	loader->Load(Uniform2fv, "Uniform2fv");
	loader->Load(Uniform2i, "Uniform2i");
	loader->Load(Uniform2iv, "Uniform2iv");
	loader->Load(Uniform3f, "Uniform3f");
	loader->Load(Uniform3fv, "Uniform3fv");
	loader->Load(Uniform3i, "Uniform3i");
	loader->Load(Uniform3iv, "Uniform3iv");
	loader->Load(Uniform4f, "Uniform4f");
	loader->Load(Uniform4fv, "Uniform4fv");
	loader->Load(Uniform4i, "Uniform4i");
	loader->Load(Uniform4iv, "Uniform4iv");
	loader->Load(UniformMatrix2fv, "UniformMatrix2fv");
	loader->Load(UniformMatrix3fv, "UniformMatrix3fv");
	loader->Load(UniformMatrix4fv, "UniformMatrix4fv");
	loader->Load(UseProgram, "UseProgram");
	loader->Load(ValidateProgram, "ValidateProgram");
	loader->Load(VertexAttrib1f, "VertexAttrib1f");
	loader->Load(VertexAttrib1fv, "VertexAttrib1fv");
	loader->Load(VertexAttrib2f, "VertexAttrib2f");
	loader->Load(VertexAttrib2fv, "VertexAttrib2fv");
	loader->Load(VertexAttrib3f, "VertexAttrib3f");
	loader->Load(VertexAttrib3fv, "VertexAttrib3fv");
	loader->Load(VertexAttrib4f, "VertexAttrib4f");
	loader->Load(VertexAttrib4fv, "VertexAttrib4fv");
	loader->Load(VertexAttribPointer, "VertexAttribPointer");
	loader->Load(Viewport, "Viewport");

	loader->Load(ReadBuffer, "ReadBuffer");
	loader->Load(DrawRangeElements, "DrawRangeElements");
	loader->Load(TexImage3D, "TexImage3D");
	loader->Load(TexSubImage3D, "TexSubImage3D");
	loader->Load(CopyTexSubImage3D, "CopyTexSubImage3D");
	loader->Load(CompressedTexImage3D, "CompressedTexImage3D");
	loader->Load(CompressedTexSubImage3D, "CompressedTexSubImage3D");
	loader->Load(GenQueries, "GenQueries");
	loader->Load(DeleteQueries, "DeleteQueries");
	loader->Load(IsQuery, "IsQuery");
	loader->Load(BeginQuery, "BeginQuery");
	loader->Load(EndQuery, "EndQuery");
	loader->Load(GetQueryiv, "GetQueryiv");
	loader->Load(GetQueryObjectuiv, "GetQueryObjectuiv");
	loader->Load(UnmapBuffer, "UnmapBuffer");
	loader->Load(GetBufferPointerv, "GetBufferPointerv");
	loader->Load(DrawBuffers, "DrawBuffers");
	loader->Load(UniformMatrix2x3fv, "UniformMatrix2x3fv");
	loader->Load(UniformMatrix3x2fv, "UniformMatrix3x2fv");
	loader->Load(UniformMatrix2x4fv, "UniformMatrix2x4fv");
	loader->Load(UniformMatrix4x2fv, "UniformMatrix4x2fv");
	loader->Load(UniformMatrix3x4fv, "UniformMatrix3x4fv");
	loader->Load(UniformMatrix4x3fv, "UniformMatrix4x3fv");
	loader->Load(BlitFramebuffer, "BlitFramebuffer");
	loader->Load(RenderbufferStorageMultisample, "RenderbufferStorageMultisample");
	loader->Load(FramebufferTextureLayer, "FramebufferTextureLayer");
	loader->Load(MapBufferRange, "MapBufferRange");
	loader->Load(FlushMappedBufferRange, "FlushMappedBufferRange");
	loader->Load(BindVertexArray, "BindVertexArray");
	loader->Load(DeleteVertexArrays, "DeleteVertexArrays");
	loader->Load(GenVertexArrays, "GenVertexArrays");
	loader->Load(IsVertexArray, "IsVertexArray");
	loader->Load(GetIntegeri_v, "GetIntegeri_v");
	loader->Load(BeginTransformFeedback, "BeginTransformFeedback");
	loader->Load(EndTransformFeedback, "EndTransformFeedback");
	loader->Load(BindBufferRange, "BindBufferRange");
	loader->Load(BindBufferBase, "BindBufferBase");
	loader->Load(TransformFeedbackVaryings, "TransformFeedbackVaryings");
	loader->Load(GetTransformFeedbackVarying, "GetTransformFeedbackVarying");
	loader->Load(VertexAttribIPointer, "VertexAttribIPointer");
	loader->Load(GetVertexAttribIiv, "GetVertexAttribIiv");
	loader->Load(GetVertexAttribIuiv, "GetVertexAttribIuiv");
	loader->Load(VertexAttribI4i, "VertexAttribI4i");
	loader->Load(VertexAttribI4ui, "VertexAttribI4ui");
	loader->Load(VertexAttribI4iv, "VertexAttribI4iv");
	loader->Load(VertexAttribI4uiv, "VertexAttribI4uiv");
	loader->Load(GetUniformuiv, "GetUniformuiv");
	loader->Load(GetFragDataLocation, "GetFragDataLocation");
	loader->Load(Uniform1ui, "Uniform1ui");
	loader->Load(Uniform2ui, "Uniform2ui");
	loader->Load(Uniform3ui, "Uniform3ui");
	loader->Load(Uniform4ui, "Uniform4ui");
	loader->Load(Uniform1uiv, "Uniform1uiv");
	loader->Load(Uniform2uiv, "Uniform2uiv");
	loader->Load(Uniform3uiv, "Uniform3uiv");
	loader->Load(Uniform4uiv, "Uniform4uiv");
	loader->Load(ClearBufferiv, "ClearBufferiv");
	loader->Load(ClearBufferuiv, "ClearBufferuiv");
	loader->Load(ClearBufferfv, "ClearBufferfv");
	loader->Load(ClearBufferfi, "ClearBufferfi");
	loader->Load(GetStringi, "GetStringi");
	loader->Load(CopyBufferSubData, "CopyBufferSubData");
	loader->Load(GetUniformIndices, "GetUniformIndices");
	loader->Load(GetActiveUniformsiv, "GetActiveUniformsiv");

	loader->Load(GetUniformBlockIndex, "GetUniformBlockIndex");
	loader->Load(GetActiveUniformBlockiv, "GetActiveUniformBlockiv");
	loader->Load(GetActiveUniformBlockName, "GetActiveUniformBlockName");
	loader->Load(UniformBlockBinding, "UniformBlockBinding");
	loader->Load(DrawArraysInstanced, "DrawArraysInstanced");
	loader->Load(DrawElementsInstanced, "DrawElementsInstanced");
	loader->Load(FenceSync, "FenceSync");
	loader->Load(IsSync, "IsSync");
	loader->Load(DeleteSync, "DeleteSync");
	loader->Load(ClientWaitSync, "ClientWaitSync");
	loader->Load(WaitSync, "WaitSync");
	loader->Load(GetInteger64v, "GetInteger64v");
	loader->Load(GetSynciv, "GetSynciv");
	loader->Load(GetInteger64i_v, "GetInteger64i_v");
	loader->Load(GetBufferParameteri64v, "GetBufferParameteri64v");
	loader->Load(GenSamplers, "GenSamplers");
	loader->Load(DeleteSamplers, "DeleteSamplers");
	loader->Load(IsSampler, "IsSampler");
	loader->Load(BindSampler, "BindSampler");
	loader->Load(SamplerParameteri, "SamplerParameteri");
	loader->Load(SamplerParameteriv, "SamplerParameteriv");
	loader->Load(SamplerParameterf, "SamplerParameterf");
	loader->Load(SamplerParameterfv, "SamplerParameterfv");
	loader->Load(GetSamplerParameteriv, "GetSamplerParameteriv");
	loader->Load(GetSamplerParameterfv, "GetSamplerParameterfv");
	loader->Load(VertexAttribDivisor, "VertexAttribDivisor");
	loader->Load(BindTransformFeedback, "BindTransformFeedback");
	loader->Load(DeleteTransformFeedbacks, "DeleteTransformFeedbacks");
	loader->Load(GenTransformFeedbacks, "GenTransformFeedbacks");
	loader->Load(IsTransformFeedback, "IsTransformFeedback");
	loader->Load(PauseTransformFeedback, "PauseTransformFeedback");
	loader->Load(ResumeTransformFeedback, "ResumeTransformFeedback");
	loader->Load(GetProgramBinary, "GetProgramBinary");
	loader->Load(ProgramBinary, "ProgramBinary");
	loader->Load(ProgramParameteri, "ProgramParameteri");
	loader->Load(InvalidateFramebuffer, "InvalidateFramebuffer");
	loader->Load(InvalidateSubFramebuffer, "InvalidateSubFramebuffer");
	loader->Load(TexStorage2D, "TexStorage2D");
	loader->Load(TexStorage3D, "TexStorage3D");
	loader->Load(GetInternalformativ, "GetInternalformativ");
}

gles3::~gles3() { delete this->loader; }

} // namespace gles_wrap
