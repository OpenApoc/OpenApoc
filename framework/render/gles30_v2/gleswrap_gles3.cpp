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

#elif defined(GLESWRAP_PLATFORM_MACHO)
#include <mach-o/dyld.h>
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

#include <cstring>
#include <string>

static const std::string FUNCTION_PREFIX = "gl";

namespace gles_wrap
{

class Gles3::Gles3Loader
{

  private:
#if defined(GLESWRAP_PLATFORM_DLFCN)
	void *dlfcn_handle;
#elif defined(GLESWRAP_PLATFORM_WGL)
	HMODULE win32_handle;
#endif

	bool desktop_extension;

  public:
	Gles3Loader(bool desktop_extension, std::string lib_name) : desktop_extension(desktop_extension)
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
			this->dlfcn_handle = dlopen(lib_name.c_str(), RTLD_NOW | RTLD_LOCAL);
			if (!this->dlfcn_handle)
			{
				LogInfo("Failed to load library \"{}\" : \"{}\"", lib_name, dlerror());
			}
#elif defined(GLESWRAP_PLATFORM_WGL)
			this->win32_handle = LoadLibraryA("opengl32.dll");
#endif
		}
	}
	~Gles3Loader()
	{
#if defined(GLESWRAP_PLATFORM_DLFCN)
		if (this->dlfcn_handle)
			dlclose(this->dlfcn_handle);
#elif defined(GLESWRAP_PLATFORM_WGL)
		if (this->win32_handle)
			FreeLibrary(this->win32_handle);
#endif
	}
	template <typename T> bool load(T &func_pointer, const std::string &proc_name) const
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
#elif defined(GLESWRAP_PLATFORM_MACHO)
			NSSymbol symbol = NULL;
			// MacOS adds a '_' to C symbols apparently
			std::string symName = std::string("_") + full_proc_name;
			if (NSIsSymbolNameDefined(symName.c_str()))
			{
				symbol = NSLookupAndBindSymbol(symName.c_str());
				func_pointer = reinterpret_cast<T>(NSAddressOfSymbol(symbol));
				return true;
			}
			return false;
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

bool Gles3::supported(bool desktop_extension, std::string lib_name)
{
	Gles3Loader tmp_loader(desktop_extension, lib_name);

	const GLubyte *(GLESWRAP_APIENTRY * LocalGetString)(GLenum name) = nullptr;

	if (!tmp_loader.load(LocalGetString, "GetString"))
	{
		LogInfo("No \"glGetString\" symbol found");
		return false;
	}

	if (desktop_extension)
	{
		std::string extension_list = reinterpret_cast<const char *>(LocalGetString(EXTENSIONS));
		LogInfo("GL_EXTENSIONS: \"{}\"", extension_list);
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
		auto major_version = version_string.substr(0, 1);
		int major_version_int = stoi(major_version);
		// We don't actually care about the minor version here
		if (major_version_int >= 3)
		{
			return true;
		}
		return false;
	}
}

Gles3::Gles3(bool desktop_extension, std::string lib_name)
    : loader(new Gles3Loader(desktop_extension, lib_name)), desktop_extension(desktop_extension)
{
	LogAssert(this->supported(desktop_extension, lib_name));
	loader->load(ActiveTexture, "ActiveTexture");
	loader->load(AttachShader, "AttachShader");
	loader->load(BindAttribLocation, "BindAttribLocation");
	loader->load(BindBuffer, "BindBuffer");
	loader->load(BindFramebuffer, "BindFramebuffer");
	loader->load(BindRenderbuffer, "BindRenderbuffer");
	loader->load(BindTexture, "BindTexture");
	loader->load(BlendColor, "BlendColor");
	loader->load(BlendEquation, "BlendEquation");
	loader->load(BlendEquationSeparate, "BlendEquationSeparate");
	loader->load(BlendFunc, "BlendFunc");
	loader->load(BlendFuncSeparate, "BlendFuncSeparate");
	loader->load(BufferData, "BufferData");
	loader->load(BufferSubData, "BufferSubData");
	loader->load(CheckFramebufferStatus, "CheckFramebufferStatus");
	loader->load(Clear, "Clear");
	loader->load(ClearColor, "ClearColor");
	loader->load(ClearDepthf, "ClearDepthf");
	loader->load(ClearStencil, "ClearStencil");
	loader->load(ColorMask, "ColorMask");
	loader->load(CompileShader, "CompileShader");
	loader->load(CompressedTexImage2D, "CompressedTexImage2D");
	loader->load(CompressedTexSubImage2D, "CompressedTexSubImage2D");
	loader->load(CopyTexImage2D, "CopyTexImage2D");
	loader->load(CopyTexSubImage2D, "CopyTexSubImage2D");
	loader->load(CreateProgram, "CreateProgram");
	loader->load(CreateShader, "CreateShader");
	loader->load(CullFace, "CullFace");
	loader->load(DeleteBuffers, "DeleteBuffers");
	loader->load(DeleteFramebuffers, "DeleteFramebuffers");
	loader->load(DeleteProgram, "DeleteProgram");
	loader->load(DeleteRenderbuffers, "DeleteRenderbuffers");
	loader->load(DeleteShader, "DeleteShader");
	loader->load(DeleteTextures, "DeleteTextures");
	loader->load(DepthFunc, "DepthFunc");
	loader->load(DepthMask, "DepthMask");
	loader->load(DepthRangef, "DepthRangef");
	loader->load(DetachShader, "DetachShader");
	loader->load(Disable, "Disable");
	loader->load(DisableVertexAttribArray, "DisableVertexAttribArray");
	loader->load(DrawArrays, "DrawArrays");
	loader->load(DrawElements, "DrawElements");
	loader->load(Enable, "Enable");
	loader->load(EnableVertexAttribArray, "EnableVertexAttribArray");
	loader->load(Finish, "Finish");
	loader->load(Flush, "Flush");
	loader->load(FramebufferRenderbuffer, "FramebufferRenderbuffer");
	loader->load(FramebufferTexture2D, "FramebufferTexture2D");
	loader->load(FrontFace, "FrontFace");
	loader->load(GenBuffers, "GenBuffers");
	loader->load(GenerateMipmap, "GenerateMipmap");
	loader->load(GenFramebuffers, "GenFramebuffers");
	loader->load(GenRenderbuffers, "GenRenderbuffers");
	loader->load(GenTextures, "GenTextures");
	loader->load(GetActiveAttrib, "GetActiveAttrib");
	loader->load(GetActiveUniform, "GetActiveUniform");
	loader->load(GetAttachedShaders, "GetAttachedShaders");
	loader->load(GetAttribLocation, "GetAttribLocation");
	loader->load(GetBooleanv, "GetBooleanv");
	loader->load(GetBufferParameteriv, "GetBufferParameteriv");
	loader->load(GetError, "GetError");
	loader->load(GetFloatv, "GetFloatv");
	loader->load(GetFramebufferAttachmentParameteriv, "GetFramebufferAttachmentParameteriv");
	loader->load(GetIntegerv, "GetIntegerv");
	loader->load(GetProgramiv, "GetProgramiv");
	loader->load(GetProgramInfoLog, "GetProgramInfoLog");
	loader->load(GetRenderbufferParameteriv, "GetRenderbufferParameteriv");
	loader->load(GetShaderiv, "GetShaderiv");
	loader->load(GetShaderInfoLog, "GetShaderInfoLog");
	loader->load(GetShaderPrecisionFormat, "GetShaderPrecisionFormat");
	loader->load(GetShaderSource, "GetShaderSource");
	loader->load(GetString, "GetString");
	loader->load(GetTexParameterfv, "GetTexParameterfv");
	loader->load(GetTexParameteriv, "GetTexParameteriv");
	loader->load(GetUniformfv, "GetUniformfv");
	loader->load(GetUniformiv, "GetUniformiv");
	loader->load(GetUniformLocation, "GetUniformLocation");
	loader->load(GetVertexAttribfv, "GetVertexAttribfv");
	loader->load(GetVertexAttribiv, "GetVertexAttribiv");
	loader->load(GetVertexAttribPointerv, "GetVertexAttribPointerv");
	loader->load(Hint, "Hint");
	loader->load(IsBuffer, "IsBuffer");
	loader->load(IsEnabled, "IsEnabled");
	loader->load(IsFramebuffer, "IsFramebuffer");
	loader->load(IsProgram, "IsProgram");
	loader->load(IsRenderbuffer, "IsRenderbuffer");
	loader->load(IsShader, "IsShader");
	loader->load(IsTexture, "IsTexture");
	loader->load(LineWidth, "LineWidth");
	loader->load(LinkProgram, "LinkProgram");
	loader->load(PixelStorei, "PixelStorei");
	loader->load(PolygonOffset, "PolygonOffset");
	loader->load(ReadPixels, "ReadPixels");
	loader->load(ReleaseShaderCompiler, "ReleaseShaderCompiler");
	loader->load(RenderbufferStorage, "RenderbufferStorage");
	loader->load(SampleCoverage, "SampleCoverage");
	loader->load(Scissor, "Scissor");
	loader->load(ShaderBinary, "ShaderBinary");
	loader->load(ShaderSource, "ShaderSource");
	loader->load(StencilFunc, "StencilFunc");
	loader->load(StencilFuncSeparate, "StencilFuncSeparate");
	loader->load(StencilMask, "StencilMask");
	loader->load(StencilMaskSeparate, "StencilMaskSeparate");
	loader->load(StencilOp, "StencilOp");
	loader->load(StencilOpSeparate, "StencilOpSeparate");
	loader->load(TexImage2D, "TexImage2D");
	loader->load(TexParameterf, "TexParameterf");
	loader->load(TexParameterfv, "TexParameterfv");
	loader->load(TexParameteri, "TexParameteri");
	loader->load(TexParameteriv, "TexParameteriv");
	loader->load(TexSubImage2D, "TexSubImage2D");
	loader->load(Uniform1f, "Uniform1f");
	loader->load(Uniform1fv, "Uniform1fv");
	loader->load(Uniform1i, "Uniform1i");
	loader->load(Uniform1iv, "Uniform1iv");
	loader->load(Uniform2f, "Uniform2f");
	loader->load(Uniform2fv, "Uniform2fv");
	loader->load(Uniform2i, "Uniform2i");
	loader->load(Uniform2iv, "Uniform2iv");
	loader->load(Uniform3f, "Uniform3f");
	loader->load(Uniform3fv, "Uniform3fv");
	loader->load(Uniform3i, "Uniform3i");
	loader->load(Uniform3iv, "Uniform3iv");
	loader->load(Uniform4f, "Uniform4f");
	loader->load(Uniform4fv, "Uniform4fv");
	loader->load(Uniform4i, "Uniform4i");
	loader->load(Uniform4iv, "Uniform4iv");
	loader->load(UniformMatrix2fv, "UniformMatrix2fv");
	loader->load(UniformMatrix3fv, "UniformMatrix3fv");
	loader->load(UniformMatrix4fv, "UniformMatrix4fv");
	loader->load(UseProgram, "UseProgram");
	loader->load(ValidateProgram, "ValidateProgram");
	loader->load(VertexAttrib1f, "VertexAttrib1f");
	loader->load(VertexAttrib1fv, "VertexAttrib1fv");
	loader->load(VertexAttrib2f, "VertexAttrib2f");
	loader->load(VertexAttrib2fv, "VertexAttrib2fv");
	loader->load(VertexAttrib3f, "VertexAttrib3f");
	loader->load(VertexAttrib3fv, "VertexAttrib3fv");
	loader->load(VertexAttrib4f, "VertexAttrib4f");
	loader->load(VertexAttrib4fv, "VertexAttrib4fv");
	loader->load(VertexAttribPointer, "VertexAttribPointer");
	loader->load(Viewport, "Viewport");

	loader->load(ReadBuffer, "ReadBuffer");
	loader->load(DrawRangeElements, "DrawRangeElements");
	loader->load(TexImage3D, "TexImage3D");
	loader->load(TexSubImage3D, "TexSubImage3D");
	loader->load(CopyTexSubImage3D, "CopyTexSubImage3D");
	loader->load(CompressedTexImage3D, "CompressedTexImage3D");
	loader->load(CompressedTexSubImage3D, "CompressedTexSubImage3D");
	loader->load(GenQueries, "GenQueries");
	loader->load(DeleteQueries, "DeleteQueries");
	loader->load(IsQuery, "IsQuery");
	loader->load(BeginQuery, "BeginQuery");
	loader->load(EndQuery, "EndQuery");
	loader->load(GetQueryiv, "GetQueryiv");
	loader->load(GetQueryObjectuiv, "GetQueryObjectuiv");
	loader->load(UnmapBuffer, "UnmapBuffer");
	loader->load(GetBufferPointerv, "GetBufferPointerv");
	loader->load(DrawBuffers, "DrawBuffers");
	loader->load(UniformMatrix2x3fv, "UniformMatrix2x3fv");
	loader->load(UniformMatrix3x2fv, "UniformMatrix3x2fv");
	loader->load(UniformMatrix2x4fv, "UniformMatrix2x4fv");
	loader->load(UniformMatrix4x2fv, "UniformMatrix4x2fv");
	loader->load(UniformMatrix3x4fv, "UniformMatrix3x4fv");
	loader->load(UniformMatrix4x3fv, "UniformMatrix4x3fv");
	loader->load(BlitFramebuffer, "BlitFramebuffer");
	loader->load(RenderbufferStorageMultisample, "RenderbufferStorageMultisample");
	loader->load(FramebufferTextureLayer, "FramebufferTextureLayer");
	loader->load(MapBufferRange, "MapBufferRange");
	loader->load(FlushMappedBufferRange, "FlushMappedBufferRange");
	loader->load(BindVertexArray, "BindVertexArray");
	loader->load(DeleteVertexArrays, "DeleteVertexArrays");
	loader->load(GenVertexArrays, "GenVertexArrays");
	loader->load(IsVertexArray, "IsVertexArray");
	loader->load(GetIntegeri_v, "GetIntegeri_v");
	loader->load(BeginTransformFeedback, "BeginTransformFeedback");
	loader->load(EndTransformFeedback, "EndTransformFeedback");
	loader->load(BindBufferRange, "BindBufferRange");
	loader->load(BindBufferBase, "BindBufferBase");
	loader->load(TransformFeedbackVaryings, "TransformFeedbackVaryings");
	loader->load(GetTransformFeedbackVarying, "GetTransformFeedbackVarying");
	loader->load(VertexAttribIPointer, "VertexAttribIPointer");
	loader->load(GetVertexAttribIiv, "GetVertexAttribIiv");
	loader->load(GetVertexAttribIuiv, "GetVertexAttribIuiv");
	loader->load(VertexAttribI4i, "VertexAttribI4i");
	loader->load(VertexAttribI4ui, "VertexAttribI4ui");
	loader->load(VertexAttribI4iv, "VertexAttribI4iv");
	loader->load(VertexAttribI4uiv, "VertexAttribI4uiv");
	loader->load(GetUniformuiv, "GetUniformuiv");
	loader->load(GetFragDataLocation, "GetFragDataLocation");
	loader->load(Uniform1ui, "Uniform1ui");
	loader->load(Uniform2ui, "Uniform2ui");
	loader->load(Uniform3ui, "Uniform3ui");
	loader->load(Uniform4ui, "Uniform4ui");
	loader->load(Uniform1uiv, "Uniform1uiv");
	loader->load(Uniform2uiv, "Uniform2uiv");
	loader->load(Uniform3uiv, "Uniform3uiv");
	loader->load(Uniform4uiv, "Uniform4uiv");
	loader->load(ClearBufferiv, "ClearBufferiv");
	loader->load(ClearBufferuiv, "ClearBufferuiv");
	loader->load(ClearBufferfv, "ClearBufferfv");
	loader->load(ClearBufferfi, "ClearBufferfi");
	loader->load(GetStringi, "GetStringi");
	loader->load(CopyBufferSubData, "CopyBufferSubData");
	loader->load(GetUniformIndices, "GetUniformIndices");
	loader->load(GetActiveUniformsiv, "GetActiveUniformsiv");

	loader->load(GetUniformBlockIndex, "GetUniformBlockIndex");
	loader->load(GetActiveUniformBlockiv, "GetActiveUniformBlockiv");
	loader->load(GetActiveUniformBlockName, "GetActiveUniformBlockName");
	loader->load(UniformBlockBinding, "UniformBlockBinding");
	loader->load(DrawArraysInstanced, "DrawArraysInstanced");
	loader->load(DrawElementsInstanced, "DrawElementsInstanced");
	loader->load(FenceSync, "FenceSync");
	loader->load(IsSync, "IsSync");
	loader->load(DeleteSync, "DeleteSync");
	loader->load(ClientWaitSync, "ClientWaitSync");
	loader->load(WaitSync, "WaitSync");
	loader->load(GetInteger64v, "GetInteger64v");
	loader->load(GetSynciv, "GetSynciv");
	loader->load(GetInteger64i_v, "GetInteger64i_v");
	loader->load(GetBufferParameteri64v, "GetBufferParameteri64v");
	loader->load(GenSamplers, "GenSamplers");
	loader->load(DeleteSamplers, "DeleteSamplers");
	loader->load(IsSampler, "IsSampler");
	loader->load(BindSampler, "BindSampler");
	loader->load(SamplerParameteri, "SamplerParameteri");
	loader->load(SamplerParameteriv, "SamplerParameteriv");
	loader->load(SamplerParameterf, "SamplerParameterf");
	loader->load(SamplerParameterfv, "SamplerParameterfv");
	loader->load(GetSamplerParameteriv, "GetSamplerParameteriv");
	loader->load(GetSamplerParameterfv, "GetSamplerParameterfv");
	loader->load(VertexAttribDivisor, "VertexAttribDivisor");
	loader->load(BindTransformFeedback, "BindTransformFeedback");
	loader->load(DeleteTransformFeedbacks, "DeleteTransformFeedbacks");
	loader->load(GenTransformFeedbacks, "GenTransformFeedbacks");
	loader->load(IsTransformFeedback, "IsTransformFeedback");
	loader->load(PauseTransformFeedback, "PauseTransformFeedback");
	loader->load(ResumeTransformFeedback, "ResumeTransformFeedback");
	loader->load(GetProgramBinary, "GetProgramBinary");
	loader->load(ProgramBinary, "ProgramBinary");
	loader->load(ProgramParameteri, "ProgramParameteri");
	loader->load(InvalidateFramebuffer, "InvalidateFramebuffer");
	loader->load(InvalidateSubFramebuffer, "InvalidateSubFramebuffer");
	loader->load(TexStorage2D, "TexStorage2D");
	loader->load(TexStorage3D, "TexStorage3D");
	loader->load(GetInternalformativ, "GetInternalformativ");

	this->VersionString = reinterpret_cast<const char *>(this->GetString(VERSION));
	this->VendorString = reinterpret_cast<const char *>(this->GetString(VENDOR));
	this->RendererString = reinterpret_cast<const char *>(this->GetString(RENDERER));
	this->ExtensionString = reinterpret_cast<const char *>(this->GetString(EXTENSIONS));

	GLint extension_count = 0;
	this->GetIntegerv(NUM_EXTENSIONS, &extension_count);

	for (int i = 0; i < extension_count; i++)
	{
		this->Extensions.insert(reinterpret_cast<const char *>(this->GetStringi(EXTENSIONS, i)));
	}

	this->KHR_debug = {this};
}

Gles3::~Gles3() { delete this->loader; }

Gles3::KhrDebug::KhrDebug(const Gles3 *parent) : supported(false), name("GL_KHR_debug")
{
	if (!parent)
		return;
	if (parent->Extensions.count(name))
	{
		LogInfo("Extension {} supported", name);
		this->supported = true;
	}
	else
	{
		LogInfo("Extension {} not supported", name);
		return;
	}
	std::string func_suffix;

	// According to the KHR_debug spec:
	//"""
	//    NOTE: when implemented in an OpenGL ES context, all entry points defined
	//    by this extension must have a "KHR" suffix. When implemented in an
	//    OpenGL context, all entry points must have NO suffix, as shown below.
	//"""
	if (parent->desktop_extension == false)
	{
		func_suffix = "KHR";
	}
	parent->loader->load(DebugMessageControl, "DebugMessageControl" + func_suffix);
	parent->loader->load(DebugMessageInsert, "DebugMessageInsert" + func_suffix);
	parent->loader->load(DebugMessageCallback, "DebugMessageCallback" + func_suffix);
	parent->loader->load(GetDebugMessageLog, "GetDebugMessageLog" + func_suffix);
	parent->loader->load(GetPointerv, "GetPointerv" + func_suffix);
	parent->loader->load(PushDebugGroup, "PushDebugGroup" + func_suffix);
	parent->loader->load(ObjectLabel, "ObjectLabel" + func_suffix);
	parent->loader->load(GetObjectLabel, "GetObjectLabel" + func_suffix);
	parent->loader->load(ObjectPtrLabel, "ObjectPtrLabel" + func_suffix);
	parent->loader->load(GetObjectPtrLabel, "GetObjectPtrLabel" + func_suffix);
}

} // namespace gles_wrap
