// Liquid.cpp is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Glararan <glararan@glararan.eu>
// Stephan Biegel <project.modcraft@googlemail.com>
// Tigurius <bstigurius@googlemail.com>

#include <noggit/Liquid.h>

#include <algorithm>
#include <string>

#include <noggit/blp_texture.h>
#include <noggit/DBC.h>
#include <noggit/Log.h>
#include <noggit/Shaders.h>
#include <noggit/TextureManager.h> // TextureManager, Texture
#include <noggit/World.h>
#include <noggit/mpq/file.h>

#include <opengl/call_list.h>
#include <opengl/texture.h>

struct LiquidVertex {
  unsigned char c[4];
  float h;
};

void Liquid::initFromTerrain(noggit::mpq::file* f, int flags)
{
  texRepeats = 4.0f;
  /*
  flags:
  8 - ocean
  4 - river
  16 - magma
  */
  ydir = 1.0f;
  if (flags & 16) {
    // magma:
    //initTextures<1,30>( "XTEXTURES\\LAVA\\lava.%d.blp" );
    initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");
    type = 0; // not colored
    pType = 2;
    mTransparency = false;
  }
  else if (flags & 4) {
    // river/lake
    //initTextures<1,30>( "XTEXTURES\\river\\lake_a.%d.blp" )
      initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");;
    type = 2; // dynamic color
    pType = 1;
    mTransparency = true;
  }
  else {
    // ocean
    //initTextures<1,30>( "XTEXTURES\\ocean\\ocean_h.%d.blp" );
    initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");
    type = 2;
    pType = 0;
    mTransparency = true;
  }
  initGeometry(f);
  trans = false;
}

void Liquid::initFromWMO(noggit::mpq::file* f, const WMOMaterial &mat, bool indoor)
{
  texRepeats = 4.0f;
  ydir = -1.0f;

  initGeometry(f);

  trans = false;

  // tmpflag is the flags value for the last drawn tile
  if (tmpflag & 1) {
    //initTextures<1,30>( "XTEXTURES\\SLIME\\slime.%d.blp" );
    initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");
    type = 0;
    texRepeats = 2.0f;
    mTransparency = false;
  }
  else if (tmpflag & 2) {
    //initTextures<1,30>( "XTEXTURES\\LAVA\\lava.%d.blp" );
    initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");
    type = 0;
    mTransparency = false;
  }
  else {
    //initTextures<1,30>( "XTEXTURES\\river\\lake_a.%d.blp" );
    initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");
    if (indoor) {
      trans = true;
      type = 1;
      col = ::math::vector_3d( ( ( mat.col2 & 0xFF0000 ) >> 16 ) / 255.0f, ( ( mat.col2 & 0xFF00 ) >> 8 ) / 255.0f, ( mat.col2 & 0xFF ) / 255.0f);
    } else {
      trans = true;
      type = 2; // outdoor water (...?)
    }
    mTransparency = true;
  }

}


void Liquid::initGeometry(noggit::mpq::file* f)
{
  // assume: f is at the appropriate starting position

  LiquidVertex *map = reinterpret_cast<LiquidVertex*>(f->getPointer());
  unsigned char *flags = reinterpret_cast<unsigned char*>(f->getPointer() + (xtiles+1)*(ytiles+1)*sizeof(LiquidVertex));

  //waterFlags=new unsigned char[(xtiles+1)*(ytiles+1)];
  //memcpy(waterFlags,flags,(xtiles+1)*(ytiles+1));

  // generate vertices
  ::math::vector_3d * lVertices = new ::math::vector_3d[(xtiles+1)*(ytiles+1)];
  //color = new unsigned char[(xtiles+1)*(ytiles+1)];
  for (int j=0; j<ytiles+1; j++) {
    for (int i=0; i<xtiles+1; ++i) {
      size_t p = j*(xtiles+1)+i;
      float h = map[p].h;
      if (h > 100000) h = pos.y();
            lVertices[p] = ::math::vector_3d(pos.x() + tilesize * i, h, pos.z() + ydir * tilesize * j);
      //color[p]= map[p].c[0];
//! \todo  if map[p].c[1] != 0, overwrite the type from the flags.
//      gLog( "%i, {%i, %i, %i, %i}: %s\n", flags[p], map[p].c[0], map[p].c[1], map[p].c[2], map[p].c[3], gLiquidTypeDB.getByID( map[p].c[1] != 0 ? map[p].c[1] : pType ).getString( LiquidTypeDB::Name ) );
    }
  }

  mDrawList = new opengl::call_list;
  mDrawList->start_recording();

  //! \todo  handle light/dark liquid colors
  glNormal3f(0, 1, 0);
  glBegin(GL_QUADS);
  // draw tiles
  for (int j=0; j<ytiles; j++) {
    for (int i=0; i<xtiles; ++i) {
      unsigned char flag = flags[j*xtiles+i];
      if ( !( flag & 8 ) )
      {
        tmpflag = flag;
        // 15 seems to be "don't draw"
        size_t p = j*(xtiles+1)+i;

        float c;

#ifdef USEBLSFILES
        c=type==2?static_cast<float>(map[p].c[0])/255.0f:1.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f(i / texRepeats, j / texRepeats);
        glVertex3fv(lVertices[p]);

        c=type==2?static_cast<float>(map[p+1].c[0])/255.0f:1.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f((i+1) / texRepeats, j / texRepeats);
        glVertex3fv(lVertices[p+1]);

        c=type==2?static_cast<float>(map[p+xtiles+1+1].c[0])/255.0f:1.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f((i+1) / texRepeats, (j+1) / texRepeats);
        glVertex3fv(lVertices[p+xtiles+1+1]);

        c=type==2?static_cast<float>(map[p+xtiles+1].c[0])/255.0f:1.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f(i / texRepeats, (j+1) / texRepeats);
        glVertex3fv(lVertices[p+xtiles+1]);
#else
        c=static_cast<float>(map[p].c[0])/255.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f(i / texRepeats, j / texRepeats);
        glVertex3fv(lVertices[p]);

        c=static_cast<float>(map[p+1].c[0])/255.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f((i+1) / texRepeats, j / texRepeats);
        glVertex3fv(lVertices[p+1]);

        c=static_cast<float>(map[p+xtiles+1+1].c[0])/255.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f((i+1) / texRepeats, (j+1) / texRepeats);
        glVertex3fv(lVertices[p+xtiles+1+1]);

        c=static_cast<float>(map[p+xtiles+1].c[0])/255.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f(i / texRepeats, (j+1) / texRepeats);
        glVertex3fv(lVertices[p+xtiles+1]);
#endif

      }
    }
  }
  glEnd();

  /*
  // debug triangles:
  //glColor4f(1,1,1,1);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  glBegin(GL_TRIANGLES);
  for (int j=0; j<ytiles+1; j++) {
    for (int i=0; i<xtiles+1; ++i) {
      size_t p = j*(xtiles+1)+i;
      ::math::vector_3d v = verts[p];
      //short s = *( (short*) (f->getPointer() + p*8) );
      //float f = s / 255.0f;
      //glColor4f(f,(1.0f-f),0,1);
      unsigned char c[4];
      c[0] = 255-map[p].c[3];
      c[1] = 255-map[p].c[2];
      c[2] = 255-map[p].c[1];
      c[3] = map[p].c[0];
      glColor4ubv(c);

      glVertex3fv(v + ::math::vector_3d(-0.5f, 1.0f, 0));
      glVertex3fv(v + ::math::vector_3d(0.5f, 1.0f, 0));
      glVertex3fv(v + ::math::vector_3d(0.0f, 2.0f, 0));
    }
  }
  glEnd();
  glColor4f(1,1,1,1);
  glEnable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
  */


  /*
  // temp: draw outlines
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_LINE_LOOP);
  ::math::vector_3d wx = ::math::vector_3d(tilesize*xtiles,0,0);
  ::math::vector_3d wy = ::math::vector_3d(0,0,tilesize*ytiles*ydir);
  glColor4f(1,0,0,1);
  glVertex3fv(pos);
  glColor4f(1,1,1,1);
  glVertex3fv(pos+wx);
  glVertex3fv(pos+wx+wy);
  glVertex3fv(pos+wy);
  glEnd();
  glEnable(GL_TEXTURE_2D);*/

  mDrawList->end_recording();

  delete[] lVertices;
  lVertices = NULL;
}

void Liquid::initFromMH2O( MH2O_Information *info, MH2O_HeightMask *HeightMap, MH2O_Render *render )
{
  texRepeats = 4.0f;
  ydir = 1.0f;

  try
  {
    DBCFile::Record lLiquidTypeRow = gLiquidTypeDB.getByID( info->LiquidType );
    //initTextures<1,30>( lLiquidTypeRow.getString( LiquidTypeDB::TextureFilenames - 1 ) );
        initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");
    mLiquidType = lLiquidTypeRow.getInt( LiquidTypeDB::Type );
    mShaderType = lLiquidTypeRow.getInt( LiquidTypeDB::ShaderType );
        mLiquidType = 0;
    mShaderType = 1;
    //! \todo  Get texRepeats too.
  }
  catch( ... )
  {
    // Fallback, when there is no information.
    //initTextures<1,30>( "XTEXTURES\\river\\lake_a.%d.blp" );
    initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");
    mLiquidType = 0;
    mShaderType = 1;
  }

  mTransparency = mShaderType & 1;

  // generate vertices
  //! \todo  Store them somehow else. Maybe an extensible array[][] over the whole ADT?
  ::math::vector_3d *lVertices = new ::math::vector_3d[info->width * info->height];
  for( int j = 0; j < info->height; j++ )
    for( int i = 0; i < info->width; ++i )
      if( render->mRender[j * info->width + i] )
        lVertices[j * info->width + i] = ::math::vector_3d( pos.x() + tilesize * i, HeightMap->mHeightValues[j][i], pos.z() + ydir * tilesize * j );

  mDrawList = new opengl::call_list;
  mDrawList->start_recording();

  glBegin( GL_QUADS );

  glNormal3f( 0.0f, 1.0f, 0.0f );

  // draw tiles
  for( int j = 0; j < info->height; j++ )
    for( int i = 0; i < info->width; ++i )
      if( render->mRender[j * info->width + i] )
      {
        size_t p = j * info->width + i;
        float c;

        c = static_cast<float>(HeightMap->mTransparency[j][i])/255.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f(i / texRepeats, j / texRepeats);
        glVertex3fv(lVertices[p]);

        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f((i+1) / texRepeats, j / texRepeats);
        glVertex3fv(lVertices[p]+::math::vector_3d(tilesize,0,0));
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f((i+1) / texRepeats, (j+1) / texRepeats);
        glVertex3fv(lVertices[p]+::math::vector_3d(tilesize,0,tilesize));
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f(i / texRepeats, (j+1) / texRepeats);
        glVertex3fv(lVertices[p]+::math::vector_3d(0,0,tilesize));

        /*c = (float)HeightMap->mTransparency[j][i+1]/255.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f((i+1) / texRepeats, j / texRepeats);
        glVertex3fv(lVertices[p+1]);

        c = (float)HeightMap->mTransparency[j+1][i+1]/255.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f((i+1) / texRepeats, (j+1) / texRepeats);
        glVertex3fv(lVertices[p+info->width+1]);

        c = (float)HeightMap->mTransparency[j+1][i]/255.0f;
        glMultiTexCoord2f(GL_TEXTURE1,c,c);
        glTexCoord2f(i / texRepeats, (j+1) / texRepeats);
        glVertex3fv(lVertices[p+info->width]);*/
      }

  glEnd();

  mDrawList->end_recording();

  delete[] lVertices;
  lVertices = NULL;
}

void Liquid::initFromMH2O()
{
  texRepeats = 4.0f;
  ydir = 1.0f;

  try
  {
    DBCFile::Record lLiquidTypeRow = gLiquidTypeDB.getByID( mTileData.mLiquidType );
    initTextures<1,30>( lLiquidTypeRow.getString( LiquidTypeDB::TextureFilenames - 1 ) );
    //initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");

    mLiquidType = lLiquidTypeRow.getInt( LiquidTypeDB::Type );
    mShaderType = lLiquidTypeRow.getInt( LiquidTypeDB::ShaderType );
    //mLiquidType = 0;
    //mShaderType = 1;
    //! \todo  Get texRepeats too.
  }
  catch( ... )
  {
    // Fallback, when there is no information.
    //initTextures<1,30>( "XTEXTURES\\river\\lake_a.%d.blp" );
    initTextures<1,30>("XTextures\\river\\lake_a.%d.blp");
    mLiquidType = 0;
    mShaderType = 1;
  }

  mTransparency = mShaderType & 1;

  // generate vertices
  ::math::vector_3d lVertices[9][9];
  for( int j = 0; j < 9; ++j )
    for( int i = 0; i < 9; ++i )
      lVertices[j][i] = ::math::vector_3d( pos.x() + tilesize * i, mTileData.mHeightmap[j][i], pos.z() + ydir * tilesize * j );

  mDrawList = new opengl::call_list;
  mDrawList->start_recording();

  glBegin( GL_QUADS );

  glNormal3f( 0.0f, 1.0f, 0.0f );

  // draw tiles
  for( int j = 0; j < 8; ++j )
    for( int i = 0; i < 8; ++i )
      if( mTileData.mRender[j][i] )
      {
        float c;
        c = mTileData.mDepth[j][i];// / 255.0f;
        glMultiTexCoord2f( GL_TEXTURE1, c, c );
        glTexCoord2f( i / texRepeats, j / texRepeats);
        glVertex3fv( lVertices[j][i] );

        c = mTileData.mDepth[j][i + 1];// / 255.0f;
        glMultiTexCoord2f( GL_TEXTURE1, c, c );
        glTexCoord2f( ( i + 1 ) / texRepeats, j / texRepeats);
        glVertex3fv( lVertices[j][i + 1] );

        c = mTileData.mDepth[j + 1][i + 1];// / 255.0f;
        glMultiTexCoord2f( GL_TEXTURE1, c, c );
        glTexCoord2f( ( i + 1 ) / texRepeats, ( j + 1 ) / texRepeats);
        glVertex3fv( lVertices[j + 1][i + 1] );

        c = mTileData.mDepth[j + 1][i];// / 255.0f;
        glMultiTexCoord2f( GL_TEXTURE1, c, c );
        glTexCoord2f( i / texRepeats, ( j + 1 ) / texRepeats);
        glVertex3fv( lVertices[j + 1][i] );
      }

  glEnd();

  mDrawList->end_recording();
}

void Liquid::setMH2OData(MH2O_Tile pTileInfo){
  //LogDebug << "Set Data to "<< number << std::endl;
  mTileData = pTileInfo;
  initFromMH2O();
}

MH2O_Tile Liquid::getMH2OData(){
  //LogDebug << "Get Data of "<< number << std::endl;
  return mTileData;
}

int Liquid::getWidth(){
  return xtiles;
}

int Liquid::getHeight(){
  return ytiles;
}

int Liquid::getXOffset(){
  int ret = 8;
  for(int i=0; i < 8; ++i) {
    for(int j=0; j < 8; ++j) {
      if(mTileData.mRender[i][j] && (j < ret)) {
        ret = j;
      }
    }
  }
  return ret;
}

int Liquid::getYOffset() {
  int ret = 8;
  for(int i=0; i < 8; ++i) {
    for(int j=0; j < 8; ++j) {
      if(mTileData.mRender[i][j] && (i < ret)) {
        ret = i;
      }
    }
  }
  return ret;
}

bool Liquid::isNotEmpty() {
  bool ret = false;
  for(int i=0; i < 8; ++i) {
    for(int j=0; j < 8; ++j) {
      ret |= mTileData.mRender[i][j];
    }
  }
  return ret;
}

bool Liquid::isRendered(int i, int j) {
  return mTileData.mRender[i][j];
}

void Liquid::setRender(int i, int j) {
  mTileData.mRender[i][j] = true;
  recalcSize();
}

void Liquid::unsetRender(int i, int j) {
  mTileData.mRender[i][j] = false;
  recalcSize();
}

void Liquid::recalcSize() {
  int h = 0;
  int w = 0;
  for(int i=0; i < 8; ++i) {
    bool hRaised = false;
    int wTmp = 0;
    for(int j=0; j < 8; ++j) {
      if(mTileData.mRender[i][j]){
        hRaised = true;
        ++wTmp;
      }
    }
    w = std::max(w,wTmp);
    if(hRaised)
      ++h;
  }
  xtiles = w;
  ytiles = h;
}

#ifdef USEBLSFILES
  BLSShader * mWaterShader;
  BLSShader * mMagmaShader;
#else
  opengl::shader  waterShader;
  opengl::shader  waterFogShader;
#endif

void loadWaterShader()
{
#ifdef USEBLSFILES
  mWaterShader = new BLSShader( "shaders\\pixel\\arbfp1\\psLiquidWater.bls" );
  mMagmaShader = new BLSShader( "shaders\\pixel\\arbfp1\\psLiquidMagma.bls" );
#else
  FILE *shader = fopen( "shaders\\water.ps", "r" );
  if( !shader )
  {
    LogError << "Unable to open water shader \"shaders\\water.ps\"." << std::endl;
  }
  else
  {
    char buffer[8192];
    int length=fread(buffer, 1, 8192, shader);
    fclose(shader);
    glGenProgramsARB(1, &waterShader);
    if(waterShader==0)
      LogError << "Failed to get program ID for water shader \"shaders\\water.ps\"." << std::endl;
    else
    {
      GLint errorPos, isNative;

      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, waterShader);
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, length, buffer);
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);

      glGetProgramiv(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);
      if( !(errorPos==-1)&&(isNative==1) )
      {
        int i, j;
        char localbuffer[256];
        LogError << "Water Shader \"shaders\\water.ps\" Fragment program failed to load \nReason:\n";
        LogError << reinterpret_cast<const char*>(glGetString(GL_PROGRAM_ERROR_STRING_ARB)) << std::endl;
        for(i=errorPos, j=0; (i<length)&&(j<128); ++i, j++)
        {
          localbuffer[j]=buffer[i];
        }
        localbuffer[j]=0;
        LogError << "START DUMP :" << std::endl << localbuffer << "END DUMP" << std::endl;
        if(isNative==0)
          LogError << "This fragment program exceeded the limit." << std::endl;
      }
    }
  }

  shader=fopen("shaders\\waterfog.ps", "r");
  if(shader==0)
    LogError << "Unable to open water shader \"shaders/waterfog.ps\"." << std::endl;
  else
  {
    char buffer[8192];
    int length=fread(buffer, 1, 8192, shader);
    fclose(shader);
    glGenProgramsARB(1, &waterFogShader);
    if(waterFogShader==0)
      LogError << "Failed to get program ID for water shader \"shaders/waterfog.ps\"." << std::endl;
    else
    {
      GLint errorPos, isNative;

      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, waterFogShader);
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, length, buffer);
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);

      glGetProgramiv(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);
      if( !(errorPos==-1)&&(isNative==1) )
      {
        int i, j;
        //const GLubyte *stringy;
        char localbuffer[256];
        LogError << "Water Shader \"shaders/waterfog.ps\" Fragment program failed to load \nReason:\n";
        LogError << reinterpret_cast<const char*>(glGetString(GL_PROGRAM_ERROR_STRING_ARB)) << std::endl;
        for(i=errorPos, j=0; (i<length)&&(j<128); ++i, j++)
        {
          localbuffer[j]=buffer[i];
        }
        localbuffer[j]=0;
        LogError << "START DUMP :" << std::endl << localbuffer << "END DUMP" << std::endl;
        if(isNative==0)
          LogError << "This fragment program exceeded the limit." << std::endl;
      }
    }
  }
#endif
}

#ifndef USEBLSFILES
void enableWaterShader()
{
  if(glIsEnabled(GL_FOG)==GL_TRUE)
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, waterFogShader);
  else
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, waterShader);
}
#endif

void CheckForGLError( const std::string& pLocation )
{
  int ErrorNum = glGetError();
  while( ErrorNum )
  {
    switch( ErrorNum )
    {
      case GL_INVALID_ENUM:
         LogError << "OpenGL: (at " << pLocation << "): GL_INVALID_ENUM" << std::endl;
         break;
      case GL_INVALID_VALUE:
         LogError << "OpenGL: (at " << pLocation << "): GL_INVALID_VALUE" << std::endl;
         break;
       case GL_INVALID_OPERATION:
         LogError << "OpenGL: (at " << pLocation << "): GL_INVALID_OPERATION" << std::endl;
         break;
       case GL_STACK_OVERFLOW:
         LogError << "OpenGL: (at " << pLocation << "): GL_STACK_OVERFLOW" << std::endl;
         break;
       case GL_STACK_UNDERFLOW:
         LogError << "OpenGL: (at " << pLocation << "): GL_STACK_UNDERFLOW" << std::endl;
         break;
       case GL_OUT_OF_MEMORY:
         LogError << "OpenGL: (at " << pLocation << "): GL_OUT_OF_MEMORY" << std::endl;
         break;
       case GL_TABLE_TOO_LARGE:
         LogError << "OpenGL: (at " << pLocation << "): GL_TABLE_TOO_LARGE" << std::endl;
         break;
       case GL_NO_ERROR:
      //! \todo  Add the missing ones.
       default:
         LogError << "OpenGL: (at " << pLocation << "): GL_NO_ERROR (wat?)" << std::endl;
     }

    ErrorNum = glGetError();
  }
}

void Liquid::draw (const Skies* skies) const
{
  glEnable(GL_FRAGMENT_PROGRAM_ARB);

#ifdef USEBLSFILES
  if( type == 2 && mWaterShader->IsOkay() )
    mWaterShader->EnableShader();
  if( type == 0 && mMagmaShader->IsOkay() )
    mMagmaShader->EnableShader();
#else
  enableWaterShader();
#endif

  ::math::vector_3d col2;
  glDisable(GL_CULL_FACE);
  glDepthFunc(GL_LESS);
  size_t texidx = (size_t)(clock() / CLOCKS_PER_SEC / 60.0f) % _textures.size();

  //glActiveTexture(GL_TEXTURE0);
  //glDisable(GL_TEXTURE_2D);
  //glBindTexture(GL_TEXTURE_2D, textures[texidx]);

  const float tcol = mTransparency ? 0.75f : 1.0f;

  if( mTransparency )
  {
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
  }

  if (type==0)
    glColor4f(0.0f,0.0f,0.0f,0.8f);
  else
  {
    ::math::vector_3d color (col);
    if (type==2)
    {
      // dynamic color lookup! ^_^
      //! \todo  add variable water color
      color = skies->colorSet[WATER_COLOR_LIGHT];
      col2 = skies->colorSet[WATER_COLOR_DARK];
    }
    glColor4f(color.x(), color.y(), color.z(), tcol);
    glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB,0,col2.x(),col2.y(),col2.z(),tcol);
#ifdef USEBLSFILES
    glSecondaryColor3f(col2.x(),col2.y(),col2.z());
#endif
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD); //! \todo  check if ARB_texture_env_add is supported? :(
  }

  opengl::texture::enable_texture (0);

  _textures[texidx]->bind();

  opengl::texture::enable_texture (1);

  if( mDrawList )
  {
    //! \todo THIS LINE THROWS GL_INVALID_OPERATION! Steff. It donwt do in anymore now. Perhaps because water rendering was called double in maptile::draw()
    CheckForGLError( "Liquid::draw:: before the draw list" );
    mDrawList->render();
    CheckForGLError( "Liquid::draw:: after the draw list" );
  }

  opengl::texture::disable_texture (1);
  opengl::texture::set_active_texture (0);

  glColor4f(1,1,1,0.4f);
  if( mTransparency )
  {
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
  }
  glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

template<int pFirst, int pLast>
void Liquid::initTextures( const std::string& pFilename )
{
  for( int i = pFirst; i <= pLast; ++i )
  {
    QString tmp (QString::fromStdString(pFilename));
    tmp = tmp.replace("%d", "%1");
    tmp = tmp.arg (i);
    std::string doo (tmp.toStdString());
    _textureFilenames.push_back( doo );
    _textures.push_back( TextureManager::newTexture( doo )) ;
  }
}


Liquid::~Liquid()
{
  delete mDrawList;
  mDrawList = NULL;

  for( std::vector<std::string>::iterator filename = _textureFilenames.begin(); filename != _textureFilenames.end(); ++filename )
  {
    TextureManager::delbyname( *filename );
  }
}
