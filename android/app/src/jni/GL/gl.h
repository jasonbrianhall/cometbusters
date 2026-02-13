#ifndef __GL_H__
#define __GL_H__
#include <GLES2/gl2.h>
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#define GL_PROJECTION 0x1701
#define GL_MODELVIEW 0x1700
inline void glMatrixMode(GLenum mode) { }
inline void glLoadIdentity(void) { }
inline void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar) { }
#endif
