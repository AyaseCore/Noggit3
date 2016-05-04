// TextureManager.cpp is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Stephan Biegel <project.modcraft@googlemail.com>
// Tigurius <bstigurius@googlemail.com>

#include <noggit/TextureManager.h>

#include <algorithm>

#include <noggit/blp_texture.h>
#include <noggit/Log.h> // LogDebug

TextureManager::mapType TextureManager::items;

void TextureManager::report()
{
  std::string output = "Still in the texture manager:\n";
  for( mapType::iterator t = items.begin(); t != items.end(); ++t )
  {
    output += "- " + t->first + "\n";
  }
  LogDebug << output;
}

void TextureManager::delbyname( std::string name )
{
  std::transform( name.begin(), name.end(), name.begin(), ::tolower );

  if( items.find( name ) != items.end() )
  {
    items[name]->removeReference();

    if( items[name]->hasNoReferences() )
    {
      delete items[name];
      items.erase( items.find( name ) );
    }
  }
}

noggit::blp_texture* TextureManager::newTexture( std::string name )
{
  std::transform( name.begin(), name.end(), name.begin(), ::tolower );

  if( items.find( name ) == items.end() )
  {
    items[name] = new noggit::blp_texture (QString::fromStdString (name));
  }

  items[name]->addReference();

  return items[name];
}

std::vector<noggit::blp_texture*> TextureManager::getAllTexturesMatching(bool (*function)( const std::string& name ) )
{
  std::vector<noggit::blp_texture*> returnVector;
  for( mapType::iterator t = items.begin(); t != items.end(); ++t )
  {
    if( function( t->first ) )
    {
      returnVector.push_back( items[t->first] );
    }
  }
  return returnVector;
}
