#include "UICloseWindow.h"

#include <string>

#include "Noggit.h" // arial16
#include "UIMinimizeButton.h"
#include "UIText.h"

UICloseWindow::UICloseWindow( float px, float py, float w, float h, const std::string& pTitle, bool pMoveable )
: UIWindow( px, py, w, h )
{
  addChild( new UIText( width() / 2.0f, 2.0f, pTitle, app.getArial16(), eJustifyCenter ) );
  addChild( new UIMinimizeButton( width() ) );
  movable( pMoveable );
}
