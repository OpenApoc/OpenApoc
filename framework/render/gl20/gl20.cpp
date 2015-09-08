#include "framework/render/gl20/gl20.h"

#ifdef _WIN32
#include "framework/render/gl20/win_gllib.inl"
#else
#include "framework/render/gl20/linux_gllib.inl"
#endif

namespace OpenApoc
{

#define LOAD_FN(name, member)                                                                      \
	if (!lib->load(name, member))                                                                  \
	{                                                                                              \
		LogWarning("Failed to load GL function gl%s", name);                                       \
		return;                                                                                    \
	}

GL20::GL20() : lib(new GLLib), loadedSuccessfully(false)
{
	if (!lib->loaded)
	{
		LogError("Failed to load library");
		return;
	}
	LOAD_FN("GetString", GetString);

	this->vendorString = (const char *)this->GetString(VENDOR);
	this->rendererString = (const char *)this->GetString(RENDERER);
	this->versionString = (const char *)this->GetString(VERSION);
	this->extensionsString = (const char *)this->GetString(EXTENSIONS);

	LogInfo("GL vendor: \"%s\"", this->vendorString.str().c_str());
	LogInfo("GL renderer: \"%s\"", this->rendererString.str().c_str());
	LogInfo("GL version: \"%s\"", this->versionString.str().c_str());
	LogInfo("GL extensions: \"%s\"", this->extensionsString.str().c_str());

	/* Parse extension string */

	auto spaceSplitVersion = versionString.split(" ");
	if (spaceSplitVersion.size() < 1)
	{
		LogWarning("version string \"%s\" failed to parse", this->versionString.str().c_str());
		return;
	}

	auto pointSplitVersion = versionString.split(".");
	if (pointSplitVersion.size() < 2)
	{
		LogWarning("version string \"%s\" failed to parse", this->versionString.str().c_str());
		return;
	}

	for (auto &num : pointSplitVersion)
	{
		if (!Strings::IsNumeric(num))
		{
			LogWarning("version string \"%s\" failed to parse", this->versionString.str().c_str());
			return;
		}
		this->version.push_back(Strings::ToInteger(num));
	}

	if (version[0] < 2)
	{
		LogWarning("Version \"%s\" too old - need at least 2.0", this->versionString.str().c_str());
		return;
	}

	/* Parse extensions */
	auto splitExtensions = extensionsString.split(" ");
	for (auto &ext : splitExtensions)
	{
		if (ext == "")
			continue;
		this->driverExtensions.insert(ext);
	}

	if (this->driverExtensions.find("GL_ARB_framebuffer_object") == this->driverExtensions.end())
	{
		LogWarning("Required extension \"GL_ARB_framebuffer_object\" missing");
		return;
	}

	LOAD_FN("AttachShader", AttachShader);
	LOAD_FN("ActiveTexture", ActiveTexture);
	LOAD_FN("BindFramebuffer", BindFramebuffer);
	LOAD_FN("BindTexture", BindTexture);
	LOAD_FN("BlendFunc", BlendFunc);
	LOAD_FN("CheckFramebufferStatus", CheckFramebufferStatus);
	LOAD_FN("Clear", Clear);
	LOAD_FN("ClearColor", ClearColor);
	LOAD_FN("CompileShader", CompileShader);
	LOAD_FN("CreateProgram", CreateProgram);
	LOAD_FN("CreateShader", CreateShader);
	LOAD_FN("DeleteFramebuffers", DeleteFramebuffers);
	LOAD_FN("DeleteProgram", DeleteProgram);
	LOAD_FN("DeleteShader", DeleteShader);
	LOAD_FN("DeleteTextures", DeleteTextures);
	LOAD_FN("DrawArrays", DrawArrays);
	LOAD_FN("EnableVertexAttribArray", EnableVertexAttribArray);
	LOAD_FN("Enable", Enable);
	LOAD_FN("FramebufferTexture2D", FramebufferTexture2D);
	LOAD_FN("GenFramebuffers", GenFramebuffers);
	LOAD_FN("GenTextures", GenTextures);
	LOAD_FN("GetAttribLocation", GetAttribLocation);
	LOAD_FN("GetIntegerv", GetIntegerv);
	LOAD_FN("GetProgramInfoLog", GetProgramInfoLog);
	LOAD_FN("GetProgramiv", GetProgramiv);
	LOAD_FN("GetShaderInfoLog", GetShaderInfoLog);
	LOAD_FN("GetShaderiv", GetShaderiv);
	// LOAD_FN("GetString", GetString); - GetString is loaded first to check version
	LOAD_FN("GetTexParameteriv", GetTexParameteriv);
	LOAD_FN("GetUniformLocation", GetUniformLocation);
	LOAD_FN("LinkProgram", LinkProgram);
	LOAD_FN("PixelStorei", PixelStorei);
	LOAD_FN("ShaderSource", ShaderSource);
	LOAD_FN("TexImage1D", TexImage1D);
	LOAD_FN("TexImage2D", TexImage2D);
	LOAD_FN("TexParameteri", TexParameteri);
	LOAD_FN("Uniform1i", Uniform1i);
	LOAD_FN("Uniform1f", Uniform1f);
	LOAD_FN("Uniform2f", Uniform2f);
	LOAD_FN("UseProgram", UseProgram);
	LOAD_FN("VertexAttribPointer", VertexAttribPointer);
	LOAD_FN("Viewport", Viewport);

	LogWarning("Successfully loaded GL20 entrypoints");
	this->loadedSuccessfully = true;
}

GL20::~GL20() {}

}; // namespace OpenApoc
