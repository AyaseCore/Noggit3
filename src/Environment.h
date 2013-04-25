#ifndef __ENVIRONMENT_H
#define __ENVIRONMENT_H

#include <string>
#include <map>

#include "Selection.h"
#include "Vec3D.h"

class Environment
{
public:
  static Environment* getInstance();
  nameEntry get_clipboard();
  void set_clipboard(nameEntry* entry);

  bool view_holelines;
  // values for areaID painting
  int selectedAreaID;
  std::map<int,Vec3D> areaIDColors; // List of all area IDs to draw them with different colors
  // hold keys
  bool ShiftDown;
  bool AltDown;
  bool CtrlDown;
  bool SpaceDown;

  bool paintMode;
  int flagPaintMode;
  int screenX;
  int screenY;

  float Pos3DX;
  float Pos3DY;
  float Pos3DZ;

  float cursorColorR;
  float cursorColorG;
  float cursorColorB;
  float cursorColorA;
  int cursorType;
private:
  Environment();
  static Environment* instance;

  nameEntry clipboard;
};

#endif
