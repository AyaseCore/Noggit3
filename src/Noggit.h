#ifndef NOGGIT_H
#define NOGGIT_H

#include <vector>
#include <string>

#include "DBC.h"
#include "FreeType.h" // fonts.

class AppState;
class AsyncLoader;

#ifdef _WIN32
#include "MSGPACK.H"
#include "WINTAB.h"
#define PACKETDATA	(PK_BUTTONS | PK_NORMAL_PRESSURE)
#define PACKETMODE	PK_BUTTONS
#include "PKTDEF.H"
#include "Utils.h"
HWND static WindowHandle;
HCTX static NEAR TabletInit(HWND hWnd);
#endif

class Noggit
{
public:
	float FPS;
#ifdef _WIN32
	UINT pressure;
	HCTX hCtx;
	BOOL tabletActive;
#endif
	Noggit();
	~Noggit();

	int start(int argc, char *argv[]);

	bool pop;

	inline AsyncLoader* loader()
	{
		return asyncLoader;
	}

	inline std::vector<AppState*>& getStates()
	{
		return states;
	}

	inline const freetype::font_data& getArial12() const
	{
		return arial12;
	}

	inline const freetype::font_data& getArialn13() const
	{
		return arialn13;
	}

	inline const freetype::font_data& getArial14() const
	{
		return arial14;
	}

	inline const freetype::font_data& getArial16() const
	{
		return arial16;
	}

	inline const freetype::font_data& getSkurri32() const
	{
		return skurri32;
	}

	inline const freetype::font_data& getFritz16() const
	{
		return fritz16;
	}

private:
	void initPath(char *argv[]);
	void initFont();
	void initEnv();

	void parseArgs(int argc, char *argv[]);
	void loadMPQs();
	void mainLoop();

	std::string getGamePath();
	std::string wowpath;

	AreaDB areaDB;
	AsyncLoader* asyncLoader;
	std::vector<AppState*> states;

	bool fullscreen;
	bool doAntiAliasing;

	freetype::font_data arialn13, arial12, arial14, arial16, arial24, arial32, morpheus40, skurri32, fritz16;
public:
	int xres;
	int yres;
};

extern Noggit app;

#endif
