#undef _UNICODE

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#ifdef __FILESAREMISSING
#include <IL/il.h>
#endif

#include "Brush.h" // brush
#include "ConfigFile.h"
#include "DBC.h"
#include "Environment.h"
#include "FreeType.h" // freetype::
#include "Log.h"
#include "MapChunk.h"
#include "MapView.h"
#include "Misc.h"
#include "ModelManager.h" // ModelManager
#include "Noggit.h" // app.getStates(), gPop, gFPS, app.getArial14(), morpheus40, arial...
#include "Project.h"
#include "Settings.h"
#include "Environment.h"
#include "TextureManager.h" // TextureManager, Texture
#include "UIAppInfo.h" // appInfo
#include "UICheckBox.h" // UICheckBox
#include "UICursorSwitcher.h" // UICursorSwitcher
#include "UIDetailInfos.h" // detailInfos
#include "UIGradient.h" // UIGradient
#include "UIMapViewGUI.h" // UIMapViewGUI
#include "UIMenuBar.h" // UIMenuBar, menu items, ..
#include "UIMinimapWindow.h" // UIMinimapWindow
#include "UISlider.h" // UISlider
#include "UIStatusBar.h" // statusBar
#include "UIText.h" // UIText
#include "UITexture.h" // textureUI
#include "UITexturePicker.h"
#include "UITextureSwitcher.h"
#include "UITexturingGUI.h"
#include "UIToggleGroup.h" // UIToggleGroup
#include "UIToolbar.h" // UIToolbar
#include "UIToolbarIcon.h" // ToolbarIcon
#include "UIZoneIDBrowser.h"
#include "UIWater.h"
#include "WMOInstance.h" // WMOInstance
#include "World.h"
#include "UIExitWarning.h"
#include "UICapsWarning.h"
#include "UIWaterSaveWarning.h"
#include "UIModelImport.h"
#include "UIHelperModels.h"
#include "MapIndex.h"
#include "UIWaterTypeBrowser.h"

static const float XSENS = 15.0f;
static const float YSENS = 15.0f;
static const float SPEED = 200.6f;

int MouseX;
int MouseY;
float mh,mv,rh,rv;

float moveratio = 0.1f;
float rotratio = 0.2f;
float keyx,keyy,keyz,keyr,keys;

int tool_settings_x;
int tool_settings_y;

bool MoveObj;

Vec3D ObjMove;
Vec3D ObjRot;


bool TestSelection=false;

extern bool DrawMapContour;
extern bool drawFlags;

extern nameEntryManager SelectionNames;

// extern row and col form Palette UI


// This variables store the current status of the
// Shift, Alt and CTRL keys


bool  leftMouse=false;
bool  leftClicked=false;
bool  rightMouse=false;
bool  painting=false;

// Vars for the ground editing toggle mode
// store the status of some view settings when
// the ground editing mode is switched on
// to restore them if switch back again

bool  alloff = true;
bool  alloff_models = false;
bool  alloff_doodads = false;
bool  alloff_contour = false;
bool  alloff_wmo = false;
bool  alloff_detailselect = false;
bool  alloff_fog = false;
bool  alloff_terrain = false;

UISlider* ground_brush_radius;
float groundBrushRadius=15.0f;
UISlider* ground_brush_speed;
float groundBrushSpeed=1.0f;
UISlider* ground_blur_speed;
float groundBlurSpeed=2.0f;
int    groundBrushType=1;

UISlider* blur_brush;
float blurBrushRadius=10.0f;
int    blurBrushType=1;

UISlider* paint_brush;

float brushPressure=0.9f;
float brushLevel=255.0f;

int terrainMode=0;
int saveterrainMode = 0;

Brush textureBrush;


UICursorSwitcher* CursorSwitcher;

bool Saving=false;

UIFrame* LastClicked;


// main GUI object
UIMapViewGUI* mainGui;

UIFrame* MapChunkWindow;

UIToggleGroup * gBlurToggleGroup;
UIToggleGroup * gGroundToggleGroup;
UIToggleGroup * gFlagsToggleGroup;

UIWindow *setting_ground;
UIWindow *setting_blur;
UIWindow *settings_paint;


//TextBox * textbox;

void setGroundBrushRadius(float f)
{
  groundBrushRadius = f;
}

void setGroundBrushSpeed(float f)
{
  groundBrushSpeed = f;
}


void setBlurBrushRadius(float f)
{
  blurBrushRadius = f;
}

void setBlurBrushSpeed(float f)
{
  groundBlurSpeed = f;
}


void setTextureBrushHardness(float f)
{
  textureBrush.setHardness(f);
}

void setTextureBrushRadius(float f)
{
  textureBrush.setRadius(f);
}

void setTextureBrushPressure(float f)
{
  brushPressure=f;
}

void setTextureBrushLevel(float f)
{
  brushLevel=(1.0f-f)*255.0f;
}

void SaveOrReload( UIFrame*, int pMode )
{
  if( pMode == 1 )
    gWorld->mapIndex->reloadTile( static_cast<int>( gWorld->camera.x ) / TILESIZE, static_cast<int>( gWorld->camera.z ) / TILESIZE );
  else if( pMode == 0 )
    gWorld->mapIndex->saveTile( static_cast<int>( gWorld->camera.x ) / TILESIZE, static_cast<int>( gWorld->camera.z ) / TILESIZE );
  else if( pMode == 2 )
    gWorld->mapIndex->saveChanged();
  else if( pMode == 3 )
    static_cast<MapView*>( app.getStates().back() )->quit();

}

void change_settings_window(int oldid, int newid)
{
  if(!setting_ground || !setting_blur || !settings_paint || (!mainGui || !mainGui->guiWater))
    return;
  mainGui->guiWaterTypeSelector->hide();
  setting_ground->hide();
  setting_blur->hide();
  settings_paint->hide();
  mainGui->guiWater->hide();
  if(!mainGui || !mainGui->TexturePalette)
    return;
  mainGui->TexturePalette->hide();
  // fetch old win position
  switch(oldid)
  {
    case 1:
      tool_settings_x=setting_ground->x();
      tool_settings_y=setting_ground->y();
      break;
    case 2:
      tool_settings_x=setting_blur->x();
      tool_settings_y=setting_blur->y();
      break;
    case 3:
      tool_settings_x=settings_paint->x();
      tool_settings_y=settings_paint->y();
      break;
    case 7:
      tool_settings_x = mainGui->guiWater->x();
      tool_settings_y = mainGui->guiWater->y();
      break;
  }
  // set new win pos and make visible
  switch(newid)
  {
    case 1:
      setting_ground->x( tool_settings_x );
      setting_ground->y( tool_settings_y );
      setting_ground->show();
      break;
    case 2:
      setting_blur->x( tool_settings_x );
      setting_blur->y( tool_settings_y );
      setting_blur->show();
      break;
    case 3:
      settings_paint->x( tool_settings_x );
      settings_paint->y( tool_settings_y );
      settings_paint->show();
      break;
    case 7:
      mainGui->guiWater->x( tool_settings_x );
      mainGui->guiWater->y( tool_settings_y );
      mainGui->guiWater->show();
      break;
  }
}

void openSwapper( UIFrame*, int )
{
  mainGui->TextureSwitcher->show();
  settings_paint->hide();
}

void openHelp( UIFrame*, int )
{
  mainGui->showHelp();
}

void openURL( UIFrame*,  int target)
{
#if defined(_WIN32) || defined(WIN32)
  if(target==1)  ShellExecute(NULL, "open", "http://modcraft.superparanoid.de", NULL, NULL, SW_SHOWNORMAL);
  if(target==2)  ShellExecute(NULL, "open", "http://modcraft.superparanoid.de/wiki/index.php5?title=Noggit_user_manual", NULL, NULL, SW_SHOWNORMAL);
#endif
}

void ResetSelectedObjectRotation( UIFrame* /*button*/, int /*id*/ )
{
  if( gWorld->IsSelection( eEntry_WMO ) )
  {
    gWorld->GetCurrentSelection()->data.wmo->resetDirection();
    gWorld->mapIndex->setChanged(gWorld->GetCurrentSelection()->data.wmo->pos.x, gWorld->GetCurrentSelection()->data.wmo->pos.z);
  }
  else if( gWorld->IsSelection( eEntry_Model ) )
  {
    gWorld->GetCurrentSelection()->data.model->resetDirection();
    gWorld->mapIndex->setChanged(gWorld->GetCurrentSelection()->data.model->pos.x, gWorld->GetCurrentSelection()->data.model->pos.z);
  }
}

void SnapSelectedObjectToGround( UIFrame* /*button*/, int /*id*/ )
{
  if( gWorld->IsSelection( eEntry_WMO ) )
  {
    Vec3D t = Vec3D( gWorld->GetCurrentSelection()->data.wmo->pos.x, gWorld->GetCurrentSelection()->data.wmo->pos.z, 0 );
    gWorld->GetVertex( gWorld->GetCurrentSelection()->data.wmo->pos.x, gWorld->GetCurrentSelection()->data.wmo->pos.z, &t );
    gWorld->GetCurrentSelection()->data.wmo->pos = t;
    gWorld->GetCurrentSelection()->data.wmo->recalcExtents();
    gWorld->mapIndex->setChanged(gWorld->GetCurrentSelection()->data.wmo->pos.x, gWorld->GetCurrentSelection()->data.wmo->pos.z);

  }
  else if( gWorld->IsSelection( eEntry_Model ) )
  {
    Vec3D t = Vec3D( gWorld->GetCurrentSelection()->data.model->pos.x, gWorld->GetCurrentSelection()->data.model->pos.z, 0 );
    gWorld->GetVertex( gWorld->GetCurrentSelection()->data.model->pos.x, gWorld->GetCurrentSelection()->data.model->pos.z, &t );
    gWorld->GetCurrentSelection()->data.model->pos = t;
    gWorld->mapIndex->setChanged(gWorld->GetCurrentSelection()->data.model->pos.x, gWorld->GetCurrentSelection()->data.model->pos.z);
  }
}

/*!
  \brief Copy selected model to clipboard
  Copy the selected m2 or WMO with getInstance()->set_clipboard()
*/
void CopySelectedObject( UIFrame* /*button*/, int /*id*/ )
{
  if( gWorld->HasSelection() )
  {
    Environment::getInstance()->set_clipboard( gWorld->GetCurrentSelection() );
  }
}

/*!
  \brief Paste a model
  Paste the current model stored in Environment::getInstance()->get_clipboard() at the cords of the selected model or chunk.
*/
void PasteSelectedObject( UIFrame* /*button*/, int /*id*/ )
{
  if( gWorld->HasSelection() )
  {

    switch( gWorld->GetCurrentSelection()->type )
    {
      case eEntry_Model:
        gWorld->addModel( Environment::getInstance()->get_clipboard(), gWorld->GetCurrentSelection()->data.model->pos,true );
        break;
      case eEntry_WMO:
        gWorld->addModel( Environment::getInstance()->get_clipboard(), gWorld->GetCurrentSelection()->data.wmo->pos,true);
        break;
      case eEntry_MapChunk:
        gWorld->addModel( Environment::getInstance()->get_clipboard(), gWorld->GetCurrentSelection()->data.mapchunk->GetSelectionPosition() ,true);
        break;
      default: break;
    }
  }
}

/*!
  \brief Delete the current selected model
*/
void DeleteSelectedObject( UIFrame* /*button*/, int /*id*/ )
{
  if( gWorld->IsSelection( eEntry_WMO ) )
    gWorld->deleteWMOInstance( gWorld->GetCurrentSelection()->data.wmo->mUniqueID );
  else if( gWorld->IsSelection( eEntry_Model ) )
    gWorld->deleteModelInstance( gWorld->GetCurrentSelection()->data.model->d1 );
}

void showHelperModels( UIFrame* /*button*/, int /*id*/ )
{
  mainGui->HelperModels->show();
}

void showImportModels( UIFrame* /*button*/, int /*id*/ )
{
  mainGui->ModelImport->show();
}

void SaveObjecttoTXT( UIFrame* /*button*/, int /*id*/ )
{

  if( !gWorld->HasSelection() )
    return;
  string path;

  if( gWorld->IsSelection( eEntry_WMO ) )
  {
    path = gWorld->GetCurrentSelection()->data.wmo->wmo->filename();
  }
  else if( gWorld->IsSelection( eEntry_Model ) )
  {
    path = gWorld->GetCurrentSelection()->data.model->model->_filename;
  }

  std::ofstream f( "Import.txt", std::ios_base::app );
  f << path << std::endl;
  f.close();
  mainGui->ModelImport->builModelList();
}


/*!
  \brief Import a new model form a text file or a hard coded one.
  Imports a model from the import.txt, the wowModelViewer log or just insert some hard coded testing models.
  \param id the id switch the import kind
*/
void InsertObject( UIFrame* /*button*/, int id )
{
  //! \todo Beautify.

  // Test if there is an selection
  if( !gWorld->HasSelection() )
    return;
  // the list of the models to import
  std::vector<std::string> m2s_to_add;
  std::vector<std::string> wmos_to_add;

  // the import file
  std::string importFile;

  const char* filesToAdd[15] = {"","","World\\Scale\\humanmalescale.m2","World\\Scale\\50x50.m2","World\\Scale\\100x100.m2","World\\Scale\\250x250.m2","World\\Scale\\500x500.m2","World\\Scale\\1000x1000.m2","World\\Scale\\50yardradiusdisc.m2","World\\Scale\\200yardradiusdisc.m2","World\\Scale\\777yardradiusdisc.m2","World\\Scale\\50yardradiussphere.m2","World\\Scale\\200yardradiussphere.m2","World\\Scale\\777yardradiussphere.m2",""};

  // MODELINSERT FROM TEXTFILE
  // is a source file set in config file?


  if((id == 0 || id == 14 || id == 15) && boost::filesystem::exists("noggit.conf"))
  {
    ConfigFile config( "noggit.conf" );
    config.readInto( importFile, "ImportFile" );
  }
  else if(id == 1)
    importFile="Import.txt"; //  use import.txt in noggit folder!
  else if(id>99)
    importFile="Import.txt"; //  use import.txt in noggit folder!
  else
    m2s_to_add.push_back( filesToAdd[id] );

  LogDebug << id << "-" << importFile << std::endl;

  std::string lastModel;
  std::string lastWMO;

  if(importFile!="")
  {
    size_t foundString;
    std::string line;
    std::string findThis;
    std::ifstream fileReader(importFile.c_str());
    int counter = 1;
    if (fileReader.is_open())
    {
      while (! fileReader.eof() )
      {
        getline (fileReader,line);
        if(line.find(".m2")!= std::string::npos || line.find(".M2")!= std::string::npos || line.find(".MDX")!= std::string::npos || line.find(".mdx")!= std::string::npos )
        {
          if( id < 99 || (id-99) == counter )
          {
            // M2 inside line
            // is it the modelviewer log then cut the log messages out
            findThis =   "Loading model: ";
            foundString = line.find(findThis);
            if(foundString!= std::string::npos)
            {
              // cut path
              line = line.substr( foundString+findThis.size() );
            }

            // swap mdx to m2
            size_t found = line.rfind( ".mdx" );
            if( found != std::string::npos )
              line.replace( found, 4, ".m2" );
            found = line.rfind( ".MDX" );
            if( found != std::string::npos )
              line.replace( found, 4, ".m2" );

            m2s_to_add.push_back( line );
            lastModel = line;
          }
          counter++;
        }
        else if(line.find(".wmo")!= std::string::npos || line.find(".WMO")!= std::string::npos )
        {
          if( id < 99 || (id-99) == counter )
          {
            // WMO inside line
            findThis = "Loading WMO ";
            foundString = line.find(findThis);
            // is it the modelviewer log then cut the log messages out
            if(foundString != std::string::npos)
            {
              // cut path
              line = line.substr( foundString+findThis.size() );
            }
            wmos_to_add.push_back(line);
            lastWMO = line;
          }
          counter++;
        }
      }
      fileReader.close();
    }
    else
    {
      // file not exist, no rights ore other error
      LogError << importFile << std::endl;
    }
  }


  Vec3D selectionPosition;
  switch( gWorld->GetCurrentSelection()->type )
  {
    case eEntry_Model:
      selectionPosition = gWorld->GetCurrentSelection()->data.model->pos;
      break;
    case eEntry_WMO:
      selectionPosition = gWorld->GetCurrentSelection()->data.wmo->pos;
      break;
    case eEntry_MapChunk:
      selectionPosition = gWorld->GetCurrentSelection()->data.mapchunk->GetSelectionPosition();
      break;
  }


  if(id==14)
  {
    LogError << "M2 Problem 14:" << lastModel << " - " << id << std::endl;
    if(lastModel!="")
      if( !MPQFile::exists(lastModel) )
        LogError << "Failed adding " << lastModel << ". It was not in any MPQ." << std::endl;
      else
        gWorld->addM2( ModelManager::add( lastModel ), selectionPosition,false );
  }
  else if(id==15)
  {
    LogError << "M2 Problem 15:" << lastModel << " - " << id << std::endl;
    if(lastWMO!="")
      if( !MPQFile::exists(lastWMO) )
        LogError << "Failed adding " << lastWMO << ". It was not in any MPQ." << std::endl;
      else
        gWorld->addWMO( WMOManager::add( lastWMO ), selectionPosition,false );
  }
  else
  {

    for( std::vector<std::string>::iterator it = wmos_to_add.begin(); it != wmos_to_add.end(); ++it )
    {

      if( !MPQFile::exists(*it) )
      {
        LogError << "Failed adding " << *it << ". It was not in any MPQ." << std::endl;
        continue;
      }

      gWorld->addWMO( WMOManager::add( *it ), selectionPosition,false );
    }

    for( std::vector<std::string>::iterator it = m2s_to_add.begin(); it != m2s_to_add.end(); ++it )
    {

      if( !MPQFile::exists(*it) )
      {

        LogError << "Failed adding " << *it << ". It was not in any MPQ." << std::endl;
        continue;
      }

      gWorld->addM2( ModelManager::add( *it ), selectionPosition ,false);
    }
  }
  //! \todo Memoryleak: These models will never get deleted.
}

void view_texture_palette( UIFrame* /*button*/, int /*id*/ )
{
  mainGui->TexturePalette->toggleVisibility();
}

void exit_tilemode(  UIFrame* /*button*/, int /*id*/ )
{
  app.pop = true;
}

void test_menu_action(  UIFrame* /*button*/, int /*id*/ )
{
  gWorld->saveWDT();
}

void moveHeightmap(  UIFrame* /*button*/, int /*id*/ )
{
  // set areaid on all chunks of the current ADT
  if(Environment::getInstance()->selectedAreaID)
    gWorld->moveHeight(Environment::getInstance()->selectedAreaID ,misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE),misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE));
}

void clearHeightmap(  UIFrame* /*button*/, int /*id*/ )
{
  // set areaid on all chunks of the current ADT
  if(Environment::getInstance()->selectedAreaID)
    gWorld->clearHeight(Environment::getInstance()->selectedAreaID ,misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE),misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE));

}

void adtSetAreaID( UIFrame* /*button*/, int /*id*/ )
{
  // set areaid on all chunks of the current ADT
  if(Environment::getInstance()->selectedAreaID)
    gWorld->setAreaID(Environment::getInstance()->selectedAreaID ,misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE),misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE));
}

void clearAllModels( UIFrame* /*button*/, int /*id*/ )
{
  // call the clearAllModelsOnADT method to clear them all on current ADT
  gWorld->clearAllModelsOnADT(misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE),misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE));
}

void menuWater( UIFrame* /*button*/, int id )
{
  // call the clearAllModelsOnADT method to clear them all on current ADT
  if(id == 1)
    gWorld->addWaterLayer(misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE),misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE));
  else if(id == 0)
    gWorld->deleteWaterLayer(misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE),misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE));
}

void changeZoneIDValue(UIFrame* /*f*/,int set)
{
  Environment::getInstance()->selectedAreaID = set;
  if( Environment::getInstance()->areaIDColors.find(set) == Environment::getInstance()->areaIDColors.end() )
  {
    Vec3D newColor = Vec3D( misc::randfloat(0.0f,1.0f) , misc::randfloat(0.0f,1.0f) , misc::randfloat(0.0f,1.0f) );
    Environment::getInstance()->areaIDColors.insert( std::pair<int,Vec3D>(set, newColor) );
  }
}

std::string getCurrentHeightmapPath()
{
  // get MapName
  std::string mapName;
  int id = gWorld->getMapID();
  for( DBCFile::Iterator i = gMapDB.begin(); i != gMapDB.end(); ++i )
  {
    if( i->getInt( MapDB::MapID ) == id)
      mapName = i->getString( MapDB::InternalName );
  }

  // build the path and filename string.
  std::stringstream png_filename;
  png_filename << Project::getInstance()->getPath() << "world\\maps\\" << mapName << "\\H_" << mapName
               << "_" << misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE) << "_" << misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE) << ".png" ;
  return png_filename.str();

}

void clearTexture(UIFrame* /*f*/,int /*set*/)
{
  // set areaid on all chunks of the current ADT
  gWorld->setBaseTexture(misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE),misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE));
}


void showCursorSwitcher(UIFrame* /*f*/, int /*set*/)
{
  mainGui->showCursorSwitcher();
}

#ifdef __FILESAREMISSING
void exportPNG(UIFrame *f,int set)
{

  // create the image and write to disc.
  GLfloat* data = new GLfloat[272*272];

  ilInit();

  int width  = 272 ;
  int height = 272 ;
  int bytesToUsePerPixel = 32;  // 16 bit per channel
  int sizeOfByte = sizeof( unsigned char ) ;
  int theSize = width * height * sizeOfByte * bytesToUsePerPixel ;

  unsigned char * imData =(unsigned char*)malloc( theSize ) ;

  int colors = 0;
  // write the height data to the image array
  for( int i = 0 ; i < theSize ; i++ )
  {
    imData[ i ] = colors ;
    if(i==100)colors = 200;
    if(i==200)colors = 4000;
  }


  ILuint ImageName; // The image name.
  ilGenImages(1, &ImageName); // Grab a new image name.
  ilBindImage(ImageName); // bind it
  ilTexImage(width,height,1,bytesToUsePerPixel,GL_LUMINANCE,IL_UNSIGNED_BYTE,NULL);
  ilSetData(imData);
  ilEnable(IL_FILE_OVERWRITE);
  //ilSave(IL_PNG, getCurrentHeightmapPath().c_str());
  ilSave(IL_PNG, "test2.png");
  free(imData);
}

void importPNG(UIFrame *f,int set)
{
  ilInit();

  //ILboolean loadImage = ilLoadImage( getCurrentHeightmapPath().c_str() ) ;
  const char *image = "test.png";
  ILboolean loadImage = ilLoadImage( image ) ;

  std::stringstream MessageText;
  if(loadImage)
  {

    LogDebug << "Image loaded: " << image << "\n";
    LogDebug <<  "ImageSize: " << ilGetInteger( IL_IMAGE_SIZE_OF_DATA ) << "\n";
    LogDebug <<  "BPP: " << ilGetInteger( IL_IMAGE_BITS_PER_PIXEL ) << "\n";
    LogDebug <<  "Format: " << ilGetInteger( IL_IMAGE_FORMAT ) << "\n";
    LogDebug <<  "SizeofData: " << ilGetInteger( IL_IMAGE_SIZE_OF_DATA ) << "\n";

  }
  else
  {
    LogDebug << "Cant load Image: " << image << "\n";
    ILenum err = ilGetError() ;

    MessageText << err << "\n";
    //MessageText << ilGetString(ilGetError()) << "\n";
    LogDebug << MessageText.str();
  }
}
#else
void exportPNG(UIFrame*, int) {}
void importPNG(UIFrame*, int) {}
#endif

void MapView::createGUI()
{
  // create main gui object that holds all other gui elements for access ( in the future ;) )
  mainGui = new UIMapViewGUI( this );
  mainGui->guiToolbar->current_texture->setClickFunc( view_texture_palette, 0 );

  mainGui->ZoneIDBrowser->setMapID( gWorld->getMapID() );
  mainGui->ZoneIDBrowser->setChangeFunc( changeZoneIDValue );
  tool_settings_x = video.xres() - 186;
  tool_settings_y = 38;

  // Raise/Lower
  setting_ground=new UIWindow( tool_settings_x, tool_settings_y, 180.0f, 160.0f );
  setting_ground->movable( true );
  mainGui->addChild( setting_ground );

  setting_ground->addChild( new UIText( 78.5f, 2.0f, "Raise / Lower", app.getArial14(), eJustifyCenter ) );

  gGroundToggleGroup = new UIToggleGroup( &groundBrushType );
  setting_ground->addChild( new UICheckBox( 6.0f, 15.0f, "Flat", gGroundToggleGroup, 0 ) );
  setting_ground->addChild( new UICheckBox( 85.0f, 15.0f, "Linear", gGroundToggleGroup, 1 ) );
  setting_ground->addChild( new UICheckBox( 6.0f, 40.0f, "Smooth", gGroundToggleGroup, 2 ) );
  setting_ground->addChild( new UICheckBox( 85.0f, 40.0f, "Polynomial", gGroundToggleGroup, 3 ) );
  setting_ground->addChild( new UICheckBox( 6.0f, 65.0f, "Trigonom", gGroundToggleGroup, 4 ) );
  setting_ground->addChild( new UICheckBox( 85.0f, 65.0f, "Quadratic", gGroundToggleGroup, 5 ) );
  gGroundToggleGroup->Activate(1);

  ground_brush_radius=new UISlider(6.0f,120.0f,167.0f,1000.0f,0.00001f);
  ground_brush_radius->setFunc(setGroundBrushRadius);
  ground_brush_radius->setValue(groundBrushRadius/1000);
  ground_brush_radius->setText( "Brush radius: " );
  setting_ground->addChild(ground_brush_radius);

  ground_brush_speed=new UISlider(6.0f,145.0f,167.0f,10.0f,0.00001f);
  ground_brush_speed->setFunc(setGroundBrushSpeed);
  ground_brush_speed->setValue(groundBrushSpeed/10);
  ground_brush_speed->setText( "Brush Speed: " );
  setting_ground->addChild(ground_brush_speed);

  // flatten/blur
  setting_blur=new UIWindow(tool_settings_x,tool_settings_y,180.0f,130.0f);
  setting_blur->movable( true );
  setting_blur->hide();
  mainGui->addChild(setting_blur);

  setting_blur->addChild( new UIText( 78.5f, 2.0f, "Flatten / Blur", app.getArial14(), eJustifyCenter ) );

  gBlurToggleGroup = new UIToggleGroup( &blurBrushType );
  setting_blur->addChild( new UICheckBox( 6.0f, 15.0f, "Flat", gBlurToggleGroup, 0 ) );
  setting_blur->addChild( new UICheckBox( 80.0f, 15.0f, "Linear", gBlurToggleGroup, 1 ) );
  setting_blur->addChild( new UICheckBox( 6.0f, 40.0f, "Smooth", gBlurToggleGroup, 2 ) );
  gBlurToggleGroup->Activate( 1 );

  blur_brush=new UISlider(6.0f,85.0f,167.0f,1000.0f,0.00001f);
  blur_brush->setFunc(setBlurBrushRadius);
  blur_brush->setValue(blurBrushRadius/1000);
  blur_brush->setText( "Brush radius: " );
  setting_blur->addChild(blur_brush);

  ground_blur_speed=new UISlider(6.0f,110.0f,167.0f,10.0f,0.00001f);
  ground_blur_speed->setFunc(setBlurBrushSpeed);
  ground_blur_speed->setValue(groundBlurSpeed/10);
  ground_blur_speed->setText( "Brush Speed: " );
  setting_blur->addChild(ground_blur_speed);

  //3D Paint settings UIWindow
  settings_paint=new UIWindow(tool_settings_x,tool_settings_y,180.0f,140.0f);
  settings_paint->hide();
  settings_paint->movable( true );

  mainGui->addChild(settings_paint);

  settings_paint->addChild( new UIText( 78.5f, 2.0f, "3D Paint", app.getArial14(), eJustifyCenter ) );


  mainGui->G1=new UIGradient;
  mainGui->G1->width( 20.0f );
  mainGui->G1->x( settings_paint->width() - 4.0f - mainGui->G1->width() );
  mainGui->G1->y( 4.0f );
  mainGui->G1->height( 92.0f );
  mainGui->G1->setMaxColor(1.0f,1.0f,1.0f,1.0f);
  mainGui->G1->setMinColor(0.0f,0.0f,0.0f,1.0f);
  mainGui->G1->horiz=false;
  mainGui->G1->setClickColor(1.0f,0.0f,0.0f,1.0f);
  mainGui->G1->setClickFunc(setTextureBrushLevel);
  mainGui->G1->setValue(0.0f);

  settings_paint->addChild(mainGui->G1);

  mainGui->S1 = new UISlider(6.0f,33.0f,145.0f,1.0f,0.0f);
  mainGui->S1->setFunc(setTextureBrushHardness);
  mainGui->S1->setValue(textureBrush.getHardness());
  mainGui->S1->setText("Hardness: ");
  settings_paint->addChild(mainGui->S1);

  paint_brush=new UISlider(6.0f,59.0f,145.0f,100.0f,0.00001);
  paint_brush->setFunc(setTextureBrushRadius);
  paint_brush->setValue(textureBrush.getRadius() / 100 );
  paint_brush->setText("Radius: ");
  settings_paint->addChild(paint_brush);

  mainGui->S1=new UISlider(6.0f,85.0f,145.0f,0.99f,0.01f);
  mainGui->S1->setFunc(setTextureBrushPressure);
  mainGui->S1->setValue(brushPressure);
  mainGui->S1->setText("Pressure: ");
  settings_paint->addChild(mainGui->S1);

  
  UIButton* B1;
  B1=new UIButton( 6.0f, 111.0f, 170.0f, 30.0f, "Texture swapper", "Interface\\BUTTONS\\UI-DialogBox-Button-Disabled.blp", "Interface\\BUTTONS\\UI-DialogBox-Button-Down.blp", openSwapper, 1 ) ;
  settings_paint->addChild(B1);


  mainGui->addChild(mainGui->TexturePalette = UITexturingGUI::createTexturePalette(mainGui));
  mainGui->TexturePalette->hide();
  //mainGui->addChild(mainGui->SelectedTexture = UITexturingGUI::createSelectedTexture());
  //mainGui->SelectedTexture->hide();
  mainGui->addChild(UITexturingGUI::createTilesetLoader());
  mainGui->addChild(UITexturingGUI::createTextureFilter());
  mainGui->addChild(MapChunkWindow = UITexturingGUI::createMapChunkWindow());
  MapChunkWindow->hide();

  // create the menu
  UIMenuBar * mbar = new UIMenuBar();

  mbar->AddMenu( "File" );
  mbar->AddMenu( "Edit" );
  mbar->AddMenu( "View" );
  mbar->AddMenu( "Assist" );
  mbar->AddMenu( "Help" );

  // mbar->GetMenu( "File" )->AddMenuItemButton( "CTRL+SHIFT+S Save current", SaveOrReload, 0 );
  mbar->GetMenu( "File" )->AddMenuItemButton( "CTRL+S Save", SaveOrReload, 2 );
  // mbar->GetMenu( "File" )->AddMenuItemButton( "SHIFT+J Reload tile", SaveOrReload, 1 );
  //  mbar->GetMenu( "File" )->AddMenuItemSeperator( "Import and Export" );
  // mbar->GetMenu( "File" )->AddMenuItemButton( "Export heightmap", exportPNG, 1 );
  // mbar->GetMenu( "File" )->AddMenuItemButton( "Import heightmap", importPNG, 1 );
  mbar->GetMenu( "File" )->AddMenuItemSeperator( " " );
  mbar->GetMenu( "File" )->AddMenuItemButton( "ESC Exit", SaveOrReload, 3 );

  //  mbar->GetMenu( "File" )->AddMenuItemSeperator( "Test" );
  //mbar->GetMenu( "File" )->AddMenuItemButton( "AreaID", test_menu_action, 1 );

  mbar->GetMenu( "Edit" )->AddMenuItemSeperator( "selected object" );
  mbar->GetMenu( "Edit" )->AddMenuItemButton( "CTRL + C copy", CopySelectedObject, 0  );
  mbar->GetMenu( "Edit" )->AddMenuItemButton( "CTRL + V past", PasteSelectedObject, 0  );
  mbar->GetMenu( "Edit" )->AddMenuItemButton( "DEL delete", DeleteSelectedObject, 0  );
  mbar->GetMenu( "Edit" )->AddMenuItemButton( "CTRL + R reset rotation", ResetSelectedObjectRotation, 0 );
  mbar->GetMenu( "Edit" )->AddMenuItemButton( "PAGE DOWN set to ground", SnapSelectedObjectToGround, 0 );
  mbar->GetMenu( "Edit" )->AddMenuItemSeperator( "Model copy options" );
  mbar->GetMenu( "Edit" )->AddMenuItemToggle( "Copy random rotation", &Settings::getInstance()->copy_rot, false  );
  mbar->GetMenu( "Edit" )->AddMenuItemToggle( "Copy random tilt", &Settings::getInstance()->copy_tile, false  );
  mbar->GetMenu( "Edit" )->AddMenuItemToggle( "Copy random size", &Settings::getInstance()->copy_size, false  );
  mbar->GetMenu( "Edit" )->AddMenuItemToggle( "Copy size and rotation", &Settings::getInstance()->copyModelStats, false  );

  mbar->GetMenu( "Edit" )->AddMenuItemSeperator( "Options" );
  mbar->GetMenu( "Edit" )->AddMenuItemToggle( "Auto select mode", &Settings::getInstance()->AutoSelectingMode, false );


  mbar->GetMenu( "Assist" )->AddMenuItemSeperator( "Model" );
  // mbar->GetMenu( "Assist" )->AddMenuItemButton( "All from MV", InsertObject, 0  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Last M2 from MV", InsertObject, 14  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Last WMO from MV", InsertObject, 15  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "From Text File", showImportModels, 1  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "To Text File", SaveObjecttoTXT, 1  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Helper models", showHelperModels, 2  );
  mbar->GetMenu( "Assist" )->AddMenuItemSeperator( "ADT" );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Set Area ID", adtSetAreaID, 0  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Clear height map", clearHeightmap, 0  );

  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Clear texture", clearTexture, 0  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Clear models", clearAllModels, 0  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Clear water", menuWater, 0  );
  mbar->GetMenu( "Assist" )->AddMenuItemButton( "Create water", menuWater, 1  );

  mbar->GetMenu( "View" )->AddMenuItemSeperator( "Windows" );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "Toolbar", mainGui->guiToolbar->hidden_evil(), true );
  // mbar->GetMenu( "View" )->AddMenuItemToggle( "Current texture", mainGui->SelectedTexture->hidden_evil(), true );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "Texture palette", mainGui->TexturePalette->hidden_evil(), true );
  mbar->GetMenu( "View" )->AddMenuItemButton( "Cursor options", showCursorSwitcher, 0);
  mbar->GetMenu( "View" )->AddMenuItemSeperator( "Toggle" );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F1 M2s", &gWorld->drawmodels );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F2 WMO doodadsets", &gWorld->drawdoodads );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F3 Terrain", &gWorld->drawterrain );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F4 Water", &gWorld->drawwater );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F6 WMOs", &gWorld->drawwmo );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F7 Lines", &gWorld->drawlines );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F8 Detail infos", mainGui->guidetailInfos->hidden_evil(), true );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F9 Map contour infos", &DrawMapContour );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "F Fog", &gWorld->drawfog );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "Hole lines always on", &Settings::getInstance()->holelinesOn, false );
  mbar->GetMenu( "View" )->AddMenuItemToggle( "Wireframe", &gWorld->drawwireframe );

  mbar->GetMenu( "Help" )->AddMenuItemButton( "Key Bindings", openHelp, 0 );
  mbar->GetMenu( "Help" )->AddMenuItemButton( "Manual online", openURL, 2 );
  mbar->GetMenu( "Help" )->AddMenuItemButton( "Homepage", openURL, 1 );

  mainGui->addChild( mbar );


  addHotkey( SDLK_ESCAPE, MOD_none,   static_cast<AppState::Function>( &MapView::quitask ) );
  addHotkey( SDLK_s,      MOD_ctrl,   static_cast<AppState::Function>( &MapView::save ) );
  addHotkey( SDLK_s,      MOD_meta,   static_cast<AppState::Function>( &MapView::save ) );

  // ESC warning
  mainGui->escWarning = new UIExitWarning(this);
  mainGui->escWarning->hide();
  mainGui->addChild( mainGui->escWarning );

  // CAPS warning
  mainGui->capsWarning = new UICapsWarning(this);
  mainGui->capsWarning->hide();
  mainGui->addChild( mainGui->capsWarning );

  // Water unable to save warning
  mainGui->waterSaveWarning = new UIWaterSaveWarning(this);
  mainGui->waterSaveWarning->hide();
  mainGui->addChild( mainGui->waterSaveWarning );

  // modelimport
  mainGui->ModelImport = new UIModelImport(this);
  mainGui->ModelImport->hide();
  mainGui->ModelImport->movable(true);
  mainGui->addChild( mainGui->ModelImport );

  // helper models
  mainGui->HelperModels = new UIHelperModels(this);
  mainGui->HelperModels->hide();
  mainGui->HelperModels->movable(true);
  mainGui->addChild( mainGui->HelperModels );
}

MapView::MapView( float ah0, float av0 )
  : ah( ah0 )
  , av( av0 )
  , _GUIDisplayingEnabled( true )
  , mTimespeed( 0.0f )
{
  LastClicked = NULL;

  moving = strafing = updown = lookat = turn = 0.0f;

  mousedir = -1.0f;

  movespd = SPEED;

  lastBrushUpdate = 0;
  textureBrush.init();

  look = false;
  mViewMode = eViewMode_3D;

  createGUI();
}

MapView::~MapView()
{
  delete mainGui;
  mainGui = NULL;

  delete gWorld;
  gWorld = NULL;
}

void MapView::tick( float t, float dt )
{
  dt = std::min( dt, 1.0f );
  
  // write some stuff into infos window for debuging
  std::stringstream appinfoText;
  appinfoText << "Project Path: " << Project::getInstance()->getPath() << std::endl;
  appinfoText << "Current cursor: " << Environment::getInstance()->cursorType << std::endl;
  mainGui->guiappInfo->setText( appinfoText.str() );
  
  if( SDL_GetAppState() & SDL_APPINPUTFOCUS )
  {
    Vec3D dir( 1.0f, 0.0f, 0.0f );
    Vec3D dirUp( 1.0f, 0.0f, 0.0f );
    Vec3D dirRight( 0.0f, 0.0f, 1.0f );
    rotate( 0.0f, 0.0f, &dir.x,&dir.y, av * PI / 180.0f );
    rotate( 0.0f, 0.0f, &dir.x,&dir.z, ah * PI / 180.0f );

    if( Environment::getInstance()->ShiftDown )
    {
      dirUp.x = 0.0f;
      dirUp.y = 1.0f;
      dirRight *= 0.0f; //! \todo  WAT?
    }
    else if( Environment::getInstance()->CtrlDown )
    {
      dirUp.x = 0.0f;
      dirUp.y = 1.0f;
      rotate( 0.0f, 0.0f, &dirUp.x, &dirUp.y, av * PI / 180.0f );
      rotate( 0.0f, 0.0f, &dirRight.x, &dirRight.y, av * PI / 180.0f );
      rotate( 0.0f, 0.0f, &dirUp.x, &dirUp.z, ah * PI / 180.0f );
      rotate( 0.0f, 0.0f, &dirRight.x, &dirRight.z, ah * PI / 180.0f );
    }
    else
    {
      rotate( 0.0f, 0.0f, &dirUp.x, &dirUp.z, ah * PI / 180.0f );
      rotate( 0.0f, 0.0f, &dirRight.x, &dirRight.z, ah * PI / 180.0f );
    }

    nameEntry * Selection = gWorld->GetCurrentSelection();


    if( Selection )
    {

      // Set move scale and rotate for numpad keys
      if(Environment::getInstance()->CtrlDown && Environment::getInstance()->ShiftDown)  moveratio=0.1f;
      else if(Environment::getInstance()->ShiftDown) moveratio=0.01f;
      else if(Environment::getInstance()->CtrlDown) moveratio=0.005f;
      else moveratio=0.001f;

      if( keyx != 0 || keyy != 0 || keyz != 0 || keyr != 0 || keys != 0)
      {
        // Move scale and rotate with numpad keys
        if( Selection->type == eEntry_WMO )
        {
          gWorld->mapIndex->setChanged(Selection->data.wmo->pos.x,Selection->data.wmo->pos.z);
          Selection->data.wmo->pos.x += keyx * moveratio;
          Selection->data.wmo->pos.y += keyy * moveratio;
          Selection->data.wmo->pos.z += keyz * moveratio;
          Selection->data.wmo->dir.y += keyr * moveratio * 5;

          Selection->data.wmo->recalcExtents();
          gWorld->mapIndex->setChanged(Selection->data.wmo->pos.x,Selection->data.wmo->pos.z);
        }

        if( Selection->type == eEntry_Model )
        {
          gWorld->mapIndex->setChanged(Selection->data.model->pos.x,Selection->data.model->pos.z);
          Selection->data.model->pos.x += keyx * moveratio;
          Selection->data.model->pos.y += keyy * moveratio;
          Selection->data.model->pos.z += keyz * moveratio;
          Selection->data.model->dir.y += keyr * moveratio * 5;
          Selection->data.model->sc += keys * moveratio / 50;
          gWorld->mapIndex->setChanged(Selection->data.model->pos.x,Selection->data.model->pos.z);
        }
      }

      Vec3D ObjPos;
      if( gWorld->IsSelection( eEntry_Model ) )
      {
        //! \todo  Tell me what this is.
        ObjPos = Selection->data.model->pos - gWorld->camera;
        rotate( 0.0f, 0.0f, &ObjPos.x, &ObjPos.y, av * PI / 180.0f );
        rotate( 0.0f, 0.0f, &ObjPos.x, &ObjPos.z, ah * PI / 180.0f );
        ObjPos.x = abs( ObjPos.x );
      }

      // moving and scaling objects
      //! \todo  Alternatively automatically align it to the terrain. Also try to move it where the mouse points.
      if( MoveObj )
      {
        if( Selection->type == eEntry_WMO )
        {
          gWorld->mapIndex->setChanged(Selection->data.wmo->pos.x,Selection->data.wmo->pos.z); // before move
          ObjPos.x = 80.0f;
          Selection->data.wmo->pos+=mv * dirUp * ObjPos.x;
          Selection->data.wmo->pos-=mh * dirRight * ObjPos.x;
          Selection->data.wmo->recalcExtents();
          gWorld->mapIndex->setChanged(Selection->data.wmo->pos.x,Selection->data.wmo->pos.z); // after move. If moved to another ADT
        }
        else if( Selection->type == eEntry_Model )
        {
          if( Environment::getInstance()->AltDown )
          {
            gWorld->mapIndex->setChanged(Selection->data.model->pos.x,Selection->data.model->pos.z);
            float ScaleAmount;

            ScaleAmount = pow( 2.0f, mv * 4.0f );
            Selection->data.model->sc *= ScaleAmount;
            if(Selection->data.model->sc > 63.9f )
              Selection->data.model->sc = 63.9f;
            else if (Selection->data.model->sc < 0.00098f )
              Selection->data.model->sc = 0.00098f;
          }
          else
          {
            gWorld->mapIndex->setChanged(Selection->data.model->pos.x,Selection->data.model->pos.z); // before move
            ObjPos.x = 80.0f;
            Selection->data.model->pos += mv * dirUp * ObjPos.x;
            Selection->data.model->pos -= mh * dirRight * ObjPos.x;
            gWorld->mapIndex->setChanged(Selection->data.model->pos.x,Selection->data.model->pos.z); // after move. If moved to another ADT
          }
        }
      }


      // rotating objects
      if( look )
      {
        float * lTarget = NULL;
        bool lModify = false;

        if( Selection->type == eEntry_Model )
        {
          gWorld->mapIndex->setChanged(Selection->data.model->pos.x,Selection->data.model->pos.z);
          lModify = Environment::getInstance()->ShiftDown | Environment::getInstance()->CtrlDown | Environment::getInstance()->AltDown;
          if( Environment::getInstance()->ShiftDown )
            lTarget = &Selection->data.model->dir.y;
          else if( Environment::getInstance()->CtrlDown )
            lTarget = &Selection->data.model->dir.x;
          else if(Environment::getInstance()->AltDown )
            lTarget = &Selection->data.model->dir.z;
        }
        else if( Selection->type == eEntry_WMO )
        {
          gWorld->mapIndex->setChanged(Selection->data.wmo->pos.x,Selection->data.wmo->pos.z);
          lModify = Environment::getInstance()->ShiftDown | Environment::getInstance()->CtrlDown | Environment::getInstance()->AltDown;
          if( Environment::getInstance()->ShiftDown )
            lTarget = &Selection->data.wmo->dir.y;
          else if( Environment::getInstance()->CtrlDown )
            lTarget = &Selection->data.wmo->dir.x;
          else if( Environment::getInstance()->AltDown )
            lTarget = &Selection->data.wmo->dir.z;
        }

        if( lModify && lTarget )
        {
          *lTarget = *lTarget + rh + rv;

          if( *lTarget > 360.0f )
            *lTarget = *lTarget - 360.0f;
          else if( *lTarget < -360.0f )
            *lTarget = *lTarget + 360.0f;

          if(Selection->type == eEntry_WMO)
            Selection->data.wmo->recalcExtents();
        }
      }

      mh = 0;
      mv = 0;
      rh = 0;
      rv = 0;


      if( leftMouse && Selection->type==eEntry_MapChunk )
      {
        float xPos, yPos, zPos;



        xPos = Environment::getInstance()->Pos3DX;
        yPos = Environment::getInstance()->Pos3DY;
        zPos = Environment::getInstance()->Pos3DZ;

        switch( terrainMode )
        {
          case 0:
            if( Environment::getInstance()->ShiftDown )
            {
              // Move ground up
              if( mViewMode == eViewMode_3D ) gWorld->changeTerrain( xPos, zPos, 7.5f * dt * groundBrushSpeed, groundBrushRadius, groundBrushType );
            }
            else if( Environment::getInstance()->CtrlDown )
            {
              // Move ground down
              if( mViewMode == eViewMode_3D ) gWorld->changeTerrain( xPos, zPos, -7.5f * dt * groundBrushSpeed, groundBrushRadius, groundBrushType );
            }
            break;

          case 1:
            if( Environment::getInstance()->ShiftDown )
              if( mViewMode == eViewMode_3D ) gWorld->flattenTerrain( xPos, zPos, yPos, pow( 0.5f, dt * groundBlurSpeed), blurBrushRadius, blurBrushType );
            if( Environment::getInstance()->CtrlDown )
            {
              if( mViewMode == eViewMode_3D ) gWorld->blurTerrain( xPos, zPos, pow( 0.5f, dt * groundBlurSpeed), std::min( blurBrushRadius, 30.0f ), blurBrushType );
            }
            break;

          case 2:
            if( Environment::getInstance()->ShiftDown && Environment::getInstance()->CtrlDown && Environment::getInstance()->AltDown)
            {
              // clear chunk texture
              if( mViewMode == eViewMode_3D )
                gWorld->eraseTextures(xPos, zPos);
              else if( mViewMode == eViewMode_2D )
                gWorld->eraseTextures( CHUNKSIZE * 4.0f * video.ratio() * ( static_cast<float>( MouseX ) / static_cast<float>( video.xres() ) - 0.5f ) / gWorld->zoom+gWorld->camera.x, CHUNKSIZE * 4.0f * ( static_cast<float>( MouseY ) / static_cast<float>( video.yres() ) - 0.5f) / gWorld->zoom+gWorld->camera.z );
            }
            else if( Environment::getInstance()->CtrlDown )
            {
              // Pick texture
              mainGui->TexturePicker->getTextures( gWorld->GetCurrentSelection());
            }
            else  if( Environment::getInstance()->ShiftDown)
            {
              // Paint 3d if shift down.
              if( UITexturingGUI::getSelectedTexture() )
              {
                if( textureBrush.needUpdate() )
                {
                  textureBrush.GenerateTexture();
                }
                if( mViewMode == eViewMode_3D )
                  if( mainGui->TextureSwitcher->hidden() )
                    gWorld->paintTexture( xPos, zPos, &textureBrush, brushLevel, 1.0f - pow( 1.0f - brushPressure, dt * 10.0f ), UITexturingGUI::getSelectedTexture() );
                  else
                    gWorld->overwriteTextureAtCurrentChunk(xPos,zPos, mainGui->TextureSwitcher->getTextures() ,UITexturingGUI::getSelectedTexture());
              }
            }
            else
            {
              // paint 2d if nothing is pressed.
              if( textureBrush.needUpdate() )
              {
                textureBrush.GenerateTexture();
              }
              if( mViewMode == eViewMode_2D && !(Environment::getInstance()->ShiftDown) && !(Environment::getInstance()->AltDown) )
                gWorld->paintTexture( CHUNKSIZE * 4.0f * video.ratio() * ( static_cast<float>( MouseX ) / static_cast<float>( video.xres() ) - 0.5f ) / gWorld->zoom+gWorld->camera.x, CHUNKSIZE * 4.0f * ( static_cast<float>( MouseY ) / static_cast<float>( video.yres() ) - 0.5f) / gWorld->zoom+gWorld->camera.z , &textureBrush, brushLevel, 1.0f - pow( 1.0f - brushPressure, dt * 10.0f ), UITexturingGUI::getSelectedTexture() );
            }
            break;

          case 3:
            if( Environment::getInstance()->ShiftDown  )
            {
              // if there is no terrain the projection method do not work. So get the cords by selection.
              Selection->data.mapchunk->getSelectionCoord( &xPos, &zPos );
              yPos = Selection->data.mapchunk->getSelectionHeight();

              if( Environment::getInstance()->AltDown )
              {
                if( mViewMode == eViewMode_3D )      gWorld->removeHole( xPos, zPos,true );
              }
              else
              {
                if( mViewMode == eViewMode_3D )      gWorld->removeHole( xPos, zPos,false);
              }
              //else if( mViewMode == eViewMode_2D )  gWorld->removeHole( CHUNKSIZE * 4.0f * video.ratio() * ( float( MouseX ) / float( video.xres() ) - 0.5f ) / gWorld->zoom+gWorld->camera.x, CHUNKSIZE * 4.0f * ( float( MouseY ) / float( video.yres() ) - 0.5f) / gWorld->zoom+gWorld->camera.z );
            }
            else if( Environment::getInstance()->CtrlDown )
            {
              if( Environment::getInstance()->AltDown )
              {
                if( mViewMode == eViewMode_3D )      gWorld->addHole( xPos, zPos,true );
              }
              else
              {
                if( mViewMode == eViewMode_3D )      gWorld->addHole( xPos, zPos,false);
              }
            }
            break;

          case 4:
            if( Environment::getInstance()->ShiftDown  )
            {
              if( mViewMode == eViewMode_3D )
              {
                // draw the selected AreaId on current selected chunk
                nameEntry * lSelection = gWorld->GetCurrentSelection();
                int mtx,mtz,mcx,mcy;
                mtx = lSelection->data.mapchunk->mt->mPositionX;
                mtz = lSelection->data.mapchunk->mt->mPositionZ ;
                mcx = lSelection->data.mapchunk->px;
                mcy = lSelection->data.mapchunk->py;
                gWorld->setAreaID( Environment::getInstance()->selectedAreaID, mtx,mtz, mcx, mcy );
              }
            }
            else if( Environment::getInstance()->CtrlDown )
            {
              if( mViewMode == eViewMode_3D )
              {
                // pick areaID from chunk
                int newID = gWorld->GetCurrentSelection()->data.mapchunk->getAreaID();
                Environment::getInstance()->selectedAreaID = newID;
                mainGui->ZoneIDBrowser->setZoneID(newID);
              }
            }

            break;

          case 5:
            if( Environment::getInstance()->ShiftDown  )
            {
              if( mViewMode == eViewMode_3D ) gWorld->mapIndex->setFlag( true, xPos, zPos );
            }
            else if( Environment::getInstance()->CtrlDown )
            {
              if( mViewMode == eViewMode_3D ) gWorld->mapIndex->setFlag( false, xPos, zPos );
            }
            break;
        }
      }
    }

    if( mViewMode != eViewMode_2D )
    {
      if(turn != 0.0f)
      {
        ah += turn;

        mainGui->minimapWindow->changePlayerLookAt(ah);
      }

      if(lookat)
      {
        av += lookat;

        if( av < -80.0f )
          av = -80.0f;
        else if ( av > 80.0f )
          av = 80.0f;

        mainGui->minimapWindow->changePlayerLookAt(ah);
      }

      if( moving )
        gWorld->camera += dir * dt * movespd * moving;

      if( strafing )
      {
        Vec3D right = dir % Vec3D( 0.0f, 1.0f ,0.0f );
        right.normalize();
        gWorld->camera += right * dt * movespd * strafing;
      }
      if( updown )
        gWorld->camera += Vec3D( 0.0f, dt * movespd * updown, 0.0f );

      gWorld->lookat = gWorld->camera + dir;
    }
    else
    {
      if( moving )
        gWorld->camera.z -= dt * movespd * moving / ( gWorld->zoom * 1.5f );
      if( strafing )
        gWorld->camera.x += dt * movespd * strafing / ( gWorld->zoom * 1.5f );
      if( updown )
        gWorld->zoom *= pow( 2.0f, dt * updown * 4.0f );

      gWorld->zoom = std::min( std::max( gWorld->zoom, 0.1f ), 2.0f );
    }
  }
  else
  {
    leftMouse = false;
    rightMouse = false;
    look = false;
    MoveObj = false;

    moving = 0;
    strafing = 0;
    updown = 0;
  }

  if( ( t - lastBrushUpdate ) > 0.1f && textureBrush.needUpdate() )
  {
    textureBrush.GenerateTexture();
  }

  gWorld->time += this->mTimespeed * dt;
  

  gWorld->animtime += dt * 1000.0f;
  globalTime = static_cast<int>( gWorld->animtime );

  gWorld->tick(dt);

  if( !MapChunkWindow->hidden() && gWorld->GetCurrentSelection() && gWorld->GetCurrentSelection()->type == eEntry_MapChunk )
  {
    UITexturingGUI::setChunkWindow( gWorld->GetCurrentSelection()->data.mapchunk );
  }
}

void MapView::doSelection( bool selectTerrainOnly )
{
  gWorld->drawSelection( MouseX, MouseY, selectTerrainOnly );
}


void MapView::displayGUIIfEnabled()
{
  if( _GUIDisplayingEnabled )
  {
    video.set2D();

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    OpenGL::Texture::disableTexture( 1 );
    OpenGL::Texture::enableTexture( 0 );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glDisable( GL_LIGHTING );
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    OpenGL::Texture::disableTexture( 0 );

    mainGui->setTilemode( mViewMode != eViewMode_3D );
    mainGui->render();

    OpenGL::Texture::enableTexture( 0 );
  }
}

void MapView::displayViewMode_2D( float /*t*/, float /*dt*/ )
{
  video.setTileMode();
  gWorld->drawTileMode( ah );

  const float mX = ( CHUNKSIZE * 4.0f * video.ratio() * ( static_cast<float>( MouseX ) / static_cast<float>( video.xres() ) - 0.5f ) / gWorld->zoom + gWorld->camera.x ) / CHUNKSIZE;
  const float mY = ( CHUNKSIZE * 4.0f * ( static_cast<float>( MouseY ) / static_cast<float>( video.yres() ) - 0.5f ) / gWorld->zoom + gWorld->camera.z ) / CHUNKSIZE;

  // draw brush
  glPushMatrix();
  glScalef(gWorld->zoom,gWorld->zoom,1.0f);
  glTranslatef(-gWorld->camera.x/CHUNKSIZE,-gWorld->camera.z/CHUNKSIZE,0);

  glColor4f(1.0f,1.0f,1.0f,0.5f);
  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);

  textureBrush.getTexture()->bind();

  const float tRadius = textureBrush.getRadius()/CHUNKSIZE;// *gWorld->zoom;
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f,0.0f);
  glVertex3f(mX-tRadius,mY+tRadius,0);
  glTexCoord2f(1.0f,0.0f);
  glVertex3f(mX+tRadius,mY+tRadius,0);
  glTexCoord2f(1.0f,1.0f);
  glVertex3f(mX+tRadius,mY-tRadius,0);
  glTexCoord2f(0.0f,1.0f);
  glVertex3f(mX-tRadius,mY-tRadius,0);
  glEnd();
  glPopMatrix();

  displayGUIIfEnabled();
}

void MapView::displayViewMode_3D( float /*t*/, float /*dt*/ )
{
  //! \note Select terrain below mouse, if no item selected or the item is map.
  if( !gWorld->IsSelection( eEntry_Model ) && !gWorld->IsSelection( eEntry_WMO ) && Settings::getInstance()->AutoSelectingMode )
  {
    doSelection( true );
  }

  video.set3D();

  gWorld->draw();

  displayGUIIfEnabled();
}

void MapView::display( float t, float dt )
{
  //! \todo  Get this out or do it somehow else. This is ugly and is a senseless if each draw.
  if( Saving )
  {
    video.setTileMode();
    gWorld->saveMap();
    Saving=false;
  }

  switch( mViewMode )
  {
    case eViewMode_2D:
      displayViewMode_2D( t, dt );
      break;

    case eViewMode_3D:
      displayViewMode_3D( t, dt );
      break;
  }
}

void MapView::save()
{
  gWorld->mapIndex->saveChanged();
  //ConfigFile::add("RedColor", RedColor);
  //ConfigFile::add("GreenColor", GreenColor);
  //ConfigFile::add("BlueColor", BlueColor);
  //ConfigFile::add("AlphaColor", AlphaColor);
}

void MapView::quit()
{
  app.pop = true;
}

void MapView::quitask()
{
  mainGui->escWarning->show();
}

void MapView::resizewindow()
{
  mainGui->resize();
}

void MapView::keypressed( SDL_KeyboardEvent *e )
{
  if( LastClicked && LastClicked->KeyBoardEvent( e ) ) return;

  if(e->keysym.mod & KMOD_CAPS)
    mainGui->capsWarning->show();
  else
    mainGui->capsWarning->hide();

  if( e->type == SDL_KEYDOWN )
  {


    if( handleHotkeys( e ) )
      return;

    // key DOWN

    if( e->keysym.sym == SDLK_LSHIFT || e->keysym.sym == SDLK_RSHIFT )
      Environment::getInstance()->ShiftDown = true;

    if( e->keysym.sym == SDLK_LALT || e->keysym.sym == SDLK_RALT )
      Environment::getInstance()->AltDown = true;

    if( e->keysym.sym == SDLK_LCTRL || e->keysym.sym == SDLK_RCTRL )
      Environment::getInstance()->CtrlDown = true;

    if( e->keysym.sym == SDLK_SPACE)
      Environment::getInstance()->SpaceDown = true;

    // movement
    if( e->keysym.sym == SDLK_w )
    {
      key_w = true;
      moving = 1.0f;
    }

    if ( e->keysym.sym == SDLK_UP )
    {
      lookat = 0.75f;
    }

    if ( e->keysym.sym ==SDLK_DOWN )
    {
      lookat = -0.75f;
    }

    if ( e->keysym.sym == SDLK_LEFT )
    {
      turn =- 0.75f;
    }

    if ( e->keysym.sym == SDLK_RIGHT )
    {
      turn = 0.75f;
    }

    // save
    if( e->keysym.sym == SDLK_s )
      moving = -1.0f;

    if( e->keysym.sym == SDLK_a )
      strafing = -1.0f;

    if( e->keysym.sym == SDLK_d )
      strafing = 1.0f;

    if( e->keysym.sym == SDLK_e )
      updown = -1.0f;

    if( e->keysym.sym == SDLK_q )
      updown = 1.0f;

    // position correction with num pad
    if( e->keysym.sym == SDLK_KP8 )
      keyx = -1;

    if( e->keysym.sym == SDLK_KP2 )
      keyx = 1;

    if( e->keysym.sym == SDLK_KP6 )
      keyz = -1;

    if( e->keysym.sym == SDLK_KP4 )
      keyz = 1;

    if( e->keysym.sym == SDLK_KP1 )
      keyy = -1;

    if( e->keysym.sym == SDLK_KP3 )
      keyy = 1;

    if( e->keysym.sym == SDLK_KP7 )
      keyr = 1;

    if( e->keysym.sym == SDLK_KP9 )
      keyr = -1;

    if( e->keysym.sym == SDLK_KP0 )
      if(terrainMode==6)
      {
        gWorld->setWaterHeight(misc::FtoIround((gWorld->camera.x-(TILESIZE/2))/TILESIZE),misc::FtoIround((gWorld->camera.z-(TILESIZE/2))/TILESIZE),0.0f);
        mainGui->guiWater->updateData();
      }


    // delete object
    if( e->keysym.sym == SDLK_DELETE )
      DeleteSelectedObject( 0, 0 );

    // copy model to clipboard
    if( e->keysym.sym == SDLK_c )
    {
      if(Environment::getInstance()->CtrlDown )
        CopySelectedObject( 0, 0 );
      else if(Environment::getInstance()->AltDown && Environment::getInstance()->CtrlDown)
        mainGui->toggleCursorSwitcher();
      else if(Environment::getInstance()->ShiftDown)
        InsertObject(0, 14);
      else if(Environment::getInstance()->AltDown)
        InsertObject(0, 15);
      else
      {
        Environment::getInstance()->cursorType++;
        if (Environment::getInstance()->cursorType>3) Environment::getInstance()->cursorType = 0;
      }
    }


    // paste model
    if( e->keysym.sym == SDLK_v )
    {
      if(Environment::getInstance()->ShiftDown)
      {
        InsertObject(0,14);
      }
      else if(Environment::getInstance()->AltDown)
      {
        InsertObject(0,15);
      }
      else if(Environment::getInstance()->CtrlDown)
      {
        PasteSelectedObject( 0, 0 );
      }
    }


    // with ctrl toggle detail window
    // without toggle the settings of the current edit mode.
    if( e->keysym.sym == SDLK_x )
    {
      if( Environment::getInstance()->CtrlDown )
      {
        // toggle detail window
        mainGui->guidetailInfos->toggleVisibility();
      }
      else
      {
        // toggle terrainMode window
        if(terrainMode==2)
          view_texture_palette( 0, 0 );
        else if(terrainMode==4)
          mainGui->ZoneIDBrowser->toggleVisibility();
      }
    }

    // invert mouse or swap paint modes
    if( e->keysym.sym == SDLK_i )
    {
      if( Environment::getInstance()->CtrlDown  )
      {
        // temp till fixe draw texture.
        if(Environment::getInstance()->paintMode) Environment::getInstance()->paintMode = false;
        else Environment::getInstance()->paintMode = true;
      }
      else
      {
        mousedir *= -1.0f;
      }

    }
    // move speed doubling or raw saving
    if( e->keysym.sym == SDLK_p )
      if( Environment::getInstance()->CtrlDown && Environment::getInstance()->ShiftDown )
        Saving = true;
      else
        movespd *= 2.0f;

    if( e->keysym.sym == SDLK_o )
      movespd *= 0.5f;

    // turn around or reset object orientation
    if( e->keysym.sym == SDLK_r )
    {
      if( Environment::getInstance()->CtrlDown )
        ResetSelectedObjectRotation( 0, 0 );
      else ah += 180.0f;
    }

    // Test windows
    if( e->keysym.sym == SDLK_t )
    {
      mainGui->toggleTest();
    }

    // clip object to ground
    if( e->keysym.sym == SDLK_PAGEDOWN )
      SnapSelectedObjectToGround( 0, 0 );

    // speed of daytime.
    if( e->keysym.sym == SDLK_n )
      this->mTimespeed += 90.0f;

    if( e->keysym.sym == SDLK_b )
      this->mTimespeed -= 90.0f;

    // no negativ time speed!
    if(this->mTimespeed < 0.0f )  this->mTimespeed = 0.0f;

    if( e->keysym.sym == SDLK_j )
      this->mTimespeed = 0.0f;



    // toggle lightning
    if( e->keysym.sym == SDLK_l )
      gWorld->lighting = !gWorld->lighting;

    // toggle interface
    if( e->keysym.sym == SDLK_TAB )
      _GUIDisplayingEnabled = !_GUIDisplayingEnabled;

    // toggle "terrain texturing mode" / draw models
    if( e->keysym.sym == SDLK_F1 )
      if( Environment::getInstance()->ShiftDown )
      {
        if( alloff )
        {
          alloff_models = gWorld->drawmodels;
          alloff_doodads = gWorld->drawdoodads;
          alloff_contour = DrawMapContour;
          alloff_wmo = gWorld->drawwmo;
          alloff_fog = gWorld->drawfog;
          alloff_terrain = gWorld->drawterrain;

          gWorld->drawmodels = false;
          gWorld->drawdoodads = false;
          DrawMapContour = true;
          gWorld->drawwmo = false;
          gWorld->drawterrain = true;
          gWorld->drawfog = false;
        }
        else
        {
          gWorld->drawmodels = alloff_models;
          gWorld->drawdoodads = alloff_doodads;
          DrawMapContour = alloff_contour;
          gWorld->drawwmo = alloff_wmo;
          gWorld->drawterrain = alloff_terrain;
          gWorld->drawfog = alloff_fog;
        }
        alloff = !alloff;
      }
      else
        gWorld->drawmodels = !gWorld->drawmodels;


    // toggle drawing of doodads in WMOs.
    if( e->keysym.sym == SDLK_F2 )
      gWorld->drawdoodads = !gWorld->drawdoodads;

    // toggle terrain
    if( e->keysym.sym == SDLK_F3 )
      gWorld->drawterrain = !gWorld->drawterrain;

    // toggle better selection mode
    if( e->keysym.sym == SDLK_F4 && Environment::getInstance()->ShiftDown )
    {
      Settings::getInstance()->AutoSelectingMode = !Settings::getInstance()->AutoSelectingMode;
    }

    // toggle draw water
    if( e->keysym.sym == SDLK_F4 && !Environment::getInstance()->ShiftDown )
      gWorld->drawwater = !gWorld->drawwater;

    // toggle chunk limitation lines
    if( e->keysym.sym == SDLK_F7 )
      if( Environment::getInstance()->ShiftDown )
      {
        Environment::getInstance()->view_holelines = !Environment::getInstance()->view_holelines;
      }
      else
        gWorld->drawlines = !gWorld->drawlines;

    // toggle drawing of WMOs
    if( e->keysym.sym == SDLK_F6 )
      gWorld->drawwmo = !gWorld->drawwmo;

    // toggle showing a lot of information about selected item
    if( e->keysym.sym == SDLK_F8 )
    {
      mainGui->guidetailInfos->toggleVisibility();
    }

    // toggle height contours on terrain
    if( e->keysym.sym == SDLK_F9 )
      DrawMapContour = !DrawMapContour;

    // toggle help window
    if( e->keysym.sym == SDLK_h )
    {
      mainGui->toggleHelp();
    }

    // draw fog
    if( e->keysym.sym == SDLK_f )
      gWorld->drawfog = !gWorld->drawfog;

    // reload a map tile STEFF out because of UID recalc. reload could kill all.
    //if( e->keysym.sym == SDLK_j && Environment::getInstance()->ShiftDown )
    //  gWorld->reloadTile( static_cast<int>( gWorld->camera.x ) / TILESIZE, static_cast<int>( gWorld->camera.z ) / TILESIZE );

    // fog distance or brush radius
    if( e->keysym.sym == SDLK_KP_PLUS || e->keysym.sym == SDLK_PLUS )
      if( Environment::getInstance()->AltDown )
      {
        switch( terrainMode )
        {
          case 0:
            groundBrushRadius += 0.01f;
            if( groundBrushRadius > 1000.0f )
              groundBrushRadius = 1000.0f;
            else if( groundBrushRadius < 0.0005f )
              groundBrushRadius = 0.0005f;
            ground_brush_radius->setValue( groundBrushRadius / 1000 );
            break;
          case 1:
            blurBrushRadius += 0.01f;
            if( blurBrushRadius > 1000.0f )
              blurBrushRadius = 1000.0f;
            else if( blurBrushRadius < 0.01f )
              blurBrushRadius = 0.01f;
            blur_brush->setValue( blurBrushRadius / 1000 );
            break;
          case 2:
            textureBrush.setRadius( textureBrush.getRadius() + 0.1f );
            if( textureBrush.getRadius() > 100.0f )
              textureBrush.setRadius(100.0f);
            else if( textureBrush.getRadius() < 0.1f )
              textureBrush.setRadius(0.1f);
            paint_brush->setValue( textureBrush.getRadius() / 100 );
            break;
          case 6:
            int x = static_cast<int>( gWorld->camera.x ) / TILESIZE;
            int z = static_cast<int>( gWorld->camera.z ) / TILESIZE;
            gWorld->setWaterHeight(x, z, std::ceil(gWorld->getWaterHeight(x, z) + 2.0f));
            mainGui->guiWater->updateData();
            break;
        }
      }
      else if((!gWorld->HasSelection() || ( gWorld->HasSelection() && gWorld->GetCurrentSelection()->type == eEntry_MapChunk)))
      {
        if(terrainMode == 6)
        {
          int x = static_cast<int>( gWorld->camera.x ) / TILESIZE;
          int z = static_cast<int>( gWorld->camera.z ) / TILESIZE;

          if(Environment::getInstance()->ShiftDown)
            gWorld->setWaterTrans(x, z,  static_cast<unsigned char>(std::ceil(static_cast<float>(gWorld->getWaterTrans(x, z)) + 1)));
          else if(Environment::getInstance()->CtrlDown)
            gWorld->setWaterHeight(x, z, std::ceil(gWorld->getWaterHeight(x, z) + 5.0f));
          else
            gWorld->setWaterHeight(x, z, std::ceil(gWorld->getWaterHeight(x, z) + 1.0f));

          mainGui->guiWater->updateData();
        }
        else if(Environment::getInstance()->ShiftDown)
        {
          gWorld->fogdistance += 60.0f;// fog change only when no model is selected!
        }
      }
      else
      {
        //change selected model size
        keys=1;
      }

    if( e->keysym.sym == SDLK_KP_MINUS || e->keysym.sym == SDLK_MINUS )
      if( Environment::getInstance()->AltDown )
      {
        switch( terrainMode )
        {
          case 0:
            groundBrushRadius -= 0.01f;
            if( groundBrushRadius > 1000.0f )
              groundBrushRadius = 1000.0f;
            else if( groundBrushRadius < 0.0005f )
              groundBrushRadius = 0.0005f;
            ground_brush_radius->setValue( groundBrushRadius / 1000 );
            break;
          case 1:
            blurBrushRadius -= 0.01f;
            if( blurBrushRadius > 1000.0f )
              blurBrushRadius = 1000.0f;
            else if( blurBrushRadius < 0.01f )
              blurBrushRadius = 0.01f;
            blur_brush->setValue( blurBrushRadius / 1000 );
            break;
          case 2:
            textureBrush.setRadius( textureBrush.getRadius() - 0.1f );
            if( textureBrush.getRadius() > 100.0f )
              textureBrush.setRadius(100.0f);
            else if( textureBrush.getRadius() < 0.1f )
              textureBrush.setRadius(0.1f);
            paint_brush->setValue( textureBrush.getRadius() / 100 );
            break;
          case 6:
            int x = static_cast<int>( gWorld->camera.x ) / TILESIZE;
            int z = static_cast<int>( gWorld->camera.z ) / TILESIZE;
            gWorld->setWaterHeight(x, z, std::ceil(gWorld->getWaterHeight(x, z) - 2.0f));
            mainGui->guiWater->updateData();
            break;
        }
      }
      else if((!gWorld->HasSelection() || ( gWorld->HasSelection() && gWorld->GetCurrentSelection()->type == eEntry_MapChunk)))
      {
        if(terrainMode == 6)
        {
          int x = static_cast<int>( gWorld->camera.x ) / TILESIZE;
          int z = static_cast<int>( gWorld->camera.z ) / TILESIZE;
          if(Environment::getInstance()->ShiftDown)
            gWorld->setWaterTrans(x, z, static_cast<unsigned char>(std::floor(static_cast<float>(gWorld->getWaterTrans(x, z))) - 1) );
          else if(Environment::getInstance()->CtrlDown)
            gWorld->setWaterHeight(x, z, std::ceil(gWorld->getWaterHeight(x, z) - 5.0f));
          else
            gWorld->setWaterHeight(x, z, std::ceil(gWorld->getWaterHeight(x, z) - 1.0f));

          mainGui->guiWater->updateData();
        }
        else if(Environment::getInstance()->ShiftDown)
        {
          gWorld->fogdistance -= 60.0f;// fog change only when no model is selected!
        }

      }
      else
      {
        //change selected model size
        keys=-1;
      }

    // minimap
    if( e->keysym.sym == SDLK_m )
      mainGui->minimapWindow->toggleVisibility();

    if( e->keysym.sym == SDLK_g )
    {
      // write teleport cords to txt file
      std::ofstream f( "ports.txt", std::ios_base::app );
      f << std::endl << std::endl << "Map: " << gAreaDB.getAreaName( gWorld->getAreaID() ) << " on ADT " << std::floor( gWorld->camera.x / TILESIZE ) << " " <<  std::floor( gWorld->camera.z / TILESIZE ) << std::endl << std::endl;
      f << "Trinity:" << std::endl << ".go " << (ZEROPOINT - gWorld->camera.z) << " " << (ZEROPOINT - gWorld->camera.x) << " " << gWorld->camera.y << " " << gWorld->getMapID() << std::endl << std::endl;
      f << "ArcEmu:" << std::endl << ".worldport " << gWorld->getMapID() << " " << (ZEROPOINT - gWorld->camera.z) << " " << (ZEROPOINT - gWorld->camera.x) << " " << gWorld->camera.y << " " << std::endl << std::endl;
      f.close();
    }


    // toogle between smooth / flat / linear
    if( e->keysym.sym == SDLK_y )
      switch( terrainMode )
      {
        case 0:
          groundBrushType++;
          groundBrushType = groundBrushType % 6;
          gGroundToggleGroup->Activate( groundBrushType );
          break;

        case 1:
          blurBrushType++;
          blurBrushType = blurBrushType % 3;
          gBlurToggleGroup->Activate( blurBrushType );
          break;
      }

    // is not used somewere else!!
    //! \todo  what is this?
    if( e->keysym.sym == SDLK_g )
      drawFlags = !drawFlags;

    // toogle tile mode
    if( e->keysym.sym == SDLK_u )
    {
      if( mViewMode == eViewMode_2D )
      {
        mViewMode = eViewMode_3D;
        terrainMode = saveterrainMode;
        // Set the right icon in toolbar
        mainGui->guiToolbar->IconSelect( terrainMode );
      }
      else
      {
        mViewMode = eViewMode_2D;
        saveterrainMode = terrainMode;
        terrainMode = 2;
        // Set the right icon in toolbar
        mainGui->guiToolbar->IconSelect( terrainMode );
      }
    }

    // doodads set
    if( e->keysym.sym >= SDLK_0 && e->keysym.sym <= SDLK_9 )
    {
      if( gWorld->IsSelection( eEntry_WMO ) )
      {
        gWorld->GetCurrentSelection()->data.wmo->doodadset = e->keysym.sym - SDLK_0;
      }
      else if( Environment::getInstance()->ShiftDown )
      {
        switch( e->keysym.sym )
        {
          case SDLK_1:
            movespd = 15.0f;
            break;

          case SDLK_2:
            movespd = 50.0f;
            break;

          case SDLK_3:
            movespd = 200.0f;
            break;

          case SDLK_4:
            movespd = 800.0f;
            break;
        }
      }
      else if( Environment::getInstance()->AltDown )
      {
        switch( e->keysym.sym )
        {
          case SDLK_1:
            mainGui->G1->setValue(0.01f);
            break;

          case SDLK_2:
            mainGui->G1->setValue(0.25f);
            break;

          case SDLK_3:
            mainGui->G1->setValue(0.50f);
            break;

          case SDLK_4:
            mainGui->G1->setValue(0.75f);
            break;

          case SDLK_5:
            mainGui->G1->setValue(0.99f);
            break;
        }
      }
      else if( e->keysym.sym >= SDLK_1 && e->keysym.sym <= SDLK_8 )
      {
        terrainMode = e->keysym.sym - SDLK_1;
        mainGui->guiToolbar->IconSelect( terrainMode );
      }
    }

    // add a new bookmark
    if( e->keysym.sym == SDLK_F5 )
    {
      std::ofstream f( "bookmarks.txt", std::ios_base::app );
      f << gWorld->getMapID() << " " << gWorld->camera.x << " " << gWorld->camera.y << " " << gWorld->camera.z << " " << ah << " " << av << " " << gWorld->getAreaID() << std::endl;
      f.close();
    }
  }
  else
  {
    // key UP
    if( e->keysym.sym == SDLK_LSHIFT || e->keysym.sym == SDLK_RSHIFT )
      Environment::getInstance()->ShiftDown = false;

    if( e->keysym.sym == SDLK_LALT || e->keysym.sym == SDLK_RALT )
      Environment::getInstance()->AltDown = false;

    if( e->keysym.sym == SDLK_LCTRL || e->keysym.sym == SDLK_RCTRL )
      Environment::getInstance()->CtrlDown = false;

    if( e->keysym.sym == SDLK_SPACE )
      Environment::getInstance()->SpaceDown = false;

    // movement
    if( e->keysym.sym == SDLK_w)
    {
      key_w = false;
      if( !(leftMouse && rightMouse) && moving > 0.0f) {

        moving = 0.0f;
      }
    }

    if( e->keysym.sym == SDLK_s && moving < 0.0f ){

      moving = 0.0f;
    }

    if ( e->keysym.sym == SDLK_UP || e->keysym.sym == SDLK_DOWN)
      lookat=0.0f;

    if ( e->keysym.sym == SDLK_LEFT || e->keysym.sym == SDLK_RIGHT )
      turn = 0.0f;

    if( e->keysym.sym == SDLK_d && strafing > 0.0f ){

      strafing = 0.0f;
    }

    if( e->keysym.sym == SDLK_a && strafing < 0.0f ){

      strafing = 0.0f;
    }

    if( e->keysym.sym == SDLK_q && updown > 0.0f )
      updown = 0.0f;

    if( e->keysym.sym == SDLK_e && updown < 0.0f )
      updown = 0.0f;

    if( e->keysym.sym == SDLK_KP8 )
      keyx = 0;

    if( e->keysym.sym == SDLK_KP2 )
      keyx = 0;

    if( e->keysym.sym == SDLK_KP6 )
      keyz = 0;

    if( e->keysym.sym == SDLK_KP4 )
      keyz = 0;

    if( e->keysym.sym == SDLK_KP1 )
      keyy = 0;

    if( e->keysym.sym == SDLK_KP3 )
      keyy = 0;

    if( e->keysym.sym == SDLK_KP7 )
      keyr = 0;

    if( e->keysym.sym == SDLK_KP9 )
      keyr = 0;

    if( e->keysym.sym == SDLK_KP_MINUS || e->keysym.sym == SDLK_MINUS || e->keysym.sym == SDLK_KP_PLUS || e->keysym.sym == SDLK_PLUS)
      keys = 0;
  }
}

void MapView::inserObjectFromExtern(int model  )
{
  InsertObject(0, model);
}


void MapView::mousemove( SDL_MouseMotionEvent *e )
{
  mainGui->minimapWindow->mousemove(e);
  if ( ( look && !( Environment::getInstance()->ShiftDown || Environment::getInstance()->CtrlDown || Environment::getInstance()->AltDown ) ) || video.fullscreen() )
  {
    ah += e->xrel / XSENS;
    av += mousedir * e->yrel / YSENS;
    if( av < -80.0f )
      av = -80.0f;
    else if ( av > 80.0f )
      av = 80.0f;

    mainGui->minimapWindow->changePlayerLookAt(ah);
  }

  if( MoveObj )
  {
    mh = -video.ratio()*e->xrel / static_cast<float>( video.xres() );
    mv = -e->yrel / static_cast<float>( video.yres() );
  }
  else
  {
    mh = 0.0f;
    mv = 0.0f;
  }

  if( Environment::getInstance()->ShiftDown || Environment::getInstance()->CtrlDown || Environment::getInstance()->AltDown )
  {
    rh = e->xrel / XSENS * 5.0f;
    rv = e->yrel / YSENS * 5.0f;
  }

  if( leftMouse && Environment::getInstance()->AltDown )
  {
    switch( terrainMode )
    {
      case 0:
        groundBrushRadius += e->xrel / XSENS;
        if( groundBrushRadius > 1000.0f )
          groundBrushRadius = 1000.0f;
        else if( groundBrushRadius < 0.01f )
          groundBrushRadius = 0.01f;
        ground_brush_radius->setValue( groundBrushRadius / 1000.0f );
        break;
      case 1:
        blurBrushRadius += e->xrel / XSENS;
        if( blurBrushRadius > 1000.0f )
          blurBrushRadius = 1000.0f;
        else if( blurBrushRadius < 0.01f )
          blurBrushRadius = 0.01f;
        blur_brush->setValue( blurBrushRadius / 1000.0f );
        break;
      case 2:
        textureBrush.setRadius( textureBrush.getRadius() + e->xrel / XSENS );
        if( textureBrush.getRadius() > 100.0f )
          textureBrush.setRadius(100.0f);
        else if( textureBrush.getRadius() < 0.1f )
          textureBrush.setRadius(0.1f);
        paint_brush->setValue( textureBrush.getRadius() / 100.0f );
        break;
    }
  }

  if( leftMouse && Environment::getInstance()->SpaceDown )
  {
    switch( terrainMode )
    {
      case 0:
        groundBrushSpeed += e->xrel / 30.0f;
        if( groundBrushSpeed > 10.0f )
          groundBrushSpeed = 10.0f;
        else if( groundBrushSpeed < 0.01f )
          groundBrushSpeed = 0.01f;
        ground_brush_speed->setValue( groundBrushSpeed / 10.0f );
        break;
      case 1:
        groundBlurSpeed += e->xrel / 30.0f;
        if( groundBlurSpeed > 10.0f )
          groundBlurSpeed = 10.0f;
        else if( groundBlurSpeed < 0.01f )
          groundBlurSpeed = 0.01f;
        ground_blur_speed->setValue( groundBlurSpeed / 10.0f );
        break;
      case 2:
        mainGui->S1->setValue(  mainGui->S1->value + e->xrel / 300.0f );
        if(  mainGui->S1->value > 1.0f )
          mainGui->S1->setValue(1.0f);
        else if(  mainGui->S1->value < 0.0001f )
          mainGui->S1->setValue(0.0001f);
        mainGui->S1->setValue(  mainGui->S1->value );
        break;
    }
  }

  if( leftMouse && LastClicked )
  {
    LastClicked->processLeftDrag( e->x - 4, e->y - 4, e->xrel, e->yrel );
  }

  if( mViewMode == eViewMode_2D && leftMouse &&  Environment::getInstance()->ShiftDown  )
  {
    strafing = ((e->xrel / XSENS) / -1) * 5.0f;
    moving = (e->yrel / YSENS) * 5.0f;
  }

  if( mViewMode == eViewMode_2D && rightMouse && Environment::getInstance()->ShiftDown  )
  {
    updown = (e->yrel / YSENS);
  }


  
  Environment::getInstance()->screenX = MouseX = e->x;
  Environment::getInstance()->screenY = MouseY = e->y;
  checkWaterSave();
}

void MapView::addModelFromTextSelection( int id )
{
  InsertObject( 0, id );
}

void MapView::mouseclick( SDL_MouseButtonEvent *e )
{
  if( e->type == SDL_MOUSEBUTTONDOWN )
  {
    switch( e->button )
    {
      case SDL_BUTTON_LEFT:
        leftMouse = true;
        break;

      case SDL_BUTTON_RIGHT:
        rightMouse = true;
        break;

      case SDL_BUTTON_MIDDLE:
        if( gWorld->HasSelection() )
          MoveObj = true;
        break;
    }

    if (leftMouse && rightMouse)
    {
      // Both buttons
      moving = 1.0f;
    }
    else if (leftMouse)
    {
      LastClicked = mainGui->processLeftClick( static_cast<float>( MouseX ), static_cast<float>( MouseY ) );
      if( mViewMode == eViewMode_3D && !LastClicked )
      {
        doSelection( false );
      }
    }
    else if (rightMouse)
    {
      look = true;
    }
  }
  else if( e->type == SDL_MOUSEBUTTONUP )
  {
    switch( e->button )
    {
      case SDL_BUTTON_LEFT:
        leftMouse = false;

        if( LastClicked )
          LastClicked->processUnclick();

        if(!key_w && moving > 0.0f )
          moving = 0.0f;

        if( mViewMode == eViewMode_2D )
        {
          strafing = 0;
          moving = 0;
        }
        break;

      case SDL_BUTTON_RIGHT:
        rightMouse = false;

        look = false;

        if(!key_w && moving > 0.0f )
          moving = 0.0f;

        if( mViewMode == eViewMode_2D )
          updown = 0;

        break;

      case SDL_BUTTON_MIDDLE:
        MoveObj = false;
        break;
    }
  }

  // check menu settings and switch hole mode
  //! \todo why the hell is this here?
  if(terrainMode!=3)
  {
    Environment::getInstance()->view_holelines = Settings::getInstance()->holelinesOn;
  }
}

void MapView::checkWaterSave(){
  if(gWorld->canWaterSave(static_cast<int>( gWorld->camera.x ) / TILESIZE, static_cast<int>( gWorld->camera.z ) / TILESIZE))
    mainGui->waterSaveWarning->hide();
  else
    mainGui->waterSaveWarning->show();
}
