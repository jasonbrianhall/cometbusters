#ifndef __GLEW_H__
#define __GLEW_H__
#include <GLES2/gl2.h>
#define GLEW_OK 0
inline GLenum glewInit(void) { return GLEW_OK; }
extern int glewExperimental;
#define GLEW_ARB_vertex_array_object 1
#define GLEW_ARB_framebuffer_object 1
#define GLEW_ARB_texture_float 1
#endif
