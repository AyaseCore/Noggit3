// Particle.h is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Stephan Biegel <project.modcraft@googlemail.com>
// Tigurius <bstigurius@googlemail.com>

#ifndef PARTICLE_H
#define PARTICLE_H

#include <list>
#include <vector>

class ParticleSystem;
class RibbonEmitter;

#include <math/vector_2d.h>
#include <math/vector_4d.h>

#include <noggit/Animated.h> // Animation::M2Value
#include <noggit/Model.h>

namespace opengl
{
  class texture;
}

namespace noggit
{
  namespace mpq
  {
    class file;
  }
}

struct Particle {
  ::math::vector_3d pos, speed, down, origin, dir;
  ::math::vector_3d  corners[4];
  //::math::vector_3d tpos;
  float size, life, maxlife;
  unsigned int tile;
  ::math::vector_4d color;
};

typedef std::list<Particle> ParticleList;

class ParticleEmitter {
protected:
  ParticleSystem *sys;
public:
  explicit ParticleEmitter(ParticleSystem *psys): sys(psys) {}
  virtual ~ParticleEmitter() {}
  virtual Particle newParticle(int anim, int time, float w, float l, float spd, float var, float spr, float spr2) = 0;
};

class PlaneParticleEmitter: public ParticleEmitter {
public:
  explicit PlaneParticleEmitter(ParticleSystem *_sys): ParticleEmitter(_sys) {}
  virtual ~PlaneParticleEmitter() {}
  Particle newParticle(int anim, int time, float w, float l, float spd, float var, float spr, float spr2);
};

class SphereParticleEmitter: public ParticleEmitter {
public:
  explicit SphereParticleEmitter(ParticleSystem *_sys): ParticleEmitter(_sys) {}
  virtual ~SphereParticleEmitter() {}
  Particle newParticle(int anim, int time, float w, float l, float spd, float var, float spr, float spr2);
};

struct TexCoordSet {
    ::math::vector_2d tc[4];
};

class ParticleSystem {
  Animation::M2Value<float> speed, variation, spread, lat, gravity, lifespan, rate, areal, areaw, deacceleration;
  Animation::M2Value<uint8_t> enabled;
  ::math::vector_4d colors[3];
  float sizes[3];
  ParticleEmitter *emitter;
  float mid, slowdown, rotation;
  ::math::vector_3d pos;
  opengl::texture* _texture;
  ParticleList particles;
  int blend,order,type;
  int manim,mtime;
  int rows, cols;
  std::vector<TexCoordSet> tiles;
  void initTile(::math::vector_2d *tc, int num);
  bool billboard;

  float rem;
  //bool transform;

  // unknown parameters omitted for now ...
  Bone *parent;
  int32_t flags;
  int16_t pType;

public:
  Model *model;
  float tofs;

  ParticleSystem(): emitter(NULL), mid(0), rem(0)
  {
    blend = 0;
    order = 0;
    type = 0;
    manim = 0;
    mtime = 0;
    rows = 0;
    cols = 0;

    model = 0;
    parent = 0;
    _texture = NULL;

    slowdown = 0;
    rotation = 0;
    tofs = 0;
  }
  virtual ~ParticleSystem()
  {
    delete emitter;
    emitter = NULL;
  }

  void init(const noggit::mpq::file& f, const ModelParticleEmitterDef &mta, int *globals);
  void update(float dt);

  void setup(int anim, int time);
  void draw();
  void drawHighlight();

  friend class PlaneParticleEmitter;
  friend class SphereParticleEmitter;
};


struct RibbonSegment {
  ::math::vector_3d pos, up, back;
  float len,len0;
};

class RibbonEmitter {
  Animation::M2Value< ::math::vector_3d> color;
  Animation::M2Value<float,int16_t> opacity;
  Animation::M2Value<float> above, below;

  Bone *parent;
  float f1, f2;

  ::math::vector_3d pos;

  int manim, mtime;
  float length, seglen;
  int numsegs;

  ::math::vector_3d tpos;
  ::math::vector_4d tcolor;
  float tabove, tbelow;

  opengl::texture* _texture;

  std::list<RibbonSegment> segs;

public:
  Model *model;

  void init(const noggit::mpq::file &f, ModelRibbonEmitterDef &mta, int *globals);
  void setup(int anim, int time);
  void draw();
};



#endif
