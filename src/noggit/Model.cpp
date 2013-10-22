// Model.cpp is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Stephan Biegel <project.modcraft@googlemail.com>
// Tigurius <bstigurius@googlemail.com>

#include <algorithm>
#include <cassert>
#include <map>
#include <sstream>
#include <string>

#include <math/vector_2d.h>

#include <noggit/blp_texture.h>
#include <noggit/Log.h>
#include <noggit/Model.h>
#include <noggit/TextureManager.h> // TextureManager, Texture
#include <noggit/World.h>

Model::Model(const std::string& filename, bool _forceAnim)
: ManagedItem( )
, forceAnim(_forceAnim)
, _filename( filename )
{
  memset( &header, 0, sizeof( ModelHeader ) );

  globalSequences = NULL;
  indices = NULL;
  anims = NULL;
  origVertices = NULL;
  bones = NULL;
  texanims = NULL;
  colors = NULL;
  transparency = NULL;
  lights = NULL;
  particleSystems = NULL;
  ribbons = NULL;

  showGeosets = NULL;

  _finished = false;
}

void Model::finish_loading()
{
  noggit::mpq::file f (QString::fromStdString (_filename));

  LogDebug << "Loading model \"" << _filename << "\"." << std::endl;

  memcpy( &header, f.getBuffer(), sizeof( ModelHeader ) );

  animated = isAnimated( f ) || forceAnim;  // isAnimated will set animGeometry and animTextures

  trans = 1.0f;

  vbuf = nbuf = tbuf = 0;

  globalSequences = 0;
  anim = 0;
  colors = 0;
  lights = 0;
  transparency = 0;
  particleSystems = 0;
  header.nParticleEmitters = 0;      //! \todo  Get Particles to 3.*? ._.
  header.nRibbonEmitters = 0;      //! \todo  Get Particles to 3.*? ._.
  ribbons = 0;
  if (header.nGlobalSequences)
  {
    globalSequences = new int[header.nGlobalSequences];
    memcpy(globalSequences, (f.getBuffer() + header.ofsGlobalSequences), header.nGlobalSequences * 4);
  }

  //! \todo  This takes a biiiiiit long. Have a look at this.
  if( animated )
    initAnimated( f );
  else
    initStatic( f );

  f.close();

  _finished = true;
}

Model::~Model()
{
  LogDebug << "Unloading model \"" << _filename << "\"." << std::endl;

  for(std::vector<std::string>::iterator it = _textureFilenames.begin(); it != _textureFilenames.end(); ++it )
  {
    TextureManager::delbyname( *it );
  }
  _textures.clear();
  _textureFilenames.clear();

  if( globalSequences )
    delete[] globalSequences;

  if(showGeosets)
    delete[] showGeosets;

  if (animated) {
    // unload all sorts of crap
    //delete[] vertices;
    //delete[] normals;
    if (colors)
      delete[] colors;
    if (transparency)
      delete[] transparency;
    if(indices)
      delete[] indices;
    if(anims)
      delete[] anims;
    if(origVertices)
      delete[] origVertices;
    if(bones)
      delete[] bones;
    if (!animGeometry) {
      glDeleteBuffers(1, &nbuf);
    }
    glDeleteBuffers(1, &vbuf);
    glDeleteBuffers(1, &tbuf);

    if (animTextures && texanims)
      delete[] texanims;
    if (lights)
      delete[] lights;

    if (particleSystems)
      delete[] particleSystems;
    if (ribbons)
      delete[] ribbons;
  } else {
    glDeleteLists(ModelDrawList, 1);
  }
}


bool Model::isAnimated(const noggit::mpq::file& f)
{
  // see if we have any animated bones
  ModelBoneDef *bo = reinterpret_cast<ModelBoneDef*>(f.getBuffer() + header.ofsBones);

  animGeometry = false;
  animBones = false;
  mPerInstanceAnimation = false;

  ModelVertex *verts = reinterpret_cast<ModelVertex*>(f.getBuffer() + header.ofsVertices);
  for (size_t i=0; i<header.nVertices && !animGeometry; ++i) {
    for (size_t b=0; b<4; b++) {
      if (verts[i].weights[b]>0) {
        ModelBoneDef& bb = bo[verts[i].bones[b]];
        if (bb.translation.type || bb.rotation.type || bb.scaling.type || (bb.flags & 8)) {
          if (bb.flags & 8) {
            // if we have billboarding, the model will need per-instance animation
            mPerInstanceAnimation = true;
          }
          animGeometry = true;
          break;
        }
      }
    }
  }

  if (animGeometry) animBones = true;
  else {
    for (size_t i=0; i<header.nBones; ++i) {
      ModelBoneDef &bb = bo[i];
      if (bb.translation.type || bb.rotation.type || bb.scaling.type) {
        animBones = true;
        break;
      }
    }
  }

  animTextures = header.nTexAnims > 0;

  bool animMisc = header.nCameras>0 || // why waste time, pretty much all models with cameras need animation
          header.nLights>0 || // same here
          header.nParticleEmitters>0 ||
          header.nRibbonEmitters>0;

  if (animMisc) animBones = true;

  // animated colors
  if (header.nColors) {
    ModelColorDef *cols = reinterpret_cast<ModelColorDef*>(f.getBuffer() + header.ofsColors);
    for (size_t i=0; i<header.nColors; ++i) {
      if (cols[i].color.type!=0 || cols[i].opacity.type!=0) {
        animMisc = true;
        break;
      }
    }
  }

  // animated opacity
  if (header.nTransparency && !animMisc) {
    ModelTransDef *trs = reinterpret_cast<ModelTransDef*>(f.getBuffer() + header.ofsTransparency);
    for (size_t i=0; i<header.nTransparency; ++i) {
      if (trs[i].trans.type!=0) {
        animMisc = true;
        break;
      }
    }
  }

  // guess not...
  return animGeometry || animTextures || animMisc;
}


::math::vector_3d fixCoordSystem (const ::math::vector_3d& v)
{
  return ::math::vector_3d (v.x(), v.z(), -v.y());
}

::math::vector_3d fixCoordSystem2 (const ::math::vector_3d& v)
{
  return ::math::vector_3d (v.x(), v.z(), v.y());
}

::math::quaternion fixCoordSystemQuat (const ::math::quaternion& v)
{
  return ::math::quaternion (-v.x(), -v.z(), v.y(), v.w());
}


void Model::initCommon(const noggit::mpq::file& f)
{
  // assume: origVertices already set
  if (!animGeometry) {
    vertices = new ::math::vector_3d[header.nVertices];
    normals = new ::math::vector_3d[header.nVertices];
  }

  // vertices, normals
  for (size_t i=0; i<header.nVertices; ++i) {
    origVertices[i].pos = fixCoordSystem(origVertices[i].pos);
    origVertices[i].normal = fixCoordSystem(origVertices[i].normal);

    if (!animGeometry) {
      vertices[i] = origVertices[i].pos;
      normals[i] = origVertices[i].normal.normalize();
    }

    float len = origVertices[i].pos.length_squared();
    if (len > rad){
      rad = len;
    }
  }
  rad = sqrtf(rad);

  // textures
  ModelTextureDef* texdef = reinterpret_cast<ModelTextureDef*>( f.getBuffer() + header.ofsTextures );
  _textures.resize( header.nTextures );
  _replaceTextures.resize( header.nTextures );
  _textureFilenames.resize( header.nTextures );
  _specialTextures.resize( header.nTextures );
  //! \todo Find correct number here.
  _useReplaceTextures.resize( 15 );

  for( size_t i = 0; i < header.nTextures; ++i )
  {
    if( texdef[i].type == 0 )
    {
      _specialTextures[i] = -1;
      _useReplaceTextures[i] = false;
      _textureFilenames[i] = std::string( f.getBuffer() + texdef[i].nameOfs, texdef[i].nameLen );
      _textures[i] = TextureManager::newTexture( _textureFilenames[i] );
    }
    else
    {
      //! \note special texture - only on characters and such... Noggit should not even render these.
      //! \todo Check if this is actually correct. Or just remove it.
      _textures[i] = NULL;
      //! \note As no other replace textures are loaded, do not replace anything but armor reflection to prevent crashes.
      _specialTextures[i] = texdef[i].type == 3 ? 3 : -2;

      _useReplaceTextures[texdef[i].type] = true;

      if (texdef[i].type == 3)
      {
        _textureFilenames[i] = "Item\\ObjectComponents\\Weapon\\ArmorReflect4.BLP";
        // a fix for weapons with type-3 textures.
        _replaceTextures[texdef[i].type] = TextureManager::newTexture( _textureFilenames[i] );
      }
    }
  }

  // init colors
  if (header.nColors) {
    colors = new ModelColor[header.nColors];
    ModelColorDef *colorDefs = reinterpret_cast<ModelColorDef*>(f.getBuffer() + header.ofsColors);
    for (size_t i=0; i<header.nColors; ++i) colors[i].init(f, colorDefs[i], globalSequences);
  }
  // init transparency
  int16_t *transLookup = reinterpret_cast<int16_t*>(f.getBuffer() + header.ofsTransparencyLookup);
  if (header.nTransparency) {
    transparency = new ModelTransparency[header.nTransparency];
    ModelTransDef *trDefs = reinterpret_cast<ModelTransDef*>(f.getBuffer() + header.ofsTransparency);
    for (size_t i=0; i<header.nTransparency; ++i) transparency[i].init(f, trDefs[i], globalSequences);
  }


  // just use the first LOD/view

  if (header.nViews > 0) {
    // indices - allocate space, too
    std::string lodname (_filename.substr (0, _filename.length() - 3));
    lodname.append("00.skin");
    noggit::mpq::file g (QString::fromStdString (lodname));

    ModelViewStruct *view = reinterpret_cast<ModelViewStruct*>(g.getBuffer());

    uint16_t *indexLookup = reinterpret_cast<uint16_t*>(g.getBuffer() + view->ofsIndex);
    uint16_t *triangles = reinterpret_cast<uint16_t*>(g.getBuffer() + view->ofsTris);
    nIndices = view->nTris;
    indices = new uint16_t[nIndices];
    for (size_t i = 0; i<nIndices; ++i) {
      indices[i] = indexLookup[triangles[i]];
    }

    // render ops
    ModelGeoset *ops = reinterpret_cast<ModelGeoset*>(g.getBuffer() + view->ofsSub);
    ModelTexUnit *tex = reinterpret_cast<ModelTexUnit*>(g.getBuffer() + view->ofsTex);
    ModelRenderFlags *renderFlags = reinterpret_cast<ModelRenderFlags*>(f.getBuffer() + header.ofsTexFlags);
    uint16_t *texlookup = reinterpret_cast<uint16_t*>(f.getBuffer() + header.ofsTexLookup);
    uint16_t *texanimlookup = reinterpret_cast<uint16_t*>(f.getBuffer() + header.ofsTexAnimLookup);
    int16_t *texunitlookup = reinterpret_cast<int16_t*>(f.getBuffer() + header.ofsTexUnitLookup);

    showGeosets = new bool[view->nSub];
    for (size_t i=0; i<view->nSub; ++i) {
      showGeosets[i] = true;
    }

    for (size_t j = 0; j<view->nTex; j++) {
      ModelRenderPass pass;

      pass.usetex2 = false;
      pass.useenvmap = false;
      pass.cull = false;
      pass.trans = false;
      pass.unlit = false;
      pass.nozwrite = false;
      pass.billboard = false;

      size_t geoset = tex[j].op;

      pass.geoset = geoset;

      pass.indexStart = ops[geoset].istart;
      pass.indexCount = ops[geoset].icount;
      pass.vertexStart = ops[geoset].vstart;
      pass.vertexEnd = pass.vertexStart + ops[geoset].vcount;

      pass.order = tex[j].shading;

      //TextureID texid = textures[texlookup[tex[j].textureid]];
      //pass.texture = texid;
      pass.tex = texlookup[tex[j].textureid];

      // TODO: figure out these flags properly -_-
      ModelRenderFlags &rf = renderFlags[tex[j].flagsIndex];


      pass.blendmode = rf.blend;
      pass.color = tex[j].colorIndex;
      pass.opacity = transLookup[tex[j].transid];

      enum RenderFlags
      {
        RENDERFLAGS_UNLIT = 1,
        RENDERFLAGS_UNsGED = 2,
        RENDERFLAGS_TWOSIDED = 4,
        RENDERFLAGS_BILLBOARD = 8,
        RENDERFLAGS_ZBUFFERED = 16,
      };

      pass.unlit = (rf.flags & RENDERFLAGS_UNLIT)!=0;
      pass.cull = (rf.flags & RENDERFLAGS_TWOSIDED)==0 && rf.blend==0;

      pass.billboard = (rf.flags & RENDERFLAGS_BILLBOARD) != 0;

      pass.useenvmap = (texunitlookup[tex[j].texunit] == -1) && pass.billboard && rf.blend>2;
      pass.nozwrite = (rf.flags & RENDERFLAGS_ZBUFFERED) != 0;

      // ToDo: Work out the correct way to get the true/false of transparency
      pass.trans = (pass.blendmode>0) && (pass.opacity>0);  // Transparency - not the correct way to get transparency

      pass.p = ops[geoset].BoundingBox[0].x();

      // Texture flags
      enum TextureFlags {
        TEXTURE_WRAPX=1,
        TEXTURE_WRAPY
      };

      pass.swrap = (texdef[pass.tex].flags & TEXTURE_WRAPX) != 0; // Texture wrap X
      pass.twrap = (texdef[pass.tex].flags & TEXTURE_WRAPY) != 0; // Texture wrap Y

      static const int TEXTUREUNIT_STATIC = 16;

      if (animTextures) {
        if (tex[j].flags & TEXTUREUNIT_STATIC) {
          pass.texanim = -1; // no texture animation
        } else {
          pass.texanim = texanimlookup[tex[j].texanimid];
        }
      } else {
        pass.texanim = -1; // no texture animation
      }

      passes.push_back(pass);
    }
    g.close();
    // transparent parts come later
    std::sort(passes.begin(), passes.end());
  }

  // zomg done
}

void Model::initStatic( const noggit::mpq::file& f )
{
  origVertices = ( ModelVertex* )( f.getBuffer() + header.ofsVertices );

  initCommon( f );

  ModelDrawList = glGenLists( 1 );
  glNewList( ModelDrawList, GL_COMPILE );
    drawModel( 0 );
  glEndList();

  SelectModelDrawList = glGenLists( 1 );
  glNewList( SelectModelDrawList, GL_COMPILE );
    drawModelSelect(0);
  glEndList();

  // clean up vertices, indices etc
  if( vertices )
    delete[] vertices;
  if( normals )
    delete[] normals;
  if( indices )
    delete[] indices;
  if( colors )
    delete[] colors;
  if( transparency )
    delete[] transparency;
}

void Model::initAnimated(const noggit::mpq::file& f)
{
  origVertices = new ModelVertex[header.nVertices];
  memcpy(origVertices, f.getBuffer() + header.ofsVertices, header.nVertices * sizeof(ModelVertex));

  glGenBuffersARB(1,&vbuf);
  glGenBuffersARB(1,&tbuf);
  const size_t size = header.nVertices * sizeof(float);
  vbufsize = 3 * size;

  initCommon(f);

  if (header.nAnimations > 0) {
    anims = new ModelAnimation[header.nAnimations];
    memcpy(anims, f.getBuffer() + header.ofsAnimations, header.nAnimations * sizeof(ModelAnimation));
    for( size_t i = 0; i < header.nAnimations; ++i )
    {
      //! \note Fix for world\kalimdor\diremaul\activedoodads\crystalcorrupter\corruptedcrystalshard.m2 having a zero length for its stand animation.
      anims[i].length = std::max( anims[i].length, 1U );
    }

    animfiles = new noggit::mpq::file*[header.nAnimations];

    std::stringstream tempname;
    for(size_t i=0; i<header.nAnimations; ++i)
    {
      std::string lodname = _filename.substr(0, _filename.length()-3);
      tempname << lodname << anims[i].animID << "-" << anims[i].subAnimID;
      const QString anim_filename (QString::fromStdString (tempname.str()));
      if (noggit::mpq::file::exists (anim_filename))
      {
        animfiles[i] = new noggit::mpq::file (anim_filename);
      }
      else
      {
        animfiles[i] = NULL;
      }
    }
  }

  if (animBones) {
    // init bones...
    bones = new Bone[header.nBones];
    ModelBoneDef *mb = reinterpret_cast<ModelBoneDef*>(f.getBuffer() + header.ofsBones);
    for (size_t i=0; i<header.nBones; ++i) {
      bones[i].init(f, mb[i], globalSequences, animfiles);
    }
  }

  if (!animGeometry) {
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbuf);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, vbufsize, vertices, GL_STATIC_DRAW_ARB);
    glGenBuffersARB(1,&nbuf);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, nbuf);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, vbufsize, normals, GL_STATIC_DRAW_ARB);
    delete[] vertices;
    delete[] normals;
  }
  ::math::vector_2d *texcoords = new ::math::vector_2d[header.nVertices];
  for (size_t i=0; i<header.nVertices; ++i)
    texcoords[i] = origVertices[i].texcoords;
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, tbuf);
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, 2*size, texcoords, GL_STATIC_DRAW_ARB);
  delete[] texcoords;

  if (animTextures) {
    texanims = new TextureAnim[header.nTexAnims];
    ModelTexAnimDef *ta = reinterpret_cast<ModelTexAnimDef*>(f.getBuffer() + header.ofsTexAnims);
    for (size_t i=0; i<header.nTexAnims; ++i) {
      texanims[i].init(f, ta[i], globalSequences);
    }
  }

  // particle systems
  if (header.nParticleEmitters) {
    ModelParticleEmitterDef *pdefs = reinterpret_cast<ModelParticleEmitterDef*>(f.getBuffer() + header.ofsParticleEmitters);
    particleSystems = new ParticleSystem[header.nParticleEmitters];
    for (size_t i=0; i<header.nParticleEmitters; ++i) {
      particleSystems[i].model = this;
      particleSystems[i].init(f, pdefs[i], globalSequences);
    }
  }

  // ribbons
  if (header.nRibbonEmitters) {
    ModelRibbonEmitterDef *rdefs = reinterpret_cast<ModelRibbonEmitterDef*>(f.getBuffer() + header.ofsRibbonEmitters);
    ribbons = new RibbonEmitter[header.nRibbonEmitters];
    for (size_t i=0; i<header.nRibbonEmitters; ++i) {
      ribbons[i].model = this;
      ribbons[i].init(f, rdefs[i], globalSequences);
    }
  }

  // just use the first camera, meh
  if (header.nCameras>0) {
    ModelCameraDef *camDefs = reinterpret_cast<ModelCameraDef*>(f.getBuffer() + header.ofsCameras);
    cam.init(f, camDefs[0], globalSequences);
  }

  // init lights
  if (header.nLights) {
    lights = new ModelLight[header.nLights];
    ModelLightDef *lDefs = reinterpret_cast<ModelLightDef*>(f.getBuffer() + header.ofsLights);
    for (size_t i=0; i<header.nLights; ++i)
      lights[i].init(f, lDefs[i], globalSequences);
  }

  animcalc = false;
}

void Model::calcBones(int _anim, int time)
{
  for (size_t i=0; i<header.nBones; ++i) {
    bones[i].calc = false;
  }

  for (size_t i=0; i<header.nBones; ++i) {
    bones[i].calcMatrix(bones, _anim, time);
  }
}

void Model::animate(int _anim, int time)
{
  anim = _anim;
  ModelAnimation &a = anims[anim];
  int tmax = a.length;
  time %= tmax;

  if (animBones) {
    calcBones(anim, time);
  }

  if (animGeometry) {

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbuf);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, 2*vbufsize, NULL, GL_STREAM_DRAW_ARB);
    vertices = reinterpret_cast< ::math::vector_3d*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY));

    // transform vertices
    ModelVertex *ov = origVertices;
    for (size_t i=0; i<header.nVertices; ++i,++ov) {
      ::math::vector_3d v(0,0,0), n(0,0,0);

      for (size_t b=0; b<4; ++b) {
        if (ov->weights[b]>0) {
          ::math::vector_3d tv = bones[ov->bones[b]].mat * ov->pos;
          ::math::vector_3d tn = bones[ov->bones[b]].mrot * ov->normal;
          v += tv * (static_cast<float>(ov->weights[b]) / 255.0f);
          n += tn * (static_cast<float>(ov->weights[b]) / 255.0f);
        }
      }

      /*
       vertices[i] = v;
       normals[i] = n;
       */
      vertices[i] = v;
      vertices[header.nVertices + i] = n.normalize(); // shouldn't these be normal by default?
    }

    glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);

  }

  for (size_t i=0; i<header.nLights; ++i) {
    if (lights[i].parent>=0) {
      lights[i].tpos = bones[lights[i].parent].mat * lights[i].pos;
      lights[i].tdir = bones[lights[i].parent].mrot * lights[i].dir;
    }
  }

  for (size_t i=0; i<header.nParticleEmitters; ++i) {
    // random time distribution for teh win ..?
    int pt = (time + static_cast<int>(tmax*particleSystems[i].tofs)) % tmax;
    particleSystems[i].setup(anim, pt);
  }

  for (size_t i=0; i<header.nRibbonEmitters; ++i) {
    ribbons[i].setup(anim, time);
  }

  if (animTextures) {
    for (size_t i=0; i<header.nTexAnims; ++i) {
      texanims[i].calc(anim, time);
    }
  }
}

bool ModelRenderPass::init(Model *m, int animtime)
{
  // May aswell check that we're going to render the geoset before doing all this crap.
  if (m->showGeosets[geoset]) {

    // COLOUR
    // Get the colour and transparency and check that we should even render
    ocol = ::math::vector_4d(1.0f, 1.0f, 1.0f, m->trans);
    ecol = ::math::vector_4d(0.0f, 0.0f, 0.0f, 0.0f);

    //if (m->trans == 1.0f)
    //  return false;

    // emissive colors
    if (color!=-1 && m->colors[color].color.uses(0))
    {
      ::math::vector_3d c (m->colors[color].color.getValue (0, animtime));
      if (m->colors[color].opacity.uses (m->anim))
      {
        ocol.w (m->colors[color].opacity.getValue (m->anim, animtime));
      }

      if (unlit)
      {
        ocol.x (c.x());
        ocol.y (c.y());
        ocol.z (c.z());
      } else {
        ocol.x (ocol.y (ocol.z (0.0f)));
      }

      ecol = ::math::vector_4d (c, ocol.w());
      glMaterialfv (GL_FRONT, GL_EMISSION, ecol);
    }

    // opacity
    if (opacity!=-1)
    {
      if (m->transparency[opacity].trans.uses (0))
      {
        ocol.w ( ocol.w()
               * m->transparency[opacity].trans.getValue (0, animtime)
               );
      }
    }

    // exit and return false before affecting the opengl render state
    if (!((ocol.w() > 0.0f) && (color==-1 || ecol.w() > 0.0f)))
      return false;

    // TEXTURE
    if ( m->_specialTextures[tex] == -1 )
      m->_textures[tex]->bind();
    //! \todo this breaks, if there are no replace textures. d'uh.
    else if ( m->_specialTextures[tex] > 0 )
      m->_replaceTextures[m->_specialTextures[tex]]->bind();

    // TODO: Add proper support for multi-texturing.

    // blend mode
    switch (blendmode) {
      case BM_OPAQUE:  // 0
        break;
      case BM_TRANSPARENT: // 1
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GEQUAL,0.7f);
        break;
      case BM_ALPHA_BLEND: // 2
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
      case BM_ADDITIVE: // 3
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        break;
      case BM_ADDITIVE_ALPHA: // 4
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
      case BM_MODULATE: // 5
        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
        break;
      case BM_MODULATE2: // 6
        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR,GL_SRC_COLOR);
        break;
      default:
        LogError << "Unknown blendmode: " << blendmode << std::endl;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    //if (cull)
    //  glEnable(GL_CULL_FACE);

    // Texture wrapping around the geometry
    if (swrap)
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    if (twrap)
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    // no writing to the depth buffer.
    if (nozwrite)
      glDepthMask(GL_FALSE);

    if (unlit) {
      glDisable(GL_LIGHTING);
    }

    // Environmental mapping, material, and effects
    if (useenvmap) {
      // Turn on the 'reflection' shine, using 18.0f as that is what WoW uses based on the reverse engineering
      // This is now set in InitGL(); - no need to call it every render.
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 18.0f);

      // env mapping
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);

      const GLint maptype = GL_SPHERE_MAP;
      //const GLint maptype = GL_REFLECTION_MAP_ARB;

      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, maptype);
      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, maptype);
    }

    if (texanim!=-1) {
      glMatrixMode(GL_TEXTURE);
      glPushMatrix();

      m->texanims[texanim].setup(texanim);
    }

    // color
    glColor4fv(ocol);
    //glMaterialfv(GL_FRONT, GL_SPECULAR, ocol);

    // don't use lighting on the surface
    if (unlit)
      glDisable(GL_LIGHTING);

    if (blendmode<=1 && ocol.w()<1.0f)
      glEnable(GL_BLEND);

    return true;
  }

  return false;
}

void ModelRenderPass::deinit()
{
  switch (blendmode) {
    case BM_OPAQUE:
      break;
    case BM_TRANSPARENT:
      break;
    case BM_ALPHA_BLEND:
      //glDepthMask(GL_TRUE);
      break;
    default:
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // default blend func
  }
  if (nozwrite) {
    glDepthMask(GL_TRUE);
  }
  if (texanim!=-1) {
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
  }
  if (unlit) {
    glEnable(GL_LIGHTING);
  }
  if (useenvmap) {
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
  }
  if (usetex2) {
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0);
  }
  //glColor4f(1,1,1,1); //???
}

void ModelHighlight( ::math::vector_4d color )
{
  glDisable( GL_ALPHA_TEST );
   glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glDisable( GL_CULL_FACE );
  glActiveTexture( GL_TEXTURE0 );
  glDisable( GL_TEXTURE_2D);
  glActiveTexture( GL_TEXTURE1 );
  glDisable( GL_TEXTURE_2D );
  glColor4fv( color );
  glMaterialfv( GL_FRONT, GL_EMISSION, color );
//  glDepthMask( GL_FALSE );
}

void ModelUnhighlight()
{
  glEnable( GL_ALPHA_TEST );
   glDisable( GL_BLEND );
  glEnable( GL_CULL_FACE );
  glActiveTexture( GL_TEXTURE0 );
  glEnable( GL_TEXTURE_2D );
  glColor4fv( ::math::vector_4d( 1, 1, 1, 1 ) );
//  glDepthMask( GL_TRUE );
}

void Model::drawModel(int animtime)
{
  // assume these client states are enabled: GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY

  if( animated )
  {
    if( animGeometry )
    {
      glBindBuffer( GL_ARRAY_BUFFER, vbuf );
      glVertexPointer( 3, GL_FLOAT, 0, 0 );
      glNormalPointer( GL_FLOAT, 0, reinterpret_cast<void*>(vbufsize) );
    }
    else
    {
      glBindBuffer( GL_ARRAY_BUFFER, vbuf );
      glVertexPointer( 3, GL_FLOAT, 0, 0 );
      glBindBuffer( GL_ARRAY_BUFFER, nbuf );
      glNormalPointer( GL_FLOAT, 0, 0 );
    }

    glBindBuffer( GL_ARRAY_BUFFER, tbuf );
    glTexCoordPointer( 2, GL_FLOAT, 0, 0 );
  }

  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glAlphaFunc( GL_GREATER, 0.3f );

  for (size_t i=0; i<passes.size(); ++i) {
    ModelRenderPass &p = passes[i];

    if (p.init(this, animtime)) {
      // we don't want to render completely transparent parts

      // render
      if (animated) {
        //glDrawElements(GL_TRIANGLES, p.indexCount, GL_UNSIGNED_SHORT, indices + p.indexStart);
        // a GDC OpenGL Performace Tuning paper recommended glDrawRangeElements over glDrawElements
        // I can't notice a difference but I guess it can't hurt
          glDrawRangeElements(GL_TRIANGLES, p.vertexStart, p.vertexEnd, p.indexCount, GL_UNSIGNED_SHORT, indices + p.indexStart);

      } else {
        glBegin(GL_TRIANGLES);
        for (size_t k = 0, b=p.indexStart; k<p.indexCount; ++k,++b) {
          uint16_t a = indices[b];
          glNormal3fv(normals[a]);
          glTexCoord2fv(origVertices[a].texcoords);
          glVertex3fv(vertices[a]);
        }
        glEnd();
      }

      p.deinit();
    }

  }
  // done with all render ops

  glAlphaFunc (GL_GREATER, 0.0f);
  glDisable (GL_ALPHA_TEST);

  GLfloat czero[4] = {0,0,0,1};
  glMaterialfv(GL_FRONT, GL_EMISSION, czero);
  glColor4f(1,1,1,1);
  glDepthMask(GL_TRUE);
}

void Model::drawModelSelect (int animtime)
{
  // assume these client states are enabled: GL_VERTEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY

  if( animated )
  {
    if( animGeometry )
    {
      glBindBuffer( GL_ARRAY_BUFFER, vbuf );
      glVertexPointer( 3, GL_FLOAT, 0, 0 );
      glNormalPointer( GL_FLOAT, 0, reinterpret_cast<void*>(vbufsize) );
    }
    else
    {
      glBindBuffer( GL_ARRAY_BUFFER, vbuf );
      glVertexPointer( 3, GL_FLOAT, 0, 0 );
      glBindBuffer( GL_ARRAY_BUFFER, nbuf );
      glNormalPointer( GL_FLOAT, 0, 0 );
    }

    glBindBuffer( GL_ARRAY_BUFFER, tbuf );
    glTexCoordPointer( 2, GL_FLOAT, 0, 0 );
  }

  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glAlphaFunc( GL_GREATER, 0.3f );

  for (size_t i=0; i<passes.size(); ++i) {
    ModelRenderPass &p = passes[i];

    if (p.init(this, animtime)) {
      // we don't want to render completely transparent parts

      // render
      if (animated) {
        //glDrawElements(GL_TRIANGLES, p.indexCount, GL_UNSIGNED_SHORT, indices + p.indexStart);
        // a GDC OpenGL Performace Tuning paper recommended glDrawRangeElements over glDrawElements
        // I can't notice a difference but I guess it can't hurt
        glDrawRangeElements(GL_TRIANGLES, p.vertexStart, p.vertexEnd, p.indexCount, GL_UNSIGNED_SHORT, indices + p.indexStart);

      }
    else
    {
        glBegin(GL_TRIANGLES);
    for (size_t k = 0, b=p.indexStart; k<p.indexCount; ++k,++b)
    {
      uint16_t a = indices[b];
      glNormal3fv(normals[a]);
      glTexCoord2fv(origVertices[a].texcoords);
      glVertex3fv(vertices[a]);
    }
        glEnd();
      }

      p.deinit();
    }

  }
  // done with all render ops

  glAlphaFunc (GL_GREATER, 0.0f);
  glDisable (GL_ALPHA_TEST);

  GLfloat czero[4] = {0,0,0,1};
  glMaterialfv(GL_FRONT, GL_EMISSION, czero);
  glColor4f(1,1,1,1);
  glDepthMask(GL_TRUE);

}

void TextureAnim::calc(int anim, int time)
{
  if (trans.uses(anim)) {
    tval = trans.getValue(anim, time);
  }
  if (rot.uses(anim)) {
        rval = rot.getValue(anim, time);
  }
  if (scale.uses(anim)) {
        sval = scale.getValue(anim, time);
  }
}

void TextureAnim::setup(int anim)
{
  glLoadIdentity();
  if (trans.uses(anim)) {
    glTranslatef(tval.x(), tval.y(), tval.z());
  }
  if (rot.uses(anim)) {
    glRotatef(rval.x(), 0, 0, 1); // this is wrong, I have no idea what I'm doing here ;)
  }
  if (scale.uses(anim)) {
    glScalef(sval.x(), sval.y(), sval.z());
  }
}

void ModelCamera::init(const noggit::mpq::file& f, const ModelCameraDef &mcd, int *global)
{
  ok = true;
    nearclip = mcd.nearclip;
  farclip = mcd.farclip;
  fov = mcd.fov;
  pos = fixCoordSystem(mcd.pos);
  target = fixCoordSystem(mcd.target);
  tPos.init(mcd.transPos, f, global);
  tTarget.init(mcd.transTarget, f, global);
  rot.init(mcd.rot, f, global);
  tPos.apply(fixCoordSystem);
  tTarget.apply(fixCoordSystem);
}

void ModelCamera::setup( int time )
{
  if( !ok )
  {
    return;
  }
  //! \todo This really needs to be done differently.
/*
  video.fov( fov * 34.5f );
  video.nearclip( nearclip );
  video.farclip( farclip );
  video.updateProjectionMatrix();

  ::math::vector_3d p = pos + tPos.getValue( 0, time );
  ::math::vector_3d t = target + tTarget.getValue( 0, time );
  ::math::vector_3d u( 0.0f, 1.0f, 0.0f );
  gluLookAt( p.x, p.y, p.z, t.x, t.y, t.z, u.x, u.y, u.z );
  */
}

void ModelColor::init(const noggit::mpq::file& f, const ModelColorDef &mcd, int *global)
{
  color.init(mcd.color, f, global);
  opacity.init(mcd.opacity, f, global);
}

void ModelTransparency::init(const noggit::mpq::file& f, const ModelTransDef &mcd, int *global)
{
  trans.init(mcd.trans, f, global);
}

void ModelLight::init(const noggit::mpq::file& f, const ModelLightDef &mld, int *global)
{
  tpos = pos = fixCoordSystem(mld.pos);
  tdir = dir = ::math::vector_3d(0,1,0); // no idea
  type = mld.type;
  parent = mld.bone;
  ambColor.init(mld.ambColor, f, global);
  ambIntensity.init(mld.ambIntensity, f, global);
  diffColor.init(mld.color, f, global);
  diffIntensity.init(mld.intensity, f, global);
}

void ModelLight::setup(int time, opengl::light l)
{
  ::math::vector_4d ambcol(ambColor.getValue(0, time) * ambIntensity.getValue(0, time), 1.0f);
  ::math::vector_4d diffcol(diffColor.getValue(0, time) * diffIntensity.getValue(0, time), 1.0f);
  ::math::vector_4d p;

  enum ModelLightTypes {
    MODELLIGHT_DIRECTIONAL=0,
    MODELLIGHT_POINT
  };

  if (type==MODELLIGHT_DIRECTIONAL) {
    // directional
    p = ::math::vector_4d(tdir, 0.0f);
  } else if (type==MODELLIGHT_POINT) {
    // point
    p = ::math::vector_4d(tpos, 1.0f);
  } else {
    p = ::math::vector_4d(tpos, 1.0f);
    LogError << "Light type " << type << " is unknown." << std::endl;
  }
  //gLog("Light %d (%f,%f,%f) (%f,%f,%f) [%f,%f,%f]\n", l-GL_LIGHT4, ambcol.x, ambcol.y, ambcol.z, diffcol.x, diffcol.y, diffcol.z, p.x, p.y, p.z);
  glLightfv(l, GL_POSITION, p);
  glLightfv(l, GL_DIFFUSE, diffcol);
  glLightfv(l, GL_AMBIENT, ambcol);
  glEnable(l);
}

void TextureAnim::init(const noggit::mpq::file& f, const ModelTexAnimDef &mta, int *global)
{
  trans.init(mta.trans, f, global);
  rot.init(mta.rot, f, global);
  scale.init(mta.scale, f, global);
}

void Bone::init(const noggit::mpq::file& f, const ModelBoneDef &b, int *global, noggit::mpq::file **animfiles)
{
  parent = b.parent;
  pivot = fixCoordSystem(b.pivot);

  static const int MODELBONE_BILLBOARD = 8;

  billboard = (b.flags & MODELBONE_BILLBOARD) != 0;

  trans.init(b.translation, f, global, animfiles);
  rot.init(b.rotation, f, global, animfiles);
  scale.init(b.scaling, f, global, animfiles);
  trans.apply(fixCoordSystem);
  rot.apply(fixCoordSystemQuat);
  scale.apply(fixCoordSystem2);
}

void Bone::calcMatrix(Bone *allbones, int anim, int time)
{
  if (calc) return;
  ::math::matrix_4x4 m;
  ::math::quaternion q;

  bool tr = rot.uses(anim) || scale.uses(anim) || trans.uses(anim) || billboard;
  if (tr) {
    m.translation(pivot);

    if (trans.uses(anim)) {
      m *= ::math::matrix_4x4::new_translation_matrix (trans.getValue (anim, time));
    }
    if (rot.uses(anim)) {
      q = rot.getValue(anim, time);
      m *= ::math::matrix_4x4::new_rotation_matrix (q);
    }
    if (scale.uses(anim)) {
      m *= ::math::matrix_4x4::new_scale_matrix (scale.getValue(anim, time));
    }
    if (billboard) {
      float modelview[16];
      glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

      ::math::vector_3d vRight (modelview[0], modelview[4], modelview[8]);
      ::math::vector_3d vUp (modelview[1], modelview[5], modelview[9]); // Spherical billboarding
      //::math::vector_3d vUp (0,1,0); // Cylindrical billboarding
      vRight = vRight * -1;
      m (0, 1, vUp.x());
      m (1, 1, vUp.y());
      m (2, 1, vUp.z());
      m (0, 2, vRight.x());
      m (1, 2, vRight.y());
      m (2, 2, vRight.z());
    }

    m *= ::math::matrix_4x4::new_translation_matrix (pivot*-1.0f);

  } else m.unit();

  if (parent>=0) {
    allbones[parent].calcMatrix(allbones, anim, time);
    mat = allbones[parent].mat * m;
  } else mat = m;

  // transform matrix for normal vectors ... ??
  if (rot.uses(anim)) {
    if (parent>=0) {
      mrot = allbones[parent].mrot * ::math::matrix_4x4::new_rotation_matrix (q);
    } else mrot = ::math::matrix_4x4::new_rotation_matrix (q);
  } else mrot.unit();

  transPivot = mat * pivot;

  calc = true;
}


void Model::draw (bool draw_fog, size_t time)
{
  if(!finished_loading())
    return;

  if (draw_fog)
    glEnable( GL_FOG );
  else
    glDisable( GL_FOG );

  if( !animated )
  {
    glCallList( ModelDrawList );
  }
  else
  {
    if( !animcalc || mPerInstanceAnimation )
    {
      animate(0, time);
      animcalc = true;
    }


    lightsOn( GL_LIGHT4, time );
    drawModel(time);
    lightsOff( GL_LIGHT4 );


    // draw particle systems & ribbons
    for( size_t i = 0; i < header.nParticleEmitters; ++i )
      particleSystems[i].draw();

    for( size_t i = 0; i < header.nRibbonEmitters; ++i )
      ribbons[i].draw();
  }
}

void Model::drawSelect (size_t time)
{
  if (!_finished)
  {
    return;
  }

  if( !animated )
    glCallList(SelectModelDrawList);
  else
  {
      if( !animcalc || mPerInstanceAnimation )
      {
        animate(0, time);
        animcalc = true;
      }

    drawModelSelect(time);

    //QUESTION: Do we need to drow this stuff for selectio??
    // draw particle systems
    // for( size_t i = 0; i < header.nParticleEmitters; ++i )
      // particleSystems[i].draw();

     //QUESTION: Do we need to drow this stuff for selectio??
    // draw ribbons
    // for( size_t i = 0; i < header.nRibbonEmitters; ++i )
       //ribbons[i].draw();
  }
}

void Model::lightsOn(opengl::light lbase, int animtime)
{
  // setup lights
  for (unsigned int i=0, l=lbase; i<header.nLights; ++i) lights[i].setup(animtime, l++);
}

void Model::lightsOff(opengl::light lbase)
{
  for (unsigned int i=0, l=lbase; i<header.nLights; ++i) glDisable(l++);
}

void Model::updateEmitters(float dt)
{
  for (size_t i=0; i<header.nParticleEmitters; ++i) {
    particleSystems[i].update(dt);
  }
}
