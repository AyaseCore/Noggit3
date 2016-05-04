// UIMapViewGUI.cpp is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Glararan <glararan@glararan.eu>
// Stephan Biegel <project.modcraft@googlemail.com>

#include <noggit/UIMapViewGUI.h>

#include <sstream>
#include <algorithm>
#include <vector>

#include <QTextEdit>

#include <noggit/blp_texture.h>
#include <noggit/DBC.h>
#include <noggit/MapChunk.h>
#include <noggit/MapView.h>
#include <noggit/application.h> // gStates, gPop, arial14, morpheus40, arial...
#include <noggit/UIDoodadSpawner.h>
#include <noggit/UIStatusBar.h> // UIStatusBar
#include <noggit/UITexturePicker.h> //
#include <noggit/UITextureSwitcher.h>
#include <noggit/UITexturingGUI.h>
#include <noggit/UIToolbar.h> // UIToolbar
#include <noggit/UIZoneIDBrowser.h>
#include <noggit/WMOInstance.h>
#include <noggit/World.h>
#include <noggit/Log.h>

UIMapViewGUI::UIMapViewGUI (World* world, MapView *map_view, float xres, float yres)
  : UIFrame( 0.0f, 0.0f, xres, yres )
  , theMapview( map_view )
  , _world (world)
{
  // UIToolbar
  guiToolbar = new UIToolbar( 6.0f, 38.0f, this );
  addChild(guiToolbar);

  // Statusbar
  guiStatusbar = new UIStatusBar( 0.0f, height() - 30.0f, width(), 30.0f );
  addChild(guiStatusbar);

  // DetailInfoWindow
  guidetailInfos = new QTextEdit (NULL);
  guidetailInfos->resize (600, 250);
  guidetailInfos->hide();

  // ZoneIDBrowser
  ZoneIDBrowser = new UIZoneIDBrowser(200, 200, 435, 400, this);
  ZoneIDBrowser->movable( true );
  ZoneIDBrowser->hide();
  addChild(ZoneIDBrowser);


  TexturePicker = new UITexturePicker(width() / 2 - 100.0f, height() / 2 - 100.0f,490.0f, 150.0f );
  TexturePicker->hide();
  TexturePicker->movable( true );
  addChild( TexturePicker);

  TextureSwitcher = new UITextureSwitcher(_world, width() / 2 - 100.0f, height() / 2 - 100.0f);
  TextureSwitcher->hide();
  TextureSwitcher->movable( true );
  addChild( TextureSwitcher);
}

void UIMapViewGUI::render( ) const
{
  UIFrame::render();

  //! \todo Make these some textUIs.
  arial16.shprint( 510, 4, gAreaDB.getAreaName( _world->getAreaID() ).toStdString() );

  int time = static_cast<int>( _world->time ) % 2880;
  std::stringstream timestrs; timestrs << "Time: " << ( time / 120 ) << ":" << ( time % 120 );
  arial16.shprint( width() - 100.0f, 5.0f, timestrs.str() );

  if ( _world->noadt )
  {
    std::string toDisplay( "No ADT at this Point" );
    arial16.shprint( width() / 2.0f - arial16.width( toDisplay ) / 2.0f, 30.0f, toDisplay );
  }

  std::ostringstream statusbarInfo;
  statusbarInfo << "tile: " << std::floor( _world->camera.x / TILESIZE ) << " " <<  std::floor( _world->camera.z / TILESIZE )
     << "; coordinates: client (x: " << _world->camera.x << ", y: " << _world->camera.z << ", z: "<<_world->camera.y
     << "), server (x: " << ( ZEROPOINT - _world->camera.x ) << ", y:" << ( ZEROPOINT - _world->camera.z ) << ", z:" << ( ZEROPOINT - _world->camera.y ) << ")" ;
  guiStatusbar->setLeftInfo( statusbarInfo.str() );

  guiStatusbar->setRightInfo( "" );

  if(guidetailInfos->isVisible() )
  {
    nameEntry * lSelection = _world->GetCurrentSelection();
    if( lSelection )
    {
      guiStatusbar->setRightInfo( lSelection->returnName() );

      std::stringstream detailInfo;

      switch( lSelection->type )
      {
      case eEntry_Model:
        detailInfo << "filename: " << lSelection->data.model->model->_filename
          << "\nunique ID: " << lSelection->data.model->d1
          << "\nposition X/Y/Z: " << lSelection->data.model->pos.x << " / " << lSelection->data.model->pos.y << " / " << lSelection->data.model->pos.z
          << "\nrotation X/Y/Z: " << lSelection->data.model->dir.x << " / " << lSelection->data.model->dir.y << " / " << lSelection->data.model->dir.z
          << "\nscale: " <<  lSelection->data.model->sc
          << "\ntextures Used: " << lSelection->data.model->model->header.nTextures;

        for( unsigned int j = 0; j < std::min( lSelection->data.model->model->header.nTextures, 6U ); j++ )
        {
          detailInfo << "\n " << ( j + 1 ) << ": " << lSelection->data.model->model->_textures[j]->filename().toStdString();
        }
        if( lSelection->data.model->model->header.nTextures > 25 )
        {
          detailInfo << "\n and more.";
        }

        detailInfo << "\n";
        break;

      case eEntry_WMO:
        detailInfo << "filename: " << lSelection->data.wmo->wmo->_filename
          << "\nunique ID: " << lSelection->data.wmo->mUniqueID
          << "\nposition X/Y/Z: " << lSelection->data.wmo->pos.x << " / " << lSelection->data.wmo->pos.y << " / " << lSelection->data.wmo->pos.z
          << "\nrotation X/Y/Z: " << lSelection->data.wmo->dir.x << " / " << lSelection->data.wmo->dir.y << " / " << lSelection->data.wmo->dir.z
          << "\ndoodad set: " << lSelection->data.wmo->doodadset
          << "\ntextures used: " << lSelection->data.wmo->wmo->nTextures;

        for( unsigned int j = 0; j < std::min( lSelection->data.wmo->wmo->nTextures, 8U ); j++ )
        {
          detailInfo << "\n " << ( j + 1 ) << ": " << lSelection->data.wmo->wmo->textures[j];
        }
        if( lSelection->data.wmo->wmo->nTextures > 25 )
        {
          detailInfo << "\n and more.";
        }

        detailInfo << "\n";
        break;

      case eEntry_MapChunk:

        int flags = lSelection->data.mapchunk->Flags;

        detailInfo << "MCNK " << lSelection->data.mapchunk->px << ", " << lSelection->data.mapchunk->py << " (" << lSelection->data.mapchunk->py * 16 + lSelection->data.mapchunk->px
          << ") of tile (" << lSelection->data.mapchunk->mt->mPositionX << " " << lSelection->data.mapchunk->mt->mPositionZ << ")"
          << "\narea ID: " << lSelection->data.mapchunk->areaID << " (\"" << gAreaDB.getAreaName( lSelection->data.mapchunk->areaID ).toStdString() << "\")"
          << "\nflags: "
          << ( flags & FLAG_SHADOW ? "shadows " : "" )
          << ( flags & FLAG_IMPASS ? "impassable " : "" )
          << ( flags & FLAG_LQ_RIVER ? "river " : "" )
          << ( flags & FLAG_LQ_OCEAN ? "ocean " : "" )
          << ( flags & FLAG_LQ_MAGMA ? "lava" : "" )
          << "\ntextures used: " << lSelection->data.mapchunk->nTextures;

        //! \todo get a list of textures and their flags as well as detail doodads.
        /*
        for( int q = 0; q < lSelection->data.mapchunk->nTextures; q++ )
        {
          //s << " ";
          //s "  Flags - " << lSelection->data.mapchunk->texFlags[q] << " Effect ID - " << lSelection->data.mapchunk->effectID[q] << std::endl;

          if( lSelection->data.mapchunk->effectID[q] != 0 )
            for( int r = 0; r < 4; r++ )
            {
              const char *EffectModel = getGroundEffectDoodad( lSelection->data.mapchunk->effectID[q], r );
              if( EffectModel )
              {
                  s << r << " - World\\NoDXT\\" << EffectModel << endl;
                  //freetype::shprint( arial16, 30, 103 + TextOffset, "%d - World\\NoDXT\\%s", r, EffectModel );
                  TextOffset += 20;
              }
            }

        }
        */

        detailInfo << "\n";

        break;
      }
      guidetailInfos->setText( QString::fromStdString (detailInfo.str()) );
    }
    else
    {
      guidetailInfos->setText( "" );
    }
  }
}
