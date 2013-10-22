// TextureManager.h is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Stephan Biegel <project.modcraft@googlemail.com>
// Tigurius <bstigurius@googlemail.com>

#ifndef __TEXTUREMANAGER_H
#define __TEXTUREMANAGER_H

#include <map>
#include <string>
#include <vector>

namespace noggit
{
  class blp_texture;
}

class TextureManager
{
public:
  static void delbyname( std::string name );
  static noggit::blp_texture* newTexture(std::string name);

  static void report();

  //! \todo This should not be there.
  //! \note This is only for getting all cached textures in UITexturingGUI.
  static std::vector<noggit::blp_texture*> getAllTexturesMatching( bool (*function)( const std::string& name ) );

private:
  typedef std::map<std::string, noggit::blp_texture*> mapType;
  static mapType items;
};

#endif
