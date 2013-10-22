// UITexture.cpp is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>
// Stephan Biegel <project.modcraft@googlemail.com>
// Tigurius <bstigurius@googlemail.com>

#include <noggit/UITexture.h>

#include <string>

#include <noggit/blp_texture.h>
#include <noggit/TextureManager.h> // TextureManager

UITexture::UITexture( float xPos, float yPos, float w, float h, const std::string& tex )
: UIFrame( xPos, yPos, w, h )
, texture( TextureManager::newTexture( tex ) )
, _textureFilename( tex )
, highlight( false )
, clickFunc( NULL )
, id( 0 )
{
}

UITexture::~UITexture()
{
  if( texture )
  {
    TextureManager::delbyname( _textureFilename );
    texture = NULL;
  }
}

void UITexture::setTexture( noggit::blp_texture* tex )
{
  //! \todo Free current texture.
  //! \todo New reference?
  texture = tex;
}

void UITexture::setTexture( const std::string& textureFilename )
{
  if( texture )
  {
    TextureManager::delbyname( _textureFilename );
    texture = NULL;
  }
  _textureFilename = textureFilename;
  texture = TextureManager::newTexture( textureFilename );
}

noggit::blp_texture* UITexture::getTexture( )
{
  return texture;
}

void UITexture::render() const
{
  glPushMatrix();
  glTranslatef( x(), y(), 0.0f );

  glColor3f( 1.0f, 1.0f, 1.0f );

  opengl::texture::enable_texture (0);

  texture->bind();

  glBegin( GL_TRIANGLE_STRIP );
  glTexCoord2f( 0.0f, 0.0f );
  glVertex2f( 0.0f, 0.0f );
  glTexCoord2f( 1.0f, 0.0f );
  glVertex2f( width(), 0.0f );
  glTexCoord2f( 0.0f, 1.0f );
  glVertex2f( 0.0f, height() );
  glTexCoord2f( 1.0f, 1.0f );
  glVertex2f( width(), height() );
  glEnd();

  opengl::texture::disable_texture (0);

  if( highlight )
  {
    glColor3f( 1.0f, 0.0f, 0.0f );
    glBegin( GL_LINE_LOOP );
    glVertex2f( -1.0f, 0.0f );
    glVertex2f( width(), 0.0f );
    glVertex2f( width(), height() );
    glVertex2f( -1.0f, height() );
    glEnd();
  }

  glPopMatrix();
}

UIFrame *UITexture::processLeftClick( float /*mx*/, float /*my*/ )
{
  if( clickFunc )
  {
    clickFunc( this, id );
    return this;
  }
  return 0;
}

void UITexture::setClickFunc( void (*f)( UIFrame *, int ), int num )
{
  clickFunc = f;
  id = num;
}
