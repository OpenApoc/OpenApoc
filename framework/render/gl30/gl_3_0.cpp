#include <algorithm>
#include <vector>
#include <string.h>
#include <stddef.h>
#include "gl_3_0.hpp"

#if defined(__APPLE__)
#include <mach-o/dyld.h>

static void *AppleGLGetProcAddress(const GLubyte *name)
{
	static const struct mach_header *image = NULL;
	NSSymbol symbol;
	char *symbolName;
	if (NULL == image) {
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

	if (h == NULL) {
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
	PROC pFunc = wglGetProcAddress((LPCSTR)name);
	if (TestPointer(pFunc)) {
		return pFunc;
	}
	glMod = GetModuleHandleA("OpenGL32.dll");
	if (!glMod)
		return nullptr;
	return (PROC)GetProcAddress(glMod, (LPCSTR)name);
}

#define IntGetProcAddress(name) WinGetProcAddress(name)
#else
#if defined(__APPLE__)
#define IntGetProcAddress(name) AppleGLGetProcAddress(name)
#else
#if defined(__sgi) || defined(__sun)
#define IntGetProcAddress(name) SunGetProcAddress(name)
#else /* GLX */
#include <GL/glx.h>

#define IntGetProcAddress(name) (*glXGetProcAddressARB)((const GLubyte *)name)
#endif
#endif
#endif

namespace gl
{
namespace exts
{
} // namespace exts
typedef void(CODEGEN_FUNCPTR *PFNACCUM)(GLenum, GLfloat);
PFNACCUM Accum = 0;
typedef void(CODEGEN_FUNCPTR *PFNALPHAFUNC)(GLenum, GLfloat);
PFNALPHAFUNC AlphaFunc = 0;
typedef void(CODEGEN_FUNCPTR *PFNBEGIN)(GLenum);
PFNBEGIN Begin = 0;
typedef void(CODEGEN_FUNCPTR *PFNBITMAP)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat,
                                         const GLubyte *);
PFNBITMAP Bitmap = 0;
typedef void(CODEGEN_FUNCPTR *PFNBLENDFUNC)(GLenum, GLenum);
PFNBLENDFUNC BlendFunc = 0;
typedef void(CODEGEN_FUNCPTR *PFNCALLLIST)(GLuint);
PFNCALLLIST CallList = 0;
typedef void(CODEGEN_FUNCPTR *PFNCALLLISTS)(GLsizei, GLenum, const GLvoid *);
PFNCALLLISTS CallLists = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEAR)(GLbitfield);
PFNCLEAR Clear = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARACCUM)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNCLEARACCUM ClearAccum = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARCOLOR)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNCLEARCOLOR ClearColor = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARDEPTH)(GLdouble);
PFNCLEARDEPTH ClearDepth = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARINDEX)(GLfloat);
PFNCLEARINDEX ClearIndex = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARSTENCIL)(GLint);
PFNCLEARSTENCIL ClearStencil = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLIPPLANE)(GLenum, const GLdouble *);
PFNCLIPPLANE ClipPlane = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3B)(GLbyte, GLbyte, GLbyte);
PFNCOLOR3B Color3b = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3BV)(const GLbyte *);
PFNCOLOR3BV Color3bv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3D)(GLdouble, GLdouble, GLdouble);
PFNCOLOR3D Color3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3DV)(const GLdouble *);
PFNCOLOR3DV Color3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3F)(GLfloat, GLfloat, GLfloat);
PFNCOLOR3F Color3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3FV)(const GLfloat *);
PFNCOLOR3FV Color3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3I)(GLint, GLint, GLint);
PFNCOLOR3I Color3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3IV)(const GLint *);
PFNCOLOR3IV Color3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3S)(GLshort, GLshort, GLshort);
PFNCOLOR3S Color3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3SV)(const GLshort *);
PFNCOLOR3SV Color3sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3UB)(GLubyte, GLubyte, GLubyte);
PFNCOLOR3UB Color3ub = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3UBV)(const GLubyte *);
PFNCOLOR3UBV Color3ubv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3UI)(GLuint, GLuint, GLuint);
PFNCOLOR3UI Color3ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3UIV)(const GLuint *);
PFNCOLOR3UIV Color3uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3US)(GLushort, GLushort, GLushort);
PFNCOLOR3US Color3us = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR3USV)(const GLushort *);
PFNCOLOR3USV Color3usv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4B)(GLbyte, GLbyte, GLbyte, GLbyte);
PFNCOLOR4B Color4b = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4BV)(const GLbyte *);
PFNCOLOR4BV Color4bv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4D)(GLdouble, GLdouble, GLdouble, GLdouble);
PFNCOLOR4D Color4d = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4DV)(const GLdouble *);
PFNCOLOR4DV Color4dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4F)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNCOLOR4F Color4f = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4FV)(const GLfloat *);
PFNCOLOR4FV Color4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4I)(GLint, GLint, GLint, GLint);
PFNCOLOR4I Color4i = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4IV)(const GLint *);
PFNCOLOR4IV Color4iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4S)(GLshort, GLshort, GLshort, GLshort);
PFNCOLOR4S Color4s = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4SV)(const GLshort *);
PFNCOLOR4SV Color4sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4UB)(GLubyte, GLubyte, GLubyte, GLubyte);
PFNCOLOR4UB Color4ub = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4UBV)(const GLubyte *);
PFNCOLOR4UBV Color4ubv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4UI)(GLuint, GLuint, GLuint, GLuint);
PFNCOLOR4UI Color4ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4UIV)(const GLuint *);
PFNCOLOR4UIV Color4uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4US)(GLushort, GLushort, GLushort, GLushort);
PFNCOLOR4US Color4us = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLOR4USV)(const GLushort *);
PFNCOLOR4USV Color4usv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLORMASK)(GLboolean, GLboolean, GLboolean, GLboolean);
PFNCOLORMASK ColorMask = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLORMATERIAL)(GLenum, GLenum);
PFNCOLORMATERIAL ColorMaterial = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOPYPIXELS)(GLint, GLint, GLsizei, GLsizei, GLenum);
PFNCOPYPIXELS CopyPixels = 0;
typedef void(CODEGEN_FUNCPTR *PFNCULLFACE)(GLenum);
PFNCULLFACE CullFace = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETELISTS)(GLuint, GLsizei);
PFNDELETELISTS DeleteLists = 0;
typedef void(CODEGEN_FUNCPTR *PFNDEPTHFUNC)(GLenum);
PFNDEPTHFUNC DepthFunc = 0;
typedef void(CODEGEN_FUNCPTR *PFNDEPTHMASK)(GLboolean);
PFNDEPTHMASK DepthMask = 0;
typedef void(CODEGEN_FUNCPTR *PFNDEPTHRANGE)(GLdouble, GLdouble);
PFNDEPTHRANGE DepthRange = 0;
typedef void(CODEGEN_FUNCPTR *PFNDISABLE)(GLenum);
PFNDISABLE Disable = 0;
typedef void(CODEGEN_FUNCPTR *PFNDRAWBUFFER)(GLenum);
PFNDRAWBUFFER DrawBuffer = 0;
typedef void(CODEGEN_FUNCPTR *PFNDRAWPIXELS)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
PFNDRAWPIXELS DrawPixels = 0;
typedef void(CODEGEN_FUNCPTR *PFNEDGEFLAG)(GLboolean);
PFNEDGEFLAG EdgeFlag = 0;
typedef void(CODEGEN_FUNCPTR *PFNEDGEFLAGV)(const GLboolean *);
PFNEDGEFLAGV EdgeFlagv = 0;
typedef void(CODEGEN_FUNCPTR *PFNENABLE)(GLenum);
PFNENABLE Enable = 0;
typedef void(CODEGEN_FUNCPTR *PFNEND)();
PFNEND End = 0;
typedef void(CODEGEN_FUNCPTR *PFNENDLIST)();
PFNENDLIST EndList = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALCOORD1D)(GLdouble);
PFNEVALCOORD1D EvalCoord1d = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALCOORD1DV)(const GLdouble *);
PFNEVALCOORD1DV EvalCoord1dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALCOORD1F)(GLfloat);
PFNEVALCOORD1F EvalCoord1f = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALCOORD1FV)(const GLfloat *);
PFNEVALCOORD1FV EvalCoord1fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALCOORD2D)(GLdouble, GLdouble);
PFNEVALCOORD2D EvalCoord2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALCOORD2DV)(const GLdouble *);
PFNEVALCOORD2DV EvalCoord2dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALCOORD2F)(GLfloat, GLfloat);
PFNEVALCOORD2F EvalCoord2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALCOORD2FV)(const GLfloat *);
PFNEVALCOORD2FV EvalCoord2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALMESH1)(GLenum, GLint, GLint);
PFNEVALMESH1 EvalMesh1 = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALMESH2)(GLenum, GLint, GLint, GLint, GLint);
PFNEVALMESH2 EvalMesh2 = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALPOINT1)(GLint);
PFNEVALPOINT1 EvalPoint1 = 0;
typedef void(CODEGEN_FUNCPTR *PFNEVALPOINT2)(GLint, GLint);
PFNEVALPOINT2 EvalPoint2 = 0;
typedef void(CODEGEN_FUNCPTR *PFNFEEDBACKBUFFER)(GLsizei, GLenum, GLfloat *);
PFNFEEDBACKBUFFER FeedbackBuffer = 0;
typedef void(CODEGEN_FUNCPTR *PFNFINISH)();
PFNFINISH Finish = 0;
typedef void(CODEGEN_FUNCPTR *PFNFLUSH)();
PFNFLUSH Flush = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGF)(GLenum, GLfloat);
PFNFOGF Fogf = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGFV)(GLenum, const GLfloat *);
PFNFOGFV Fogfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGI)(GLenum, GLint);
PFNFOGI Fogi = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGIV)(GLenum, const GLint *);
PFNFOGIV Fogiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNFRONTFACE)(GLenum);
PFNFRONTFACE FrontFace = 0;
typedef void(CODEGEN_FUNCPTR *PFNFRUSTUM)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble,
                                          GLdouble);
PFNFRUSTUM Frustum = 0;
typedef GLuint(CODEGEN_FUNCPTR *PFNGENLISTS)(GLsizei);
PFNGENLISTS GenLists = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETBOOLEANV)(GLenum, GLboolean *);
PFNGETBOOLEANV GetBooleanv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETCLIPPLANE)(GLenum, GLdouble *);
PFNGETCLIPPLANE GetClipPlane = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETDOUBLEV)(GLenum, GLdouble *);
PFNGETDOUBLEV GetDoublev = 0;
typedef GLenum(CODEGEN_FUNCPTR *PFNGETERROR)();
PFNGETERROR GetError = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETFLOATV)(GLenum, GLfloat *);
PFNGETFLOATV GetFloatv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETINTEGERV)(GLenum, GLint *);
PFNGETINTEGERV GetIntegerv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETLIGHTFV)(GLenum, GLenum, GLfloat *);
PFNGETLIGHTFV GetLightfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETLIGHTIV)(GLenum, GLenum, GLint *);
PFNGETLIGHTIV GetLightiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETMAPDV)(GLenum, GLenum, GLdouble *);
PFNGETMAPDV GetMapdv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETMAPFV)(GLenum, GLenum, GLfloat *);
PFNGETMAPFV GetMapfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETMAPIV)(GLenum, GLenum, GLint *);
PFNGETMAPIV GetMapiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETMATERIALFV)(GLenum, GLenum, GLfloat *);
PFNGETMATERIALFV GetMaterialfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETMATERIALIV)(GLenum, GLenum, GLint *);
PFNGETMATERIALIV GetMaterialiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETPIXELMAPFV)(GLenum, GLfloat *);
PFNGETPIXELMAPFV GetPixelMapfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETPIXELMAPUIV)(GLenum, GLuint *);
PFNGETPIXELMAPUIV GetPixelMapuiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETPIXELMAPUSV)(GLenum, GLushort *);
PFNGETPIXELMAPUSV GetPixelMapusv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETPOLYGONSTIPPLE)(GLubyte *);
PFNGETPOLYGONSTIPPLE GetPolygonStipple = 0;
typedef const GLubyte *(CODEGEN_FUNCPTR *PFNGETSTRING)(GLenum);
PFNGETSTRING GetString = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXENVFV)(GLenum, GLenum, GLfloat *);
PFNGETTEXENVFV GetTexEnvfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXENVIV)(GLenum, GLenum, GLint *);
PFNGETTEXENVIV GetTexEnviv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXGENDV)(GLenum, GLenum, GLdouble *);
PFNGETTEXGENDV GetTexGendv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXGENFV)(GLenum, GLenum, GLfloat *);
PFNGETTEXGENFV GetTexGenfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXGENIV)(GLenum, GLenum, GLint *);
PFNGETTEXGENIV GetTexGeniv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXIMAGE)(GLenum, GLint, GLenum, GLenum, GLvoid *);
PFNGETTEXIMAGE GetTexImage = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXLEVELPARAMETERFV)(GLenum, GLint, GLenum, GLfloat *);
PFNGETTEXLEVELPARAMETERFV GetTexLevelParameterfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXLEVELPARAMETERIV)(GLenum, GLint, GLenum, GLint *);
PFNGETTEXLEVELPARAMETERIV GetTexLevelParameteriv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXPARAMETERFV)(GLenum, GLenum, GLfloat *);
PFNGETTEXPARAMETERFV GetTexParameterfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXPARAMETERIV)(GLenum, GLenum, GLint *);
PFNGETTEXPARAMETERIV GetTexParameteriv = 0;
typedef void(CODEGEN_FUNCPTR *PFNHINT)(GLenum, GLenum);
PFNHINT Hint = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXMASK)(GLuint);
PFNINDEXMASK IndexMask = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXD)(GLdouble);
PFNINDEXD Indexd = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXDV)(const GLdouble *);
PFNINDEXDV Indexdv = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXF)(GLfloat);
PFNINDEXF Indexf = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXFV)(const GLfloat *);
PFNINDEXFV Indexfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXI)(GLint);
PFNINDEXI Indexi = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXIV)(const GLint *);
PFNINDEXIV Indexiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXS)(GLshort);
PFNINDEXS Indexs = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXSV)(const GLshort *);
PFNINDEXSV Indexsv = 0;
typedef void(CODEGEN_FUNCPTR *PFNINITNAMES)();
PFNINITNAMES InitNames = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISENABLED)(GLenum);
PFNISENABLED IsEnabled = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISLIST)(GLuint);
PFNISLIST IsList = 0;
typedef void(CODEGEN_FUNCPTR *PFNLIGHTMODELF)(GLenum, GLfloat);
PFNLIGHTMODELF LightModelf = 0;
typedef void(CODEGEN_FUNCPTR *PFNLIGHTMODELFV)(GLenum, const GLfloat *);
PFNLIGHTMODELFV LightModelfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNLIGHTMODELI)(GLenum, GLint);
PFNLIGHTMODELI LightModeli = 0;
typedef void(CODEGEN_FUNCPTR *PFNLIGHTMODELIV)(GLenum, const GLint *);
PFNLIGHTMODELIV LightModeliv = 0;
typedef void(CODEGEN_FUNCPTR *PFNLIGHTF)(GLenum, GLenum, GLfloat);
PFNLIGHTF Lightf = 0;
typedef void(CODEGEN_FUNCPTR *PFNLIGHTFV)(GLenum, GLenum, const GLfloat *);
PFNLIGHTFV Lightfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNLIGHTI)(GLenum, GLenum, GLint);
PFNLIGHTI Lighti = 0;
typedef void(CODEGEN_FUNCPTR *PFNLIGHTIV)(GLenum, GLenum, const GLint *);
PFNLIGHTIV Lightiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNLINESTIPPLE)(GLint, GLushort);
PFNLINESTIPPLE LineStipple = 0;
typedef void(CODEGEN_FUNCPTR *PFNLINEWIDTH)(GLfloat);
PFNLINEWIDTH LineWidth = 0;
typedef void(CODEGEN_FUNCPTR *PFNLISTBASE)(GLuint);
PFNLISTBASE ListBase = 0;
typedef void(CODEGEN_FUNCPTR *PFNLOADIDENTITY)();
PFNLOADIDENTITY LoadIdentity = 0;
typedef void(CODEGEN_FUNCPTR *PFNLOADMATRIXD)(const GLdouble *);
PFNLOADMATRIXD LoadMatrixd = 0;
typedef void(CODEGEN_FUNCPTR *PFNLOADMATRIXF)(const GLfloat *);
PFNLOADMATRIXF LoadMatrixf = 0;
typedef void(CODEGEN_FUNCPTR *PFNLOADNAME)(GLuint);
PFNLOADNAME LoadName = 0;
typedef void(CODEGEN_FUNCPTR *PFNLOGICOP)(GLenum);
PFNLOGICOP LogicOp = 0;
typedef void(CODEGEN_FUNCPTR *PFNMAP1D)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
PFNMAP1D Map1d = 0;
typedef void(CODEGEN_FUNCPTR *PFNMAP1F)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
PFNMAP1F Map1f = 0;
typedef void(CODEGEN_FUNCPTR *PFNMAP2D)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble,
                                        GLdouble, GLint, GLint, const GLdouble *);
PFNMAP2D Map2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNMAP2F)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat,
                                        GLint, GLint, const GLfloat *);
PFNMAP2F Map2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNMAPGRID1D)(GLint, GLdouble, GLdouble);
PFNMAPGRID1D MapGrid1d = 0;
typedef void(CODEGEN_FUNCPTR *PFNMAPGRID1F)(GLint, GLfloat, GLfloat);
PFNMAPGRID1F MapGrid1f = 0;
typedef void(CODEGEN_FUNCPTR *PFNMAPGRID2D)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
PFNMAPGRID2D MapGrid2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNMAPGRID2F)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
PFNMAPGRID2F MapGrid2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNMATERIALF)(GLenum, GLenum, GLfloat);
PFNMATERIALF Materialf = 0;
typedef void(CODEGEN_FUNCPTR *PFNMATERIALFV)(GLenum, GLenum, const GLfloat *);
PFNMATERIALFV Materialfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMATERIALI)(GLenum, GLenum, GLint);
PFNMATERIALI Materiali = 0;
typedef void(CODEGEN_FUNCPTR *PFNMATERIALIV)(GLenum, GLenum, const GLint *);
PFNMATERIALIV Materialiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMATRIXMODE)(GLenum);
PFNMATRIXMODE MatrixMode = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTMATRIXD)(const GLdouble *);
PFNMULTMATRIXD MultMatrixd = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTMATRIXF)(const GLfloat *);
PFNMULTMATRIXF MultMatrixf = 0;
typedef void(CODEGEN_FUNCPTR *PFNNEWLIST)(GLuint, GLenum);
PFNNEWLIST NewList = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3B)(GLbyte, GLbyte, GLbyte);
PFNNORMAL3B Normal3b = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3BV)(const GLbyte *);
PFNNORMAL3BV Normal3bv = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3D)(GLdouble, GLdouble, GLdouble);
PFNNORMAL3D Normal3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3DV)(const GLdouble *);
PFNNORMAL3DV Normal3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3F)(GLfloat, GLfloat, GLfloat);
PFNNORMAL3F Normal3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3FV)(const GLfloat *);
PFNNORMAL3FV Normal3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3I)(GLint, GLint, GLint);
PFNNORMAL3I Normal3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3IV)(const GLint *);
PFNNORMAL3IV Normal3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3S)(GLshort, GLshort, GLshort);
PFNNORMAL3S Normal3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMAL3SV)(const GLshort *);
PFNNORMAL3SV Normal3sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNORTHO)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
PFNORTHO Ortho = 0;
typedef void(CODEGEN_FUNCPTR *PFNPASSTHROUGH)(GLfloat);
PFNPASSTHROUGH PassThrough = 0;
typedef void(CODEGEN_FUNCPTR *PFNPIXELMAPFV)(GLenum, GLsizei, const GLfloat *);
PFNPIXELMAPFV PixelMapfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNPIXELMAPUIV)(GLenum, GLsizei, const GLuint *);
PFNPIXELMAPUIV PixelMapuiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNPIXELMAPUSV)(GLenum, GLsizei, const GLushort *);
PFNPIXELMAPUSV PixelMapusv = 0;
typedef void(CODEGEN_FUNCPTR *PFNPIXELSTOREF)(GLenum, GLfloat);
PFNPIXELSTOREF PixelStoref = 0;
typedef void(CODEGEN_FUNCPTR *PFNPIXELSTOREI)(GLenum, GLint);
PFNPIXELSTOREI PixelStorei = 0;
typedef void(CODEGEN_FUNCPTR *PFNPIXELTRANSFERF)(GLenum, GLfloat);
PFNPIXELTRANSFERF PixelTransferf = 0;
typedef void(CODEGEN_FUNCPTR *PFNPIXELTRANSFERI)(GLenum, GLint);
PFNPIXELTRANSFERI PixelTransferi = 0;
typedef void(CODEGEN_FUNCPTR *PFNPIXELZOOM)(GLfloat, GLfloat);
PFNPIXELZOOM PixelZoom = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOINTSIZE)(GLfloat);
PFNPOINTSIZE PointSize = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOLYGONMODE)(GLenum, GLenum);
PFNPOLYGONMODE PolygonMode = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOLYGONSTIPPLE)(const GLubyte *);
PFNPOLYGONSTIPPLE PolygonStipple = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOPATTRIB)();
PFNPOPATTRIB PopAttrib = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOPMATRIX)();
PFNPOPMATRIX PopMatrix = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOPNAME)();
PFNPOPNAME PopName = 0;
typedef void(CODEGEN_FUNCPTR *PFNPUSHATTRIB)(GLbitfield);
PFNPUSHATTRIB PushAttrib = 0;
typedef void(CODEGEN_FUNCPTR *PFNPUSHMATRIX)();
PFNPUSHMATRIX PushMatrix = 0;
typedef void(CODEGEN_FUNCPTR *PFNPUSHNAME)(GLuint);
PFNPUSHNAME PushName = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS2D)(GLdouble, GLdouble);
PFNRASTERPOS2D RasterPos2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS2DV)(const GLdouble *);
PFNRASTERPOS2DV RasterPos2dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS2F)(GLfloat, GLfloat);
PFNRASTERPOS2F RasterPos2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS2FV)(const GLfloat *);
PFNRASTERPOS2FV RasterPos2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS2I)(GLint, GLint);
PFNRASTERPOS2I RasterPos2i = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS2IV)(const GLint *);
PFNRASTERPOS2IV RasterPos2iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS2S)(GLshort, GLshort);
PFNRASTERPOS2S RasterPos2s = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS2SV)(const GLshort *);
PFNRASTERPOS2SV RasterPos2sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS3D)(GLdouble, GLdouble, GLdouble);
PFNRASTERPOS3D RasterPos3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS3DV)(const GLdouble *);
PFNRASTERPOS3DV RasterPos3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS3F)(GLfloat, GLfloat, GLfloat);
PFNRASTERPOS3F RasterPos3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS3FV)(const GLfloat *);
PFNRASTERPOS3FV RasterPos3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS3I)(GLint, GLint, GLint);
PFNRASTERPOS3I RasterPos3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS3IV)(const GLint *);
PFNRASTERPOS3IV RasterPos3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS3S)(GLshort, GLshort, GLshort);
PFNRASTERPOS3S RasterPos3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS3SV)(const GLshort *);
PFNRASTERPOS3SV RasterPos3sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS4D)(GLdouble, GLdouble, GLdouble, GLdouble);
PFNRASTERPOS4D RasterPos4d = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS4DV)(const GLdouble *);
PFNRASTERPOS4DV RasterPos4dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS4F)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNRASTERPOS4F RasterPos4f = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS4FV)(const GLfloat *);
PFNRASTERPOS4FV RasterPos4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS4I)(GLint, GLint, GLint, GLint);
PFNRASTERPOS4I RasterPos4i = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS4IV)(const GLint *);
PFNRASTERPOS4IV RasterPos4iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS4S)(GLshort, GLshort, GLshort, GLshort);
PFNRASTERPOS4S RasterPos4s = 0;
typedef void(CODEGEN_FUNCPTR *PFNRASTERPOS4SV)(const GLshort *);
PFNRASTERPOS4SV RasterPos4sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNREADBUFFER)(GLenum);
PFNREADBUFFER ReadBuffer = 0;
typedef void(CODEGEN_FUNCPTR *PFNREADPIXELS)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum,
                                             GLvoid *);
PFNREADPIXELS ReadPixels = 0;
typedef void(CODEGEN_FUNCPTR *PFNRECTD)(GLdouble, GLdouble, GLdouble, GLdouble);
PFNRECTD Rectd = 0;
typedef void(CODEGEN_FUNCPTR *PFNRECTDV)(const GLdouble *, const GLdouble *);
PFNRECTDV Rectdv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRECTF)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNRECTF Rectf = 0;
typedef void(CODEGEN_FUNCPTR *PFNRECTFV)(const GLfloat *, const GLfloat *);
PFNRECTFV Rectfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRECTI)(GLint, GLint, GLint, GLint);
PFNRECTI Recti = 0;
typedef void(CODEGEN_FUNCPTR *PFNRECTIV)(const GLint *, const GLint *);
PFNRECTIV Rectiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNRECTS)(GLshort, GLshort, GLshort, GLshort);
PFNRECTS Rects = 0;
typedef void(CODEGEN_FUNCPTR *PFNRECTSV)(const GLshort *, const GLshort *);
PFNRECTSV Rectsv = 0;
typedef GLint(CODEGEN_FUNCPTR *PFNRENDERMODE)(GLenum);
PFNRENDERMODE RenderMode = 0;
typedef void(CODEGEN_FUNCPTR *PFNROTATED)(GLdouble, GLdouble, GLdouble, GLdouble);
PFNROTATED Rotated = 0;
typedef void(CODEGEN_FUNCPTR *PFNROTATEF)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNROTATEF Rotatef = 0;
typedef void(CODEGEN_FUNCPTR *PFNSCALED)(GLdouble, GLdouble, GLdouble);
PFNSCALED Scaled = 0;
typedef void(CODEGEN_FUNCPTR *PFNSCALEF)(GLfloat, GLfloat, GLfloat);
PFNSCALEF Scalef = 0;
typedef void(CODEGEN_FUNCPTR *PFNSCISSOR)(GLint, GLint, GLsizei, GLsizei);
PFNSCISSOR Scissor = 0;
typedef void(CODEGEN_FUNCPTR *PFNSELECTBUFFER)(GLsizei, GLuint *);
PFNSELECTBUFFER SelectBuffer = 0;
typedef void(CODEGEN_FUNCPTR *PFNSHADEMODEL)(GLenum);
PFNSHADEMODEL ShadeModel = 0;
typedef void(CODEGEN_FUNCPTR *PFNSTENCILFUNC)(GLenum, GLint, GLuint);
PFNSTENCILFUNC StencilFunc = 0;
typedef void(CODEGEN_FUNCPTR *PFNSTENCILMASK)(GLuint);
PFNSTENCILMASK StencilMask = 0;
typedef void(CODEGEN_FUNCPTR *PFNSTENCILOP)(GLenum, GLenum, GLenum);
PFNSTENCILOP StencilOp = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD1D)(GLdouble);
PFNTEXCOORD1D TexCoord1d = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD1DV)(const GLdouble *);
PFNTEXCOORD1DV TexCoord1dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD1F)(GLfloat);
PFNTEXCOORD1F TexCoord1f = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD1FV)(const GLfloat *);
PFNTEXCOORD1FV TexCoord1fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD1I)(GLint);
PFNTEXCOORD1I TexCoord1i = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD1IV)(const GLint *);
PFNTEXCOORD1IV TexCoord1iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD1S)(GLshort);
PFNTEXCOORD1S TexCoord1s = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD1SV)(const GLshort *);
PFNTEXCOORD1SV TexCoord1sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD2D)(GLdouble, GLdouble);
PFNTEXCOORD2D TexCoord2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD2DV)(const GLdouble *);
PFNTEXCOORD2DV TexCoord2dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD2F)(GLfloat, GLfloat);
PFNTEXCOORD2F TexCoord2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD2FV)(const GLfloat *);
PFNTEXCOORD2FV TexCoord2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD2I)(GLint, GLint);
PFNTEXCOORD2I TexCoord2i = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD2IV)(const GLint *);
PFNTEXCOORD2IV TexCoord2iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD2S)(GLshort, GLshort);
PFNTEXCOORD2S TexCoord2s = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD2SV)(const GLshort *);
PFNTEXCOORD2SV TexCoord2sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD3D)(GLdouble, GLdouble, GLdouble);
PFNTEXCOORD3D TexCoord3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD3DV)(const GLdouble *);
PFNTEXCOORD3DV TexCoord3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD3F)(GLfloat, GLfloat, GLfloat);
PFNTEXCOORD3F TexCoord3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD3FV)(const GLfloat *);
PFNTEXCOORD3FV TexCoord3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD3I)(GLint, GLint, GLint);
PFNTEXCOORD3I TexCoord3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD3IV)(const GLint *);
PFNTEXCOORD3IV TexCoord3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD3S)(GLshort, GLshort, GLshort);
PFNTEXCOORD3S TexCoord3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD3SV)(const GLshort *);
PFNTEXCOORD3SV TexCoord3sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD4D)(GLdouble, GLdouble, GLdouble, GLdouble);
PFNTEXCOORD4D TexCoord4d = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD4DV)(const GLdouble *);
PFNTEXCOORD4DV TexCoord4dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD4F)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNTEXCOORD4F TexCoord4f = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD4FV)(const GLfloat *);
PFNTEXCOORD4FV TexCoord4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD4I)(GLint, GLint, GLint, GLint);
PFNTEXCOORD4I TexCoord4i = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD4IV)(const GLint *);
PFNTEXCOORD4IV TexCoord4iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD4S)(GLshort, GLshort, GLshort, GLshort);
PFNTEXCOORD4S TexCoord4s = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORD4SV)(const GLshort *);
PFNTEXCOORD4SV TexCoord4sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXENVF)(GLenum, GLenum, GLfloat);
PFNTEXENVF TexEnvf = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXENVFV)(GLenum, GLenum, const GLfloat *);
PFNTEXENVFV TexEnvfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXENVI)(GLenum, GLenum, GLint);
PFNTEXENVI TexEnvi = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXENVIV)(GLenum, GLenum, const GLint *);
PFNTEXENVIV TexEnviv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXGEND)(GLenum, GLenum, GLdouble);
PFNTEXGEND TexGend = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXGENDV)(GLenum, GLenum, const GLdouble *);
PFNTEXGENDV TexGendv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXGENF)(GLenum, GLenum, GLfloat);
PFNTEXGENF TexGenf = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXGENFV)(GLenum, GLenum, const GLfloat *);
PFNTEXGENFV TexGenfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXGENI)(GLenum, GLenum, GLint);
PFNTEXGENI TexGeni = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXGENIV)(GLenum, GLenum, const GLint *);
PFNTEXGENIV TexGeniv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXIMAGE1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum,
                                             const GLvoid *);
PFNTEXIMAGE1D TexImage1D = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXIMAGE2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum,
                                             GLenum, const GLvoid *);
PFNTEXIMAGE2D TexImage2D = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXPARAMETERF)(GLenum, GLenum, GLfloat);
PFNTEXPARAMETERF TexParameterf = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXPARAMETERFV)(GLenum, GLenum, const GLfloat *);
PFNTEXPARAMETERFV TexParameterfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXPARAMETERI)(GLenum, GLenum, GLint);
PFNTEXPARAMETERI TexParameteri = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXPARAMETERIV)(GLenum, GLenum, const GLint *);
PFNTEXPARAMETERIV TexParameteriv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTRANSLATED)(GLdouble, GLdouble, GLdouble);
PFNTRANSLATED Translated = 0;
typedef void(CODEGEN_FUNCPTR *PFNTRANSLATEF)(GLfloat, GLfloat, GLfloat);
PFNTRANSLATEF Translatef = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX2D)(GLdouble, GLdouble);
PFNVERTEX2D Vertex2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX2DV)(const GLdouble *);
PFNVERTEX2DV Vertex2dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX2F)(GLfloat, GLfloat);
PFNVERTEX2F Vertex2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX2FV)(const GLfloat *);
PFNVERTEX2FV Vertex2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX2I)(GLint, GLint);
PFNVERTEX2I Vertex2i = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX2IV)(const GLint *);
PFNVERTEX2IV Vertex2iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX2S)(GLshort, GLshort);
PFNVERTEX2S Vertex2s = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX2SV)(const GLshort *);
PFNVERTEX2SV Vertex2sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX3D)(GLdouble, GLdouble, GLdouble);
PFNVERTEX3D Vertex3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX3DV)(const GLdouble *);
PFNVERTEX3DV Vertex3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX3F)(GLfloat, GLfloat, GLfloat);
PFNVERTEX3F Vertex3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX3FV)(const GLfloat *);
PFNVERTEX3FV Vertex3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX3I)(GLint, GLint, GLint);
PFNVERTEX3I Vertex3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX3IV)(const GLint *);
PFNVERTEX3IV Vertex3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX3S)(GLshort, GLshort, GLshort);
PFNVERTEX3S Vertex3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX3SV)(const GLshort *);
PFNVERTEX3SV Vertex3sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX4D)(GLdouble, GLdouble, GLdouble, GLdouble);
PFNVERTEX4D Vertex4d = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX4DV)(const GLdouble *);
PFNVERTEX4DV Vertex4dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX4F)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNVERTEX4F Vertex4f = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX4FV)(const GLfloat *);
PFNVERTEX4FV Vertex4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX4I)(GLint, GLint, GLint, GLint);
PFNVERTEX4I Vertex4i = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX4IV)(const GLint *);
PFNVERTEX4IV Vertex4iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX4S)(GLshort, GLshort, GLshort, GLshort);
PFNVERTEX4S Vertex4s = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEX4SV)(const GLshort *);
PFNVERTEX4SV Vertex4sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVIEWPORT)(GLint, GLint, GLsizei, GLsizei);
PFNVIEWPORT Viewport = 0;

typedef GLboolean(CODEGEN_FUNCPTR *PFNARETEXTURESRESIDENT)(GLsizei, const GLuint *, GLboolean *);
PFNARETEXTURESRESIDENT AreTexturesResident = 0;
typedef void(CODEGEN_FUNCPTR *PFNARRAYELEMENT)(GLint);
PFNARRAYELEMENT ArrayElement = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDTEXTURE)(GLenum, GLuint);
PFNBINDTEXTURE BindTexture = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLORPOINTER)(GLint, GLenum, GLsizei, const GLvoid *);
PFNCOLORPOINTER ColorPointer = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOPYTEXIMAGE1D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei,
                                                 GLint);
PFNCOPYTEXIMAGE1D CopyTexImage1D = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOPYTEXIMAGE2D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei,
                                                 GLsizei, GLint);
PFNCOPYTEXIMAGE2D CopyTexImage2D = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOPYTEXSUBIMAGE1D)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
PFNCOPYTEXSUBIMAGE1D CopyTexSubImage1D = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOPYTEXSUBIMAGE2D)(GLenum, GLint, GLint, GLint, GLint, GLint,
                                                    GLsizei, GLsizei);
PFNCOPYTEXSUBIMAGE2D CopyTexSubImage2D = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETETEXTURES)(GLsizei, const GLuint *);
PFNDELETETEXTURES DeleteTextures = 0;
typedef void(CODEGEN_FUNCPTR *PFNDISABLECLIENTSTATE)(GLenum);
PFNDISABLECLIENTSTATE DisableClientState = 0;
typedef void(CODEGEN_FUNCPTR *PFNDRAWARRAYS)(GLenum, GLint, GLsizei);
PFNDRAWARRAYS DrawArrays = 0;
typedef void(CODEGEN_FUNCPTR *PFNDRAWELEMENTS)(GLenum, GLsizei, GLenum, const GLvoid *);
PFNDRAWELEMENTS DrawElements = 0;
typedef void(CODEGEN_FUNCPTR *PFNEDGEFLAGPOINTER)(GLsizei, const GLvoid *);
PFNEDGEFLAGPOINTER EdgeFlagPointer = 0;
typedef void(CODEGEN_FUNCPTR *PFNENABLECLIENTSTATE)(GLenum);
PFNENABLECLIENTSTATE EnableClientState = 0;
typedef void(CODEGEN_FUNCPTR *PFNGENTEXTURES)(GLsizei, GLuint *);
PFNGENTEXTURES GenTextures = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETPOINTERV)(GLenum, GLvoid **);
PFNGETPOINTERV GetPointerv = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXPOINTER)(GLenum, GLsizei, const GLvoid *);
PFNINDEXPOINTER IndexPointer = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXUB)(GLubyte);
PFNINDEXUB Indexub = 0;
typedef void(CODEGEN_FUNCPTR *PFNINDEXUBV)(const GLubyte *);
PFNINDEXUBV Indexubv = 0;
typedef void(CODEGEN_FUNCPTR *PFNINTERLEAVEDARRAYS)(GLenum, GLsizei, const GLvoid *);
PFNINTERLEAVEDARRAYS InterleavedArrays = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISTEXTURE)(GLuint);
PFNISTEXTURE IsTexture = 0;
typedef void(CODEGEN_FUNCPTR *PFNNORMALPOINTER)(GLenum, GLsizei, const GLvoid *);
PFNNORMALPOINTER NormalPointer = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOLYGONOFFSET)(GLfloat, GLfloat);
PFNPOLYGONOFFSET PolygonOffset = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOPCLIENTATTRIB)();
PFNPOPCLIENTATTRIB PopClientAttrib = 0;
typedef void(CODEGEN_FUNCPTR *PFNPRIORITIZETEXTURES)(GLsizei, const GLuint *, const GLfloat *);
PFNPRIORITIZETEXTURES PrioritizeTextures = 0;
typedef void(CODEGEN_FUNCPTR *PFNPUSHCLIENTATTRIB)(GLbitfield);
PFNPUSHCLIENTATTRIB PushClientAttrib = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXCOORDPOINTER)(GLint, GLenum, GLsizei, const GLvoid *);
PFNTEXCOORDPOINTER TexCoordPointer = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXSUBIMAGE1D)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum,
                                                const GLvoid *);
PFNTEXSUBIMAGE1D TexSubImage1D = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXSUBIMAGE2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei,
                                                GLenum, GLenum, const GLvoid *);
PFNTEXSUBIMAGE2D TexSubImage2D = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXPOINTER)(GLint, GLenum, GLsizei, const GLvoid *);
PFNVERTEXPOINTER VertexPointer = 0;

typedef void(CODEGEN_FUNCPTR *PFNBLENDCOLOR)(GLfloat, GLfloat, GLfloat, GLfloat);
PFNBLENDCOLOR BlendColor = 0;
typedef void(CODEGEN_FUNCPTR *PFNBLENDEQUATION)(GLenum);
PFNBLENDEQUATION BlendEquation = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOPYTEXSUBIMAGE3D)(GLenum, GLint, GLint, GLint, GLint, GLint,
                                                    GLint, GLsizei, GLsizei);
PFNCOPYTEXSUBIMAGE3D CopyTexSubImage3D = 0;
typedef void(CODEGEN_FUNCPTR *PFNDRAWRANGEELEMENTS)(GLenum, GLuint, GLuint, GLsizei, GLenum,
                                                    const GLvoid *);
PFNDRAWRANGEELEMENTS DrawRangeElements = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXIMAGE3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint,
                                             GLenum, GLenum, const GLvoid *);
PFNTEXIMAGE3D TexImage3D = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXSUBIMAGE3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei,
                                                GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
PFNTEXSUBIMAGE3D TexSubImage3D = 0;

typedef void(CODEGEN_FUNCPTR *PFNACTIVETEXTURE)(GLenum);
PFNACTIVETEXTURE ActiveTexture = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLIENTACTIVETEXTURE)(GLenum);
PFNCLIENTACTIVETEXTURE ClientActiveTexture = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXIMAGE1D)(GLenum, GLint, GLenum, GLsizei, GLint,
                                                       GLsizei, const GLvoid *);
PFNCOMPRESSEDTEXIMAGE1D CompressedTexImage1D = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXIMAGE2D)(GLenum, GLint, GLenum, GLsizei, GLsizei,
                                                       GLint, GLsizei, const GLvoid *);
PFNCOMPRESSEDTEXIMAGE2D CompressedTexImage2D = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXIMAGE3D)(GLenum, GLint, GLenum, GLsizei, GLsizei,
                                                       GLsizei, GLint, GLsizei, const GLvoid *);
PFNCOMPRESSEDTEXIMAGE3D CompressedTexImage3D = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXSUBIMAGE1D)(GLenum, GLint, GLint, GLsizei, GLenum,
                                                          GLsizei, const GLvoid *);
PFNCOMPRESSEDTEXSUBIMAGE1D CompressedTexSubImage1D = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXSUBIMAGE2D)(GLenum, GLint, GLint, GLint, GLsizei,
                                                          GLsizei, GLenum, GLsizei, const GLvoid *);
PFNCOMPRESSEDTEXSUBIMAGE2D CompressedTexSubImage2D = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOMPRESSEDTEXSUBIMAGE3D)(GLenum, GLint, GLint, GLint, GLint,
                                                          GLsizei, GLsizei, GLsizei, GLenum,
                                                          GLsizei, const GLvoid *);
PFNCOMPRESSEDTEXSUBIMAGE3D CompressedTexSubImage3D = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETCOMPRESSEDTEXIMAGE)(GLenum, GLint, GLvoid *);
PFNGETCOMPRESSEDTEXIMAGE GetCompressedTexImage = 0;
typedef void(CODEGEN_FUNCPTR *PFNLOADTRANSPOSEMATRIXD)(const GLdouble *);
PFNLOADTRANSPOSEMATRIXD LoadTransposeMatrixd = 0;
typedef void(CODEGEN_FUNCPTR *PFNLOADTRANSPOSEMATRIXF)(const GLfloat *);
PFNLOADTRANSPOSEMATRIXF LoadTransposeMatrixf = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTTRANSPOSEMATRIXD)(const GLdouble *);
PFNMULTTRANSPOSEMATRIXD MultTransposeMatrixd = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTTRANSPOSEMATRIXF)(const GLfloat *);
PFNMULTTRANSPOSEMATRIXF MultTransposeMatrixf = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD1D)(GLenum, GLdouble);
PFNMULTITEXCOORD1D MultiTexCoord1d = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD1DV)(GLenum, const GLdouble *);
PFNMULTITEXCOORD1DV MultiTexCoord1dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD1F)(GLenum, GLfloat);
PFNMULTITEXCOORD1F MultiTexCoord1f = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD1FV)(GLenum, const GLfloat *);
PFNMULTITEXCOORD1FV MultiTexCoord1fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD1I)(GLenum, GLint);
PFNMULTITEXCOORD1I MultiTexCoord1i = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD1IV)(GLenum, const GLint *);
PFNMULTITEXCOORD1IV MultiTexCoord1iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD1S)(GLenum, GLshort);
PFNMULTITEXCOORD1S MultiTexCoord1s = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD1SV)(GLenum, const GLshort *);
PFNMULTITEXCOORD1SV MultiTexCoord1sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD2D)(GLenum, GLdouble, GLdouble);
PFNMULTITEXCOORD2D MultiTexCoord2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD2DV)(GLenum, const GLdouble *);
PFNMULTITEXCOORD2DV MultiTexCoord2dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD2F)(GLenum, GLfloat, GLfloat);
PFNMULTITEXCOORD2F MultiTexCoord2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD2FV)(GLenum, const GLfloat *);
PFNMULTITEXCOORD2FV MultiTexCoord2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD2I)(GLenum, GLint, GLint);
PFNMULTITEXCOORD2I MultiTexCoord2i = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD2IV)(GLenum, const GLint *);
PFNMULTITEXCOORD2IV MultiTexCoord2iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD2S)(GLenum, GLshort, GLshort);
PFNMULTITEXCOORD2S MultiTexCoord2s = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD2SV)(GLenum, const GLshort *);
PFNMULTITEXCOORD2SV MultiTexCoord2sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD3D)(GLenum, GLdouble, GLdouble, GLdouble);
PFNMULTITEXCOORD3D MultiTexCoord3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD3DV)(GLenum, const GLdouble *);
PFNMULTITEXCOORD3DV MultiTexCoord3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD3F)(GLenum, GLfloat, GLfloat, GLfloat);
PFNMULTITEXCOORD3F MultiTexCoord3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD3FV)(GLenum, const GLfloat *);
PFNMULTITEXCOORD3FV MultiTexCoord3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD3I)(GLenum, GLint, GLint, GLint);
PFNMULTITEXCOORD3I MultiTexCoord3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD3IV)(GLenum, const GLint *);
PFNMULTITEXCOORD3IV MultiTexCoord3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD3S)(GLenum, GLshort, GLshort, GLshort);
PFNMULTITEXCOORD3S MultiTexCoord3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD3SV)(GLenum, const GLshort *);
PFNMULTITEXCOORD3SV MultiTexCoord3sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD4D)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
PFNMULTITEXCOORD4D MultiTexCoord4d = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD4DV)(GLenum, const GLdouble *);
PFNMULTITEXCOORD4DV MultiTexCoord4dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD4F)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
PFNMULTITEXCOORD4F MultiTexCoord4f = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD4FV)(GLenum, const GLfloat *);
PFNMULTITEXCOORD4FV MultiTexCoord4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD4I)(GLenum, GLint, GLint, GLint, GLint);
PFNMULTITEXCOORD4I MultiTexCoord4i = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD4IV)(GLenum, const GLint *);
PFNMULTITEXCOORD4IV MultiTexCoord4iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD4S)(GLenum, GLshort, GLshort, GLshort, GLshort);
PFNMULTITEXCOORD4S MultiTexCoord4s = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTITEXCOORD4SV)(GLenum, const GLshort *);
PFNMULTITEXCOORD4SV MultiTexCoord4sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSAMPLECOVERAGE)(GLfloat, GLboolean);
PFNSAMPLECOVERAGE SampleCoverage = 0;

typedef void(CODEGEN_FUNCPTR *PFNBLENDFUNCSEPARATE)(GLenum, GLenum, GLenum, GLenum);
PFNBLENDFUNCSEPARATE BlendFuncSeparate = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGCOORDPOINTER)(GLenum, GLsizei, const GLvoid *);
PFNFOGCOORDPOINTER FogCoordPointer = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGCOORDD)(GLdouble);
PFNFOGCOORDD FogCoordd = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGCOORDDV)(const GLdouble *);
PFNFOGCOORDDV FogCoorddv = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGCOORDF)(GLfloat);
PFNFOGCOORDF FogCoordf = 0;
typedef void(CODEGEN_FUNCPTR *PFNFOGCOORDFV)(const GLfloat *);
PFNFOGCOORDFV FogCoordfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTIDRAWARRAYS)(GLenum, const GLint *, const GLsizei *, GLsizei);
PFNMULTIDRAWARRAYS MultiDrawArrays = 0;
typedef void(CODEGEN_FUNCPTR *PFNMULTIDRAWELEMENTS)(GLenum, const GLsizei *, GLenum,
                                                    const GLvoid *const *, GLsizei);
PFNMULTIDRAWELEMENTS MultiDrawElements = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOINTPARAMETERF)(GLenum, GLfloat);
PFNPOINTPARAMETERF PointParameterf = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOINTPARAMETERFV)(GLenum, const GLfloat *);
PFNPOINTPARAMETERFV PointParameterfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOINTPARAMETERI)(GLenum, GLint);
PFNPOINTPARAMETERI PointParameteri = 0;
typedef void(CODEGEN_FUNCPTR *PFNPOINTPARAMETERIV)(GLenum, const GLint *);
PFNPOINTPARAMETERIV PointParameteriv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3B)(GLbyte, GLbyte, GLbyte);
PFNSECONDARYCOLOR3B SecondaryColor3b = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3BV)(const GLbyte *);
PFNSECONDARYCOLOR3BV SecondaryColor3bv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3D)(GLdouble, GLdouble, GLdouble);
PFNSECONDARYCOLOR3D SecondaryColor3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3DV)(const GLdouble *);
PFNSECONDARYCOLOR3DV SecondaryColor3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3F)(GLfloat, GLfloat, GLfloat);
PFNSECONDARYCOLOR3F SecondaryColor3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3FV)(const GLfloat *);
PFNSECONDARYCOLOR3FV SecondaryColor3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3I)(GLint, GLint, GLint);
PFNSECONDARYCOLOR3I SecondaryColor3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3IV)(const GLint *);
PFNSECONDARYCOLOR3IV SecondaryColor3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3S)(GLshort, GLshort, GLshort);
PFNSECONDARYCOLOR3S SecondaryColor3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3SV)(const GLshort *);
PFNSECONDARYCOLOR3SV SecondaryColor3sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3UB)(GLubyte, GLubyte, GLubyte);
PFNSECONDARYCOLOR3UB SecondaryColor3ub = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3UBV)(const GLubyte *);
PFNSECONDARYCOLOR3UBV SecondaryColor3ubv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3UI)(GLuint, GLuint, GLuint);
PFNSECONDARYCOLOR3UI SecondaryColor3ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3UIV)(const GLuint *);
PFNSECONDARYCOLOR3UIV SecondaryColor3uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3US)(GLushort, GLushort, GLushort);
PFNSECONDARYCOLOR3US SecondaryColor3us = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLOR3USV)(const GLushort *);
PFNSECONDARYCOLOR3USV SecondaryColor3usv = 0;
typedef void(CODEGEN_FUNCPTR *PFNSECONDARYCOLORPOINTER)(GLint, GLenum, GLsizei, const GLvoid *);
PFNSECONDARYCOLORPOINTER SecondaryColorPointer = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS2D)(GLdouble, GLdouble);
PFNWINDOWPOS2D WindowPos2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS2DV)(const GLdouble *);
PFNWINDOWPOS2DV WindowPos2dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS2F)(GLfloat, GLfloat);
PFNWINDOWPOS2F WindowPos2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS2FV)(const GLfloat *);
PFNWINDOWPOS2FV WindowPos2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS2I)(GLint, GLint);
PFNWINDOWPOS2I WindowPos2i = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS2IV)(const GLint *);
PFNWINDOWPOS2IV WindowPos2iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS2S)(GLshort, GLshort);
PFNWINDOWPOS2S WindowPos2s = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS2SV)(const GLshort *);
PFNWINDOWPOS2SV WindowPos2sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS3D)(GLdouble, GLdouble, GLdouble);
PFNWINDOWPOS3D WindowPos3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS3DV)(const GLdouble *);
PFNWINDOWPOS3DV WindowPos3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS3F)(GLfloat, GLfloat, GLfloat);
PFNWINDOWPOS3F WindowPos3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS3FV)(const GLfloat *);
PFNWINDOWPOS3FV WindowPos3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS3I)(GLint, GLint, GLint);
PFNWINDOWPOS3I WindowPos3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS3IV)(const GLint *);
PFNWINDOWPOS3IV WindowPos3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS3S)(GLshort, GLshort, GLshort);
PFNWINDOWPOS3S WindowPos3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNWINDOWPOS3SV)(const GLshort *);
PFNWINDOWPOS3SV WindowPos3sv = 0;

typedef void(CODEGEN_FUNCPTR *PFNBEGINQUERY)(GLenum, GLuint);
PFNBEGINQUERY BeginQuery = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDBUFFER)(GLenum, GLuint);
PFNBINDBUFFER BindBuffer = 0;
typedef void(CODEGEN_FUNCPTR *PFNBUFFERDATA)(GLenum, GLsizeiptr, const GLvoid *, GLenum);
PFNBUFFERDATA BufferData = 0;
typedef void(CODEGEN_FUNCPTR *PFNBUFFERSUBDATA)(GLenum, GLintptr, GLsizeiptr, const GLvoid *);
PFNBUFFERSUBDATA BufferSubData = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETEBUFFERS)(GLsizei, const GLuint *);
PFNDELETEBUFFERS DeleteBuffers = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETEQUERIES)(GLsizei, const GLuint *);
PFNDELETEQUERIES DeleteQueries = 0;
typedef void(CODEGEN_FUNCPTR *PFNENDQUERY)(GLenum);
PFNENDQUERY EndQuery = 0;
typedef void(CODEGEN_FUNCPTR *PFNGENBUFFERS)(GLsizei, GLuint *);
PFNGENBUFFERS GenBuffers = 0;
typedef void(CODEGEN_FUNCPTR *PFNGENQUERIES)(GLsizei, GLuint *);
PFNGENQUERIES GenQueries = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETBUFFERPARAMETERIV)(GLenum, GLenum, GLint *);
PFNGETBUFFERPARAMETERIV GetBufferParameteriv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETBUFFERPOINTERV)(GLenum, GLenum, GLvoid **);
PFNGETBUFFERPOINTERV GetBufferPointerv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETBUFFERSUBDATA)(GLenum, GLintptr, GLsizeiptr, GLvoid *);
PFNGETBUFFERSUBDATA GetBufferSubData = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETQUERYOBJECTIV)(GLuint, GLenum, GLint *);
PFNGETQUERYOBJECTIV GetQueryObjectiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETQUERYOBJECTUIV)(GLuint, GLenum, GLuint *);
PFNGETQUERYOBJECTUIV GetQueryObjectuiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETQUERYIV)(GLenum, GLenum, GLint *);
PFNGETQUERYIV GetQueryiv = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISBUFFER)(GLuint);
PFNISBUFFER IsBuffer = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISQUERY)(GLuint);
PFNISQUERY IsQuery = 0;
typedef void *(CODEGEN_FUNCPTR *PFNMAPBUFFER)(GLenum, GLenum);
PFNMAPBUFFER MapBuffer = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNUNMAPBUFFER)(GLenum);
PFNUNMAPBUFFER UnmapBuffer = 0;

typedef void(CODEGEN_FUNCPTR *PFNATTACHSHADER)(GLuint, GLuint);
PFNATTACHSHADER AttachShader = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDATTRIBLOCATION)(GLuint, GLuint, const GLchar *);
PFNBINDATTRIBLOCATION BindAttribLocation = 0;
typedef void(CODEGEN_FUNCPTR *PFNBLENDEQUATIONSEPARATE)(GLenum, GLenum);
PFNBLENDEQUATIONSEPARATE BlendEquationSeparate = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOMPILESHADER)(GLuint);
PFNCOMPILESHADER CompileShader = 0;
typedef GLuint(CODEGEN_FUNCPTR *PFNCREATEPROGRAM)();
PFNCREATEPROGRAM CreateProgram = 0;
typedef GLuint(CODEGEN_FUNCPTR *PFNCREATESHADER)(GLenum);
PFNCREATESHADER CreateShader = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETEPROGRAM)(GLuint);
PFNDELETEPROGRAM DeleteProgram = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETESHADER)(GLuint);
PFNDELETESHADER DeleteShader = 0;
typedef void(CODEGEN_FUNCPTR *PFNDETACHSHADER)(GLuint, GLuint);
PFNDETACHSHADER DetachShader = 0;
typedef void(CODEGEN_FUNCPTR *PFNDISABLEVERTEXATTRIBARRAY)(GLuint);
PFNDISABLEVERTEXATTRIBARRAY DisableVertexAttribArray = 0;
typedef void(CODEGEN_FUNCPTR *PFNDRAWBUFFERS)(GLsizei, const GLenum *);
PFNDRAWBUFFERS DrawBuffers = 0;
typedef void(CODEGEN_FUNCPTR *PFNENABLEVERTEXATTRIBARRAY)(GLuint);
PFNENABLEVERTEXATTRIBARRAY EnableVertexAttribArray = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETACTIVEATTRIB)(GLuint, GLuint, GLsizei, GLsizei *, GLint *,
                                                  GLenum *, GLchar *);
PFNGETACTIVEATTRIB GetActiveAttrib = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETACTIVEUNIFORM)(GLuint, GLuint, GLsizei, GLsizei *, GLint *,
                                                   GLenum *, GLchar *);
PFNGETACTIVEUNIFORM GetActiveUniform = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETATTACHEDSHADERS)(GLuint, GLsizei, GLsizei *, GLuint *);
PFNGETATTACHEDSHADERS GetAttachedShaders = 0;
typedef GLint(CODEGEN_FUNCPTR *PFNGETATTRIBLOCATION)(GLuint, const GLchar *);
PFNGETATTRIBLOCATION GetAttribLocation = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETPROGRAMINFOLOG)(GLuint, GLsizei, GLsizei *, GLchar *);
PFNGETPROGRAMINFOLOG GetProgramInfoLog = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETPROGRAMIV)(GLuint, GLenum, GLint *);
PFNGETPROGRAMIV GetProgramiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETSHADERINFOLOG)(GLuint, GLsizei, GLsizei *, GLchar *);
PFNGETSHADERINFOLOG GetShaderInfoLog = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETSHADERSOURCE)(GLuint, GLsizei, GLsizei *, GLchar *);
PFNGETSHADERSOURCE GetShaderSource = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETSHADERIV)(GLuint, GLenum, GLint *);
PFNGETSHADERIV GetShaderiv = 0;
typedef GLint(CODEGEN_FUNCPTR *PFNGETUNIFORMLOCATION)(GLuint, const GLchar *);
PFNGETUNIFORMLOCATION GetUniformLocation = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETUNIFORMFV)(GLuint, GLint, GLfloat *);
PFNGETUNIFORMFV GetUniformfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETUNIFORMIV)(GLuint, GLint, GLint *);
PFNGETUNIFORMIV GetUniformiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBPOINTERV)(GLuint, GLenum, GLvoid **);
PFNGETVERTEXATTRIBPOINTERV GetVertexAttribPointerv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBDV)(GLuint, GLenum, GLdouble *);
PFNGETVERTEXATTRIBDV GetVertexAttribdv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBFV)(GLuint, GLenum, GLfloat *);
PFNGETVERTEXATTRIBFV GetVertexAttribfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBIV)(GLuint, GLenum, GLint *);
PFNGETVERTEXATTRIBIV GetVertexAttribiv = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISPROGRAM)(GLuint);
PFNISPROGRAM IsProgram = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISSHADER)(GLuint);
PFNISSHADER IsShader = 0;
typedef void(CODEGEN_FUNCPTR *PFNLINKPROGRAM)(GLuint);
PFNLINKPROGRAM LinkProgram = 0;
typedef void(CODEGEN_FUNCPTR *PFNSHADERSOURCE)(GLuint, GLsizei, const GLchar *const *,
                                               const GLint *);
PFNSHADERSOURCE ShaderSource = 0;
typedef void(CODEGEN_FUNCPTR *PFNSTENCILFUNCSEPARATE)(GLenum, GLenum, GLint, GLuint);
PFNSTENCILFUNCSEPARATE StencilFuncSeparate = 0;
typedef void(CODEGEN_FUNCPTR *PFNSTENCILMASKSEPARATE)(GLenum, GLuint);
PFNSTENCILMASKSEPARATE StencilMaskSeparate = 0;
typedef void(CODEGEN_FUNCPTR *PFNSTENCILOPSEPARATE)(GLenum, GLenum, GLenum, GLenum);
PFNSTENCILOPSEPARATE StencilOpSeparate = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM1F)(GLint, GLfloat);
PFNUNIFORM1F Uniform1f = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM1FV)(GLint, GLsizei, const GLfloat *);
PFNUNIFORM1FV Uniform1fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM1I)(GLint, GLint);
PFNUNIFORM1I Uniform1i = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM1IV)(GLint, GLsizei, const GLint *);
PFNUNIFORM1IV Uniform1iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM2F)(GLint, GLfloat, GLfloat);
PFNUNIFORM2F Uniform2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM2FV)(GLint, GLsizei, const GLfloat *);
PFNUNIFORM2FV Uniform2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM2I)(GLint, GLint, GLint);
PFNUNIFORM2I Uniform2i = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM2IV)(GLint, GLsizei, const GLint *);
PFNUNIFORM2IV Uniform2iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM3F)(GLint, GLfloat, GLfloat, GLfloat);
PFNUNIFORM3F Uniform3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM3FV)(GLint, GLsizei, const GLfloat *);
PFNUNIFORM3FV Uniform3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM3I)(GLint, GLint, GLint, GLint);
PFNUNIFORM3I Uniform3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM3IV)(GLint, GLsizei, const GLint *);
PFNUNIFORM3IV Uniform3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM4F)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
PFNUNIFORM4F Uniform4f = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM4FV)(GLint, GLsizei, const GLfloat *);
PFNUNIFORM4FV Uniform4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM4I)(GLint, GLint, GLint, GLint, GLint);
PFNUNIFORM4I Uniform4i = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM4IV)(GLint, GLsizei, const GLint *);
PFNUNIFORM4IV Uniform4iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX2FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX2FV UniformMatrix2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX3FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX3FV UniformMatrix3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX4FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX4FV UniformMatrix4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUSEPROGRAM)(GLuint);
PFNUSEPROGRAM UseProgram = 0;
typedef void(CODEGEN_FUNCPTR *PFNVALIDATEPROGRAM)(GLuint);
PFNVALIDATEPROGRAM ValidateProgram = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB1D)(GLuint, GLdouble);
PFNVERTEXATTRIB1D VertexAttrib1d = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB1DV)(GLuint, const GLdouble *);
PFNVERTEXATTRIB1DV VertexAttrib1dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB1F)(GLuint, GLfloat);
PFNVERTEXATTRIB1F VertexAttrib1f = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB1FV)(GLuint, const GLfloat *);
PFNVERTEXATTRIB1FV VertexAttrib1fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB1S)(GLuint, GLshort);
PFNVERTEXATTRIB1S VertexAttrib1s = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB1SV)(GLuint, const GLshort *);
PFNVERTEXATTRIB1SV VertexAttrib1sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB2D)(GLuint, GLdouble, GLdouble);
PFNVERTEXATTRIB2D VertexAttrib2d = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB2DV)(GLuint, const GLdouble *);
PFNVERTEXATTRIB2DV VertexAttrib2dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB2F)(GLuint, GLfloat, GLfloat);
PFNVERTEXATTRIB2F VertexAttrib2f = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB2FV)(GLuint, const GLfloat *);
PFNVERTEXATTRIB2FV VertexAttrib2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB2S)(GLuint, GLshort, GLshort);
PFNVERTEXATTRIB2S VertexAttrib2s = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB2SV)(GLuint, const GLshort *);
PFNVERTEXATTRIB2SV VertexAttrib2sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB3D)(GLuint, GLdouble, GLdouble, GLdouble);
PFNVERTEXATTRIB3D VertexAttrib3d = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB3DV)(GLuint, const GLdouble *);
PFNVERTEXATTRIB3DV VertexAttrib3dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB3F)(GLuint, GLfloat, GLfloat, GLfloat);
PFNVERTEXATTRIB3F VertexAttrib3f = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB3FV)(GLuint, const GLfloat *);
PFNVERTEXATTRIB3FV VertexAttrib3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB3S)(GLuint, GLshort, GLshort, GLshort);
PFNVERTEXATTRIB3S VertexAttrib3s = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB3SV)(GLuint, const GLshort *);
PFNVERTEXATTRIB3SV VertexAttrib3sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NBV)(GLuint, const GLbyte *);
PFNVERTEXATTRIB4NBV VertexAttrib4Nbv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NIV)(GLuint, const GLint *);
PFNVERTEXATTRIB4NIV VertexAttrib4Niv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NSV)(GLuint, const GLshort *);
PFNVERTEXATTRIB4NSV VertexAttrib4Nsv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NUB)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
PFNVERTEXATTRIB4NUB VertexAttrib4Nub = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NUBV)(GLuint, const GLubyte *);
PFNVERTEXATTRIB4NUBV VertexAttrib4Nubv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NUIV)(GLuint, const GLuint *);
PFNVERTEXATTRIB4NUIV VertexAttrib4Nuiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4NUSV)(GLuint, const GLushort *);
PFNVERTEXATTRIB4NUSV VertexAttrib4Nusv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4BV)(GLuint, const GLbyte *);
PFNVERTEXATTRIB4BV VertexAttrib4bv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4D)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
PFNVERTEXATTRIB4D VertexAttrib4d = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4DV)(GLuint, const GLdouble *);
PFNVERTEXATTRIB4DV VertexAttrib4dv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4F)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
PFNVERTEXATTRIB4F VertexAttrib4f = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4FV)(GLuint, const GLfloat *);
PFNVERTEXATTRIB4FV VertexAttrib4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4IV)(GLuint, const GLint *);
PFNVERTEXATTRIB4IV VertexAttrib4iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4S)(GLuint, GLshort, GLshort, GLshort, GLshort);
PFNVERTEXATTRIB4S VertexAttrib4s = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4SV)(GLuint, const GLshort *);
PFNVERTEXATTRIB4SV VertexAttrib4sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4UBV)(GLuint, const GLubyte *);
PFNVERTEXATTRIB4UBV VertexAttrib4ubv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4UIV)(GLuint, const GLuint *);
PFNVERTEXATTRIB4UIV VertexAttrib4uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIB4USV)(GLuint, const GLushort *);
PFNVERTEXATTRIB4USV VertexAttrib4usv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBPOINTER)(GLuint, GLint, GLenum, GLboolean, GLsizei,
                                                      const GLvoid *);
PFNVERTEXATTRIBPOINTER VertexAttribPointer = 0;

typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX2X3FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX2X3FV UniformMatrix2x3fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX2X4FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX2X4FV UniformMatrix2x4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX3X2FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX3X2FV UniformMatrix3x2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX3X4FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX3X4FV UniformMatrix3x4fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX4X2FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX4X2FV UniformMatrix4x2fv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORMMATRIX4X3FV)(GLint, GLsizei, GLboolean, const GLfloat *);
PFNUNIFORMMATRIX4X3FV UniformMatrix4x3fv = 0;

typedef void(CODEGEN_FUNCPTR *PFNBEGINCONDITIONALRENDER)(GLuint, GLenum);
PFNBEGINCONDITIONALRENDER BeginConditionalRender = 0;
typedef void(CODEGEN_FUNCPTR *PFNBEGINTRANSFORMFEEDBACK)(GLenum);
PFNBEGINTRANSFORMFEEDBACK BeginTransformFeedback = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDBUFFERBASE)(GLenum, GLuint, GLuint);
PFNBINDBUFFERBASE BindBufferBase = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDBUFFERRANGE)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr);
PFNBINDBUFFERRANGE BindBufferRange = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDFRAGDATALOCATION)(GLuint, GLuint, const GLchar *);
PFNBINDFRAGDATALOCATION BindFragDataLocation = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDFRAMEBUFFER)(GLenum, GLuint);
PFNBINDFRAMEBUFFER BindFramebuffer = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDRENDERBUFFER)(GLenum, GLuint);
PFNBINDRENDERBUFFER BindRenderbuffer = 0;
typedef void(CODEGEN_FUNCPTR *PFNBINDVERTEXARRAY)(GLuint);
PFNBINDVERTEXARRAY BindVertexArray = 0;
typedef void(CODEGEN_FUNCPTR *PFNBLITFRAMEBUFFER)(GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                                                  GLint, GLbitfield, GLenum);
PFNBLITFRAMEBUFFER BlitFramebuffer = 0;
typedef GLenum(CODEGEN_FUNCPTR *PFNCHECKFRAMEBUFFERSTATUS)(GLenum);
PFNCHECKFRAMEBUFFERSTATUS CheckFramebufferStatus = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLAMPCOLOR)(GLenum, GLenum);
PFNCLAMPCOLOR ClampColor = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARBUFFERFI)(GLenum, GLint, GLfloat, GLint);
PFNCLEARBUFFERFI ClearBufferfi = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARBUFFERFV)(GLenum, GLint, const GLfloat *);
PFNCLEARBUFFERFV ClearBufferfv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARBUFFERIV)(GLenum, GLint, const GLint *);
PFNCLEARBUFFERIV ClearBufferiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCLEARBUFFERUIV)(GLenum, GLint, const GLuint *);
PFNCLEARBUFFERUIV ClearBufferuiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNCOLORMASKI)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean);
PFNCOLORMASKI ColorMaski = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETEFRAMEBUFFERS)(GLsizei, const GLuint *);
PFNDELETEFRAMEBUFFERS DeleteFramebuffers = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETERENDERBUFFERS)(GLsizei, const GLuint *);
PFNDELETERENDERBUFFERS DeleteRenderbuffers = 0;
typedef void(CODEGEN_FUNCPTR *PFNDELETEVERTEXARRAYS)(GLsizei, const GLuint *);
PFNDELETEVERTEXARRAYS DeleteVertexArrays = 0;
typedef void(CODEGEN_FUNCPTR *PFNDISABLEI)(GLenum, GLuint);
PFNDISABLEI Disablei = 0;
typedef void(CODEGEN_FUNCPTR *PFNENABLEI)(GLenum, GLuint);
PFNENABLEI Enablei = 0;
typedef void(CODEGEN_FUNCPTR *PFNENDCONDITIONALRENDER)();
PFNENDCONDITIONALRENDER EndConditionalRender = 0;
typedef void(CODEGEN_FUNCPTR *PFNENDTRANSFORMFEEDBACK)();
PFNENDTRANSFORMFEEDBACK EndTransformFeedback = 0;
typedef void(CODEGEN_FUNCPTR *PFNFLUSHMAPPEDBUFFERRANGE)(GLenum, GLintptr, GLsizeiptr);
PFNFLUSHMAPPEDBUFFERRANGE FlushMappedBufferRange = 0;
typedef void(CODEGEN_FUNCPTR *PFNFRAMEBUFFERRENDERBUFFER)(GLenum, GLenum, GLenum, GLuint);
PFNFRAMEBUFFERRENDERBUFFER FramebufferRenderbuffer = 0;
typedef void(CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURE1D)(GLenum, GLenum, GLenum, GLuint, GLint);
PFNFRAMEBUFFERTEXTURE1D FramebufferTexture1D = 0;
typedef void(CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURE2D)(GLenum, GLenum, GLenum, GLuint, GLint);
PFNFRAMEBUFFERTEXTURE2D FramebufferTexture2D = 0;
typedef void(CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURE3D)(GLenum, GLenum, GLenum, GLuint, GLint,
                                                       GLint);
PFNFRAMEBUFFERTEXTURE3D FramebufferTexture3D = 0;
typedef void(CODEGEN_FUNCPTR *PFNFRAMEBUFFERTEXTURELAYER)(GLenum, GLenum, GLuint, GLint, GLint);
PFNFRAMEBUFFERTEXTURELAYER FramebufferTextureLayer = 0;
typedef void(CODEGEN_FUNCPTR *PFNGENFRAMEBUFFERS)(GLsizei, GLuint *);
PFNGENFRAMEBUFFERS GenFramebuffers = 0;
typedef void(CODEGEN_FUNCPTR *PFNGENRENDERBUFFERS)(GLsizei, GLuint *);
PFNGENRENDERBUFFERS GenRenderbuffers = 0;
typedef void(CODEGEN_FUNCPTR *PFNGENVERTEXARRAYS)(GLsizei, GLuint *);
PFNGENVERTEXARRAYS GenVertexArrays = 0;
typedef void(CODEGEN_FUNCPTR *PFNGENERATEMIPMAP)(GLenum);
PFNGENERATEMIPMAP GenerateMipmap = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETBOOLEANI_V)(GLenum, GLuint, GLboolean *);
PFNGETBOOLEANI_V GetBooleani_v = 0;
typedef GLint(CODEGEN_FUNCPTR *PFNGETFRAGDATALOCATION)(GLuint, const GLchar *);
PFNGETFRAGDATALOCATION GetFragDataLocation = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETFRAMEBUFFERATTACHMENTPARAMETERIV)(GLenum, GLenum, GLenum,
                                                                      GLint *);
PFNGETFRAMEBUFFERATTACHMENTPARAMETERIV GetFramebufferAttachmentParameteriv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETINTEGERI_V)(GLenum, GLuint, GLint *);
PFNGETINTEGERI_V GetIntegeri_v = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETRENDERBUFFERPARAMETERIV)(GLenum, GLenum, GLint *);
PFNGETRENDERBUFFERPARAMETERIV GetRenderbufferParameteriv = 0;
typedef const GLubyte *(CODEGEN_FUNCPTR *PFNGETSTRINGI)(GLenum, GLuint);
PFNGETSTRINGI GetStringi = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXPARAMETERIIV)(GLenum, GLenum, GLint *);
PFNGETTEXPARAMETERIIV GetTexParameterIiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTEXPARAMETERIUIV)(GLenum, GLenum, GLuint *);
PFNGETTEXPARAMETERIUIV GetTexParameterIuiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETTRANSFORMFEEDBACKVARYING)(GLuint, GLuint, GLsizei, GLsizei *,
                                                              GLsizei *, GLenum *, GLchar *);
PFNGETTRANSFORMFEEDBACKVARYING GetTransformFeedbackVarying = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETUNIFORMUIV)(GLuint, GLint, GLuint *);
PFNGETUNIFORMUIV GetUniformuiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBIIV)(GLuint, GLenum, GLint *);
PFNGETVERTEXATTRIBIIV GetVertexAttribIiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNGETVERTEXATTRIBIUIV)(GLuint, GLenum, GLuint *);
PFNGETVERTEXATTRIBIUIV GetVertexAttribIuiv = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISENABLEDI)(GLenum, GLuint);
PFNISENABLEDI IsEnabledi = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISFRAMEBUFFER)(GLuint);
PFNISFRAMEBUFFER IsFramebuffer = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISRENDERBUFFER)(GLuint);
PFNISRENDERBUFFER IsRenderbuffer = 0;
typedef GLboolean(CODEGEN_FUNCPTR *PFNISVERTEXARRAY)(GLuint);
PFNISVERTEXARRAY IsVertexArray = 0;
typedef void *(CODEGEN_FUNCPTR *PFNMAPBUFFERRANGE)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
PFNMAPBUFFERRANGE MapBufferRange = 0;
typedef void(CODEGEN_FUNCPTR *PFNRENDERBUFFERSTORAGE)(GLenum, GLenum, GLsizei, GLsizei);
PFNRENDERBUFFERSTORAGE RenderbufferStorage = 0;
typedef void(CODEGEN_FUNCPTR *PFNRENDERBUFFERSTORAGEMULTISAMPLE)(GLenum, GLsizei, GLenum, GLsizei,
                                                                 GLsizei);
PFNRENDERBUFFERSTORAGEMULTISAMPLE RenderbufferStorageMultisample = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXPARAMETERIIV)(GLenum, GLenum, const GLint *);
PFNTEXPARAMETERIIV TexParameterIiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTEXPARAMETERIUIV)(GLenum, GLenum, const GLuint *);
PFNTEXPARAMETERIUIV TexParameterIuiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNTRANSFORMFEEDBACKVARYINGS)(GLuint, GLsizei, const GLchar *const *,
                                                            GLenum);
PFNTRANSFORMFEEDBACKVARYINGS TransformFeedbackVaryings = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM1UI)(GLint, GLuint);
PFNUNIFORM1UI Uniform1ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM1UIV)(GLint, GLsizei, const GLuint *);
PFNUNIFORM1UIV Uniform1uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM2UI)(GLint, GLuint, GLuint);
PFNUNIFORM2UI Uniform2ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM2UIV)(GLint, GLsizei, const GLuint *);
PFNUNIFORM2UIV Uniform2uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM3UI)(GLint, GLuint, GLuint, GLuint);
PFNUNIFORM3UI Uniform3ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM3UIV)(GLint, GLsizei, const GLuint *);
PFNUNIFORM3UIV Uniform3uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM4UI)(GLint, GLuint, GLuint, GLuint, GLuint);
PFNUNIFORM4UI Uniform4ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNUNIFORM4UIV)(GLint, GLsizei, const GLuint *);
PFNUNIFORM4UIV Uniform4uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI1I)(GLuint, GLint);
PFNVERTEXATTRIBI1I VertexAttribI1i = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI1IV)(GLuint, const GLint *);
PFNVERTEXATTRIBI1IV VertexAttribI1iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI1UI)(GLuint, GLuint);
PFNVERTEXATTRIBI1UI VertexAttribI1ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI1UIV)(GLuint, const GLuint *);
PFNVERTEXATTRIBI1UIV VertexAttribI1uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI2I)(GLuint, GLint, GLint);
PFNVERTEXATTRIBI2I VertexAttribI2i = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI2IV)(GLuint, const GLint *);
PFNVERTEXATTRIBI2IV VertexAttribI2iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI2UI)(GLuint, GLuint, GLuint);
PFNVERTEXATTRIBI2UI VertexAttribI2ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI2UIV)(GLuint, const GLuint *);
PFNVERTEXATTRIBI2UIV VertexAttribI2uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI3I)(GLuint, GLint, GLint, GLint);
PFNVERTEXATTRIBI3I VertexAttribI3i = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI3IV)(GLuint, const GLint *);
PFNVERTEXATTRIBI3IV VertexAttribI3iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI3UI)(GLuint, GLuint, GLuint, GLuint);
PFNVERTEXATTRIBI3UI VertexAttribI3ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI3UIV)(GLuint, const GLuint *);
PFNVERTEXATTRIBI3UIV VertexAttribI3uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4BV)(GLuint, const GLbyte *);
PFNVERTEXATTRIBI4BV VertexAttribI4bv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4I)(GLuint, GLint, GLint, GLint, GLint);
PFNVERTEXATTRIBI4I VertexAttribI4i = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4IV)(GLuint, const GLint *);
PFNVERTEXATTRIBI4IV VertexAttribI4iv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4SV)(GLuint, const GLshort *);
PFNVERTEXATTRIBI4SV VertexAttribI4sv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4UBV)(GLuint, const GLubyte *);
PFNVERTEXATTRIBI4UBV VertexAttribI4ubv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4UI)(GLuint, GLuint, GLuint, GLuint, GLuint);
PFNVERTEXATTRIBI4UI VertexAttribI4ui = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4UIV)(GLuint, const GLuint *);
PFNVERTEXATTRIBI4UIV VertexAttribI4uiv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBI4USV)(GLuint, const GLushort *);
PFNVERTEXATTRIBI4USV VertexAttribI4usv = 0;
typedef void(CODEGEN_FUNCPTR *PFNVERTEXATTRIBIPOINTER)(GLuint, GLint, GLenum, GLsizei,
                                                       const GLvoid *);
PFNVERTEXATTRIBIPOINTER VertexAttribIPointer = 0;

static int LoadCoreFunctions()
{
	int numFailed = 0;
	Accum = reinterpret_cast<PFNACCUM>(IntGetProcAddress("glAccum"));
	if (!Accum)
		++numFailed;
	AlphaFunc = reinterpret_cast<PFNALPHAFUNC>(IntGetProcAddress("glAlphaFunc"));
	if (!AlphaFunc)
		++numFailed;
	Begin = reinterpret_cast<PFNBEGIN>(IntGetProcAddress("glBegin"));
	if (!Begin)
		++numFailed;
	Bitmap = reinterpret_cast<PFNBITMAP>(IntGetProcAddress("glBitmap"));
	if (!Bitmap)
		++numFailed;
	BlendFunc = reinterpret_cast<PFNBLENDFUNC>(IntGetProcAddress("glBlendFunc"));
	if (!BlendFunc)
		++numFailed;
	CallList = reinterpret_cast<PFNCALLLIST>(IntGetProcAddress("glCallList"));
	if (!CallList)
		++numFailed;
	CallLists = reinterpret_cast<PFNCALLLISTS>(IntGetProcAddress("glCallLists"));
	if (!CallLists)
		++numFailed;
	Clear = reinterpret_cast<PFNCLEAR>(IntGetProcAddress("glClear"));
	if (!Clear)
		++numFailed;
	ClearAccum = reinterpret_cast<PFNCLEARACCUM>(IntGetProcAddress("glClearAccum"));
	if (!ClearAccum)
		++numFailed;
	ClearColor = reinterpret_cast<PFNCLEARCOLOR>(IntGetProcAddress("glClearColor"));
	if (!ClearColor)
		++numFailed;
	ClearDepth = reinterpret_cast<PFNCLEARDEPTH>(IntGetProcAddress("glClearDepth"));
	if (!ClearDepth)
		++numFailed;
	ClearIndex = reinterpret_cast<PFNCLEARINDEX>(IntGetProcAddress("glClearIndex"));
	if (!ClearIndex)
		++numFailed;
	ClearStencil = reinterpret_cast<PFNCLEARSTENCIL>(IntGetProcAddress("glClearStencil"));
	if (!ClearStencil)
		++numFailed;
	ClipPlane = reinterpret_cast<PFNCLIPPLANE>(IntGetProcAddress("glClipPlane"));
	if (!ClipPlane)
		++numFailed;
	Color3b = reinterpret_cast<PFNCOLOR3B>(IntGetProcAddress("glColor3b"));
	if (!Color3b)
		++numFailed;
	Color3bv = reinterpret_cast<PFNCOLOR3BV>(IntGetProcAddress("glColor3bv"));
	if (!Color3bv)
		++numFailed;
	Color3d = reinterpret_cast<PFNCOLOR3D>(IntGetProcAddress("glColor3d"));
	if (!Color3d)
		++numFailed;
	Color3dv = reinterpret_cast<PFNCOLOR3DV>(IntGetProcAddress("glColor3dv"));
	if (!Color3dv)
		++numFailed;
	Color3f = reinterpret_cast<PFNCOLOR3F>(IntGetProcAddress("glColor3f"));
	if (!Color3f)
		++numFailed;
	Color3fv = reinterpret_cast<PFNCOLOR3FV>(IntGetProcAddress("glColor3fv"));
	if (!Color3fv)
		++numFailed;
	Color3i = reinterpret_cast<PFNCOLOR3I>(IntGetProcAddress("glColor3i"));
	if (!Color3i)
		++numFailed;
	Color3iv = reinterpret_cast<PFNCOLOR3IV>(IntGetProcAddress("glColor3iv"));
	if (!Color3iv)
		++numFailed;
	Color3s = reinterpret_cast<PFNCOLOR3S>(IntGetProcAddress("glColor3s"));
	if (!Color3s)
		++numFailed;
	Color3sv = reinterpret_cast<PFNCOLOR3SV>(IntGetProcAddress("glColor3sv"));
	if (!Color3sv)
		++numFailed;
	Color3ub = reinterpret_cast<PFNCOLOR3UB>(IntGetProcAddress("glColor3ub"));
	if (!Color3ub)
		++numFailed;
	Color3ubv = reinterpret_cast<PFNCOLOR3UBV>(IntGetProcAddress("glColor3ubv"));
	if (!Color3ubv)
		++numFailed;
	Color3ui = reinterpret_cast<PFNCOLOR3UI>(IntGetProcAddress("glColor3ui"));
	if (!Color3ui)
		++numFailed;
	Color3uiv = reinterpret_cast<PFNCOLOR3UIV>(IntGetProcAddress("glColor3uiv"));
	if (!Color3uiv)
		++numFailed;
	Color3us = reinterpret_cast<PFNCOLOR3US>(IntGetProcAddress("glColor3us"));
	if (!Color3us)
		++numFailed;
	Color3usv = reinterpret_cast<PFNCOLOR3USV>(IntGetProcAddress("glColor3usv"));
	if (!Color3usv)
		++numFailed;
	Color4b = reinterpret_cast<PFNCOLOR4B>(IntGetProcAddress("glColor4b"));
	if (!Color4b)
		++numFailed;
	Color4bv = reinterpret_cast<PFNCOLOR4BV>(IntGetProcAddress("glColor4bv"));
	if (!Color4bv)
		++numFailed;
	Color4d = reinterpret_cast<PFNCOLOR4D>(IntGetProcAddress("glColor4d"));
	if (!Color4d)
		++numFailed;
	Color4dv = reinterpret_cast<PFNCOLOR4DV>(IntGetProcAddress("glColor4dv"));
	if (!Color4dv)
		++numFailed;
	Color4f = reinterpret_cast<PFNCOLOR4F>(IntGetProcAddress("glColor4f"));
	if (!Color4f)
		++numFailed;
	Color4fv = reinterpret_cast<PFNCOLOR4FV>(IntGetProcAddress("glColor4fv"));
	if (!Color4fv)
		++numFailed;
	Color4i = reinterpret_cast<PFNCOLOR4I>(IntGetProcAddress("glColor4i"));
	if (!Color4i)
		++numFailed;
	Color4iv = reinterpret_cast<PFNCOLOR4IV>(IntGetProcAddress("glColor4iv"));
	if (!Color4iv)
		++numFailed;
	Color4s = reinterpret_cast<PFNCOLOR4S>(IntGetProcAddress("glColor4s"));
	if (!Color4s)
		++numFailed;
	Color4sv = reinterpret_cast<PFNCOLOR4SV>(IntGetProcAddress("glColor4sv"));
	if (!Color4sv)
		++numFailed;
	Color4ub = reinterpret_cast<PFNCOLOR4UB>(IntGetProcAddress("glColor4ub"));
	if (!Color4ub)
		++numFailed;
	Color4ubv = reinterpret_cast<PFNCOLOR4UBV>(IntGetProcAddress("glColor4ubv"));
	if (!Color4ubv)
		++numFailed;
	Color4ui = reinterpret_cast<PFNCOLOR4UI>(IntGetProcAddress("glColor4ui"));
	if (!Color4ui)
		++numFailed;
	Color4uiv = reinterpret_cast<PFNCOLOR4UIV>(IntGetProcAddress("glColor4uiv"));
	if (!Color4uiv)
		++numFailed;
	Color4us = reinterpret_cast<PFNCOLOR4US>(IntGetProcAddress("glColor4us"));
	if (!Color4us)
		++numFailed;
	Color4usv = reinterpret_cast<PFNCOLOR4USV>(IntGetProcAddress("glColor4usv"));
	if (!Color4usv)
		++numFailed;
	ColorMask = reinterpret_cast<PFNCOLORMASK>(IntGetProcAddress("glColorMask"));
	if (!ColorMask)
		++numFailed;
	ColorMaterial = reinterpret_cast<PFNCOLORMATERIAL>(IntGetProcAddress("glColorMaterial"));
	if (!ColorMaterial)
		++numFailed;
	CopyPixels = reinterpret_cast<PFNCOPYPIXELS>(IntGetProcAddress("glCopyPixels"));
	if (!CopyPixels)
		++numFailed;
	CullFace = reinterpret_cast<PFNCULLFACE>(IntGetProcAddress("glCullFace"));
	if (!CullFace)
		++numFailed;
	DeleteLists = reinterpret_cast<PFNDELETELISTS>(IntGetProcAddress("glDeleteLists"));
	if (!DeleteLists)
		++numFailed;
	DepthFunc = reinterpret_cast<PFNDEPTHFUNC>(IntGetProcAddress("glDepthFunc"));
	if (!DepthFunc)
		++numFailed;
	DepthMask = reinterpret_cast<PFNDEPTHMASK>(IntGetProcAddress("glDepthMask"));
	if (!DepthMask)
		++numFailed;
	DepthRange = reinterpret_cast<PFNDEPTHRANGE>(IntGetProcAddress("glDepthRange"));
	if (!DepthRange)
		++numFailed;
	Disable = reinterpret_cast<PFNDISABLE>(IntGetProcAddress("glDisable"));
	if (!Disable)
		++numFailed;
	DrawBuffer = reinterpret_cast<PFNDRAWBUFFER>(IntGetProcAddress("glDrawBuffer"));
	if (!DrawBuffer)
		++numFailed;
	DrawPixels = reinterpret_cast<PFNDRAWPIXELS>(IntGetProcAddress("glDrawPixels"));
	if (!DrawPixels)
		++numFailed;
	EdgeFlag = reinterpret_cast<PFNEDGEFLAG>(IntGetProcAddress("glEdgeFlag"));
	if (!EdgeFlag)
		++numFailed;
	EdgeFlagv = reinterpret_cast<PFNEDGEFLAGV>(IntGetProcAddress("glEdgeFlagv"));
	if (!EdgeFlagv)
		++numFailed;
	Enable = reinterpret_cast<PFNENABLE>(IntGetProcAddress("glEnable"));
	if (!Enable)
		++numFailed;
	End = reinterpret_cast<PFNEND>(IntGetProcAddress("glEnd"));
	if (!End)
		++numFailed;
	EndList = reinterpret_cast<PFNENDLIST>(IntGetProcAddress("glEndList"));
	if (!EndList)
		++numFailed;
	EvalCoord1d = reinterpret_cast<PFNEVALCOORD1D>(IntGetProcAddress("glEvalCoord1d"));
	if (!EvalCoord1d)
		++numFailed;
	EvalCoord1dv = reinterpret_cast<PFNEVALCOORD1DV>(IntGetProcAddress("glEvalCoord1dv"));
	if (!EvalCoord1dv)
		++numFailed;
	EvalCoord1f = reinterpret_cast<PFNEVALCOORD1F>(IntGetProcAddress("glEvalCoord1f"));
	if (!EvalCoord1f)
		++numFailed;
	EvalCoord1fv = reinterpret_cast<PFNEVALCOORD1FV>(IntGetProcAddress("glEvalCoord1fv"));
	if (!EvalCoord1fv)
		++numFailed;
	EvalCoord2d = reinterpret_cast<PFNEVALCOORD2D>(IntGetProcAddress("glEvalCoord2d"));
	if (!EvalCoord2d)
		++numFailed;
	EvalCoord2dv = reinterpret_cast<PFNEVALCOORD2DV>(IntGetProcAddress("glEvalCoord2dv"));
	if (!EvalCoord2dv)
		++numFailed;
	EvalCoord2f = reinterpret_cast<PFNEVALCOORD2F>(IntGetProcAddress("glEvalCoord2f"));
	if (!EvalCoord2f)
		++numFailed;
	EvalCoord2fv = reinterpret_cast<PFNEVALCOORD2FV>(IntGetProcAddress("glEvalCoord2fv"));
	if (!EvalCoord2fv)
		++numFailed;
	EvalMesh1 = reinterpret_cast<PFNEVALMESH1>(IntGetProcAddress("glEvalMesh1"));
	if (!EvalMesh1)
		++numFailed;
	EvalMesh2 = reinterpret_cast<PFNEVALMESH2>(IntGetProcAddress("glEvalMesh2"));
	if (!EvalMesh2)
		++numFailed;
	EvalPoint1 = reinterpret_cast<PFNEVALPOINT1>(IntGetProcAddress("glEvalPoint1"));
	if (!EvalPoint1)
		++numFailed;
	EvalPoint2 = reinterpret_cast<PFNEVALPOINT2>(IntGetProcAddress("glEvalPoint2"));
	if (!EvalPoint2)
		++numFailed;
	FeedbackBuffer = reinterpret_cast<PFNFEEDBACKBUFFER>(IntGetProcAddress("glFeedbackBuffer"));
	if (!FeedbackBuffer)
		++numFailed;
	Finish = reinterpret_cast<PFNFINISH>(IntGetProcAddress("glFinish"));
	if (!Finish)
		++numFailed;
	Flush = reinterpret_cast<PFNFLUSH>(IntGetProcAddress("glFlush"));
	if (!Flush)
		++numFailed;
	Fogf = reinterpret_cast<PFNFOGF>(IntGetProcAddress("glFogf"));
	if (!Fogf)
		++numFailed;
	Fogfv = reinterpret_cast<PFNFOGFV>(IntGetProcAddress("glFogfv"));
	if (!Fogfv)
		++numFailed;
	Fogi = reinterpret_cast<PFNFOGI>(IntGetProcAddress("glFogi"));
	if (!Fogi)
		++numFailed;
	Fogiv = reinterpret_cast<PFNFOGIV>(IntGetProcAddress("glFogiv"));
	if (!Fogiv)
		++numFailed;
	FrontFace = reinterpret_cast<PFNFRONTFACE>(IntGetProcAddress("glFrontFace"));
	if (!FrontFace)
		++numFailed;
	Frustum = reinterpret_cast<PFNFRUSTUM>(IntGetProcAddress("glFrustum"));
	if (!Frustum)
		++numFailed;
	GenLists = reinterpret_cast<PFNGENLISTS>(IntGetProcAddress("glGenLists"));
	if (!GenLists)
		++numFailed;
	GetBooleanv = reinterpret_cast<PFNGETBOOLEANV>(IntGetProcAddress("glGetBooleanv"));
	if (!GetBooleanv)
		++numFailed;
	GetClipPlane = reinterpret_cast<PFNGETCLIPPLANE>(IntGetProcAddress("glGetClipPlane"));
	if (!GetClipPlane)
		++numFailed;
	GetDoublev = reinterpret_cast<PFNGETDOUBLEV>(IntGetProcAddress("glGetDoublev"));
	if (!GetDoublev)
		++numFailed;
	GetError = reinterpret_cast<PFNGETERROR>(IntGetProcAddress("glGetError"));
	if (!GetError)
		++numFailed;
	GetFloatv = reinterpret_cast<PFNGETFLOATV>(IntGetProcAddress("glGetFloatv"));
	if (!GetFloatv)
		++numFailed;
	GetIntegerv = reinterpret_cast<PFNGETINTEGERV>(IntGetProcAddress("glGetIntegerv"));
	if (!GetIntegerv)
		++numFailed;
	GetLightfv = reinterpret_cast<PFNGETLIGHTFV>(IntGetProcAddress("glGetLightfv"));
	if (!GetLightfv)
		++numFailed;
	GetLightiv = reinterpret_cast<PFNGETLIGHTIV>(IntGetProcAddress("glGetLightiv"));
	if (!GetLightiv)
		++numFailed;
	GetMapdv = reinterpret_cast<PFNGETMAPDV>(IntGetProcAddress("glGetMapdv"));
	if (!GetMapdv)
		++numFailed;
	GetMapfv = reinterpret_cast<PFNGETMAPFV>(IntGetProcAddress("glGetMapfv"));
	if (!GetMapfv)
		++numFailed;
	GetMapiv = reinterpret_cast<PFNGETMAPIV>(IntGetProcAddress("glGetMapiv"));
	if (!GetMapiv)
		++numFailed;
	GetMaterialfv = reinterpret_cast<PFNGETMATERIALFV>(IntGetProcAddress("glGetMaterialfv"));
	if (!GetMaterialfv)
		++numFailed;
	GetMaterialiv = reinterpret_cast<PFNGETMATERIALIV>(IntGetProcAddress("glGetMaterialiv"));
	if (!GetMaterialiv)
		++numFailed;
	GetPixelMapfv = reinterpret_cast<PFNGETPIXELMAPFV>(IntGetProcAddress("glGetPixelMapfv"));
	if (!GetPixelMapfv)
		++numFailed;
	GetPixelMapuiv = reinterpret_cast<PFNGETPIXELMAPUIV>(IntGetProcAddress("glGetPixelMapuiv"));
	if (!GetPixelMapuiv)
		++numFailed;
	GetPixelMapusv = reinterpret_cast<PFNGETPIXELMAPUSV>(IntGetProcAddress("glGetPixelMapusv"));
	if (!GetPixelMapusv)
		++numFailed;
	GetPolygonStipple =
	    reinterpret_cast<PFNGETPOLYGONSTIPPLE>(IntGetProcAddress("glGetPolygonStipple"));
	if (!GetPolygonStipple)
		++numFailed;
	GetString = reinterpret_cast<PFNGETSTRING>(IntGetProcAddress("glGetString"));
	if (!GetString)
		++numFailed;
	GetTexEnvfv = reinterpret_cast<PFNGETTEXENVFV>(IntGetProcAddress("glGetTexEnvfv"));
	if (!GetTexEnvfv)
		++numFailed;
	GetTexEnviv = reinterpret_cast<PFNGETTEXENVIV>(IntGetProcAddress("glGetTexEnviv"));
	if (!GetTexEnviv)
		++numFailed;
	GetTexGendv = reinterpret_cast<PFNGETTEXGENDV>(IntGetProcAddress("glGetTexGendv"));
	if (!GetTexGendv)
		++numFailed;
	GetTexGenfv = reinterpret_cast<PFNGETTEXGENFV>(IntGetProcAddress("glGetTexGenfv"));
	if (!GetTexGenfv)
		++numFailed;
	GetTexGeniv = reinterpret_cast<PFNGETTEXGENIV>(IntGetProcAddress("glGetTexGeniv"));
	if (!GetTexGeniv)
		++numFailed;
	GetTexImage = reinterpret_cast<PFNGETTEXIMAGE>(IntGetProcAddress("glGetTexImage"));
	if (!GetTexImage)
		++numFailed;
	GetTexLevelParameterfv =
	    reinterpret_cast<PFNGETTEXLEVELPARAMETERFV>(IntGetProcAddress("glGetTexLevelParameterfv"));
	if (!GetTexLevelParameterfv)
		++numFailed;
	GetTexLevelParameteriv =
	    reinterpret_cast<PFNGETTEXLEVELPARAMETERIV>(IntGetProcAddress("glGetTexLevelParameteriv"));
	if (!GetTexLevelParameteriv)
		++numFailed;
	GetTexParameterfv =
	    reinterpret_cast<PFNGETTEXPARAMETERFV>(IntGetProcAddress("glGetTexParameterfv"));
	if (!GetTexParameterfv)
		++numFailed;
	GetTexParameteriv =
	    reinterpret_cast<PFNGETTEXPARAMETERIV>(IntGetProcAddress("glGetTexParameteriv"));
	if (!GetTexParameteriv)
		++numFailed;
	Hint = reinterpret_cast<PFNHINT>(IntGetProcAddress("glHint"));
	if (!Hint)
		++numFailed;
	IndexMask = reinterpret_cast<PFNINDEXMASK>(IntGetProcAddress("glIndexMask"));
	if (!IndexMask)
		++numFailed;
	Indexd = reinterpret_cast<PFNINDEXD>(IntGetProcAddress("glIndexd"));
	if (!Indexd)
		++numFailed;
	Indexdv = reinterpret_cast<PFNINDEXDV>(IntGetProcAddress("glIndexdv"));
	if (!Indexdv)
		++numFailed;
	Indexf = reinterpret_cast<PFNINDEXF>(IntGetProcAddress("glIndexf"));
	if (!Indexf)
		++numFailed;
	Indexfv = reinterpret_cast<PFNINDEXFV>(IntGetProcAddress("glIndexfv"));
	if (!Indexfv)
		++numFailed;
	Indexi = reinterpret_cast<PFNINDEXI>(IntGetProcAddress("glIndexi"));
	if (!Indexi)
		++numFailed;
	Indexiv = reinterpret_cast<PFNINDEXIV>(IntGetProcAddress("glIndexiv"));
	if (!Indexiv)
		++numFailed;
	Indexs = reinterpret_cast<PFNINDEXS>(IntGetProcAddress("glIndexs"));
	if (!Indexs)
		++numFailed;
	Indexsv = reinterpret_cast<PFNINDEXSV>(IntGetProcAddress("glIndexsv"));
	if (!Indexsv)
		++numFailed;
	InitNames = reinterpret_cast<PFNINITNAMES>(IntGetProcAddress("glInitNames"));
	if (!InitNames)
		++numFailed;
	IsEnabled = reinterpret_cast<PFNISENABLED>(IntGetProcAddress("glIsEnabled"));
	if (!IsEnabled)
		++numFailed;
	IsList = reinterpret_cast<PFNISLIST>(IntGetProcAddress("glIsList"));
	if (!IsList)
		++numFailed;
	LightModelf = reinterpret_cast<PFNLIGHTMODELF>(IntGetProcAddress("glLightModelf"));
	if (!LightModelf)
		++numFailed;
	LightModelfv = reinterpret_cast<PFNLIGHTMODELFV>(IntGetProcAddress("glLightModelfv"));
	if (!LightModelfv)
		++numFailed;
	LightModeli = reinterpret_cast<PFNLIGHTMODELI>(IntGetProcAddress("glLightModeli"));
	if (!LightModeli)
		++numFailed;
	LightModeliv = reinterpret_cast<PFNLIGHTMODELIV>(IntGetProcAddress("glLightModeliv"));
	if (!LightModeliv)
		++numFailed;
	Lightf = reinterpret_cast<PFNLIGHTF>(IntGetProcAddress("glLightf"));
	if (!Lightf)
		++numFailed;
	Lightfv = reinterpret_cast<PFNLIGHTFV>(IntGetProcAddress("glLightfv"));
	if (!Lightfv)
		++numFailed;
	Lighti = reinterpret_cast<PFNLIGHTI>(IntGetProcAddress("glLighti"));
	if (!Lighti)
		++numFailed;
	Lightiv = reinterpret_cast<PFNLIGHTIV>(IntGetProcAddress("glLightiv"));
	if (!Lightiv)
		++numFailed;
	LineStipple = reinterpret_cast<PFNLINESTIPPLE>(IntGetProcAddress("glLineStipple"));
	if (!LineStipple)
		++numFailed;
	LineWidth = reinterpret_cast<PFNLINEWIDTH>(IntGetProcAddress("glLineWidth"));
	if (!LineWidth)
		++numFailed;
	ListBase = reinterpret_cast<PFNLISTBASE>(IntGetProcAddress("glListBase"));
	if (!ListBase)
		++numFailed;
	LoadIdentity = reinterpret_cast<PFNLOADIDENTITY>(IntGetProcAddress("glLoadIdentity"));
	if (!LoadIdentity)
		++numFailed;
	LoadMatrixd = reinterpret_cast<PFNLOADMATRIXD>(IntGetProcAddress("glLoadMatrixd"));
	if (!LoadMatrixd)
		++numFailed;
	LoadMatrixf = reinterpret_cast<PFNLOADMATRIXF>(IntGetProcAddress("glLoadMatrixf"));
	if (!LoadMatrixf)
		++numFailed;
	LoadName = reinterpret_cast<PFNLOADNAME>(IntGetProcAddress("glLoadName"));
	if (!LoadName)
		++numFailed;
	LogicOp = reinterpret_cast<PFNLOGICOP>(IntGetProcAddress("glLogicOp"));
	if (!LogicOp)
		++numFailed;
	Map1d = reinterpret_cast<PFNMAP1D>(IntGetProcAddress("glMap1d"));
	if (!Map1d)
		++numFailed;
	Map1f = reinterpret_cast<PFNMAP1F>(IntGetProcAddress("glMap1f"));
	if (!Map1f)
		++numFailed;
	Map2d = reinterpret_cast<PFNMAP2D>(IntGetProcAddress("glMap2d"));
	if (!Map2d)
		++numFailed;
	Map2f = reinterpret_cast<PFNMAP2F>(IntGetProcAddress("glMap2f"));
	if (!Map2f)
		++numFailed;
	MapGrid1d = reinterpret_cast<PFNMAPGRID1D>(IntGetProcAddress("glMapGrid1d"));
	if (!MapGrid1d)
		++numFailed;
	MapGrid1f = reinterpret_cast<PFNMAPGRID1F>(IntGetProcAddress("glMapGrid1f"));
	if (!MapGrid1f)
		++numFailed;
	MapGrid2d = reinterpret_cast<PFNMAPGRID2D>(IntGetProcAddress("glMapGrid2d"));
	if (!MapGrid2d)
		++numFailed;
	MapGrid2f = reinterpret_cast<PFNMAPGRID2F>(IntGetProcAddress("glMapGrid2f"));
	if (!MapGrid2f)
		++numFailed;
	Materialf = reinterpret_cast<PFNMATERIALF>(IntGetProcAddress("glMaterialf"));
	if (!Materialf)
		++numFailed;
	Materialfv = reinterpret_cast<PFNMATERIALFV>(IntGetProcAddress("glMaterialfv"));
	if (!Materialfv)
		++numFailed;
	Materiali = reinterpret_cast<PFNMATERIALI>(IntGetProcAddress("glMateriali"));
	if (!Materiali)
		++numFailed;
	Materialiv = reinterpret_cast<PFNMATERIALIV>(IntGetProcAddress("glMaterialiv"));
	if (!Materialiv)
		++numFailed;
	MatrixMode = reinterpret_cast<PFNMATRIXMODE>(IntGetProcAddress("glMatrixMode"));
	if (!MatrixMode)
		++numFailed;
	MultMatrixd = reinterpret_cast<PFNMULTMATRIXD>(IntGetProcAddress("glMultMatrixd"));
	if (!MultMatrixd)
		++numFailed;
	MultMatrixf = reinterpret_cast<PFNMULTMATRIXF>(IntGetProcAddress("glMultMatrixf"));
	if (!MultMatrixf)
		++numFailed;
	NewList = reinterpret_cast<PFNNEWLIST>(IntGetProcAddress("glNewList"));
	if (!NewList)
		++numFailed;
	Normal3b = reinterpret_cast<PFNNORMAL3B>(IntGetProcAddress("glNormal3b"));
	if (!Normal3b)
		++numFailed;
	Normal3bv = reinterpret_cast<PFNNORMAL3BV>(IntGetProcAddress("glNormal3bv"));
	if (!Normal3bv)
		++numFailed;
	Normal3d = reinterpret_cast<PFNNORMAL3D>(IntGetProcAddress("glNormal3d"));
	if (!Normal3d)
		++numFailed;
	Normal3dv = reinterpret_cast<PFNNORMAL3DV>(IntGetProcAddress("glNormal3dv"));
	if (!Normal3dv)
		++numFailed;
	Normal3f = reinterpret_cast<PFNNORMAL3F>(IntGetProcAddress("glNormal3f"));
	if (!Normal3f)
		++numFailed;
	Normal3fv = reinterpret_cast<PFNNORMAL3FV>(IntGetProcAddress("glNormal3fv"));
	if (!Normal3fv)
		++numFailed;
	Normal3i = reinterpret_cast<PFNNORMAL3I>(IntGetProcAddress("glNormal3i"));
	if (!Normal3i)
		++numFailed;
	Normal3iv = reinterpret_cast<PFNNORMAL3IV>(IntGetProcAddress("glNormal3iv"));
	if (!Normal3iv)
		++numFailed;
	Normal3s = reinterpret_cast<PFNNORMAL3S>(IntGetProcAddress("glNormal3s"));
	if (!Normal3s)
		++numFailed;
	Normal3sv = reinterpret_cast<PFNNORMAL3SV>(IntGetProcAddress("glNormal3sv"));
	if (!Normal3sv)
		++numFailed;
	Ortho = reinterpret_cast<PFNORTHO>(IntGetProcAddress("glOrtho"));
	if (!Ortho)
		++numFailed;
	PassThrough = reinterpret_cast<PFNPASSTHROUGH>(IntGetProcAddress("glPassThrough"));
	if (!PassThrough)
		++numFailed;
	PixelMapfv = reinterpret_cast<PFNPIXELMAPFV>(IntGetProcAddress("glPixelMapfv"));
	if (!PixelMapfv)
		++numFailed;
	PixelMapuiv = reinterpret_cast<PFNPIXELMAPUIV>(IntGetProcAddress("glPixelMapuiv"));
	if (!PixelMapuiv)
		++numFailed;
	PixelMapusv = reinterpret_cast<PFNPIXELMAPUSV>(IntGetProcAddress("glPixelMapusv"));
	if (!PixelMapusv)
		++numFailed;
	PixelStoref = reinterpret_cast<PFNPIXELSTOREF>(IntGetProcAddress("glPixelStoref"));
	if (!PixelStoref)
		++numFailed;
	PixelStorei = reinterpret_cast<PFNPIXELSTOREI>(IntGetProcAddress("glPixelStorei"));
	if (!PixelStorei)
		++numFailed;
	PixelTransferf = reinterpret_cast<PFNPIXELTRANSFERF>(IntGetProcAddress("glPixelTransferf"));
	if (!PixelTransferf)
		++numFailed;
	PixelTransferi = reinterpret_cast<PFNPIXELTRANSFERI>(IntGetProcAddress("glPixelTransferi"));
	if (!PixelTransferi)
		++numFailed;
	PixelZoom = reinterpret_cast<PFNPIXELZOOM>(IntGetProcAddress("glPixelZoom"));
	if (!PixelZoom)
		++numFailed;
	PointSize = reinterpret_cast<PFNPOINTSIZE>(IntGetProcAddress("glPointSize"));
	if (!PointSize)
		++numFailed;
	PolygonMode = reinterpret_cast<PFNPOLYGONMODE>(IntGetProcAddress("glPolygonMode"));
	if (!PolygonMode)
		++numFailed;
	PolygonStipple = reinterpret_cast<PFNPOLYGONSTIPPLE>(IntGetProcAddress("glPolygonStipple"));
	if (!PolygonStipple)
		++numFailed;
	PopAttrib = reinterpret_cast<PFNPOPATTRIB>(IntGetProcAddress("glPopAttrib"));
	if (!PopAttrib)
		++numFailed;
	PopMatrix = reinterpret_cast<PFNPOPMATRIX>(IntGetProcAddress("glPopMatrix"));
	if (!PopMatrix)
		++numFailed;
	PopName = reinterpret_cast<PFNPOPNAME>(IntGetProcAddress("glPopName"));
	if (!PopName)
		++numFailed;
	PushAttrib = reinterpret_cast<PFNPUSHATTRIB>(IntGetProcAddress("glPushAttrib"));
	if (!PushAttrib)
		++numFailed;
	PushMatrix = reinterpret_cast<PFNPUSHMATRIX>(IntGetProcAddress("glPushMatrix"));
	if (!PushMatrix)
		++numFailed;
	PushName = reinterpret_cast<PFNPUSHNAME>(IntGetProcAddress("glPushName"));
	if (!PushName)
		++numFailed;
	RasterPos2d = reinterpret_cast<PFNRASTERPOS2D>(IntGetProcAddress("glRasterPos2d"));
	if (!RasterPos2d)
		++numFailed;
	RasterPos2dv = reinterpret_cast<PFNRASTERPOS2DV>(IntGetProcAddress("glRasterPos2dv"));
	if (!RasterPos2dv)
		++numFailed;
	RasterPos2f = reinterpret_cast<PFNRASTERPOS2F>(IntGetProcAddress("glRasterPos2f"));
	if (!RasterPos2f)
		++numFailed;
	RasterPos2fv = reinterpret_cast<PFNRASTERPOS2FV>(IntGetProcAddress("glRasterPos2fv"));
	if (!RasterPos2fv)
		++numFailed;
	RasterPos2i = reinterpret_cast<PFNRASTERPOS2I>(IntGetProcAddress("glRasterPos2i"));
	if (!RasterPos2i)
		++numFailed;
	RasterPos2iv = reinterpret_cast<PFNRASTERPOS2IV>(IntGetProcAddress("glRasterPos2iv"));
	if (!RasterPos2iv)
		++numFailed;
	RasterPos2s = reinterpret_cast<PFNRASTERPOS2S>(IntGetProcAddress("glRasterPos2s"));
	if (!RasterPos2s)
		++numFailed;
	RasterPos2sv = reinterpret_cast<PFNRASTERPOS2SV>(IntGetProcAddress("glRasterPos2sv"));
	if (!RasterPos2sv)
		++numFailed;
	RasterPos3d = reinterpret_cast<PFNRASTERPOS3D>(IntGetProcAddress("glRasterPos3d"));
	if (!RasterPos3d)
		++numFailed;
	RasterPos3dv = reinterpret_cast<PFNRASTERPOS3DV>(IntGetProcAddress("glRasterPos3dv"));
	if (!RasterPos3dv)
		++numFailed;
	RasterPos3f = reinterpret_cast<PFNRASTERPOS3F>(IntGetProcAddress("glRasterPos3f"));
	if (!RasterPos3f)
		++numFailed;
	RasterPos3fv = reinterpret_cast<PFNRASTERPOS3FV>(IntGetProcAddress("glRasterPos3fv"));
	if (!RasterPos3fv)
		++numFailed;
	RasterPos3i = reinterpret_cast<PFNRASTERPOS3I>(IntGetProcAddress("glRasterPos3i"));
	if (!RasterPos3i)
		++numFailed;
	RasterPos3iv = reinterpret_cast<PFNRASTERPOS3IV>(IntGetProcAddress("glRasterPos3iv"));
	if (!RasterPos3iv)
		++numFailed;
	RasterPos3s = reinterpret_cast<PFNRASTERPOS3S>(IntGetProcAddress("glRasterPos3s"));
	if (!RasterPos3s)
		++numFailed;
	RasterPos3sv = reinterpret_cast<PFNRASTERPOS3SV>(IntGetProcAddress("glRasterPos3sv"));
	if (!RasterPos3sv)
		++numFailed;
	RasterPos4d = reinterpret_cast<PFNRASTERPOS4D>(IntGetProcAddress("glRasterPos4d"));
	if (!RasterPos4d)
		++numFailed;
	RasterPos4dv = reinterpret_cast<PFNRASTERPOS4DV>(IntGetProcAddress("glRasterPos4dv"));
	if (!RasterPos4dv)
		++numFailed;
	RasterPos4f = reinterpret_cast<PFNRASTERPOS4F>(IntGetProcAddress("glRasterPos4f"));
	if (!RasterPos4f)
		++numFailed;
	RasterPos4fv = reinterpret_cast<PFNRASTERPOS4FV>(IntGetProcAddress("glRasterPos4fv"));
	if (!RasterPos4fv)
		++numFailed;
	RasterPos4i = reinterpret_cast<PFNRASTERPOS4I>(IntGetProcAddress("glRasterPos4i"));
	if (!RasterPos4i)
		++numFailed;
	RasterPos4iv = reinterpret_cast<PFNRASTERPOS4IV>(IntGetProcAddress("glRasterPos4iv"));
	if (!RasterPos4iv)
		++numFailed;
	RasterPos4s = reinterpret_cast<PFNRASTERPOS4S>(IntGetProcAddress("glRasterPos4s"));
	if (!RasterPos4s)
		++numFailed;
	RasterPos4sv = reinterpret_cast<PFNRASTERPOS4SV>(IntGetProcAddress("glRasterPos4sv"));
	if (!RasterPos4sv)
		++numFailed;
	ReadBuffer = reinterpret_cast<PFNREADBUFFER>(IntGetProcAddress("glReadBuffer"));
	if (!ReadBuffer)
		++numFailed;
	ReadPixels = reinterpret_cast<PFNREADPIXELS>(IntGetProcAddress("glReadPixels"));
	if (!ReadPixels)
		++numFailed;
	Rectd = reinterpret_cast<PFNRECTD>(IntGetProcAddress("glRectd"));
	if (!Rectd)
		++numFailed;
	Rectdv = reinterpret_cast<PFNRECTDV>(IntGetProcAddress("glRectdv"));
	if (!Rectdv)
		++numFailed;
	Rectf = reinterpret_cast<PFNRECTF>(IntGetProcAddress("glRectf"));
	if (!Rectf)
		++numFailed;
	Rectfv = reinterpret_cast<PFNRECTFV>(IntGetProcAddress("glRectfv"));
	if (!Rectfv)
		++numFailed;
	Recti = reinterpret_cast<PFNRECTI>(IntGetProcAddress("glRecti"));
	if (!Recti)
		++numFailed;
	Rectiv = reinterpret_cast<PFNRECTIV>(IntGetProcAddress("glRectiv"));
	if (!Rectiv)
		++numFailed;
	Rects = reinterpret_cast<PFNRECTS>(IntGetProcAddress("glRects"));
	if (!Rects)
		++numFailed;
	Rectsv = reinterpret_cast<PFNRECTSV>(IntGetProcAddress("glRectsv"));
	if (!Rectsv)
		++numFailed;
	RenderMode = reinterpret_cast<PFNRENDERMODE>(IntGetProcAddress("glRenderMode"));
	if (!RenderMode)
		++numFailed;
	Rotated = reinterpret_cast<PFNROTATED>(IntGetProcAddress("glRotated"));
	if (!Rotated)
		++numFailed;
	Rotatef = reinterpret_cast<PFNROTATEF>(IntGetProcAddress("glRotatef"));
	if (!Rotatef)
		++numFailed;
	Scaled = reinterpret_cast<PFNSCALED>(IntGetProcAddress("glScaled"));
	if (!Scaled)
		++numFailed;
	Scalef = reinterpret_cast<PFNSCALEF>(IntGetProcAddress("glScalef"));
	if (!Scalef)
		++numFailed;
	Scissor = reinterpret_cast<PFNSCISSOR>(IntGetProcAddress("glScissor"));
	if (!Scissor)
		++numFailed;
	SelectBuffer = reinterpret_cast<PFNSELECTBUFFER>(IntGetProcAddress("glSelectBuffer"));
	if (!SelectBuffer)
		++numFailed;
	ShadeModel = reinterpret_cast<PFNSHADEMODEL>(IntGetProcAddress("glShadeModel"));
	if (!ShadeModel)
		++numFailed;
	StencilFunc = reinterpret_cast<PFNSTENCILFUNC>(IntGetProcAddress("glStencilFunc"));
	if (!StencilFunc)
		++numFailed;
	StencilMask = reinterpret_cast<PFNSTENCILMASK>(IntGetProcAddress("glStencilMask"));
	if (!StencilMask)
		++numFailed;
	StencilOp = reinterpret_cast<PFNSTENCILOP>(IntGetProcAddress("glStencilOp"));
	if (!StencilOp)
		++numFailed;
	TexCoord1d = reinterpret_cast<PFNTEXCOORD1D>(IntGetProcAddress("glTexCoord1d"));
	if (!TexCoord1d)
		++numFailed;
	TexCoord1dv = reinterpret_cast<PFNTEXCOORD1DV>(IntGetProcAddress("glTexCoord1dv"));
	if (!TexCoord1dv)
		++numFailed;
	TexCoord1f = reinterpret_cast<PFNTEXCOORD1F>(IntGetProcAddress("glTexCoord1f"));
	if (!TexCoord1f)
		++numFailed;
	TexCoord1fv = reinterpret_cast<PFNTEXCOORD1FV>(IntGetProcAddress("glTexCoord1fv"));
	if (!TexCoord1fv)
		++numFailed;
	TexCoord1i = reinterpret_cast<PFNTEXCOORD1I>(IntGetProcAddress("glTexCoord1i"));
	if (!TexCoord1i)
		++numFailed;
	TexCoord1iv = reinterpret_cast<PFNTEXCOORD1IV>(IntGetProcAddress("glTexCoord1iv"));
	if (!TexCoord1iv)
		++numFailed;
	TexCoord1s = reinterpret_cast<PFNTEXCOORD1S>(IntGetProcAddress("glTexCoord1s"));
	if (!TexCoord1s)
		++numFailed;
	TexCoord1sv = reinterpret_cast<PFNTEXCOORD1SV>(IntGetProcAddress("glTexCoord1sv"));
	if (!TexCoord1sv)
		++numFailed;
	TexCoord2d = reinterpret_cast<PFNTEXCOORD2D>(IntGetProcAddress("glTexCoord2d"));
	if (!TexCoord2d)
		++numFailed;
	TexCoord2dv = reinterpret_cast<PFNTEXCOORD2DV>(IntGetProcAddress("glTexCoord2dv"));
	if (!TexCoord2dv)
		++numFailed;
	TexCoord2f = reinterpret_cast<PFNTEXCOORD2F>(IntGetProcAddress("glTexCoord2f"));
	if (!TexCoord2f)
		++numFailed;
	TexCoord2fv = reinterpret_cast<PFNTEXCOORD2FV>(IntGetProcAddress("glTexCoord2fv"));
	if (!TexCoord2fv)
		++numFailed;
	TexCoord2i = reinterpret_cast<PFNTEXCOORD2I>(IntGetProcAddress("glTexCoord2i"));
	if (!TexCoord2i)
		++numFailed;
	TexCoord2iv = reinterpret_cast<PFNTEXCOORD2IV>(IntGetProcAddress("glTexCoord2iv"));
	if (!TexCoord2iv)
		++numFailed;
	TexCoord2s = reinterpret_cast<PFNTEXCOORD2S>(IntGetProcAddress("glTexCoord2s"));
	if (!TexCoord2s)
		++numFailed;
	TexCoord2sv = reinterpret_cast<PFNTEXCOORD2SV>(IntGetProcAddress("glTexCoord2sv"));
	if (!TexCoord2sv)
		++numFailed;
	TexCoord3d = reinterpret_cast<PFNTEXCOORD3D>(IntGetProcAddress("glTexCoord3d"));
	if (!TexCoord3d)
		++numFailed;
	TexCoord3dv = reinterpret_cast<PFNTEXCOORD3DV>(IntGetProcAddress("glTexCoord3dv"));
	if (!TexCoord3dv)
		++numFailed;
	TexCoord3f = reinterpret_cast<PFNTEXCOORD3F>(IntGetProcAddress("glTexCoord3f"));
	if (!TexCoord3f)
		++numFailed;
	TexCoord3fv = reinterpret_cast<PFNTEXCOORD3FV>(IntGetProcAddress("glTexCoord3fv"));
	if (!TexCoord3fv)
		++numFailed;
	TexCoord3i = reinterpret_cast<PFNTEXCOORD3I>(IntGetProcAddress("glTexCoord3i"));
	if (!TexCoord3i)
		++numFailed;
	TexCoord3iv = reinterpret_cast<PFNTEXCOORD3IV>(IntGetProcAddress("glTexCoord3iv"));
	if (!TexCoord3iv)
		++numFailed;
	TexCoord3s = reinterpret_cast<PFNTEXCOORD3S>(IntGetProcAddress("glTexCoord3s"));
	if (!TexCoord3s)
		++numFailed;
	TexCoord3sv = reinterpret_cast<PFNTEXCOORD3SV>(IntGetProcAddress("glTexCoord3sv"));
	if (!TexCoord3sv)
		++numFailed;
	TexCoord4d = reinterpret_cast<PFNTEXCOORD4D>(IntGetProcAddress("glTexCoord4d"));
	if (!TexCoord4d)
		++numFailed;
	TexCoord4dv = reinterpret_cast<PFNTEXCOORD4DV>(IntGetProcAddress("glTexCoord4dv"));
	if (!TexCoord4dv)
		++numFailed;
	TexCoord4f = reinterpret_cast<PFNTEXCOORD4F>(IntGetProcAddress("glTexCoord4f"));
	if (!TexCoord4f)
		++numFailed;
	TexCoord4fv = reinterpret_cast<PFNTEXCOORD4FV>(IntGetProcAddress("glTexCoord4fv"));
	if (!TexCoord4fv)
		++numFailed;
	TexCoord4i = reinterpret_cast<PFNTEXCOORD4I>(IntGetProcAddress("glTexCoord4i"));
	if (!TexCoord4i)
		++numFailed;
	TexCoord4iv = reinterpret_cast<PFNTEXCOORD4IV>(IntGetProcAddress("glTexCoord4iv"));
	if (!TexCoord4iv)
		++numFailed;
	TexCoord4s = reinterpret_cast<PFNTEXCOORD4S>(IntGetProcAddress("glTexCoord4s"));
	if (!TexCoord4s)
		++numFailed;
	TexCoord4sv = reinterpret_cast<PFNTEXCOORD4SV>(IntGetProcAddress("glTexCoord4sv"));
	if (!TexCoord4sv)
		++numFailed;
	TexEnvf = reinterpret_cast<PFNTEXENVF>(IntGetProcAddress("glTexEnvf"));
	if (!TexEnvf)
		++numFailed;
	TexEnvfv = reinterpret_cast<PFNTEXENVFV>(IntGetProcAddress("glTexEnvfv"));
	if (!TexEnvfv)
		++numFailed;
	TexEnvi = reinterpret_cast<PFNTEXENVI>(IntGetProcAddress("glTexEnvi"));
	if (!TexEnvi)
		++numFailed;
	TexEnviv = reinterpret_cast<PFNTEXENVIV>(IntGetProcAddress("glTexEnviv"));
	if (!TexEnviv)
		++numFailed;
	TexGend = reinterpret_cast<PFNTEXGEND>(IntGetProcAddress("glTexGend"));
	if (!TexGend)
		++numFailed;
	TexGendv = reinterpret_cast<PFNTEXGENDV>(IntGetProcAddress("glTexGendv"));
	if (!TexGendv)
		++numFailed;
	TexGenf = reinterpret_cast<PFNTEXGENF>(IntGetProcAddress("glTexGenf"));
	if (!TexGenf)
		++numFailed;
	TexGenfv = reinterpret_cast<PFNTEXGENFV>(IntGetProcAddress("glTexGenfv"));
	if (!TexGenfv)
		++numFailed;
	TexGeni = reinterpret_cast<PFNTEXGENI>(IntGetProcAddress("glTexGeni"));
	if (!TexGeni)
		++numFailed;
	TexGeniv = reinterpret_cast<PFNTEXGENIV>(IntGetProcAddress("glTexGeniv"));
	if (!TexGeniv)
		++numFailed;
	TexImage1D = reinterpret_cast<PFNTEXIMAGE1D>(IntGetProcAddress("glTexImage1D"));
	if (!TexImage1D)
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
	Translated = reinterpret_cast<PFNTRANSLATED>(IntGetProcAddress("glTranslated"));
	if (!Translated)
		++numFailed;
	Translatef = reinterpret_cast<PFNTRANSLATEF>(IntGetProcAddress("glTranslatef"));
	if (!Translatef)
		++numFailed;
	Vertex2d = reinterpret_cast<PFNVERTEX2D>(IntGetProcAddress("glVertex2d"));
	if (!Vertex2d)
		++numFailed;
	Vertex2dv = reinterpret_cast<PFNVERTEX2DV>(IntGetProcAddress("glVertex2dv"));
	if (!Vertex2dv)
		++numFailed;
	Vertex2f = reinterpret_cast<PFNVERTEX2F>(IntGetProcAddress("glVertex2f"));
	if (!Vertex2f)
		++numFailed;
	Vertex2fv = reinterpret_cast<PFNVERTEX2FV>(IntGetProcAddress("glVertex2fv"));
	if (!Vertex2fv)
		++numFailed;
	Vertex2i = reinterpret_cast<PFNVERTEX2I>(IntGetProcAddress("glVertex2i"));
	if (!Vertex2i)
		++numFailed;
	Vertex2iv = reinterpret_cast<PFNVERTEX2IV>(IntGetProcAddress("glVertex2iv"));
	if (!Vertex2iv)
		++numFailed;
	Vertex2s = reinterpret_cast<PFNVERTEX2S>(IntGetProcAddress("glVertex2s"));
	if (!Vertex2s)
		++numFailed;
	Vertex2sv = reinterpret_cast<PFNVERTEX2SV>(IntGetProcAddress("glVertex2sv"));
	if (!Vertex2sv)
		++numFailed;
	Vertex3d = reinterpret_cast<PFNVERTEX3D>(IntGetProcAddress("glVertex3d"));
	if (!Vertex3d)
		++numFailed;
	Vertex3dv = reinterpret_cast<PFNVERTEX3DV>(IntGetProcAddress("glVertex3dv"));
	if (!Vertex3dv)
		++numFailed;
	Vertex3f = reinterpret_cast<PFNVERTEX3F>(IntGetProcAddress("glVertex3f"));
	if (!Vertex3f)
		++numFailed;
	Vertex3fv = reinterpret_cast<PFNVERTEX3FV>(IntGetProcAddress("glVertex3fv"));
	if (!Vertex3fv)
		++numFailed;
	Vertex3i = reinterpret_cast<PFNVERTEX3I>(IntGetProcAddress("glVertex3i"));
	if (!Vertex3i)
		++numFailed;
	Vertex3iv = reinterpret_cast<PFNVERTEX3IV>(IntGetProcAddress("glVertex3iv"));
	if (!Vertex3iv)
		++numFailed;
	Vertex3s = reinterpret_cast<PFNVERTEX3S>(IntGetProcAddress("glVertex3s"));
	if (!Vertex3s)
		++numFailed;
	Vertex3sv = reinterpret_cast<PFNVERTEX3SV>(IntGetProcAddress("glVertex3sv"));
	if (!Vertex3sv)
		++numFailed;
	Vertex4d = reinterpret_cast<PFNVERTEX4D>(IntGetProcAddress("glVertex4d"));
	if (!Vertex4d)
		++numFailed;
	Vertex4dv = reinterpret_cast<PFNVERTEX4DV>(IntGetProcAddress("glVertex4dv"));
	if (!Vertex4dv)
		++numFailed;
	Vertex4f = reinterpret_cast<PFNVERTEX4F>(IntGetProcAddress("glVertex4f"));
	if (!Vertex4f)
		++numFailed;
	Vertex4fv = reinterpret_cast<PFNVERTEX4FV>(IntGetProcAddress("glVertex4fv"));
	if (!Vertex4fv)
		++numFailed;
	Vertex4i = reinterpret_cast<PFNVERTEX4I>(IntGetProcAddress("glVertex4i"));
	if (!Vertex4i)
		++numFailed;
	Vertex4iv = reinterpret_cast<PFNVERTEX4IV>(IntGetProcAddress("glVertex4iv"));
	if (!Vertex4iv)
		++numFailed;
	Vertex4s = reinterpret_cast<PFNVERTEX4S>(IntGetProcAddress("glVertex4s"));
	if (!Vertex4s)
		++numFailed;
	Vertex4sv = reinterpret_cast<PFNVERTEX4SV>(IntGetProcAddress("glVertex4sv"));
	if (!Vertex4sv)
		++numFailed;
	Viewport = reinterpret_cast<PFNVIEWPORT>(IntGetProcAddress("glViewport"));
	if (!Viewport)
		++numFailed;
	AreTexturesResident =
	    reinterpret_cast<PFNARETEXTURESRESIDENT>(IntGetProcAddress("glAreTexturesResident"));
	if (!AreTexturesResident)
		++numFailed;
	ArrayElement = reinterpret_cast<PFNARRAYELEMENT>(IntGetProcAddress("glArrayElement"));
	if (!ArrayElement)
		++numFailed;
	BindTexture = reinterpret_cast<PFNBINDTEXTURE>(IntGetProcAddress("glBindTexture"));
	if (!BindTexture)
		++numFailed;
	ColorPointer = reinterpret_cast<PFNCOLORPOINTER>(IntGetProcAddress("glColorPointer"));
	if (!ColorPointer)
		++numFailed;
	CopyTexImage1D = reinterpret_cast<PFNCOPYTEXIMAGE1D>(IntGetProcAddress("glCopyTexImage1D"));
	if (!CopyTexImage1D)
		++numFailed;
	CopyTexImage2D = reinterpret_cast<PFNCOPYTEXIMAGE2D>(IntGetProcAddress("glCopyTexImage2D"));
	if (!CopyTexImage2D)
		++numFailed;
	CopyTexSubImage1D =
	    reinterpret_cast<PFNCOPYTEXSUBIMAGE1D>(IntGetProcAddress("glCopyTexSubImage1D"));
	if (!CopyTexSubImage1D)
		++numFailed;
	CopyTexSubImage2D =
	    reinterpret_cast<PFNCOPYTEXSUBIMAGE2D>(IntGetProcAddress("glCopyTexSubImage2D"));
	if (!CopyTexSubImage2D)
		++numFailed;
	DeleteTextures = reinterpret_cast<PFNDELETETEXTURES>(IntGetProcAddress("glDeleteTextures"));
	if (!DeleteTextures)
		++numFailed;
	DisableClientState =
	    reinterpret_cast<PFNDISABLECLIENTSTATE>(IntGetProcAddress("glDisableClientState"));
	if (!DisableClientState)
		++numFailed;
	DrawArrays = reinterpret_cast<PFNDRAWARRAYS>(IntGetProcAddress("glDrawArrays"));
	if (!DrawArrays)
		++numFailed;
	DrawElements = reinterpret_cast<PFNDRAWELEMENTS>(IntGetProcAddress("glDrawElements"));
	if (!DrawElements)
		++numFailed;
	EdgeFlagPointer = reinterpret_cast<PFNEDGEFLAGPOINTER>(IntGetProcAddress("glEdgeFlagPointer"));
	if (!EdgeFlagPointer)
		++numFailed;
	EnableClientState =
	    reinterpret_cast<PFNENABLECLIENTSTATE>(IntGetProcAddress("glEnableClientState"));
	if (!EnableClientState)
		++numFailed;
	GenTextures = reinterpret_cast<PFNGENTEXTURES>(IntGetProcAddress("glGenTextures"));
	if (!GenTextures)
		++numFailed;
	GetPointerv = reinterpret_cast<PFNGETPOINTERV>(IntGetProcAddress("glGetPointerv"));
	if (!GetPointerv)
		++numFailed;
	IndexPointer = reinterpret_cast<PFNINDEXPOINTER>(IntGetProcAddress("glIndexPointer"));
	if (!IndexPointer)
		++numFailed;
	Indexub = reinterpret_cast<PFNINDEXUB>(IntGetProcAddress("glIndexub"));
	if (!Indexub)
		++numFailed;
	Indexubv = reinterpret_cast<PFNINDEXUBV>(IntGetProcAddress("glIndexubv"));
	if (!Indexubv)
		++numFailed;
	InterleavedArrays =
	    reinterpret_cast<PFNINTERLEAVEDARRAYS>(IntGetProcAddress("glInterleavedArrays"));
	if (!InterleavedArrays)
		++numFailed;
	IsTexture = reinterpret_cast<PFNISTEXTURE>(IntGetProcAddress("glIsTexture"));
	if (!IsTexture)
		++numFailed;
	NormalPointer = reinterpret_cast<PFNNORMALPOINTER>(IntGetProcAddress("glNormalPointer"));
	if (!NormalPointer)
		++numFailed;
	PolygonOffset = reinterpret_cast<PFNPOLYGONOFFSET>(IntGetProcAddress("glPolygonOffset"));
	if (!PolygonOffset)
		++numFailed;
	PopClientAttrib = reinterpret_cast<PFNPOPCLIENTATTRIB>(IntGetProcAddress("glPopClientAttrib"));
	if (!PopClientAttrib)
		++numFailed;
	PrioritizeTextures =
	    reinterpret_cast<PFNPRIORITIZETEXTURES>(IntGetProcAddress("glPrioritizeTextures"));
	if (!PrioritizeTextures)
		++numFailed;
	PushClientAttrib =
	    reinterpret_cast<PFNPUSHCLIENTATTRIB>(IntGetProcAddress("glPushClientAttrib"));
	if (!PushClientAttrib)
		++numFailed;
	TexCoordPointer = reinterpret_cast<PFNTEXCOORDPOINTER>(IntGetProcAddress("glTexCoordPointer"));
	if (!TexCoordPointer)
		++numFailed;
	TexSubImage1D = reinterpret_cast<PFNTEXSUBIMAGE1D>(IntGetProcAddress("glTexSubImage1D"));
	if (!TexSubImage1D)
		++numFailed;
	TexSubImage2D = reinterpret_cast<PFNTEXSUBIMAGE2D>(IntGetProcAddress("glTexSubImage2D"));
	if (!TexSubImage2D)
		++numFailed;
	VertexPointer = reinterpret_cast<PFNVERTEXPOINTER>(IntGetProcAddress("glVertexPointer"));
	if (!VertexPointer)
		++numFailed;
	BlendColor = reinterpret_cast<PFNBLENDCOLOR>(IntGetProcAddress("glBlendColor"));
	if (!BlendColor)
		++numFailed;
	BlendEquation = reinterpret_cast<PFNBLENDEQUATION>(IntGetProcAddress("glBlendEquation"));
	if (!BlendEquation)
		++numFailed;
	CopyTexSubImage3D =
	    reinterpret_cast<PFNCOPYTEXSUBIMAGE3D>(IntGetProcAddress("glCopyTexSubImage3D"));
	if (!CopyTexSubImage3D)
		++numFailed;
	DrawRangeElements =
	    reinterpret_cast<PFNDRAWRANGEELEMENTS>(IntGetProcAddress("glDrawRangeElements"));
	if (!DrawRangeElements)
		++numFailed;
	TexImage3D = reinterpret_cast<PFNTEXIMAGE3D>(IntGetProcAddress("glTexImage3D"));
	if (!TexImage3D)
		++numFailed;
	TexSubImage3D = reinterpret_cast<PFNTEXSUBIMAGE3D>(IntGetProcAddress("glTexSubImage3D"));
	if (!TexSubImage3D)
		++numFailed;
	ActiveTexture = reinterpret_cast<PFNACTIVETEXTURE>(IntGetProcAddress("glActiveTexture"));
	if (!ActiveTexture)
		++numFailed;
	ClientActiveTexture =
	    reinterpret_cast<PFNCLIENTACTIVETEXTURE>(IntGetProcAddress("glClientActiveTexture"));
	if (!ClientActiveTexture)
		++numFailed;
	CompressedTexImage1D =
	    reinterpret_cast<PFNCOMPRESSEDTEXIMAGE1D>(IntGetProcAddress("glCompressedTexImage1D"));
	if (!CompressedTexImage1D)
		++numFailed;
	CompressedTexImage2D =
	    reinterpret_cast<PFNCOMPRESSEDTEXIMAGE2D>(IntGetProcAddress("glCompressedTexImage2D"));
	if (!CompressedTexImage2D)
		++numFailed;
	CompressedTexImage3D =
	    reinterpret_cast<PFNCOMPRESSEDTEXIMAGE3D>(IntGetProcAddress("glCompressedTexImage3D"));
	if (!CompressedTexImage3D)
		++numFailed;
	CompressedTexSubImage1D = reinterpret_cast<PFNCOMPRESSEDTEXSUBIMAGE1D>(
	    IntGetProcAddress("glCompressedTexSubImage1D"));
	if (!CompressedTexSubImage1D)
		++numFailed;
	CompressedTexSubImage2D = reinterpret_cast<PFNCOMPRESSEDTEXSUBIMAGE2D>(
	    IntGetProcAddress("glCompressedTexSubImage2D"));
	if (!CompressedTexSubImage2D)
		++numFailed;
	CompressedTexSubImage3D = reinterpret_cast<PFNCOMPRESSEDTEXSUBIMAGE3D>(
	    IntGetProcAddress("glCompressedTexSubImage3D"));
	if (!CompressedTexSubImage3D)
		++numFailed;
	GetCompressedTexImage =
	    reinterpret_cast<PFNGETCOMPRESSEDTEXIMAGE>(IntGetProcAddress("glGetCompressedTexImage"));
	if (!GetCompressedTexImage)
		++numFailed;
	LoadTransposeMatrixd =
	    reinterpret_cast<PFNLOADTRANSPOSEMATRIXD>(IntGetProcAddress("glLoadTransposeMatrixd"));
	if (!LoadTransposeMatrixd)
		++numFailed;
	LoadTransposeMatrixf =
	    reinterpret_cast<PFNLOADTRANSPOSEMATRIXF>(IntGetProcAddress("glLoadTransposeMatrixf"));
	if (!LoadTransposeMatrixf)
		++numFailed;
	MultTransposeMatrixd =
	    reinterpret_cast<PFNMULTTRANSPOSEMATRIXD>(IntGetProcAddress("glMultTransposeMatrixd"));
	if (!MultTransposeMatrixd)
		++numFailed;
	MultTransposeMatrixf =
	    reinterpret_cast<PFNMULTTRANSPOSEMATRIXF>(IntGetProcAddress("glMultTransposeMatrixf"));
	if (!MultTransposeMatrixf)
		++numFailed;
	MultiTexCoord1d = reinterpret_cast<PFNMULTITEXCOORD1D>(IntGetProcAddress("glMultiTexCoord1d"));
	if (!MultiTexCoord1d)
		++numFailed;
	MultiTexCoord1dv =
	    reinterpret_cast<PFNMULTITEXCOORD1DV>(IntGetProcAddress("glMultiTexCoord1dv"));
	if (!MultiTexCoord1dv)
		++numFailed;
	MultiTexCoord1f = reinterpret_cast<PFNMULTITEXCOORD1F>(IntGetProcAddress("glMultiTexCoord1f"));
	if (!MultiTexCoord1f)
		++numFailed;
	MultiTexCoord1fv =
	    reinterpret_cast<PFNMULTITEXCOORD1FV>(IntGetProcAddress("glMultiTexCoord1fv"));
	if (!MultiTexCoord1fv)
		++numFailed;
	MultiTexCoord1i = reinterpret_cast<PFNMULTITEXCOORD1I>(IntGetProcAddress("glMultiTexCoord1i"));
	if (!MultiTexCoord1i)
		++numFailed;
	MultiTexCoord1iv =
	    reinterpret_cast<PFNMULTITEXCOORD1IV>(IntGetProcAddress("glMultiTexCoord1iv"));
	if (!MultiTexCoord1iv)
		++numFailed;
	MultiTexCoord1s = reinterpret_cast<PFNMULTITEXCOORD1S>(IntGetProcAddress("glMultiTexCoord1s"));
	if (!MultiTexCoord1s)
		++numFailed;
	MultiTexCoord1sv =
	    reinterpret_cast<PFNMULTITEXCOORD1SV>(IntGetProcAddress("glMultiTexCoord1sv"));
	if (!MultiTexCoord1sv)
		++numFailed;
	MultiTexCoord2d = reinterpret_cast<PFNMULTITEXCOORD2D>(IntGetProcAddress("glMultiTexCoord2d"));
	if (!MultiTexCoord2d)
		++numFailed;
	MultiTexCoord2dv =
	    reinterpret_cast<PFNMULTITEXCOORD2DV>(IntGetProcAddress("glMultiTexCoord2dv"));
	if (!MultiTexCoord2dv)
		++numFailed;
	MultiTexCoord2f = reinterpret_cast<PFNMULTITEXCOORD2F>(IntGetProcAddress("glMultiTexCoord2f"));
	if (!MultiTexCoord2f)
		++numFailed;
	MultiTexCoord2fv =
	    reinterpret_cast<PFNMULTITEXCOORD2FV>(IntGetProcAddress("glMultiTexCoord2fv"));
	if (!MultiTexCoord2fv)
		++numFailed;
	MultiTexCoord2i = reinterpret_cast<PFNMULTITEXCOORD2I>(IntGetProcAddress("glMultiTexCoord2i"));
	if (!MultiTexCoord2i)
		++numFailed;
	MultiTexCoord2iv =
	    reinterpret_cast<PFNMULTITEXCOORD2IV>(IntGetProcAddress("glMultiTexCoord2iv"));
	if (!MultiTexCoord2iv)
		++numFailed;
	MultiTexCoord2s = reinterpret_cast<PFNMULTITEXCOORD2S>(IntGetProcAddress("glMultiTexCoord2s"));
	if (!MultiTexCoord2s)
		++numFailed;
	MultiTexCoord2sv =
	    reinterpret_cast<PFNMULTITEXCOORD2SV>(IntGetProcAddress("glMultiTexCoord2sv"));
	if (!MultiTexCoord2sv)
		++numFailed;
	MultiTexCoord3d = reinterpret_cast<PFNMULTITEXCOORD3D>(IntGetProcAddress("glMultiTexCoord3d"));
	if (!MultiTexCoord3d)
		++numFailed;
	MultiTexCoord3dv =
	    reinterpret_cast<PFNMULTITEXCOORD3DV>(IntGetProcAddress("glMultiTexCoord3dv"));
	if (!MultiTexCoord3dv)
		++numFailed;
	MultiTexCoord3f = reinterpret_cast<PFNMULTITEXCOORD3F>(IntGetProcAddress("glMultiTexCoord3f"));
	if (!MultiTexCoord3f)
		++numFailed;
	MultiTexCoord3fv =
	    reinterpret_cast<PFNMULTITEXCOORD3FV>(IntGetProcAddress("glMultiTexCoord3fv"));
	if (!MultiTexCoord3fv)
		++numFailed;
	MultiTexCoord3i = reinterpret_cast<PFNMULTITEXCOORD3I>(IntGetProcAddress("glMultiTexCoord3i"));
	if (!MultiTexCoord3i)
		++numFailed;
	MultiTexCoord3iv =
	    reinterpret_cast<PFNMULTITEXCOORD3IV>(IntGetProcAddress("glMultiTexCoord3iv"));
	if (!MultiTexCoord3iv)
		++numFailed;
	MultiTexCoord3s = reinterpret_cast<PFNMULTITEXCOORD3S>(IntGetProcAddress("glMultiTexCoord3s"));
	if (!MultiTexCoord3s)
		++numFailed;
	MultiTexCoord3sv =
	    reinterpret_cast<PFNMULTITEXCOORD3SV>(IntGetProcAddress("glMultiTexCoord3sv"));
	if (!MultiTexCoord3sv)
		++numFailed;
	MultiTexCoord4d = reinterpret_cast<PFNMULTITEXCOORD4D>(IntGetProcAddress("glMultiTexCoord4d"));
	if (!MultiTexCoord4d)
		++numFailed;
	MultiTexCoord4dv =
	    reinterpret_cast<PFNMULTITEXCOORD4DV>(IntGetProcAddress("glMultiTexCoord4dv"));
	if (!MultiTexCoord4dv)
		++numFailed;
	MultiTexCoord4f = reinterpret_cast<PFNMULTITEXCOORD4F>(IntGetProcAddress("glMultiTexCoord4f"));
	if (!MultiTexCoord4f)
		++numFailed;
	MultiTexCoord4fv =
	    reinterpret_cast<PFNMULTITEXCOORD4FV>(IntGetProcAddress("glMultiTexCoord4fv"));
	if (!MultiTexCoord4fv)
		++numFailed;
	MultiTexCoord4i = reinterpret_cast<PFNMULTITEXCOORD4I>(IntGetProcAddress("glMultiTexCoord4i"));
	if (!MultiTexCoord4i)
		++numFailed;
	MultiTexCoord4iv =
	    reinterpret_cast<PFNMULTITEXCOORD4IV>(IntGetProcAddress("glMultiTexCoord4iv"));
	if (!MultiTexCoord4iv)
		++numFailed;
	MultiTexCoord4s = reinterpret_cast<PFNMULTITEXCOORD4S>(IntGetProcAddress("glMultiTexCoord4s"));
	if (!MultiTexCoord4s)
		++numFailed;
	MultiTexCoord4sv =
	    reinterpret_cast<PFNMULTITEXCOORD4SV>(IntGetProcAddress("glMultiTexCoord4sv"));
	if (!MultiTexCoord4sv)
		++numFailed;
	SampleCoverage = reinterpret_cast<PFNSAMPLECOVERAGE>(IntGetProcAddress("glSampleCoverage"));
	if (!SampleCoverage)
		++numFailed;
	BlendFuncSeparate =
	    reinterpret_cast<PFNBLENDFUNCSEPARATE>(IntGetProcAddress("glBlendFuncSeparate"));
	if (!BlendFuncSeparate)
		++numFailed;
	FogCoordPointer = reinterpret_cast<PFNFOGCOORDPOINTER>(IntGetProcAddress("glFogCoordPointer"));
	if (!FogCoordPointer)
		++numFailed;
	FogCoordd = reinterpret_cast<PFNFOGCOORDD>(IntGetProcAddress("glFogCoordd"));
	if (!FogCoordd)
		++numFailed;
	FogCoorddv = reinterpret_cast<PFNFOGCOORDDV>(IntGetProcAddress("glFogCoorddv"));
	if (!FogCoorddv)
		++numFailed;
	FogCoordf = reinterpret_cast<PFNFOGCOORDF>(IntGetProcAddress("glFogCoordf"));
	if (!FogCoordf)
		++numFailed;
	FogCoordfv = reinterpret_cast<PFNFOGCOORDFV>(IntGetProcAddress("glFogCoordfv"));
	if (!FogCoordfv)
		++numFailed;
	MultiDrawArrays = reinterpret_cast<PFNMULTIDRAWARRAYS>(IntGetProcAddress("glMultiDrawArrays"));
	if (!MultiDrawArrays)
		++numFailed;
	MultiDrawElements =
	    reinterpret_cast<PFNMULTIDRAWELEMENTS>(IntGetProcAddress("glMultiDrawElements"));
	if (!MultiDrawElements)
		++numFailed;
	PointParameterf = reinterpret_cast<PFNPOINTPARAMETERF>(IntGetProcAddress("glPointParameterf"));
	if (!PointParameterf)
		++numFailed;
	PointParameterfv =
	    reinterpret_cast<PFNPOINTPARAMETERFV>(IntGetProcAddress("glPointParameterfv"));
	if (!PointParameterfv)
		++numFailed;
	PointParameteri = reinterpret_cast<PFNPOINTPARAMETERI>(IntGetProcAddress("glPointParameteri"));
	if (!PointParameteri)
		++numFailed;
	PointParameteriv =
	    reinterpret_cast<PFNPOINTPARAMETERIV>(IntGetProcAddress("glPointParameteriv"));
	if (!PointParameteriv)
		++numFailed;
	SecondaryColor3b =
	    reinterpret_cast<PFNSECONDARYCOLOR3B>(IntGetProcAddress("glSecondaryColor3b"));
	if (!SecondaryColor3b)
		++numFailed;
	SecondaryColor3bv =
	    reinterpret_cast<PFNSECONDARYCOLOR3BV>(IntGetProcAddress("glSecondaryColor3bv"));
	if (!SecondaryColor3bv)
		++numFailed;
	SecondaryColor3d =
	    reinterpret_cast<PFNSECONDARYCOLOR3D>(IntGetProcAddress("glSecondaryColor3d"));
	if (!SecondaryColor3d)
		++numFailed;
	SecondaryColor3dv =
	    reinterpret_cast<PFNSECONDARYCOLOR3DV>(IntGetProcAddress("glSecondaryColor3dv"));
	if (!SecondaryColor3dv)
		++numFailed;
	SecondaryColor3f =
	    reinterpret_cast<PFNSECONDARYCOLOR3F>(IntGetProcAddress("glSecondaryColor3f"));
	if (!SecondaryColor3f)
		++numFailed;
	SecondaryColor3fv =
	    reinterpret_cast<PFNSECONDARYCOLOR3FV>(IntGetProcAddress("glSecondaryColor3fv"));
	if (!SecondaryColor3fv)
		++numFailed;
	SecondaryColor3i =
	    reinterpret_cast<PFNSECONDARYCOLOR3I>(IntGetProcAddress("glSecondaryColor3i"));
	if (!SecondaryColor3i)
		++numFailed;
	SecondaryColor3iv =
	    reinterpret_cast<PFNSECONDARYCOLOR3IV>(IntGetProcAddress("glSecondaryColor3iv"));
	if (!SecondaryColor3iv)
		++numFailed;
	SecondaryColor3s =
	    reinterpret_cast<PFNSECONDARYCOLOR3S>(IntGetProcAddress("glSecondaryColor3s"));
	if (!SecondaryColor3s)
		++numFailed;
	SecondaryColor3sv =
	    reinterpret_cast<PFNSECONDARYCOLOR3SV>(IntGetProcAddress("glSecondaryColor3sv"));
	if (!SecondaryColor3sv)
		++numFailed;
	SecondaryColor3ub =
	    reinterpret_cast<PFNSECONDARYCOLOR3UB>(IntGetProcAddress("glSecondaryColor3ub"));
	if (!SecondaryColor3ub)
		++numFailed;
	SecondaryColor3ubv =
	    reinterpret_cast<PFNSECONDARYCOLOR3UBV>(IntGetProcAddress("glSecondaryColor3ubv"));
	if (!SecondaryColor3ubv)
		++numFailed;
	SecondaryColor3ui =
	    reinterpret_cast<PFNSECONDARYCOLOR3UI>(IntGetProcAddress("glSecondaryColor3ui"));
	if (!SecondaryColor3ui)
		++numFailed;
	SecondaryColor3uiv =
	    reinterpret_cast<PFNSECONDARYCOLOR3UIV>(IntGetProcAddress("glSecondaryColor3uiv"));
	if (!SecondaryColor3uiv)
		++numFailed;
	SecondaryColor3us =
	    reinterpret_cast<PFNSECONDARYCOLOR3US>(IntGetProcAddress("glSecondaryColor3us"));
	if (!SecondaryColor3us)
		++numFailed;
	SecondaryColor3usv =
	    reinterpret_cast<PFNSECONDARYCOLOR3USV>(IntGetProcAddress("glSecondaryColor3usv"));
	if (!SecondaryColor3usv)
		++numFailed;
	SecondaryColorPointer =
	    reinterpret_cast<PFNSECONDARYCOLORPOINTER>(IntGetProcAddress("glSecondaryColorPointer"));
	if (!SecondaryColorPointer)
		++numFailed;
	WindowPos2d = reinterpret_cast<PFNWINDOWPOS2D>(IntGetProcAddress("glWindowPos2d"));
	if (!WindowPos2d)
		++numFailed;
	WindowPos2dv = reinterpret_cast<PFNWINDOWPOS2DV>(IntGetProcAddress("glWindowPos2dv"));
	if (!WindowPos2dv)
		++numFailed;
	WindowPos2f = reinterpret_cast<PFNWINDOWPOS2F>(IntGetProcAddress("glWindowPos2f"));
	if (!WindowPos2f)
		++numFailed;
	WindowPos2fv = reinterpret_cast<PFNWINDOWPOS2FV>(IntGetProcAddress("glWindowPos2fv"));
	if (!WindowPos2fv)
		++numFailed;
	WindowPos2i = reinterpret_cast<PFNWINDOWPOS2I>(IntGetProcAddress("glWindowPos2i"));
	if (!WindowPos2i)
		++numFailed;
	WindowPos2iv = reinterpret_cast<PFNWINDOWPOS2IV>(IntGetProcAddress("glWindowPos2iv"));
	if (!WindowPos2iv)
		++numFailed;
	WindowPos2s = reinterpret_cast<PFNWINDOWPOS2S>(IntGetProcAddress("glWindowPos2s"));
	if (!WindowPos2s)
		++numFailed;
	WindowPos2sv = reinterpret_cast<PFNWINDOWPOS2SV>(IntGetProcAddress("glWindowPos2sv"));
	if (!WindowPos2sv)
		++numFailed;
	WindowPos3d = reinterpret_cast<PFNWINDOWPOS3D>(IntGetProcAddress("glWindowPos3d"));
	if (!WindowPos3d)
		++numFailed;
	WindowPos3dv = reinterpret_cast<PFNWINDOWPOS3DV>(IntGetProcAddress("glWindowPos3dv"));
	if (!WindowPos3dv)
		++numFailed;
	WindowPos3f = reinterpret_cast<PFNWINDOWPOS3F>(IntGetProcAddress("glWindowPos3f"));
	if (!WindowPos3f)
		++numFailed;
	WindowPos3fv = reinterpret_cast<PFNWINDOWPOS3FV>(IntGetProcAddress("glWindowPos3fv"));
	if (!WindowPos3fv)
		++numFailed;
	WindowPos3i = reinterpret_cast<PFNWINDOWPOS3I>(IntGetProcAddress("glWindowPos3i"));
	if (!WindowPos3i)
		++numFailed;
	WindowPos3iv = reinterpret_cast<PFNWINDOWPOS3IV>(IntGetProcAddress("glWindowPos3iv"));
	if (!WindowPos3iv)
		++numFailed;
	WindowPos3s = reinterpret_cast<PFNWINDOWPOS3S>(IntGetProcAddress("glWindowPos3s"));
	if (!WindowPos3s)
		++numFailed;
	WindowPos3sv = reinterpret_cast<PFNWINDOWPOS3SV>(IntGetProcAddress("glWindowPos3sv"));
	if (!WindowPos3sv)
		++numFailed;
	BeginQuery = reinterpret_cast<PFNBEGINQUERY>(IntGetProcAddress("glBeginQuery"));
	if (!BeginQuery)
		++numFailed;
	BindBuffer = reinterpret_cast<PFNBINDBUFFER>(IntGetProcAddress("glBindBuffer"));
	if (!BindBuffer)
		++numFailed;
	BufferData = reinterpret_cast<PFNBUFFERDATA>(IntGetProcAddress("glBufferData"));
	if (!BufferData)
		++numFailed;
	BufferSubData = reinterpret_cast<PFNBUFFERSUBDATA>(IntGetProcAddress("glBufferSubData"));
	if (!BufferSubData)
		++numFailed;
	DeleteBuffers = reinterpret_cast<PFNDELETEBUFFERS>(IntGetProcAddress("glDeleteBuffers"));
	if (!DeleteBuffers)
		++numFailed;
	DeleteQueries = reinterpret_cast<PFNDELETEQUERIES>(IntGetProcAddress("glDeleteQueries"));
	if (!DeleteQueries)
		++numFailed;
	EndQuery = reinterpret_cast<PFNENDQUERY>(IntGetProcAddress("glEndQuery"));
	if (!EndQuery)
		++numFailed;
	GenBuffers = reinterpret_cast<PFNGENBUFFERS>(IntGetProcAddress("glGenBuffers"));
	if (!GenBuffers)
		++numFailed;
	GenQueries = reinterpret_cast<PFNGENQUERIES>(IntGetProcAddress("glGenQueries"));
	if (!GenQueries)
		++numFailed;
	GetBufferParameteriv =
	    reinterpret_cast<PFNGETBUFFERPARAMETERIV>(IntGetProcAddress("glGetBufferParameteriv"));
	if (!GetBufferParameteriv)
		++numFailed;
	GetBufferPointerv =
	    reinterpret_cast<PFNGETBUFFERPOINTERV>(IntGetProcAddress("glGetBufferPointerv"));
	if (!GetBufferPointerv)
		++numFailed;
	GetBufferSubData =
	    reinterpret_cast<PFNGETBUFFERSUBDATA>(IntGetProcAddress("glGetBufferSubData"));
	if (!GetBufferSubData)
		++numFailed;
	GetQueryObjectiv =
	    reinterpret_cast<PFNGETQUERYOBJECTIV>(IntGetProcAddress("glGetQueryObjectiv"));
	if (!GetQueryObjectiv)
		++numFailed;
	GetQueryObjectuiv =
	    reinterpret_cast<PFNGETQUERYOBJECTUIV>(IntGetProcAddress("glGetQueryObjectuiv"));
	if (!GetQueryObjectuiv)
		++numFailed;
	GetQueryiv = reinterpret_cast<PFNGETQUERYIV>(IntGetProcAddress("glGetQueryiv"));
	if (!GetQueryiv)
		++numFailed;
	IsBuffer = reinterpret_cast<PFNISBUFFER>(IntGetProcAddress("glIsBuffer"));
	if (!IsBuffer)
		++numFailed;
	IsQuery = reinterpret_cast<PFNISQUERY>(IntGetProcAddress("glIsQuery"));
	if (!IsQuery)
		++numFailed;
	MapBuffer = reinterpret_cast<PFNMAPBUFFER>(IntGetProcAddress("glMapBuffer"));
	if (!MapBuffer)
		++numFailed;
	UnmapBuffer = reinterpret_cast<PFNUNMAPBUFFER>(IntGetProcAddress("glUnmapBuffer"));
	if (!UnmapBuffer)
		++numFailed;
	AttachShader = reinterpret_cast<PFNATTACHSHADER>(IntGetProcAddress("glAttachShader"));
	if (!AttachShader)
		++numFailed;
	BindAttribLocation =
	    reinterpret_cast<PFNBINDATTRIBLOCATION>(IntGetProcAddress("glBindAttribLocation"));
	if (!BindAttribLocation)
		++numFailed;
	BlendEquationSeparate =
	    reinterpret_cast<PFNBLENDEQUATIONSEPARATE>(IntGetProcAddress("glBlendEquationSeparate"));
	if (!BlendEquationSeparate)
		++numFailed;
	CompileShader = reinterpret_cast<PFNCOMPILESHADER>(IntGetProcAddress("glCompileShader"));
	if (!CompileShader)
		++numFailed;
	CreateProgram = reinterpret_cast<PFNCREATEPROGRAM>(IntGetProcAddress("glCreateProgram"));
	if (!CreateProgram)
		++numFailed;
	CreateShader = reinterpret_cast<PFNCREATESHADER>(IntGetProcAddress("glCreateShader"));
	if (!CreateShader)
		++numFailed;
	DeleteProgram = reinterpret_cast<PFNDELETEPROGRAM>(IntGetProcAddress("glDeleteProgram"));
	if (!DeleteProgram)
		++numFailed;
	DeleteShader = reinterpret_cast<PFNDELETESHADER>(IntGetProcAddress("glDeleteShader"));
	if (!DeleteShader)
		++numFailed;
	DetachShader = reinterpret_cast<PFNDETACHSHADER>(IntGetProcAddress("glDetachShader"));
	if (!DetachShader)
		++numFailed;
	DisableVertexAttribArray = reinterpret_cast<PFNDISABLEVERTEXATTRIBARRAY>(
	    IntGetProcAddress("glDisableVertexAttribArray"));
	if (!DisableVertexAttribArray)
		++numFailed;
	DrawBuffers = reinterpret_cast<PFNDRAWBUFFERS>(IntGetProcAddress("glDrawBuffers"));
	if (!DrawBuffers)
		++numFailed;
	EnableVertexAttribArray = reinterpret_cast<PFNENABLEVERTEXATTRIBARRAY>(
	    IntGetProcAddress("glEnableVertexAttribArray"));
	if (!EnableVertexAttribArray)
		++numFailed;
	GetActiveAttrib = reinterpret_cast<PFNGETACTIVEATTRIB>(IntGetProcAddress("glGetActiveAttrib"));
	if (!GetActiveAttrib)
		++numFailed;
	GetActiveUniform =
	    reinterpret_cast<PFNGETACTIVEUNIFORM>(IntGetProcAddress("glGetActiveUniform"));
	if (!GetActiveUniform)
		++numFailed;
	GetAttachedShaders =
	    reinterpret_cast<PFNGETATTACHEDSHADERS>(IntGetProcAddress("glGetAttachedShaders"));
	if (!GetAttachedShaders)
		++numFailed;
	GetAttribLocation =
	    reinterpret_cast<PFNGETATTRIBLOCATION>(IntGetProcAddress("glGetAttribLocation"));
	if (!GetAttribLocation)
		++numFailed;
	GetProgramInfoLog =
	    reinterpret_cast<PFNGETPROGRAMINFOLOG>(IntGetProcAddress("glGetProgramInfoLog"));
	if (!GetProgramInfoLog)
		++numFailed;
	GetProgramiv = reinterpret_cast<PFNGETPROGRAMIV>(IntGetProcAddress("glGetProgramiv"));
	if (!GetProgramiv)
		++numFailed;
	GetShaderInfoLog =
	    reinterpret_cast<PFNGETSHADERINFOLOG>(IntGetProcAddress("glGetShaderInfoLog"));
	if (!GetShaderInfoLog)
		++numFailed;
	GetShaderSource = reinterpret_cast<PFNGETSHADERSOURCE>(IntGetProcAddress("glGetShaderSource"));
	if (!GetShaderSource)
		++numFailed;
	GetShaderiv = reinterpret_cast<PFNGETSHADERIV>(IntGetProcAddress("glGetShaderiv"));
	if (!GetShaderiv)
		++numFailed;
	GetUniformLocation =
	    reinterpret_cast<PFNGETUNIFORMLOCATION>(IntGetProcAddress("glGetUniformLocation"));
	if (!GetUniformLocation)
		++numFailed;
	GetUniformfv = reinterpret_cast<PFNGETUNIFORMFV>(IntGetProcAddress("glGetUniformfv"));
	if (!GetUniformfv)
		++numFailed;
	GetUniformiv = reinterpret_cast<PFNGETUNIFORMIV>(IntGetProcAddress("glGetUniformiv"));
	if (!GetUniformiv)
		++numFailed;
	GetVertexAttribPointerv = reinterpret_cast<PFNGETVERTEXATTRIBPOINTERV>(
	    IntGetProcAddress("glGetVertexAttribPointerv"));
	if (!GetVertexAttribPointerv)
		++numFailed;
	GetVertexAttribdv =
	    reinterpret_cast<PFNGETVERTEXATTRIBDV>(IntGetProcAddress("glGetVertexAttribdv"));
	if (!GetVertexAttribdv)
		++numFailed;
	GetVertexAttribfv =
	    reinterpret_cast<PFNGETVERTEXATTRIBFV>(IntGetProcAddress("glGetVertexAttribfv"));
	if (!GetVertexAttribfv)
		++numFailed;
	GetVertexAttribiv =
	    reinterpret_cast<PFNGETVERTEXATTRIBIV>(IntGetProcAddress("glGetVertexAttribiv"));
	if (!GetVertexAttribiv)
		++numFailed;
	IsProgram = reinterpret_cast<PFNISPROGRAM>(IntGetProcAddress("glIsProgram"));
	if (!IsProgram)
		++numFailed;
	IsShader = reinterpret_cast<PFNISSHADER>(IntGetProcAddress("glIsShader"));
	if (!IsShader)
		++numFailed;
	LinkProgram = reinterpret_cast<PFNLINKPROGRAM>(IntGetProcAddress("glLinkProgram"));
	if (!LinkProgram)
		++numFailed;
	ShaderSource = reinterpret_cast<PFNSHADERSOURCE>(IntGetProcAddress("glShaderSource"));
	if (!ShaderSource)
		++numFailed;
	StencilFuncSeparate =
	    reinterpret_cast<PFNSTENCILFUNCSEPARATE>(IntGetProcAddress("glStencilFuncSeparate"));
	if (!StencilFuncSeparate)
		++numFailed;
	StencilMaskSeparate =
	    reinterpret_cast<PFNSTENCILMASKSEPARATE>(IntGetProcAddress("glStencilMaskSeparate"));
	if (!StencilMaskSeparate)
		++numFailed;
	StencilOpSeparate =
	    reinterpret_cast<PFNSTENCILOPSEPARATE>(IntGetProcAddress("glStencilOpSeparate"));
	if (!StencilOpSeparate)
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
	UniformMatrix2fv =
	    reinterpret_cast<PFNUNIFORMMATRIX2FV>(IntGetProcAddress("glUniformMatrix2fv"));
	if (!UniformMatrix2fv)
		++numFailed;
	UniformMatrix3fv =
	    reinterpret_cast<PFNUNIFORMMATRIX3FV>(IntGetProcAddress("glUniformMatrix3fv"));
	if (!UniformMatrix3fv)
		++numFailed;
	UniformMatrix4fv =
	    reinterpret_cast<PFNUNIFORMMATRIX4FV>(IntGetProcAddress("glUniformMatrix4fv"));
	if (!UniformMatrix4fv)
		++numFailed;
	UseProgram = reinterpret_cast<PFNUSEPROGRAM>(IntGetProcAddress("glUseProgram"));
	if (!UseProgram)
		++numFailed;
	ValidateProgram = reinterpret_cast<PFNVALIDATEPROGRAM>(IntGetProcAddress("glValidateProgram"));
	if (!ValidateProgram)
		++numFailed;
	VertexAttrib1d = reinterpret_cast<PFNVERTEXATTRIB1D>(IntGetProcAddress("glVertexAttrib1d"));
	if (!VertexAttrib1d)
		++numFailed;
	VertexAttrib1dv = reinterpret_cast<PFNVERTEXATTRIB1DV>(IntGetProcAddress("glVertexAttrib1dv"));
	if (!VertexAttrib1dv)
		++numFailed;
	VertexAttrib1f = reinterpret_cast<PFNVERTEXATTRIB1F>(IntGetProcAddress("glVertexAttrib1f"));
	if (!VertexAttrib1f)
		++numFailed;
	VertexAttrib1fv = reinterpret_cast<PFNVERTEXATTRIB1FV>(IntGetProcAddress("glVertexAttrib1fv"));
	if (!VertexAttrib1fv)
		++numFailed;
	VertexAttrib1s = reinterpret_cast<PFNVERTEXATTRIB1S>(IntGetProcAddress("glVertexAttrib1s"));
	if (!VertexAttrib1s)
		++numFailed;
	VertexAttrib1sv = reinterpret_cast<PFNVERTEXATTRIB1SV>(IntGetProcAddress("glVertexAttrib1sv"));
	if (!VertexAttrib1sv)
		++numFailed;
	VertexAttrib2d = reinterpret_cast<PFNVERTEXATTRIB2D>(IntGetProcAddress("glVertexAttrib2d"));
	if (!VertexAttrib2d)
		++numFailed;
	VertexAttrib2dv = reinterpret_cast<PFNVERTEXATTRIB2DV>(IntGetProcAddress("glVertexAttrib2dv"));
	if (!VertexAttrib2dv)
		++numFailed;
	VertexAttrib2f = reinterpret_cast<PFNVERTEXATTRIB2F>(IntGetProcAddress("glVertexAttrib2f"));
	if (!VertexAttrib2f)
		++numFailed;
	VertexAttrib2fv = reinterpret_cast<PFNVERTEXATTRIB2FV>(IntGetProcAddress("glVertexAttrib2fv"));
	if (!VertexAttrib2fv)
		++numFailed;
	VertexAttrib2s = reinterpret_cast<PFNVERTEXATTRIB2S>(IntGetProcAddress("glVertexAttrib2s"));
	if (!VertexAttrib2s)
		++numFailed;
	VertexAttrib2sv = reinterpret_cast<PFNVERTEXATTRIB2SV>(IntGetProcAddress("glVertexAttrib2sv"));
	if (!VertexAttrib2sv)
		++numFailed;
	VertexAttrib3d = reinterpret_cast<PFNVERTEXATTRIB3D>(IntGetProcAddress("glVertexAttrib3d"));
	if (!VertexAttrib3d)
		++numFailed;
	VertexAttrib3dv = reinterpret_cast<PFNVERTEXATTRIB3DV>(IntGetProcAddress("glVertexAttrib3dv"));
	if (!VertexAttrib3dv)
		++numFailed;
	VertexAttrib3f = reinterpret_cast<PFNVERTEXATTRIB3F>(IntGetProcAddress("glVertexAttrib3f"));
	if (!VertexAttrib3f)
		++numFailed;
	VertexAttrib3fv = reinterpret_cast<PFNVERTEXATTRIB3FV>(IntGetProcAddress("glVertexAttrib3fv"));
	if (!VertexAttrib3fv)
		++numFailed;
	VertexAttrib3s = reinterpret_cast<PFNVERTEXATTRIB3S>(IntGetProcAddress("glVertexAttrib3s"));
	if (!VertexAttrib3s)
		++numFailed;
	VertexAttrib3sv = reinterpret_cast<PFNVERTEXATTRIB3SV>(IntGetProcAddress("glVertexAttrib3sv"));
	if (!VertexAttrib3sv)
		++numFailed;
	VertexAttrib4Nbv =
	    reinterpret_cast<PFNVERTEXATTRIB4NBV>(IntGetProcAddress("glVertexAttrib4Nbv"));
	if (!VertexAttrib4Nbv)
		++numFailed;
	VertexAttrib4Niv =
	    reinterpret_cast<PFNVERTEXATTRIB4NIV>(IntGetProcAddress("glVertexAttrib4Niv"));
	if (!VertexAttrib4Niv)
		++numFailed;
	VertexAttrib4Nsv =
	    reinterpret_cast<PFNVERTEXATTRIB4NSV>(IntGetProcAddress("glVertexAttrib4Nsv"));
	if (!VertexAttrib4Nsv)
		++numFailed;
	VertexAttrib4Nub =
	    reinterpret_cast<PFNVERTEXATTRIB4NUB>(IntGetProcAddress("glVertexAttrib4Nub"));
	if (!VertexAttrib4Nub)
		++numFailed;
	VertexAttrib4Nubv =
	    reinterpret_cast<PFNVERTEXATTRIB4NUBV>(IntGetProcAddress("glVertexAttrib4Nubv"));
	if (!VertexAttrib4Nubv)
		++numFailed;
	VertexAttrib4Nuiv =
	    reinterpret_cast<PFNVERTEXATTRIB4NUIV>(IntGetProcAddress("glVertexAttrib4Nuiv"));
	if (!VertexAttrib4Nuiv)
		++numFailed;
	VertexAttrib4Nusv =
	    reinterpret_cast<PFNVERTEXATTRIB4NUSV>(IntGetProcAddress("glVertexAttrib4Nusv"));
	if (!VertexAttrib4Nusv)
		++numFailed;
	VertexAttrib4bv = reinterpret_cast<PFNVERTEXATTRIB4BV>(IntGetProcAddress("glVertexAttrib4bv"));
	if (!VertexAttrib4bv)
		++numFailed;
	VertexAttrib4d = reinterpret_cast<PFNVERTEXATTRIB4D>(IntGetProcAddress("glVertexAttrib4d"));
	if (!VertexAttrib4d)
		++numFailed;
	VertexAttrib4dv = reinterpret_cast<PFNVERTEXATTRIB4DV>(IntGetProcAddress("glVertexAttrib4dv"));
	if (!VertexAttrib4dv)
		++numFailed;
	VertexAttrib4f = reinterpret_cast<PFNVERTEXATTRIB4F>(IntGetProcAddress("glVertexAttrib4f"));
	if (!VertexAttrib4f)
		++numFailed;
	VertexAttrib4fv = reinterpret_cast<PFNVERTEXATTRIB4FV>(IntGetProcAddress("glVertexAttrib4fv"));
	if (!VertexAttrib4fv)
		++numFailed;
	VertexAttrib4iv = reinterpret_cast<PFNVERTEXATTRIB4IV>(IntGetProcAddress("glVertexAttrib4iv"));
	if (!VertexAttrib4iv)
		++numFailed;
	VertexAttrib4s = reinterpret_cast<PFNVERTEXATTRIB4S>(IntGetProcAddress("glVertexAttrib4s"));
	if (!VertexAttrib4s)
		++numFailed;
	VertexAttrib4sv = reinterpret_cast<PFNVERTEXATTRIB4SV>(IntGetProcAddress("glVertexAttrib4sv"));
	if (!VertexAttrib4sv)
		++numFailed;
	VertexAttrib4ubv =
	    reinterpret_cast<PFNVERTEXATTRIB4UBV>(IntGetProcAddress("glVertexAttrib4ubv"));
	if (!VertexAttrib4ubv)
		++numFailed;
	VertexAttrib4uiv =
	    reinterpret_cast<PFNVERTEXATTRIB4UIV>(IntGetProcAddress("glVertexAttrib4uiv"));
	if (!VertexAttrib4uiv)
		++numFailed;
	VertexAttrib4usv =
	    reinterpret_cast<PFNVERTEXATTRIB4USV>(IntGetProcAddress("glVertexAttrib4usv"));
	if (!VertexAttrib4usv)
		++numFailed;
	VertexAttribPointer =
	    reinterpret_cast<PFNVERTEXATTRIBPOINTER>(IntGetProcAddress("glVertexAttribPointer"));
	if (!VertexAttribPointer)
		++numFailed;
	UniformMatrix2x3fv =
	    reinterpret_cast<PFNUNIFORMMATRIX2X3FV>(IntGetProcAddress("glUniformMatrix2x3fv"));
	if (!UniformMatrix2x3fv)
		++numFailed;
	UniformMatrix2x4fv =
	    reinterpret_cast<PFNUNIFORMMATRIX2X4FV>(IntGetProcAddress("glUniformMatrix2x4fv"));
	if (!UniformMatrix2x4fv)
		++numFailed;
	UniformMatrix3x2fv =
	    reinterpret_cast<PFNUNIFORMMATRIX3X2FV>(IntGetProcAddress("glUniformMatrix3x2fv"));
	if (!UniformMatrix3x2fv)
		++numFailed;
	UniformMatrix3x4fv =
	    reinterpret_cast<PFNUNIFORMMATRIX3X4FV>(IntGetProcAddress("glUniformMatrix3x4fv"));
	if (!UniformMatrix3x4fv)
		++numFailed;
	UniformMatrix4x2fv =
	    reinterpret_cast<PFNUNIFORMMATRIX4X2FV>(IntGetProcAddress("glUniformMatrix4x2fv"));
	if (!UniformMatrix4x2fv)
		++numFailed;
	UniformMatrix4x3fv =
	    reinterpret_cast<PFNUNIFORMMATRIX4X3FV>(IntGetProcAddress("glUniformMatrix4x3fv"));
	if (!UniformMatrix4x3fv)
		++numFailed;
	BeginConditionalRender =
	    reinterpret_cast<PFNBEGINCONDITIONALRENDER>(IntGetProcAddress("glBeginConditionalRender"));
	if (!BeginConditionalRender)
		++numFailed;
	BeginTransformFeedback =
	    reinterpret_cast<PFNBEGINTRANSFORMFEEDBACK>(IntGetProcAddress("glBeginTransformFeedback"));
	if (!BeginTransformFeedback)
		++numFailed;
	BindBufferBase = reinterpret_cast<PFNBINDBUFFERBASE>(IntGetProcAddress("glBindBufferBase"));
	if (!BindBufferBase)
		++numFailed;
	BindBufferRange = reinterpret_cast<PFNBINDBUFFERRANGE>(IntGetProcAddress("glBindBufferRange"));
	if (!BindBufferRange)
		++numFailed;
	BindFragDataLocation =
	    reinterpret_cast<PFNBINDFRAGDATALOCATION>(IntGetProcAddress("glBindFragDataLocation"));
	if (!BindFragDataLocation)
		++numFailed;
	BindFramebuffer = reinterpret_cast<PFNBINDFRAMEBUFFER>(IntGetProcAddress("glBindFramebuffer"));
	if (!BindFramebuffer)
		++numFailed;
	BindRenderbuffer =
	    reinterpret_cast<PFNBINDRENDERBUFFER>(IntGetProcAddress("glBindRenderbuffer"));
	if (!BindRenderbuffer)
		++numFailed;
	BindVertexArray = reinterpret_cast<PFNBINDVERTEXARRAY>(IntGetProcAddress("glBindVertexArray"));
	if (!BindVertexArray)
		++numFailed;
	BlitFramebuffer = reinterpret_cast<PFNBLITFRAMEBUFFER>(IntGetProcAddress("glBlitFramebuffer"));
	if (!BlitFramebuffer)
		++numFailed;
	CheckFramebufferStatus =
	    reinterpret_cast<PFNCHECKFRAMEBUFFERSTATUS>(IntGetProcAddress("glCheckFramebufferStatus"));
	if (!CheckFramebufferStatus)
		++numFailed;
	ClampColor = reinterpret_cast<PFNCLAMPCOLOR>(IntGetProcAddress("glClampColor"));
	if (!ClampColor)
		++numFailed;
	ClearBufferfi = reinterpret_cast<PFNCLEARBUFFERFI>(IntGetProcAddress("glClearBufferfi"));
	if (!ClearBufferfi)
		++numFailed;
	ClearBufferfv = reinterpret_cast<PFNCLEARBUFFERFV>(IntGetProcAddress("glClearBufferfv"));
	if (!ClearBufferfv)
		++numFailed;
	ClearBufferiv = reinterpret_cast<PFNCLEARBUFFERIV>(IntGetProcAddress("glClearBufferiv"));
	if (!ClearBufferiv)
		++numFailed;
	ClearBufferuiv = reinterpret_cast<PFNCLEARBUFFERUIV>(IntGetProcAddress("glClearBufferuiv"));
	if (!ClearBufferuiv)
		++numFailed;
	ColorMaski = reinterpret_cast<PFNCOLORMASKI>(IntGetProcAddress("glColorMaski"));
	if (!ColorMaski)
		++numFailed;
	DeleteFramebuffers =
	    reinterpret_cast<PFNDELETEFRAMEBUFFERS>(IntGetProcAddress("glDeleteFramebuffers"));
	if (!DeleteFramebuffers)
		++numFailed;
	DeleteRenderbuffers =
	    reinterpret_cast<PFNDELETERENDERBUFFERS>(IntGetProcAddress("glDeleteRenderbuffers"));
	if (!DeleteRenderbuffers)
		++numFailed;
	DeleteVertexArrays =
	    reinterpret_cast<PFNDELETEVERTEXARRAYS>(IntGetProcAddress("glDeleteVertexArrays"));
	if (!DeleteVertexArrays)
		++numFailed;
	Disablei = reinterpret_cast<PFNDISABLEI>(IntGetProcAddress("glDisablei"));
	if (!Disablei)
		++numFailed;
	Enablei = reinterpret_cast<PFNENABLEI>(IntGetProcAddress("glEnablei"));
	if (!Enablei)
		++numFailed;
	EndConditionalRender =
	    reinterpret_cast<PFNENDCONDITIONALRENDER>(IntGetProcAddress("glEndConditionalRender"));
	if (!EndConditionalRender)
		++numFailed;
	EndTransformFeedback =
	    reinterpret_cast<PFNENDTRANSFORMFEEDBACK>(IntGetProcAddress("glEndTransformFeedback"));
	if (!EndTransformFeedback)
		++numFailed;
	FlushMappedBufferRange =
	    reinterpret_cast<PFNFLUSHMAPPEDBUFFERRANGE>(IntGetProcAddress("glFlushMappedBufferRange"));
	if (!FlushMappedBufferRange)
		++numFailed;
	FramebufferRenderbuffer = reinterpret_cast<PFNFRAMEBUFFERRENDERBUFFER>(
	    IntGetProcAddress("glFramebufferRenderbuffer"));
	if (!FramebufferRenderbuffer)
		++numFailed;
	FramebufferTexture1D =
	    reinterpret_cast<PFNFRAMEBUFFERTEXTURE1D>(IntGetProcAddress("glFramebufferTexture1D"));
	if (!FramebufferTexture1D)
		++numFailed;
	FramebufferTexture2D =
	    reinterpret_cast<PFNFRAMEBUFFERTEXTURE2D>(IntGetProcAddress("glFramebufferTexture2D"));
	if (!FramebufferTexture2D)
		++numFailed;
	FramebufferTexture3D =
	    reinterpret_cast<PFNFRAMEBUFFERTEXTURE3D>(IntGetProcAddress("glFramebufferTexture3D"));
	if (!FramebufferTexture3D)
		++numFailed;
	FramebufferTextureLayer = reinterpret_cast<PFNFRAMEBUFFERTEXTURELAYER>(
	    IntGetProcAddress("glFramebufferTextureLayer"));
	if (!FramebufferTextureLayer)
		++numFailed;
	GenFramebuffers = reinterpret_cast<PFNGENFRAMEBUFFERS>(IntGetProcAddress("glGenFramebuffers"));
	if (!GenFramebuffers)
		++numFailed;
	GenRenderbuffers =
	    reinterpret_cast<PFNGENRENDERBUFFERS>(IntGetProcAddress("glGenRenderbuffers"));
	if (!GenRenderbuffers)
		++numFailed;
	GenVertexArrays = reinterpret_cast<PFNGENVERTEXARRAYS>(IntGetProcAddress("glGenVertexArrays"));
	if (!GenVertexArrays)
		++numFailed;
	GenerateMipmap = reinterpret_cast<PFNGENERATEMIPMAP>(IntGetProcAddress("glGenerateMipmap"));
	if (!GenerateMipmap)
		++numFailed;
	GetBooleani_v = reinterpret_cast<PFNGETBOOLEANI_V>(IntGetProcAddress("glGetBooleani_v"));
	if (!GetBooleani_v)
		++numFailed;
	GetFragDataLocation =
	    reinterpret_cast<PFNGETFRAGDATALOCATION>(IntGetProcAddress("glGetFragDataLocation"));
	if (!GetFragDataLocation)
		++numFailed;
	GetFramebufferAttachmentParameteriv = reinterpret_cast<PFNGETFRAMEBUFFERATTACHMENTPARAMETERIV>(
	    IntGetProcAddress("glGetFramebufferAttachmentParameteriv"));
	if (!GetFramebufferAttachmentParameteriv)
		++numFailed;
	GetIntegeri_v = reinterpret_cast<PFNGETINTEGERI_V>(IntGetProcAddress("glGetIntegeri_v"));
	if (!GetIntegeri_v)
		++numFailed;
	GetRenderbufferParameteriv = reinterpret_cast<PFNGETRENDERBUFFERPARAMETERIV>(
	    IntGetProcAddress("glGetRenderbufferParameteriv"));
	if (!GetRenderbufferParameteriv)
		++numFailed;
	GetStringi = reinterpret_cast<PFNGETSTRINGI>(IntGetProcAddress("glGetStringi"));
	if (!GetStringi)
		++numFailed;
	GetTexParameterIiv =
	    reinterpret_cast<PFNGETTEXPARAMETERIIV>(IntGetProcAddress("glGetTexParameterIiv"));
	if (!GetTexParameterIiv)
		++numFailed;
	GetTexParameterIuiv =
	    reinterpret_cast<PFNGETTEXPARAMETERIUIV>(IntGetProcAddress("glGetTexParameterIuiv"));
	if (!GetTexParameterIuiv)
		++numFailed;
	GetTransformFeedbackVarying = reinterpret_cast<PFNGETTRANSFORMFEEDBACKVARYING>(
	    IntGetProcAddress("glGetTransformFeedbackVarying"));
	if (!GetTransformFeedbackVarying)
		++numFailed;
	GetUniformuiv = reinterpret_cast<PFNGETUNIFORMUIV>(IntGetProcAddress("glGetUniformuiv"));
	if (!GetUniformuiv)
		++numFailed;
	GetVertexAttribIiv =
	    reinterpret_cast<PFNGETVERTEXATTRIBIIV>(IntGetProcAddress("glGetVertexAttribIiv"));
	if (!GetVertexAttribIiv)
		++numFailed;
	GetVertexAttribIuiv =
	    reinterpret_cast<PFNGETVERTEXATTRIBIUIV>(IntGetProcAddress("glGetVertexAttribIuiv"));
	if (!GetVertexAttribIuiv)
		++numFailed;
	IsEnabledi = reinterpret_cast<PFNISENABLEDI>(IntGetProcAddress("glIsEnabledi"));
	if (!IsEnabledi)
		++numFailed;
	IsFramebuffer = reinterpret_cast<PFNISFRAMEBUFFER>(IntGetProcAddress("glIsFramebuffer"));
	if (!IsFramebuffer)
		++numFailed;
	IsRenderbuffer = reinterpret_cast<PFNISRENDERBUFFER>(IntGetProcAddress("glIsRenderbuffer"));
	if (!IsRenderbuffer)
		++numFailed;
	IsVertexArray = reinterpret_cast<PFNISVERTEXARRAY>(IntGetProcAddress("glIsVertexArray"));
	if (!IsVertexArray)
		++numFailed;
	MapBufferRange = reinterpret_cast<PFNMAPBUFFERRANGE>(IntGetProcAddress("glMapBufferRange"));
	if (!MapBufferRange)
		++numFailed;
	RenderbufferStorage =
	    reinterpret_cast<PFNRENDERBUFFERSTORAGE>(IntGetProcAddress("glRenderbufferStorage"));
	if (!RenderbufferStorage)
		++numFailed;
	RenderbufferStorageMultisample = reinterpret_cast<PFNRENDERBUFFERSTORAGEMULTISAMPLE>(
	    IntGetProcAddress("glRenderbufferStorageMultisample"));
	if (!RenderbufferStorageMultisample)
		++numFailed;
	TexParameterIiv = reinterpret_cast<PFNTEXPARAMETERIIV>(IntGetProcAddress("glTexParameterIiv"));
	if (!TexParameterIiv)
		++numFailed;
	TexParameterIuiv =
	    reinterpret_cast<PFNTEXPARAMETERIUIV>(IntGetProcAddress("glTexParameterIuiv"));
	if (!TexParameterIuiv)
		++numFailed;
	TransformFeedbackVaryings = reinterpret_cast<PFNTRANSFORMFEEDBACKVARYINGS>(
	    IntGetProcAddress("glTransformFeedbackVaryings"));
	if (!TransformFeedbackVaryings)
		++numFailed;
	Uniform1ui = reinterpret_cast<PFNUNIFORM1UI>(IntGetProcAddress("glUniform1ui"));
	if (!Uniform1ui)
		++numFailed;
	Uniform1uiv = reinterpret_cast<PFNUNIFORM1UIV>(IntGetProcAddress("glUniform1uiv"));
	if (!Uniform1uiv)
		++numFailed;
	Uniform2ui = reinterpret_cast<PFNUNIFORM2UI>(IntGetProcAddress("glUniform2ui"));
	if (!Uniform2ui)
		++numFailed;
	Uniform2uiv = reinterpret_cast<PFNUNIFORM2UIV>(IntGetProcAddress("glUniform2uiv"));
	if (!Uniform2uiv)
		++numFailed;
	Uniform3ui = reinterpret_cast<PFNUNIFORM3UI>(IntGetProcAddress("glUniform3ui"));
	if (!Uniform3ui)
		++numFailed;
	Uniform3uiv = reinterpret_cast<PFNUNIFORM3UIV>(IntGetProcAddress("glUniform3uiv"));
	if (!Uniform3uiv)
		++numFailed;
	Uniform4ui = reinterpret_cast<PFNUNIFORM4UI>(IntGetProcAddress("glUniform4ui"));
	if (!Uniform4ui)
		++numFailed;
	Uniform4uiv = reinterpret_cast<PFNUNIFORM4UIV>(IntGetProcAddress("glUniform4uiv"));
	if (!Uniform4uiv)
		++numFailed;
	VertexAttribI1i = reinterpret_cast<PFNVERTEXATTRIBI1I>(IntGetProcAddress("glVertexAttribI1i"));
	if (!VertexAttribI1i)
		++numFailed;
	VertexAttribI1iv =
	    reinterpret_cast<PFNVERTEXATTRIBI1IV>(IntGetProcAddress("glVertexAttribI1iv"));
	if (!VertexAttribI1iv)
		++numFailed;
	VertexAttribI1ui =
	    reinterpret_cast<PFNVERTEXATTRIBI1UI>(IntGetProcAddress("glVertexAttribI1ui"));
	if (!VertexAttribI1ui)
		++numFailed;
	VertexAttribI1uiv =
	    reinterpret_cast<PFNVERTEXATTRIBI1UIV>(IntGetProcAddress("glVertexAttribI1uiv"));
	if (!VertexAttribI1uiv)
		++numFailed;
	VertexAttribI2i = reinterpret_cast<PFNVERTEXATTRIBI2I>(IntGetProcAddress("glVertexAttribI2i"));
	if (!VertexAttribI2i)
		++numFailed;
	VertexAttribI2iv =
	    reinterpret_cast<PFNVERTEXATTRIBI2IV>(IntGetProcAddress("glVertexAttribI2iv"));
	if (!VertexAttribI2iv)
		++numFailed;
	VertexAttribI2ui =
	    reinterpret_cast<PFNVERTEXATTRIBI2UI>(IntGetProcAddress("glVertexAttribI2ui"));
	if (!VertexAttribI2ui)
		++numFailed;
	VertexAttribI2uiv =
	    reinterpret_cast<PFNVERTEXATTRIBI2UIV>(IntGetProcAddress("glVertexAttribI2uiv"));
	if (!VertexAttribI2uiv)
		++numFailed;
	VertexAttribI3i = reinterpret_cast<PFNVERTEXATTRIBI3I>(IntGetProcAddress("glVertexAttribI3i"));
	if (!VertexAttribI3i)
		++numFailed;
	VertexAttribI3iv =
	    reinterpret_cast<PFNVERTEXATTRIBI3IV>(IntGetProcAddress("glVertexAttribI3iv"));
	if (!VertexAttribI3iv)
		++numFailed;
	VertexAttribI3ui =
	    reinterpret_cast<PFNVERTEXATTRIBI3UI>(IntGetProcAddress("glVertexAttribI3ui"));
	if (!VertexAttribI3ui)
		++numFailed;
	VertexAttribI3uiv =
	    reinterpret_cast<PFNVERTEXATTRIBI3UIV>(IntGetProcAddress("glVertexAttribI3uiv"));
	if (!VertexAttribI3uiv)
		++numFailed;
	VertexAttribI4bv =
	    reinterpret_cast<PFNVERTEXATTRIBI4BV>(IntGetProcAddress("glVertexAttribI4bv"));
	if (!VertexAttribI4bv)
		++numFailed;
	VertexAttribI4i = reinterpret_cast<PFNVERTEXATTRIBI4I>(IntGetProcAddress("glVertexAttribI4i"));
	if (!VertexAttribI4i)
		++numFailed;
	VertexAttribI4iv =
	    reinterpret_cast<PFNVERTEXATTRIBI4IV>(IntGetProcAddress("glVertexAttribI4iv"));
	if (!VertexAttribI4iv)
		++numFailed;
	VertexAttribI4sv =
	    reinterpret_cast<PFNVERTEXATTRIBI4SV>(IntGetProcAddress("glVertexAttribI4sv"));
	if (!VertexAttribI4sv)
		++numFailed;
	VertexAttribI4ubv =
	    reinterpret_cast<PFNVERTEXATTRIBI4UBV>(IntGetProcAddress("glVertexAttribI4ubv"));
	if (!VertexAttribI4ubv)
		++numFailed;
	VertexAttribI4ui =
	    reinterpret_cast<PFNVERTEXATTRIBI4UI>(IntGetProcAddress("glVertexAttribI4ui"));
	if (!VertexAttribI4ui)
		++numFailed;
	VertexAttribI4uiv =
	    reinterpret_cast<PFNVERTEXATTRIBI4UIV>(IntGetProcAddress("glVertexAttribI4uiv"));
	if (!VertexAttribI4uiv)
		++numFailed;
	VertexAttribI4usv =
	    reinterpret_cast<PFNVERTEXATTRIBI4USV>(IntGetProcAddress("glVertexAttribI4usv"));
	if (!VertexAttribI4usv)
		++numFailed;
	VertexAttribIPointer =
	    reinterpret_cast<PFNVERTEXATTRIBIPOINTER>(IntGetProcAddress("glVertexAttribIPointer"));
	if (!VertexAttribIPointer)
		++numFailed;
	return numFailed;
}

namespace sys
{
namespace
{
typedef int (*PFN_LOADEXTENSION)();
struct MapEntry {
	MapEntry(const char *_extName, exts::LoadTest *_extVariable)
	    : extName(_extName), extVariable(_extVariable), loaderFunc(0)
	{
	}

	MapEntry(const char *_extName, exts::LoadTest *_extVariable, PFN_LOADEXTENSION _loaderFunc)
	    : extName(_extName), extVariable(_extVariable), loaderFunc(_loaderFunc)
	{
	}

	const char *extName;
	exts::LoadTest *extVariable;
	PFN_LOADEXTENSION loaderFunc;
};

struct MapCompare {
	MapCompare(const char *test_) : test(test_) {}
	bool operator()(const MapEntry &other) { return strcmp(test, other.extName) == 0; }
	const char *test;
};

void InitializeMappingTable(std::vector<MapEntry> &table) { table.reserve(0); }

void ClearExtensionVars() {}

void LoadExtByName(std::vector<MapEntry> &table, const char *extensionName)
{
	auto entry = std::find_if(table.begin(), table.end(), MapCompare(extensionName));

	if (entry != table.end()) {
		if (entry->loaderFunc)
			(*entry->extVariable) = exts::LoadTest(true, entry->loaderFunc());
		else
			(*entry->extVariable) = exts::LoadTest(true, 0);
	}
}
} // namespace

namespace
{
static void ProcExtsFromExtList(std::vector<MapEntry> &table)
{
	GLint iLoop;
	GLint iNumExtensions = 0;
	gl::GetIntegerv(gl::NUM_EXTENSIONS, &iNumExtensions);

	for (iLoop = 0; iLoop < iNumExtensions; iLoop++) {
		const char *strExtensionName = (const char *)gl::GetStringi(gl::EXTENSIONS, iLoop);
		LoadExtByName(table, strExtensionName);
	}
}

} // namespace

exts::LoadTest LoadFunctions()
{
	ClearExtensionVars();
	std::vector<MapEntry> table;
	InitializeMappingTable(table);

	GetIntegerv = reinterpret_cast<PFNGETINTEGERV>(IntGetProcAddress("glGetIntegerv"));
	if (!GetIntegerv)
		return exts::LoadTest();
	GetStringi = reinterpret_cast<PFNGETSTRINGI>(IntGetProcAddress("glGetStringi"));
	if (!GetStringi)
		return exts::LoadTest();

	ProcExtsFromExtList(table);

	int numFailed = LoadCoreFunctions();
	return exts::LoadTest(true, numFailed);
}

static int g_major_version = 0;
static int g_minor_version = 0;

static void GetGLVersion()
{
	GetIntegerv(MAJOR_VERSION, &g_major_version);
	GetIntegerv(MINOR_VERSION, &g_minor_version);
}

int GetMajorVersion()
{
	if (g_major_version == 0)
		GetGLVersion();
	return g_major_version;
}

int GetMinorVersion()
{
	if (g_major_version == 0) // Yes, check the major version to get the minor one.
		GetGLVersion();
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

} // namespace sys
} // namespace gl
