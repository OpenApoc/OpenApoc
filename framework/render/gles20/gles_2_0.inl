#include "gles_2_0.hpp"

#if defined(__APPLE__)
#include <mach-o/dyld.h>

static void *AppleGLGetProcAddress(const GLubyte *name)
{
	static const struct mach_header *image = NULL;
	NSSymbol symbol;
	char *symbolName;
	if (NULL == image)
	{
		image = NSAddImage("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL",
			NSADDIMAGE_OPTION_RETURN_ON_ERROR);
	}
	/* prepend a '_' for the Unix C symbol mangling convention */
	symbolName = malloc(strlen((const char *)name) + 2);
	strcpy(symbolName + 1, (const char *)name);
	symbolName[0] = '_';
	symbol = NULL;
	/* if (NSIsSymbolNameDefined(symbolName))
	symbol = NSLookupAndBindSymbol(symbolName); */
	symbol = image ? NSLookupSymbolInImage(image, symbolName,
		NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
		NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR)
		: NULL;
	free(symbolName);
	return symbol ? NSAddressOfSymbol(symbol) : NULL;
}
#endif /* __APPLE__ */

#if defined(__sgi) || defined(__sun)
#include <dlfcn.h>
#include <stdio.h>

static void *SunGetProcAddress(const GLubyte *name)
{
	static void *h = NULL;
	static void *gpa;

	if (h == NULL)
	{
		if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL)
			return NULL;
		gpa = dlsym(h, "glXGetProcAddress");
	}

	if (gpa != NULL)
		return ((void *(*)(const GLubyte *))gpa)(name);
	else
		return dlsym(h, (const char *)name);
}
#endif /* __sgi || __sun */

#if defined(_WIN32)

#ifdef _MSC_VER
#pragma warning(disable : 4055)
#pragma warning(disable : 4054)
#endif

#define WIN_GLES2_LIBRARY_NAME "libGLESv2.dll"

static int TestPointer(const PROC pTest)
{
	ptrdiff_t iTest;
	if (!pTest)
		return 0;
	iTest = (ptrdiff_t)pTest;

	if (iTest == 1 || iTest == 2 || iTest == 3 || iTest == -1)
		return 0;

	return 1;
}

static PROC WinGetProcAddress(const char *name)
{
	HMODULE glMod = NULL;
	// GLES2.0: Don't try to use wglGetProcAddress, just try libGLESv2.dll
	/*
	PROC pFunc = wglGetProcAddress((LPCSTR)name);
	if (TestPointer(pFunc))
	{
		return pFunc;
	}
	*/
	//glMod = GetModuleHandleA(WIN_GLES2_LIBRARY_NAME);
	glMod = LoadLibraryA(WIN_GLES2_LIBRARY_NAME);
	if (!glMod)
		return nullptr;
	return (PROC)GetProcAddress(glMod, (LPCSTR)name);
}

#define IntGetProcAddress(name) WinGetProcAddress(name)

#undef WIN_GLES2_LIBRARY_NAME 
#else
#if defined(__APPLE__)
#define IntGetProcAddress(name) AppleGLGetProcAddress(name)
#else
#if defined(__sgi) || defined(__sun)
#define IntGetProcAddress(name) SunGetProcAddress(name)
#else
#if defined(__ANDROID__)
#include <dlfcn.h>
static void* AndroidGetProcAddress(const char* name)
{
	static void* glesLibHandle = NULL;

	if (!glesLibHandle)
	{
		glesLibHandle = dlopen("libGLESv2.so", RTLD_LAZY);
	}

	return dlsym(glesLibHandle, name);
}

#define IntGetProcAddress(name) AndroidGetProcAddress(name)

#else /* GLX */
#include <GL/glx.h>
#define IntGetProcAddress(name) (*glXGetProcAddressARB)(reinterpret_cast<const GLubyte *>(name))
#endif
#endif
#endif
#endif

namespace gl
{

typedef void (CODEGEN_FUNCPTR *PFNACTIVETEXTURE)(GLenum texture);
PFNACTIVETEXTURE ActiveTexture = 0;
typedef void (CODEGEN_FUNCPTR *PFNATTACHSHADER)(GLuint program, GLuint shader);
PFNATTACHSHADER AttachShader = 0; typedef void (CODEGEN_FUNCPTR *PFNBINDATTRIBLOCATION)(GLuint program, GLuint index, const GLchar* name);
PFNBINDATTRIBLOCATION BindAttribLocation = 0; typedef void (CODEGEN_FUNCPTR *PFNBINDBUFFER)(GLenum target, GLuint buffer);
PFNBINDBUFFER BindBuffer = 0; typedef void (CODEGEN_FUNCPTR *PFNBINDFRAMEBUFFER)(GLenum target, GLuint framebuffer);
PFNBINDFRAMEBUFFER BindFramebuffer = 0; typedef void (CODEGEN_FUNCPTR *PFNBINDRENDERBUFFER)(GLenum target, GLuint renderbuffer);
PFNBINDRENDERBUFFER BindRenderbuffer = 0; typedef void (CODEGEN_FUNCPTR *PFNBINDTEXTURE)(GLenum target, GLuint texture);
PFNBINDTEXTURE BindTexture = 0; typedef void (CODEGEN_FUNCPTR *PFNBLENDCOLOR)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
PFNBLENDCOLOR BlendColor = 0; typedef void (CODEGEN_FUNCPTR *PFNBLENDEQUATION)(GLenum mode);
PFNBLENDEQUATION BlendEquation = 0; typedef void (CODEGEN_FUNCPTR *PFNBLENDEQUATIONSEPARATE)(GLenum modeRGB, GLenum modeAlpha);
PFNBLENDEQUATIONSEPARATE BlendEquationSeparate = 0; typedef void (CODEGEN_FUNCPTR *PFNBLENDFUNC)(GLenum sfactor, GLenum dfactor);
PFNBLENDFUNC BlendFunc = 0; typedef void (CODEGEN_FUNCPTR *PFNBLENDFUNCSEPARATE)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
PFNBLENDFUNCSEPARATE BlendFuncSeparate = 0; typedef void (CODEGEN_FUNCPTR *PFNBUFFERDATA)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
PFNBUFFERDATA BufferData = 0; typedef void (CODEGEN_FUNCPTR *PFNBUFFERSUBDATA)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
PFNBUFFERSUBDATA BufferSubData = 0; typedef GLenum(CODEGEN_FUNCPTR *PFNCHECKFRAMEBUFFERSTATUS)(GLenum target);
PFNCHECKFRAMEBUFFERSTATUS CheckFramebufferStatus = 0; typedef void (CODEGEN_FUNCPTR *PFNCLEAR)(GLbitfield mask);
PFNCLEAR Clear = 0; typedef void (CODEGEN_FUNCPTR *PFNCLEARCOLOR)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
PFNCLEARCOLOR ClearColor = 0; typedef void (CODEGEN_FUNCPTR *PFNCLEARDEPTHF)(GLclampf depth);
PFNCLEARDEPTHF ClearDepthf = 0; typedef void (CODEGEN_FUNCPTR *PFNCLEARSTENCIL)(GLint s);
PFNCLEARSTENCIL ClearStencil = 0; typedef void (CODEGEN_FUNCPTR *PFNCOLORMASK)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
PFNCOLORMASK ColorMask = 0; typedef void (CODEGEN_FUNCPTR *PFNCOMPILESHADER)(GLuint shader);
PFNCOMPILESHADER CompileShader = 0; typedef void (CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXIMAGE2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
PFNCOMPRESSEDTEXIMAGE2D CompressedTexImage2D = 0; typedef void (CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXSUBIMAGE2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
PFNCOMPRESSEDTEXSUBIMAGE2D CompressedTexSubImage2D = 0; typedef void (CODEGEN_FUNCPTR *PFNCOPYTEXIMAGE2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
PFNCOPYTEXIMAGE2D CopyTexImage2D = 0; typedef void (CODEGEN_FUNCPTR *PFNCOPYTEXSUBIMAGE2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
PFNCOPYTEXSUBIMAGE2D CopyTexSubImage2D = 0; typedef GLuint(CODEGEN_FUNCPTR *PFNCREATEPROGRAM)(void);
PFNCREATEPROGRAM CreateProgram = 0; typedef GLuint(CODEGEN_FUNCPTR *PFNCREATESHADER)(GLenum type);
PFNCREATESHADER CreateShader = 0; typedef void (CODEGEN_FUNCPTR *PFNCULLFACE)(GLenum mode);
PFNCULLFACE CullFace = 0; typedef void (CODEGEN_FUNCPTR *PFNDELETEBUFFERS)(GLsizei n, const GLuint* buffers);
PFNDELETEBUFFERS DeleteBuffers = 0; typedef void (CODEGEN_FUNCPTR *PFNDELETEFRAMEBUFFERS)(GLsizei n, const GLuint* framebuffers);
PFNDELETEFRAMEBUFFERS DeleteFramebuffers = 0; typedef void (CODEGEN_FUNCPTR *PFNDELETEPROGRAM)(GLuint program);
PFNDELETEPROGRAM DeleteProgram = 0; typedef void (CODEGEN_FUNCPTR *PFNDELETERENDERBUFFERS)(GLsizei n, const GLuint* renderbuffers);
PFNDELETERENDERBUFFERS DeleteRenderbuffers = 0; typedef void (CODEGEN_FUNCPTR *PFNDELETESHADER)(GLuint shader);
PFNDELETESHADER DeleteShader = 0; typedef void (CODEGEN_FUNCPTR *PFNDELETETEXTURES)(GLsizei n, const GLuint* textures);
PFNDELETETEXTURES DeleteTextures = 0; typedef void (CODEGEN_FUNCPTR *PFNDEPTHFUNC)(GLenum func);
PFNDEPTHFUNC DepthFunc = 0; typedef void (CODEGEN_FUNCPTR *PFNDEPTHMASK)(GLboolean flag);
PFNDEPTHMASK DepthMask = 0; typedef void (CODEGEN_FUNCPTR *PFNDEPTHRANGEF)(GLclampf zNear, GLclampf zFar);
PFNDEPTHRANGEF DepthRangef = 0; typedef void (CODEGEN_FUNCPTR *PFNDETACHSHADER)(GLuint program, GLuint shader);
PFNDETACHSHADER DetachShader = 0; typedef void (CODEGEN_FUNCPTR *PFNDISABLE)(GLenum cap);
PFNDISABLE Disable = 0; typedef void (CODEGEN_FUNCPTR *PFNDISABLEVERTEXATTRIBARRAY)(GLuint index);
PFNDISABLEVERTEXATTRIBARRAY DisableVertexAttribArray = 0; typedef void (CODEGEN_FUNCPTR *PFNDRAWARRAYS)(GLenum mode, GLint first, GLsizei count);
PFNDRAWARRAYS DrawArrays = 0; typedef void (CODEGEN_FUNCPTR *PFNDRAWELEMENTS)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
PFNDRAWELEMENTS DrawElements = 0; typedef void (CODEGEN_FUNCPTR *PFNENABLE)(GLenum cap);
PFNENABLE Enable = 0; typedef void (CODEGEN_FUNCPTR *PFNENABLEVERTEXATTRIBARRAY)(GLuint index);
PFNENABLEVERTEXATTRIBARRAY EnableVertexAttribArray = 0; typedef void (CODEGEN_FUNCPTR *PFNFINISH)(void);
PFNFINISH Finish = 0; typedef void (CODEGEN_FUNCPTR *PFNFLUSH)(void);
PFNFLUSH Flush = 0; typedef void (CODEGEN_FUNCPTR *PFNFRAMEBUFFERRENDERBUFFER)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
PFNFRAMEBUFFERRENDERBUFFER FramebufferRenderbuffer = 0; typedef void (CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURE2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PFNFRAMEBUFFERTEXTURE2D FramebufferTexture2D = 0; typedef void (CODEGEN_FUNCPTR *PFNFRONTFACE)(GLenum mode);
PFNFRONTFACE FrontFace = 0; typedef void (CODEGEN_FUNCPTR *PFNGENBUFFERS)(GLsizei n, GLuint* buffers);
PFNGENBUFFERS GenBuffers = 0; typedef void (CODEGEN_FUNCPTR *PFNGENERATEMIPMAP)(GLenum target);
PFNGENERATEMIPMAP GenerateMipmap = 0; typedef void (CODEGEN_FUNCPTR *PFNGENFRAMEBUFFERS)(GLsizei n, GLuint* framebuffers);
PFNGENFRAMEBUFFERS GenFramebuffers = 0; typedef void (CODEGEN_FUNCPTR *PFNGENRENDERBUFFERS)(GLsizei n, GLuint* renderbuffers);
PFNGENRENDERBUFFERS GenRenderbuffers = 0; typedef void (CODEGEN_FUNCPTR *PFNGENTEXTURES)(GLsizei n, GLuint* textures);
PFNGENTEXTURES GenTextures = 0; typedef void (CODEGEN_FUNCPTR *PFNGETACTIVEATTRIB)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
PFNGETACTIVEATTRIB GetActiveAttrib = 0; typedef void (CODEGEN_FUNCPTR *PFNGETACTIVEUNIFORM)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
PFNGETACTIVEUNIFORM GetActiveUniform = 0; typedef void (CODEGEN_FUNCPTR *PFNGETATTACHEDSHADERS)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
PFNGETATTACHEDSHADERS GetAttachedShaders = 0; typedef GLint(CODEGEN_FUNCPTR *PFNGETATTRIBLOCATION)(GLuint program, const GLchar* name);
PFNGETATTRIBLOCATION GetAttribLocation = 0; typedef void (CODEGEN_FUNCPTR *PFNGETBOOLEANV)(GLenum pname, GLboolean* params);
PFNGETBOOLEANV GetBooleanv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETBUFFERPARAMETERIV)(GLenum target, GLenum pname, GLint* params);
PFNGETBUFFERPARAMETERIV GetBufferParameteriv = 0; typedef GLenum(CODEGEN_FUNCPTR *PFNGETERROR)(void);
PFNGETERROR GetError = 0; typedef void (CODEGEN_FUNCPTR *PFNGETFLOATV)(GLenum pname, GLfloat* params);
PFNGETFLOATV GetFloatv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETFRAMEBUFFERATTACHMENTPARAMETERIV)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
PFNGETFRAMEBUFFERATTACHMENTPARAMETERIV GetFramebufferAttachmentParameteriv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETINTEGERV)(GLenum pname, GLint* params);
PFNGETINTEGERV GetIntegerv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETPROGRAMIV)(GLuint program, GLenum pname, GLint* params);
PFNGETPROGRAMIV GetProgramiv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETPROGRAMINFOLOG)(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
PFNGETPROGRAMINFOLOG GetProgramInfoLog = 0; typedef void (CODEGEN_FUNCPTR *PFNGETRENDERBUFFERPARAMETERIV)(GLenum target, GLenum pname, GLint* params);
PFNGETRENDERBUFFERPARAMETERIV GetRenderbufferParameteriv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETSHADERIV)(GLuint shader, GLenum pname, GLint* params);
PFNGETSHADERIV GetShaderiv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETSHADERINFOLOG)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
PFNGETSHADERINFOLOG GetShaderInfoLog = 0; typedef void (CODEGEN_FUNCPTR *PFNGETSHADERPRECISIONFORMAT)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
PFNGETSHADERPRECISIONFORMAT GetShaderPrecisionFormat = 0; typedef void (CODEGEN_FUNCPTR *PFNGETSHADERSOURCE)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
PFNGETSHADERSOURCE GetShaderSource = 0; typedef const GLubyte* (CODEGEN_FUNCPTR *PFNGETSTRING)(GLenum name);
PFNGETSTRING GetString = 0; typedef void (CODEGEN_FUNCPTR *PFNGETTEXPARAMETERFV)(GLenum target, GLenum pname, GLfloat* params);
PFNGETTEXPARAMETERFV GetTexParameterfv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETTEXPARAMETERIV)(GLenum target, GLenum pname, GLint* params);
PFNGETTEXPARAMETERIV GetTexParameteriv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETUNIFORMFV)(GLuint program, GLint location, GLfloat* params);
PFNGETUNIFORMFV GetUniformfv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETUNIFORMIV)(GLuint program, GLint location, GLint* params);
PFNGETUNIFORMIV GetUniformiv = 0; typedef GLint(CODEGEN_FUNCPTR *PFNGETUNIFORMLOCATION)(GLuint program, const GLchar* name);
PFNGETUNIFORMLOCATION GetUniformLocation = 0; typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBFV)(GLuint index, GLenum pname, GLfloat* params);
PFNGETVERTEXATTRIBFV GetVertexAttribfv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBIV)(GLuint index, GLenum pname, GLint* params);
PFNGETVERTEXATTRIBIV GetVertexAttribiv = 0; typedef void (CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBPOINTERV)(GLuint index, GLenum pname, GLvoid** pointer);
PFNGETVERTEXATTRIBPOINTERV GetVertexAttribPointerv = 0; typedef void (CODEGEN_FUNCPTR *PFNHINT)(GLenum target, GLenum mode);
PFNHINT Hint = 0; typedef GLboolean(CODEGEN_FUNCPTR *PFNISBUFFER)(GLuint buffer);
PFNISBUFFER IsBuffer = 0; typedef GLboolean(CODEGEN_FUNCPTR *PFNISENABLED)(GLenum cap);
PFNISENABLED IsEnabled = 0; typedef GLboolean(CODEGEN_FUNCPTR *PFNISFRAMEBUFFER)(GLuint framebuffer);
PFNISFRAMEBUFFER IsFramebuffer = 0; typedef GLboolean(CODEGEN_FUNCPTR *PFNISPROGRAM)(GLuint program);
PFNISPROGRAM IsProgram = 0; typedef GLboolean(CODEGEN_FUNCPTR *PFNISRENDERBUFFER)(GLuint renderbuffer);
PFNISRENDERBUFFER IsRenderbuffer = 0; typedef GLboolean(CODEGEN_FUNCPTR *PFNISSHADER)(GLuint shader);
PFNISSHADER IsShader = 0; typedef GLboolean(CODEGEN_FUNCPTR *PFNISTEXTURE)(GLuint texture);
PFNISTEXTURE IsTexture = 0; typedef void (CODEGEN_FUNCPTR *PFNLINEWIDTH)(GLfloat width);
PFNLINEWIDTH LineWidth = 0; typedef void (CODEGEN_FUNCPTR *PFNLINKPROGRAM)(GLuint program);
PFNLINKPROGRAM LinkProgram = 0; typedef void (CODEGEN_FUNCPTR *PFNPIXELSTOREI)(GLenum pname, GLint param);
PFNPIXELSTOREI PixelStorei = 0; typedef void (CODEGEN_FUNCPTR *PFNPOLYGONOFFSET)(GLfloat factor, GLfloat units);
PFNPOLYGONOFFSET PolygonOffset = 0; typedef void (CODEGEN_FUNCPTR *PFNREADPIXELS)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
PFNREADPIXELS ReadPixels = 0; typedef void (CODEGEN_FUNCPTR *PFNRELEASESHADERCOMPILER)(void);
PFNRELEASESHADERCOMPILER ReleaseShaderCompiler = 0; typedef void (CODEGEN_FUNCPTR *PFNRENDERBUFFERSTORAGE)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
PFNRENDERBUFFERSTORAGE RenderbufferStorage = 0; typedef void (CODEGEN_FUNCPTR *PFNSAMPLECOVERAGE)(GLclampf value, GLboolean invert);
PFNSAMPLECOVERAGE SampleCoverage = 0; typedef void (CODEGEN_FUNCPTR *PFNSCISSOR)(GLint x, GLint y, GLsizei width, GLsizei height);
PFNSCISSOR Scissor = 0; typedef void (CODEGEN_FUNCPTR *PFNSHADERBINARY)(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
PFNSHADERBINARY ShaderBinary = 0; typedef void (CODEGEN_FUNCPTR *PFNSHADERSOURCE)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
PFNSHADERSOURCE ShaderSource = 0; typedef void (CODEGEN_FUNCPTR *PFNSTENCILFUNC)(GLenum func, GLint ref, GLuint mask);
PFNSTENCILFUNC StencilFunc = 0; typedef void (CODEGEN_FUNCPTR *PFNSTENCILFUNCSEPARATE)(GLenum face, GLenum func, GLint ref, GLuint mask);
PFNSTENCILFUNCSEPARATE StencilFuncSeparate = 0; typedef void (CODEGEN_FUNCPTR *PFNSTENCILMASK)(GLuint mask);
PFNSTENCILMASK StencilMask = 0; typedef void (CODEGEN_FUNCPTR *PFNSTENCILMASKSEPARATE)(GLenum face, GLuint mask);
PFNSTENCILMASKSEPARATE StencilMaskSeparate = 0; typedef void (CODEGEN_FUNCPTR *PFNSTENCILOP)(GLenum fail, GLenum zfail, GLenum zpass);
PFNSTENCILOP StencilOp = 0; typedef void (CODEGEN_FUNCPTR *PFNSTENCILOPSEPARATE)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
PFNSTENCILOPSEPARATE StencilOpSeparate = 0; typedef void (CODEGEN_FUNCPTR *PFNTEXIMAGE2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
PFNTEXIMAGE2D TexImage2D = 0; typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERF)(GLenum target, GLenum pname, GLfloat param);
PFNTEXPARAMETERF TexParameterf = 0; typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERFV)(GLenum target, GLenum pname, const GLfloat* params);
PFNTEXPARAMETERFV TexParameterfv = 0; typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERI)(GLenum target, GLenum pname, GLint param);
PFNTEXPARAMETERI TexParameteri = 0; typedef void (CODEGEN_FUNCPTR *PFNTEXPARAMETERIV)(GLenum target, GLenum pname, const GLint* params);
PFNTEXPARAMETERIV TexParameteriv = 0; typedef void (CODEGEN_FUNCPTR *PFNTEXSUBIMAGE2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
PFNTEXSUBIMAGE2D TexSubImage2D = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1F)(GLint location, GLfloat x);
PFNUNIFORM1F Uniform1f = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1FV)(GLint location, GLsizei count, const GLfloat* v);
PFNUNIFORM1FV Uniform1fv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1I)(GLint location, GLint x);
PFNUNIFORM1I Uniform1i = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM1IV)(GLint location, GLsizei count, const GLint* v);
PFNUNIFORM1IV Uniform1iv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2F)(GLint location, GLfloat x, GLfloat y);
PFNUNIFORM2F Uniform2f = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2FV)(GLint location, GLsizei count, const GLfloat* v);
PFNUNIFORM2FV Uniform2fv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2I)(GLint location, GLint x, GLint y);
PFNUNIFORM2I Uniform2i = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM2IV)(GLint location, GLsizei count, const GLint* v);
PFNUNIFORM2IV Uniform2iv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3F)(GLint location, GLfloat x, GLfloat y, GLfloat z);
PFNUNIFORM3F Uniform3f = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3FV)(GLint location, GLsizei count, const GLfloat* v);
PFNUNIFORM3FV Uniform3fv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3I)(GLint location, GLint x, GLint y, GLint z);
PFNUNIFORM3I Uniform3i = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM3IV)(GLint location, GLsizei count, const GLint* v);
PFNUNIFORM3IV Uniform3iv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4F)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
PFNUNIFORM4F Uniform4f = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4FV)(GLint location, GLsizei count, const GLfloat* v);
PFNUNIFORM4FV Uniform4fv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4I)(GLint location, GLint x, GLint y, GLint z, GLint w);
PFNUNIFORM4I Uniform4i = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORM4IV)(GLint location, GLsizei count, const GLint* v);
PFNUNIFORM4IV Uniform4iv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX2FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PFNUNIFORMMATRIX2FV UniformMatrix2fv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX3FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PFNUNIFORMMATRIX3FV UniformMatrix3fv = 0; typedef void (CODEGEN_FUNCPTR *PFNUNIFORMMATRIX4FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
PFNUNIFORMMATRIX4FV UniformMatrix4fv = 0; typedef void (CODEGEN_FUNCPTR *PFNUSEPROGRAM)(GLuint program);
PFNUSEPROGRAM UseProgram = 0; typedef void (CODEGEN_FUNCPTR *PFNVALIDATEPROGRAM)(GLuint program);
PFNVALIDATEPROGRAM ValidateProgram = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB1F)(GLuint indx, GLfloat x);
PFNVERTEXATTRIB1F VertexAttrib1f = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB1FV)(GLuint indx, const GLfloat* values);
PFNVERTEXATTRIB1FV VertexAttrib1fv = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB2F)(GLuint indx, GLfloat x, GLfloat y);
PFNVERTEXATTRIB2F VertexAttrib2f = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB2FV)(GLuint indx, const GLfloat* values);
PFNVERTEXATTRIB2FV VertexAttrib2fv = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB3F)(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
PFNVERTEXATTRIB3F VertexAttrib3f = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB3FV)(GLuint indx, const GLfloat* values);
PFNVERTEXATTRIB3FV VertexAttrib3fv = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4F)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
PFNVERTEXATTRIB4F VertexAttrib4f = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIB4FV)(GLuint indx, const GLfloat* values);
PFNVERTEXATTRIB4FV VertexAttrib4fv = 0; typedef void (CODEGEN_FUNCPTR *PFNVERTEXATTRIBPOINTER)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
PFNVERTEXATTRIBPOINTER VertexAttribPointer = 0; typedef void (CODEGEN_FUNCPTR *PFNVIEWPORT)(GLint x, GLint y, GLsizei width, GLsizei height);
PFNVIEWPORT Viewport = 0;
static int LoadCoreFunctions()
{
	int numFailed = 0;
	ActiveTexture = reinterpret_cast<PFNACTIVETEXTURE>(IntGetProcAddress("glActiveTexture"));
	if (!ActiveTexture)
		++numFailed;
	AttachShader = reinterpret_cast<PFNATTACHSHADER>(IntGetProcAddress("glAttachShader"));
	if (!AttachShader)
		++numFailed;
	BindAttribLocation = reinterpret_cast<PFNBINDATTRIBLOCATION>(IntGetProcAddress("glBindAttribLocation"));
	if (!BindAttribLocation)
		++numFailed;
	BindBuffer = reinterpret_cast<PFNBINDBUFFER>(IntGetProcAddress("glBindBuffer"));
	if (!BindBuffer)
		++numFailed;
	BindFramebuffer = reinterpret_cast<PFNBINDFRAMEBUFFER>(IntGetProcAddress("glBindFramebuffer"));
	if (!BindFramebuffer)
		++numFailed;
	BindRenderbuffer = reinterpret_cast<PFNBINDRENDERBUFFER>(IntGetProcAddress("glBindRenderbuffer"));
	if (!BindRenderbuffer)
		++numFailed;
	BindTexture = reinterpret_cast<PFNBINDTEXTURE>(IntGetProcAddress("glBindTexture"));
	if (!BindTexture)
		++numFailed;
	BlendColor = reinterpret_cast<PFNBLENDCOLOR>(IntGetProcAddress("glBlendColor"));
	if (!BlendColor)
		++numFailed;
	BlendEquation = reinterpret_cast<PFNBLENDEQUATION>(IntGetProcAddress("glBlendEquation"));
	if (!BlendEquation)
		++numFailed;
	BlendEquationSeparate = reinterpret_cast<PFNBLENDEQUATIONSEPARATE>(IntGetProcAddress("glBlendEquationSeparate"));
	if (!BlendEquationSeparate)
		++numFailed;
	BlendFunc = reinterpret_cast<PFNBLENDFUNC>(IntGetProcAddress("glBlendFunc"));
	if (!BlendFunc)
		++numFailed;
	BlendFuncSeparate = reinterpret_cast<PFNBLENDFUNCSEPARATE>(IntGetProcAddress("glBlendFuncSeparate"));
	if (!BlendFuncSeparate)
		++numFailed;
	BufferData = reinterpret_cast<PFNBUFFERDATA>(IntGetProcAddress("glBufferData"));
	if (!BufferData)
		++numFailed;
	BufferSubData = reinterpret_cast<PFNBUFFERSUBDATA>(IntGetProcAddress("glBufferSubData"));
	if (!BufferSubData)
		++numFailed;
	CheckFramebufferStatus = reinterpret_cast<PFNCHECKFRAMEBUFFERSTATUS>(IntGetProcAddress("glCheckFramebufferStatus"));
	if (!CheckFramebufferStatus)
		++numFailed;
	Clear = reinterpret_cast<PFNCLEAR>(IntGetProcAddress("glClear"));
	if (!Clear)
		++numFailed;
	ClearColor = reinterpret_cast<PFNCLEARCOLOR>(IntGetProcAddress("glClearColor"));
	if (!ClearColor)
		++numFailed;
	ClearDepthf = reinterpret_cast<PFNCLEARDEPTHF>(IntGetProcAddress("glClearDepthf"));
	if (!ClearDepthf)
		++numFailed;
	ClearStencil = reinterpret_cast<PFNCLEARSTENCIL>(IntGetProcAddress("glClearStencil"));
	if (!ClearStencil)
		++numFailed;
	ColorMask = reinterpret_cast<PFNCOLORMASK>(IntGetProcAddress("glColorMask"));
	if (!ColorMask)
		++numFailed;
	CompileShader = reinterpret_cast<PFNCOMPILESHADER>(IntGetProcAddress("glCompileShader"));
	if (!CompileShader)
		++numFailed;
	CompressedTexImage2D = reinterpret_cast<PFNCOMPRESSEDTEXIMAGE2D>(IntGetProcAddress("glCompressedTexImage2D"));
	if (!CompressedTexImage2D)
		++numFailed;
	CompressedTexSubImage2D = reinterpret_cast<PFNCOMPRESSEDTEXSUBIMAGE2D>(IntGetProcAddress("glCompressedTexSubImage2D"));
	if (!CompressedTexSubImage2D)
		++numFailed;
	CopyTexImage2D = reinterpret_cast<PFNCOPYTEXIMAGE2D>(IntGetProcAddress("glCopyTexImage2D"));
	if (!CopyTexImage2D)
		++numFailed;
	CopyTexSubImage2D = reinterpret_cast<PFNCOPYTEXSUBIMAGE2D>(IntGetProcAddress("glCopyTexSubImage2D"));
	if (!CopyTexSubImage2D)
		++numFailed;
	CreateProgram = reinterpret_cast<PFNCREATEPROGRAM>(IntGetProcAddress("glCreateProgram"));
	if (!CreateProgram)
		++numFailed;
	CreateShader = reinterpret_cast<PFNCREATESHADER>(IntGetProcAddress("glCreateShader"));
	if (!CreateShader)
		++numFailed;
	CullFace = reinterpret_cast<PFNCULLFACE>(IntGetProcAddress("glCullFace"));
	if (!CullFace)
		++numFailed;
	DeleteBuffers = reinterpret_cast<PFNDELETEBUFFERS>(IntGetProcAddress("glDeleteBuffers"));
	if (!DeleteBuffers)
		++numFailed;
	DeleteFramebuffers = reinterpret_cast<PFNDELETEFRAMEBUFFERS>(IntGetProcAddress("glDeleteFramebuffers"));
	if (!DeleteFramebuffers)
		++numFailed;
	DeleteProgram = reinterpret_cast<PFNDELETEPROGRAM>(IntGetProcAddress("glDeleteProgram"));
	if (!DeleteProgram)
		++numFailed;
	DeleteRenderbuffers = reinterpret_cast<PFNDELETERENDERBUFFERS>(IntGetProcAddress("glDeleteRenderbuffers"));
	if (!DeleteRenderbuffers)
		++numFailed;
	DeleteShader = reinterpret_cast<PFNDELETESHADER>(IntGetProcAddress("glDeleteShader"));
	if (!DeleteShader)
		++numFailed;
	DeleteTextures = reinterpret_cast<PFNDELETETEXTURES>(IntGetProcAddress("glDeleteTextures"));
	if (!DeleteTextures)
		++numFailed;
	DepthFunc = reinterpret_cast<PFNDEPTHFUNC>(IntGetProcAddress("glDepthFunc"));
	if (!DepthFunc)
		++numFailed;
	DepthMask = reinterpret_cast<PFNDEPTHMASK>(IntGetProcAddress("glDepthMask"));
	if (!DepthMask)
		++numFailed;
	DepthRangef = reinterpret_cast<PFNDEPTHRANGEF>(IntGetProcAddress("glDepthRangef"));
	if (!DepthRangef)
		++numFailed;
	DetachShader = reinterpret_cast<PFNDETACHSHADER>(IntGetProcAddress("glDetachShader"));
	if (!DetachShader)
		++numFailed;
	Disable = reinterpret_cast<PFNDISABLE>(IntGetProcAddress("glDisable"));
	if (!Disable)
		++numFailed;
	DisableVertexAttribArray = reinterpret_cast<PFNDISABLEVERTEXATTRIBARRAY>(IntGetProcAddress("glDisableVertexAttribArray"));
	if (!DisableVertexAttribArray)
		++numFailed;
	DrawArrays = reinterpret_cast<PFNDRAWARRAYS>(IntGetProcAddress("glDrawArrays"));
	if (!DrawArrays)
		++numFailed;
	DrawElements = reinterpret_cast<PFNDRAWELEMENTS>(IntGetProcAddress("glDrawElements"));
	if (!DrawElements)
		++numFailed;
	Enable = reinterpret_cast<PFNENABLE>(IntGetProcAddress("glEnable"));
	if (!Enable)
		++numFailed;
	EnableVertexAttribArray = reinterpret_cast<PFNENABLEVERTEXATTRIBARRAY>(IntGetProcAddress("glEnableVertexAttribArray"));
	if (!EnableVertexAttribArray)
		++numFailed;
	Finish = reinterpret_cast<PFNFINISH>(IntGetProcAddress("glFinish"));
	if (!Finish)
		++numFailed;
	Flush = reinterpret_cast<PFNFLUSH>(IntGetProcAddress("glFlush"));
	if (!Flush)
		++numFailed;
	FramebufferRenderbuffer = reinterpret_cast<PFNFRAMEBUFFERRENDERBUFFER>(IntGetProcAddress("glFramebufferRenderbuffer"));
	if (!FramebufferRenderbuffer)
		++numFailed;
	FramebufferTexture2D = reinterpret_cast<PFNFRAMEBUFFERTEXTURE2D>(IntGetProcAddress("glFramebufferTexture2D"));
	if (!FramebufferTexture2D)
		++numFailed;
	FrontFace = reinterpret_cast<PFNFRONTFACE>(IntGetProcAddress("glFrontFace"));
	if (!FrontFace)
		++numFailed;
	GenBuffers = reinterpret_cast<PFNGENBUFFERS>(IntGetProcAddress("glGenBuffers"));
	if (!GenBuffers)
		++numFailed;
	GenerateMipmap = reinterpret_cast<PFNGENERATEMIPMAP>(IntGetProcAddress("glGenerateMipmap"));
	if (!GenerateMipmap)
		++numFailed;
	GenFramebuffers = reinterpret_cast<PFNGENFRAMEBUFFERS>(IntGetProcAddress("glGenFramebuffers"));
	if (!GenFramebuffers)
		++numFailed;
	GenRenderbuffers = reinterpret_cast<PFNGENRENDERBUFFERS>(IntGetProcAddress("glGenRenderbuffers"));
	if (!GenRenderbuffers)
		++numFailed;
	GenTextures = reinterpret_cast<PFNGENTEXTURES>(IntGetProcAddress("glGenTextures"));
	if (!GenTextures)
		++numFailed;
	GetActiveAttrib = reinterpret_cast<PFNGETACTIVEATTRIB>(IntGetProcAddress("glGetActiveAttrib"));
	if (!GetActiveAttrib)
		++numFailed;
	GetActiveUniform = reinterpret_cast<PFNGETACTIVEUNIFORM>(IntGetProcAddress("glGetActiveUniform"));
	if (!GetActiveUniform)
		++numFailed;
	GetAttachedShaders = reinterpret_cast<PFNGETATTACHEDSHADERS>(IntGetProcAddress("glGetAttachedShaders"));
	if (!GetAttachedShaders)
		++numFailed;
	GetAttribLocation = reinterpret_cast<PFNGETATTRIBLOCATION>(IntGetProcAddress("glGetAttribLocation"));
	if (!GetAttribLocation)
		++numFailed;
	GetBooleanv = reinterpret_cast<PFNGETBOOLEANV>(IntGetProcAddress("glGetBooleanv"));
	if (!GetBooleanv)
		++numFailed;
	GetBufferParameteriv = reinterpret_cast<PFNGETBUFFERPARAMETERIV>(IntGetProcAddress("glGetBufferParameteriv"));
	if (!GetBufferParameteriv)
		++numFailed;
	GetError = reinterpret_cast<PFNGETERROR>(IntGetProcAddress("glGetError"));
	if (!GetError)
		++numFailed;
	GetFloatv = reinterpret_cast<PFNGETFLOATV>(IntGetProcAddress("glGetFloatv"));
	if (!GetFloatv)
		++numFailed;
	GetFramebufferAttachmentParameteriv = reinterpret_cast<PFNGETFRAMEBUFFERATTACHMENTPARAMETERIV>(IntGetProcAddress("glGetFramebufferAttachmentParameteriv"));
	if (!GetFramebufferAttachmentParameteriv)
		++numFailed;
	GetIntegerv = reinterpret_cast<PFNGETINTEGERV>(IntGetProcAddress("glGetIntegerv"));
	if (!GetIntegerv)
		++numFailed;
	GetProgramiv = reinterpret_cast<PFNGETPROGRAMIV>(IntGetProcAddress("glGetProgramiv"));
	if (!GetProgramiv)
		++numFailed;
	GetProgramInfoLog = reinterpret_cast<PFNGETPROGRAMINFOLOG>(IntGetProcAddress("glGetProgramInfoLog"));
	if (!GetProgramInfoLog)
		++numFailed;
	GetRenderbufferParameteriv = reinterpret_cast<PFNGETRENDERBUFFERPARAMETERIV>(IntGetProcAddress("glGetRenderbufferParameteriv"));
	if (!GetRenderbufferParameteriv)
		++numFailed;
	GetShaderiv = reinterpret_cast<PFNGETSHADERIV>(IntGetProcAddress("glGetShaderiv"));
	if (!GetShaderiv)
		++numFailed;
	GetShaderInfoLog = reinterpret_cast<PFNGETSHADERINFOLOG>(IntGetProcAddress("glGetShaderInfoLog"));
	if (!GetShaderInfoLog)
		++numFailed;
	GetShaderPrecisionFormat = reinterpret_cast<PFNGETSHADERPRECISIONFORMAT>(IntGetProcAddress("glGetShaderPrecisionFormat"));
	if (!GetShaderPrecisionFormat)
		++numFailed;
	GetShaderSource = reinterpret_cast<PFNGETSHADERSOURCE>(IntGetProcAddress("glGetShaderSource"));
	if (!GetShaderSource)
		++numFailed;
	GetString = reinterpret_cast<PFNGETSTRING>(IntGetProcAddress("glGetString"));
	if (!GetString)
		++numFailed;
	GetTexParameterfv = reinterpret_cast<PFNGETTEXPARAMETERFV>(IntGetProcAddress("glGetTexParameterfv"));
	if (!GetTexParameterfv)
		++numFailed;
	GetTexParameteriv = reinterpret_cast<PFNGETTEXPARAMETERIV>(IntGetProcAddress("glGetTexParameteriv"));
	if (!GetTexParameteriv)
		++numFailed;
	GetUniformfv = reinterpret_cast<PFNGETUNIFORMFV>(IntGetProcAddress("glGetUniformfv"));
	if (!GetUniformfv)
		++numFailed;
	GetUniformiv = reinterpret_cast<PFNGETUNIFORMIV>(IntGetProcAddress("glGetUniformiv"));
	if (!GetUniformiv)
		++numFailed;
	GetUniformLocation = reinterpret_cast<PFNGETUNIFORMLOCATION>(IntGetProcAddress("glGetUniformLocation"));
	if (!GetUniformLocation)
		++numFailed;
	GetVertexAttribfv = reinterpret_cast<PFNGETVERTEXATTRIBFV>(IntGetProcAddress("glGetVertexAttribfv"));
	if (!GetVertexAttribfv)
		++numFailed;
	GetVertexAttribiv = reinterpret_cast<PFNGETVERTEXATTRIBIV>(IntGetProcAddress("glGetVertexAttribiv"));
	if (!GetVertexAttribiv)
		++numFailed;
	GetVertexAttribPointerv = reinterpret_cast<PFNGETVERTEXATTRIBPOINTERV>(IntGetProcAddress("glGetVertexAttribPointerv"));
	if (!GetVertexAttribPointerv)
		++numFailed;
	Hint = reinterpret_cast<PFNHINT>(IntGetProcAddress("glHint"));
	if (!Hint)
		++numFailed;
	IsBuffer = reinterpret_cast<PFNISBUFFER>(IntGetProcAddress("glIsBuffer"));
	if (!IsBuffer)
		++numFailed;
	IsEnabled = reinterpret_cast<PFNISENABLED>(IntGetProcAddress("glIsEnabled"));
	if (!IsEnabled)
		++numFailed;
	IsFramebuffer = reinterpret_cast<PFNISFRAMEBUFFER>(IntGetProcAddress("glIsFramebuffer"));
	if (!IsFramebuffer)
		++numFailed;
	IsProgram = reinterpret_cast<PFNISPROGRAM>(IntGetProcAddress("glIsProgram"));
	if (!IsProgram)
		++numFailed;
	IsRenderbuffer = reinterpret_cast<PFNISRENDERBUFFER>(IntGetProcAddress("glIsRenderbuffer"));
	if (!IsRenderbuffer)
		++numFailed;
	IsShader = reinterpret_cast<PFNISSHADER>(IntGetProcAddress("glIsShader"));
	if (!IsShader)
		++numFailed;
	IsTexture = reinterpret_cast<PFNISTEXTURE>(IntGetProcAddress("glIsTexture"));
	if (!IsTexture)
		++numFailed;
	LineWidth = reinterpret_cast<PFNLINEWIDTH>(IntGetProcAddress("glLineWidth"));
	if (!LineWidth)
		++numFailed;
	LinkProgram = reinterpret_cast<PFNLINKPROGRAM>(IntGetProcAddress("glLinkProgram"));
	if (!LinkProgram)
		++numFailed;
	PixelStorei = reinterpret_cast<PFNPIXELSTOREI>(IntGetProcAddress("glPixelStorei"));
	if (!PixelStorei)
		++numFailed;
	PolygonOffset = reinterpret_cast<PFNPOLYGONOFFSET>(IntGetProcAddress("glPolygonOffset"));
	if (!PolygonOffset)
		++numFailed;
	ReadPixels = reinterpret_cast<PFNREADPIXELS>(IntGetProcAddress("glReadPixels"));
	if (!ReadPixels)
		++numFailed;
	ReleaseShaderCompiler = reinterpret_cast<PFNRELEASESHADERCOMPILER>(IntGetProcAddress("glReleaseShaderCompiler"));
	if (!ReleaseShaderCompiler)
		++numFailed;
	RenderbufferStorage = reinterpret_cast<PFNRENDERBUFFERSTORAGE>(IntGetProcAddress("glRenderbufferStorage"));
	if (!RenderbufferStorage)
		++numFailed;
	SampleCoverage = reinterpret_cast<PFNSAMPLECOVERAGE>(IntGetProcAddress("glSampleCoverage"));
	if (!SampleCoverage)
		++numFailed;
	Scissor = reinterpret_cast<PFNSCISSOR>(IntGetProcAddress("glScissor"));
	if (!Scissor)
		++numFailed;
	ShaderBinary = reinterpret_cast<PFNSHADERBINARY>(IntGetProcAddress("glShaderBinary"));
	if (!ShaderBinary)
		++numFailed;
	ShaderSource = reinterpret_cast<PFNSHADERSOURCE>(IntGetProcAddress("glShaderSource"));
	if (!ShaderSource)
		++numFailed;
	StencilFunc = reinterpret_cast<PFNSTENCILFUNC>(IntGetProcAddress("glStencilFunc"));
	if (!StencilFunc)
		++numFailed;
	StencilFuncSeparate = reinterpret_cast<PFNSTENCILFUNCSEPARATE>(IntGetProcAddress("glStencilFuncSeparate"));
	if (!StencilFuncSeparate)
		++numFailed;
	StencilMask = reinterpret_cast<PFNSTENCILMASK>(IntGetProcAddress("glStencilMask"));
	if (!StencilMask)
		++numFailed;
	StencilMaskSeparate = reinterpret_cast<PFNSTENCILMASKSEPARATE>(IntGetProcAddress("glStencilMaskSeparate"));
	if (!StencilMaskSeparate)
		++numFailed;
	StencilOp = reinterpret_cast<PFNSTENCILOP>(IntGetProcAddress("glStencilOp"));
	if (!StencilOp)
		++numFailed;
	StencilOpSeparate = reinterpret_cast<PFNSTENCILOPSEPARATE>(IntGetProcAddress("glStencilOpSeparate"));
	if (!StencilOpSeparate)
		++numFailed;
	TexImage2D = reinterpret_cast<PFNTEXIMAGE2D>(IntGetProcAddress("glTexImage2D"));
	if (!TexImage2D)
		++numFailed;
	TexParameterf = reinterpret_cast<PFNTEXPARAMETERF>(IntGetProcAddress("glTexParameterf"));
	if (!TexParameterf)
		++numFailed;
	TexParameterfv = reinterpret_cast<PFNTEXPARAMETERFV>(IntGetProcAddress("glTexParameterfv"));
	if (!TexParameterfv)
		++numFailed;
	TexParameteri = reinterpret_cast<PFNTEXPARAMETERI>(IntGetProcAddress("glTexParameteri"));
	if (!TexParameteri)
		++numFailed;
	TexParameteriv = reinterpret_cast<PFNTEXPARAMETERIV>(IntGetProcAddress("glTexParameteriv"));
	if (!TexParameteriv)
		++numFailed;
	TexSubImage2D = reinterpret_cast<PFNTEXSUBIMAGE2D>(IntGetProcAddress("glTexSubImage2D"));
	if (!TexSubImage2D)
		++numFailed;
	Uniform1f = reinterpret_cast<PFNUNIFORM1F>(IntGetProcAddress("glUniform1f"));
	if (!Uniform1f)
		++numFailed;
	Uniform1fv = reinterpret_cast<PFNUNIFORM1FV>(IntGetProcAddress("glUniform1fv"));
	if (!Uniform1fv)
		++numFailed;
	Uniform1i = reinterpret_cast<PFNUNIFORM1I>(IntGetProcAddress("glUniform1i"));
	if (!Uniform1i)
		++numFailed;
	Uniform1iv = reinterpret_cast<PFNUNIFORM1IV>(IntGetProcAddress("glUniform1iv"));
	if (!Uniform1iv)
		++numFailed;
	Uniform2f = reinterpret_cast<PFNUNIFORM2F>(IntGetProcAddress("glUniform2f"));
	if (!Uniform2f)
		++numFailed;
	Uniform2fv = reinterpret_cast<PFNUNIFORM2FV>(IntGetProcAddress("glUniform2fv"));
	if (!Uniform2fv)
		++numFailed;
	Uniform2i = reinterpret_cast<PFNUNIFORM2I>(IntGetProcAddress("glUniform2i"));
	if (!Uniform2i)
		++numFailed;
	Uniform2iv = reinterpret_cast<PFNUNIFORM2IV>(IntGetProcAddress("glUniform2iv"));
	if (!Uniform2iv)
		++numFailed;
	Uniform3f = reinterpret_cast<PFNUNIFORM3F>(IntGetProcAddress("glUniform3f"));
	if (!Uniform3f)
		++numFailed;
	Uniform3fv = reinterpret_cast<PFNUNIFORM3FV>(IntGetProcAddress("glUniform3fv"));
	if (!Uniform3fv)
		++numFailed;
	Uniform3i = reinterpret_cast<PFNUNIFORM3I>(IntGetProcAddress("glUniform3i"));
	if (!Uniform3i)
		++numFailed;
	Uniform3iv = reinterpret_cast<PFNUNIFORM3IV>(IntGetProcAddress("glUniform3iv"));
	if (!Uniform3iv)
		++numFailed;
	Uniform4f = reinterpret_cast<PFNUNIFORM4F>(IntGetProcAddress("glUniform4f"));
	if (!Uniform4f)
		++numFailed;
	Uniform4fv = reinterpret_cast<PFNUNIFORM4FV>(IntGetProcAddress("glUniform4fv"));
	if (!Uniform4fv)
		++numFailed;
	Uniform4i = reinterpret_cast<PFNUNIFORM4I>(IntGetProcAddress("glUniform4i"));
	if (!Uniform4i)
		++numFailed;
	Uniform4iv = reinterpret_cast<PFNUNIFORM4IV>(IntGetProcAddress("glUniform4iv"));
	if (!Uniform4iv)
		++numFailed;
	UniformMatrix2fv = reinterpret_cast<PFNUNIFORMMATRIX2FV>(IntGetProcAddress("glUniformMatrix2fv"));
	if (!UniformMatrix2fv)
		++numFailed;
	UniformMatrix3fv = reinterpret_cast<PFNUNIFORMMATRIX3FV>(IntGetProcAddress("glUniformMatrix3fv"));
	if (!UniformMatrix3fv)
		++numFailed;
	UniformMatrix4fv = reinterpret_cast<PFNUNIFORMMATRIX4FV>(IntGetProcAddress("glUniformMatrix4fv"));
	if (!UniformMatrix4fv)
		++numFailed;
	UseProgram = reinterpret_cast<PFNUSEPROGRAM>(IntGetProcAddress("glUseProgram"));
	if (!UseProgram)
		++numFailed;
	ValidateProgram = reinterpret_cast<PFNVALIDATEPROGRAM>(IntGetProcAddress("glValidateProgram"));
	if (!ValidateProgram)
		++numFailed;
	VertexAttrib1f = reinterpret_cast<PFNVERTEXATTRIB1F>(IntGetProcAddress("glVertexAttrib1f"));
	if (!VertexAttrib1f)
		++numFailed;
	VertexAttrib1fv = reinterpret_cast<PFNVERTEXATTRIB1FV>(IntGetProcAddress("glVertexAttrib1fv"));
	if (!VertexAttrib1fv)
		++numFailed;
	VertexAttrib2f = reinterpret_cast<PFNVERTEXATTRIB2F>(IntGetProcAddress("glVertexAttrib2f"));
	if (!VertexAttrib2f)
		++numFailed;
	VertexAttrib2fv = reinterpret_cast<PFNVERTEXATTRIB2FV>(IntGetProcAddress("glVertexAttrib2fv"));
	if (!VertexAttrib2fv)
		++numFailed;
	VertexAttrib3f = reinterpret_cast<PFNVERTEXATTRIB3F>(IntGetProcAddress("glVertexAttrib3f"));
	if (!VertexAttrib3f)
		++numFailed;
	VertexAttrib3fv = reinterpret_cast<PFNVERTEXATTRIB3FV>(IntGetProcAddress("glVertexAttrib3fv"));
	if (!VertexAttrib3fv)
		++numFailed;
	VertexAttrib4f = reinterpret_cast<PFNVERTEXATTRIB4F>(IntGetProcAddress("glVertexAttrib4f"));
	if (!VertexAttrib4f)
		++numFailed;
	VertexAttrib4fv = reinterpret_cast<PFNVERTEXATTRIB4FV>(IntGetProcAddress("glVertexAttrib4fv"));
	if (!VertexAttrib4fv)
		++numFailed;
	VertexAttribPointer = reinterpret_cast<PFNVERTEXATTRIBPOINTER>(IntGetProcAddress("glVertexAttribPointer"));
	if (!VertexAttribPointer)
		++numFailed;
	Viewport = reinterpret_cast<PFNVIEWPORT>(IntGetProcAddress("glViewport"));
	if (!Viewport)
		++numFailed;
	return numFailed;

}

namespace sys
{
	exts::LoadTest LoadFunctions()
	{
		int numFailed = LoadCoreFunctions();
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
