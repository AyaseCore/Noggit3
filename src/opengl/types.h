// types.h is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>

#pragma once

#ifdef __linux__
#include <GL/glew.h>
#else
#include <gl/glew.h>
#endif

namespace opengl
{
  typedef GLuint shader;
  typedef GLuint light;
}
