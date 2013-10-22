// Shaders.h is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Stephan Biegel <project.modcraft@googlemail.com>

#ifndef __SHADERS_H
#define __SHADERS_H

#include <string>
#include <QString>
#include <opengl/types.h>

#ifdef USEBLSSHADER
class BLSShader
{
private:
  opengl::shader mShader;
  bool mOkay;
  int mProgramType;

public:
  explicit BLSShader( const QString & pFilename );

  void EnableShader()
  {
    if( IsOkay() )
      glBindProgramARB( mProgramType, mShader );
  }

  bool IsOkay() { return mOkay; }
};
#endif

//Shaders WoWMapView 3.x

void initShaders();
void reloadShaders();

class Shader {
  opengl::shader id;
  GLenum target;
public:
  bool ok;

  Shader(GLenum target, const char *program, bool fromFile = false);
  virtual ~Shader();

  virtual void bind();
  virtual void unbind();
};

class ShaderPair {
  Shader *vertex;
  Shader *fragment;
public:

  ShaderPair():vertex(0),fragment(0) {}
  ShaderPair(Shader *vs, Shader *ps):vertex(vs), fragment(ps) {}
  ShaderPair(const char *vprog, const char *fprog, bool fromFile = false);

  void bind();
  void unbind();
};

extern ShaderPair *terrainShaders[4], *wmoShader, *waterShaders[1];

#endif
