#ifndef GLESWRAP_H
#define GLESWRAP_H

#include <cstdint>
#include <string>

#if defined(_WIN32)
// FIXME: GLESWRAP_APIENTRY is needed on windows?
#define GLESWRAP_APIENTRY __stdcall
#else
#define GLESWRAP_APIENTRY
#endif

#ifdef _WIN64
typedef signed long long int ssize_t;
#else
typedef signed long int ssize_t;
#endif

#define IN_GLES_WRAP_H
#ifdef GLESWRAP_GLES2
#include "gleswrap_gles2.h"
#endif
#ifdef GLESWRAP_GLES3
#include "gleswrap_gles3.h"
#endif
#ifdef GLESWRAP_GLES31
#include "gleswrap_gles31.h"
#endif
#undef IN_GLES_WRAP_H

#endif // GLESWRAP_H
