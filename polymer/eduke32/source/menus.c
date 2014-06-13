//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "net.h"
#include "player.h"
#include "mouse.h"
#include "joystick.h"
#include "osd.h"
#include "osdcmds.h"
#include "gamedef.h"
#include "gameexec.h"
#include "savegame.h"
#include "premap.h"
#include "demo.h"
#include "xxhash.h"
#include "common.h"
#include "common_game.h"
#include "input.h"
#include "menus.h"

#include <sys/stat.h>

int32_t g_skillSoundVoice = -1;

extern int32_t voting;

#define USERMAPENTRYLENGTH 25

#define mgametext(x,y,t,s,dabits) G_PrintGameText(2,STARTALPHANUM, x,y,t,s,0,dabits,0, 0, xdim-1, ydim-1, 65536)
#define mgametextpal(x,y,t,s,p) G_PrintGameText(2,STARTALPHANUM, x,y,t,s,p,26,0, 0, xdim-1, ydim-1, 65536)



int32_t menutext_(int32_t x,int32_t y,int32_t s,int32_t p,char *t,int32_t bits)
{
    vec2_t dim;
    int32_t f = TEXT_BIGALPHANUM|TEXT_UPPERCASE|TEXT_LITERALESCAPE;

    if (x == 160)
        f |= TEXT_XCENTER;

    dim = G_ScreenText(BIGALPHANUM, x, y-12, 65536L, 0, 0, t, s, p, bits, 0, 5, 16, 0, 0, f, 0, 0, xdim-1, ydim-1);

    return dim.x;
}

#pragma pack(push,1)
static savehead_t savehead;
//static struct savehead_ savehead;
#pragma pack(pop)

static void M_DrawBackground(void)
{
    rotatesprite_fs(160<<16,200<<15,65536L,0,MENUSCREEN,16,0,10+64);
}


static void M_DrawTopBar(const char *caption)
{
    rotatesprite_fs(160<<16,19<<16,65536L,0,MENUBAR,16,0,10);
    menutext(160,24,0,0,caption);
}

extern int32_t g_quitDeadline;













/*
All MAKE_* macros are generally for the purpose of keeping state initialization
separate from actual data. Alternatively, they can serve to factor out repetitive
stuff and keep the important bits from getting lost to our eyes.
*/


// common font types
// tilenums are set after namesdyn runs
static MenuTextType_t MF_Redfont          = { -1, 10, 0,  1,  5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE, };
static MenuTextType_t MF_RedfontBlue      = { -1, 10, 1,  1,  5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE, };
static MenuTextType_t MF_RedfontGreen     = { -1, 10, 8,  1,  5<<16, 15<<16,  0<<16, 0<<16, TEXT_BIGALPHANUM | TEXT_UPPERCASE, };
static MenuTextType_t MF_Bluefont         = { -1, 10, 0,  16, 5<<16,  7<<16, -1<<16, 0<<16, 0, };
static MenuTextType_t MF_BluefontRed      = { -1, 10, 10, 16, 5<<16,  7<<16, -1<<16, 0<<16, 0, };
static MenuTextType_t MF_Minifont         = { -1, 10, 0,  16, 4<<16,  5<<16,  1<<16, 1<<16, 0, };
static MenuTextType_t MF_MinifontRed      = { -1, 16, 21, 16, 4<<16,  5<<16,  1<<16, 1<<16, 0, };
static MenuTextType_t MF_MinifontDarkGray = { -1, 10, 13, 16, 4<<16,  5<<16,  1<<16, 1<<16, 0, };


// common positions
#define MENU_MARGIN_REGULAR 40
#define MENU_MARGIN_WIDE    32
#define MENU_MARGIN_CENTER  160
#define MENU_HEIGHT_CENTER  100

static MenuPos_t MP_TOP_MAIN =             { { MENU_MARGIN_CENTER<<16,       55<<16, }, 0,     2<<16,  0,       190<<16, 110<<16, 65536, };
static MenuPos_t MP_TOP_EPISODE =          { { MENU_MARGIN_CENTER<<16,       48<<16, }, 0,     5<<16,  0,       190<<16, 110<<16, 65536, };
static MenuPos_t MP_TOP_SKILL =            { { MENU_MARGIN_CENTER<<16,       58<<16, }, 0,     4<<16,  0,       190<<16, 110<<16, 65536, };
static MenuPos_t MP_TOP_OPTIONS =          { { MENU_MARGIN_CENTER<<16,       38<<16, }, 0,     3<<16,  0,       190<<16, 110<<16, 65536, };
static MenuPos_t MP_TOP_JOYSTICK_NETWORK = { { MENU_MARGIN_CENTER<<16,       70<<16, }, 0,     3<<16,  0,       190<<16, 110<<16, 65536, };
static MenuPos_t MP_OPTIONS =              { { MENU_MARGIN_WIDE<<16,         37<<16, }, 4<<16, 1<<16,  216<<16, 160<<16,  10<<16, 32768, };
static MenuPos_t MP_PLAYER_1 =             { { MENU_MARGIN_WIDE<<16,         37<<16, }, 4<<16, 1<<16,   90<<16, 160<<16,  10<<16, 32768, };
static MenuPos_t MP_PLAYER_2 =             { { MENU_MARGIN_WIDE<<16,         37<<16, }, 8<<16, 1<<16,   90<<16, 160<<16,  10<<16, 32768, };
static MenuPos_t MP_PLAYER_3 =             { { MENU_MARGIN_WIDE<<16,         37<<16, }, 8<<16, 1<<16,  190<<16, 160<<16,  10<<16, 32768, };
static MenuPos_t MP_MACROS =               { { 26<<16,                       40<<16, }, 4<<16, 2<<16,    1<<16, 160<<16,  10<<16, 32768, };
static MenuPos_t MP_VIDEOSETUP =           { { MENU_MARGIN_REGULAR<<16,      38<<16, }, 6<<16, 2<<16,  168<<16, 190<<16,  20<<16, 65536, };
static MenuPos_t MP_VIDEOSETUP_APPLY =     { { (MENU_MARGIN_REGULAR+16)<<16, 38<<16, }, 6<<16, 2<<16,  168<<16, 190<<16,  36<<16, 65536, };
static MenuPos_t MP_KEYBOARDSETUPFUNCS =   { { 70<<16,                       34<<16, }, 4<<16, 3<<16,  100<<16, 151<<16,  10<<16, 32768, };
static MenuPos_t MP_KEYBOARDSETUP =        { { MENU_MARGIN_CENTER<<16,       37<<16, }, 4<<16, 0<<16,  0,       160<<16,  92<<16, 32768, };
static MenuPos_t MP_MOUSEJOYSETUPBTNS =    { { 76<<16,                       34<<16, }, 4<<16, 3<<16,  100<<16, 143<<16,  10<<16, 32768, };
static MenuPos_t MP_GAMEFUNCLIST =         { { 100<<16,                      51<<16, }, 4<<16, 3<<16,  100<<16, 152<<16,  10<<16, 32768, };
static MenuPos_t MP_MOUSESETUP =           { { MENU_MARGIN_REGULAR<<16,      37<<16, }, 4<<16, 2<<16,  200<<16, 160<<16,  10<<16, 32768, };
static MenuPos_t MP_COLCORR =              { { MENU_MARGIN_REGULAR<<16,      86<<16, }, 8<<16, 2<<16, -240<<16, 190<<16,  20<<16, 65536, };
static MenuPos_t MP_REDSLIDE =             { { MENU_MARGIN_WIDE<<16,         37<<16, }, 8<<16, 2<<16,  170<<16, 190<<16,  20<<16, 65536, };
static MenuPos_t MP_LOADSAVE =             { { 223<<16,                      48<<16, }, 4<<16, 7<<16,    1<<16, 320<<16,  20<<16, 65536, };
static MenuPos_t MP_NETSETUP =             { { (MENU_MARGIN_REGULAR-4)<<16,  38<<16, }, 4<<16, 2<<16,  114<<16, 190<<16,  20<<16, 65536, };
static MenuPos_t MP_ADULTMODE =            { { (MENU_MARGIN_REGULAR+20)<<16, 54<<16, }, 4<<16, 1<<16,  200<<16, 190<<16,  20<<16, 65536, };


// common menu option sets
#define MAKE_MENUOPTIONSET(a, b, c) { a, b, c, ARRAY_SIZE(b), &MP_GAMEFUNCLIST, -1, 0, }
#define MAKE_MENUOPTIONSETLIST(a, b, c, d) { a, b, c, ARRAY_SIZE(b), d, -1, 0, }
#define MAKE_MENUOPTIONSETDYN(...) { __VA_ARGS__, &MP_GAMEFUNCLIST, -1, 0, }
#define MAKE_MENUOPTIONSETDYNLIST(...) { __VA_ARGS__, -1, 0, }

static char *MEOSN_OffOn[] = { "Off", "On", };
static MenuOptionSet_t MEOS_OffOn = MAKE_MENUOPTIONSET( 0x3, MEOSN_OffOn, NULL );
static char *MEOSN_OnOff[] = { "On", "Off", };
static MenuOptionSet_t MEOS_OnOff = MAKE_MENUOPTIONSET( 0x3, MEOSN_OnOff, NULL );
static char *MEOSN_NoYes[] = { "No", "Yes", };
static MenuOptionSet_t MEOS_NoYes = MAKE_MENUOPTIONSET( 0x3, MEOSN_NoYes, NULL );
static char *MEOSN_YesNo[] = { "Yes", "No", };
static MenuOptionSet_t MEOS_YesNo = MAKE_MENUOPTIONSET( 0x3, MEOSN_YesNo, NULL );


static char MenuGameFuncs[NUMGAMEFUNCTIONS][MAXGAMEFUNCLEN];
static char *MenuGameFuncNone = "  -None-";
static char *MEOSN_Gamefuncs[NUMGAMEFUNCTIONS+1];
static MenuOptionSet_t MEOS_Gamefuncs = MAKE_MENUOPTIONSET( 0x1, MEOSN_Gamefuncs, NULL );



/*
MenuEntry_t and MenuGroup_t are passed in arrays of pointers so that the callback function
that is called when an entry is modified or activated can test equality of the current
entry/group pointer directly against the known ones, instead of relying on an ID number.

That way, individual menu entries/groups can be ifdef'd out painlessly.
*/

static MenuLink_t MEO_NULL = { MENU_NULL, };
static const char* MenuCustom = "Custom";

#define MAKE_MENUSTRING(...) { __VA_ARGS__, NULL, }
#define MAKE_MENUOPTION(...) { __VA_ARGS__, -1, }
#define MAKE_MENURANGE(...) { __VA_ARGS__, }
#define MAKE_MENUENTRY(...) { __VA_ARGS__, 0, 0, 0, }

#define MAKE_MENU_TOP_ENTRYLINK(EntryName, LinkID, Title) \
static MenuLink_t MEO_ ## EntryName = { LinkID, };\
static MenuEntry_t ME_ ## EntryName = MAKE_MENUENTRY( &MF_Redfont, Title, Link, &MEO_ ## EntryName )


MAKE_MENU_TOP_ENTRYLINK( MAIN_NEWGAME, MENU_EPISODE, "New Game" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_NEWGAME_INGAME, MENU_NEWVERIFY, "New Game" );
static MenuLink_t MEO_MAIN_NEWGAME_NETWORK = { MENU_NETWORK, };
MAKE_MENU_TOP_ENTRYLINK( MAIN_SAVEGAME, MENU_SAVE, "Save Game" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_LOADGAME, MENU_LOAD, "Load Game" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_OPTIONS, MENU_OPTIONS, "Options" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_HELP, MENU_STORY, "Help" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_HOWTOORDER, MENU_ORDERING, "How To Order" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_CREDITS, MENU_CREDITS, "Credits" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_QUITTOTITLE, MENU_QUITTOTITLE, "Quit To Title" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_QUIT, MENU_QUIT, "Quit" );
MAKE_MENU_TOP_ENTRYLINK( MAIN_QUITGAME, MENU_QUIT, "Quit Game" );

static MenuEntry_t *MEL_MAIN[] = {
    &ME_MAIN_NEWGAME,
    &ME_MAIN_OPTIONS,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_HELP,
    &ME_MAIN_CREDITS,
    &ME_MAIN_QUIT,
};
static MenuEntry_t *MEL_MAIN_SHAREWARE[] = {
    &ME_MAIN_NEWGAME,
    &ME_MAIN_OPTIONS,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_HELP,
    &ME_MAIN_HOWTOORDER,
    &ME_MAIN_CREDITS,
    &ME_MAIN_QUIT,
};
static MenuEntry_t *MEL_MAIN_INGAME[] = {
    &ME_MAIN_NEWGAME_INGAME,
    &ME_MAIN_SAVEGAME,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_OPTIONS,
    &ME_MAIN_HELP,
    &ME_MAIN_QUITTOTITLE,
    &ME_MAIN_QUITGAME,
};
static MenuEntry_t *MEL_MAIN_INGAME_SHAREWARE[] = {
    &ME_MAIN_NEWGAME_INGAME,
    &ME_MAIN_SAVEGAME,
    &ME_MAIN_LOADGAME,
    &ME_MAIN_OPTIONS,
    &ME_MAIN_HELP,
    &ME_MAIN_HOWTOORDER,
    &ME_MAIN_QUITTOTITLE,
    &ME_MAIN_QUITGAME,
};

// Episode and Skill will be dynamically generated after CONs are parsed
static MenuLink_t MEO_EPISODE = { MENU_SKILL, };
static MenuLink_t MEO_EPISODE_SHAREWARE = { MENU_BUYDUKE, };
static MenuEntry_t ME_EPISODE_TEMPLATE = MAKE_MENUENTRY( &MF_Redfont, NULL, Link, &MEO_EPISODE );
static MenuEntry_t ME_EPISODE[MAXVOLUMES];
static MenuEntry_t *MEL_EPISODE[MAXVOLUMES];

static MenuLink_t MEO_EPISODE_USERMAP = { MENU_USERMAP, };
static MenuEntry_t ME_EPISODE_USERMAP = MAKE_MENUENTRY( &MF_Redfont, "User Map", Link, &MEO_EPISODE_USERMAP );
static MenuEntry_t *MEL_EPISODE_USERMAP[] = {
    &ME_EPISODE_USERMAP,
};

static MenuEntry_t ME_SKILL_TEMPLATE = MAKE_MENUENTRY( &MF_Redfont, NULL, Link, &MEO_NULL );
static MenuEntry_t ME_SKILL[MAXSKILLS];
static MenuEntry_t *MEL_SKILL[MAXSKILLS];

// All NULL MenuEntry_t are to-do.

static MenuOption_t MEO_GAMESETUP_STARTWIN = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.config.ForceSetup );
static MenuEntry_t ME_GAMESETUP_STARTWIN = MAKE_MENUENTRY( &MF_BluefontRed, "Show setup window at start", Option, &MEO_GAMESETUP_STARTWIN );
static MenuOption_t MEO_GAMESETUP_CROSSHAIR = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.crosshair );
static MenuEntry_t ME_GAMESETUP_CROSSHAIR = MAKE_MENUENTRY( &MF_BluefontRed, "Show crosshair", Option, &MEO_GAMESETUP_CROSSHAIR );
static MenuRangeInt32_t MEO_GAMESETUP_CROSSHAIRSIZE = MAKE_MENURANGE( &MF_Bluefont, 2, 25, 100, 16, 0, &ud.crosshairscale );
static MenuEntry_t ME_GAMESETUP_CROSSHAIRSIZE = MAKE_MENUENTRY( &MF_BluefontRed, "Crosshair size", RangeInt32, &MEO_GAMESETUP_CROSSHAIRSIZE );
static int32_t vpsize;
static MenuRangeInt32_t MEO_GAMESETUP_SCREENSIZE = MAKE_MENURANGE( &MF_Bluefont, 0, 68, 0, 18, 0, &vpsize );
static MenuEntry_t ME_GAMESETUP_SCREENSIZE = MAKE_MENUENTRY( &MF_BluefontRed, "Screen size", RangeInt32, &MEO_GAMESETUP_SCREENSIZE );
static MenuRangeInt32_t MEO_GAMESETUP_SBARSIZE = MAKE_MENURANGE( &MF_Bluefont, 2, 36, 100, 17, 0, &ud.statusbarscale );
static MenuEntry_t ME_GAMESETUP_SBARSIZE = MAKE_MENUENTRY( &MF_BluefontRed, "Status bar size", RangeInt32, &MEO_GAMESETUP_SBARSIZE );
static MenuRangeInt32_t MEO_GAMESETUP_TEXTSIZE = MAKE_MENURANGE( &MF_Bluefont, 2, 100, 400, 7, 0, &ud.textscale );
static MenuEntry_t ME_GAMESETUP_TEXTSIZE = MAKE_MENUENTRY( &MF_BluefontRed, "Stats and chat text size", RangeInt32, &MEO_GAMESETUP_TEXTSIZE );
static MenuOption_t MEO_GAMESETUP_LEVELSTATS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.levelstats );
static MenuEntry_t ME_GAMESETUP_LEVELSTATS = MAKE_MENUENTRY( &MF_BluefontRed, "Show level stats", Option, &MEO_GAMESETUP_LEVELSTATS );
static MenuOption_t MEO_GAMESETUP_WALKWITHAUTORUN = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_YesNo, &ud.runkey_mode );
static MenuEntry_t ME_GAMESETUP_WALKWITHAUTORUN = MAKE_MENUENTRY( &MF_BluefontRed, "Allow walk with autorun", Option, &MEO_GAMESETUP_WALKWITHAUTORUN );
static MenuOption_t MEO_GAMESETUP_SHADOWS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.shadows );
static MenuEntry_t ME_GAMESETUP_SHADOWS = MAKE_MENUENTRY( &MF_BluefontRed, "Shadows", Option, &MEO_GAMESETUP_SHADOWS );
static MenuOption_t MEO_GAMESETUP_SCREENTILTING = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.screen_tilting );
static MenuEntry_t ME_GAMESETUP_SCREENTILTING = MAKE_MENUENTRY( &MF_BluefontRed, "Screen tilting", Option, &MEO_GAMESETUP_SCREENTILTING );
static MenuOption_t MEO_GAMESETUP_FRAMERATE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.tickrate );
static MenuEntry_t ME_GAMESETUP_FRAMERATE = MAKE_MENUENTRY( &MF_BluefontRed, "Show framerate", Option, &MEO_GAMESETUP_FRAMERATE );
static char *MEOSN_DemoRec[] = { "Off", "Running", };
static MenuOptionSet_t MEOS_DemoRec = MAKE_MENUOPTIONSET( 0x3, MEOSN_DemoRec, NULL );
static MenuOption_t MEO_GAMESETUP_DEMOREC = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.m_recstat );
static MenuEntry_t ME_GAMESETUP_DEMOREC = MAKE_MENUENTRY( &MF_BluefontRed, "Demo recording", Option, &MEO_GAMESETUP_DEMOREC );
static MenuLink_t MEO_GAMESETUP_PARENTALLOCK = { MENU_ADULTMODE, };
static MenuEntry_t ME_GAMESETUP_PARENTALLOCK = MAKE_MENUENTRY( &MF_BluefontRed, "Parental lock", Link, &MEO_GAMESETUP_PARENTALLOCK );
static MenuOption_t MEO_GAMESETUP_SHOWPICKUPMESSAGES = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.fta_on );
static MenuEntry_t ME_GAMESETUP_SHOWPICKUPMESSAGES = MAKE_MENUENTRY( &MF_BluefontRed, "Show inv & pickup messages", Option, &MEO_GAMESETUP_SHOWPICKUPMESSAGES );
static char *MEOSN_GAMESETUP_DISPCURRWEAPON[] = { "Off", "On", "Icon only", };
static MenuOptionSet_t MEOS_GAMESETUP_DISPCURRWEAPON = MAKE_MENUOPTIONSET( 0x3, MEOSN_GAMESETUP_DISPCURRWEAPON, NULL );
static MenuOption_t MEO_GAMESETUP_DISPCURRWEAPON = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_GAMESETUP_DISPCURRWEAPON, &ud.drawweapon );
static MenuEntry_t ME_GAMESETUP_DISPCURRWEAPON = MAKE_MENUENTRY( &MF_BluefontRed, "Display current weapon", Option, &MEO_GAMESETUP_DISPCURRWEAPON );
static MenuOption_t MEO_GAMESETUP_NEWSTATUSBAR = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.althud );
static MenuEntry_t ME_GAMESETUP_NEWSTATUSBAR = MAKE_MENUENTRY( &MF_BluefontRed, "Upgraded status bar", Option, &MEO_GAMESETUP_NEWSTATUSBAR );
static MenuOption_t MEO_GAMESETUP_CAMERAVIEWINDEMOS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.democams );
static MenuEntry_t ME_GAMESETUP_CAMERAVIEWINDEMOS = MAKE_MENUENTRY( &MF_BluefontRed, "Camera view in demos", Option, &MEO_GAMESETUP_CAMERAVIEWINDEMOS );
static char *MEOSN_GAMESETUP_DM_IGNOREMAPVOTES[] = { "Off", "Vote No", "Vote Yes", };
static MenuOptionSet_t MEOS_GAMESETUP_DM_IGNOREMAPVOTES = MAKE_MENUOPTIONSET( 0x3, MEOSN_GAMESETUP_DM_IGNOREMAPVOTES, NULL );
static MenuOption_t MEO_GAMESETUP_DM_IGNOREMAPVOTES = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_GAMESETUP_DM_IGNOREMAPVOTES, &ud.autovote );
static MenuEntry_t ME_GAMESETUP_DM_IGNOREMAPVOTES = MAKE_MENUENTRY( &MF_BluefontRed, "DM: Ignore map votes", Option, &MEO_GAMESETUP_DM_IGNOREMAPVOTES );
static MenuOption_t MEO_GAMESETUP_DM_USEPRIVATEMESSAGES = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OnOff, &ud.automsg );
static MenuEntry_t ME_GAMESETUP_DM_USEPRIVATEMESSAGES = MAKE_MENUENTRY( &MF_BluefontRed, "DM: Use private messages", Option, &MEO_GAMESETUP_DM_USEPRIVATEMESSAGES );
static MenuOption_t MEO_GAMESETUP_DM_SHOWPLAYERNAMES = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.idplayers );
static MenuEntry_t ME_GAMESETUP_DM_SHOWPLAYERNAMES = MAKE_MENUENTRY( &MF_BluefontRed, "DM: Show player names", Option, &MEO_GAMESETUP_DM_SHOWPLAYERNAMES );
static MenuOption_t MEO_GAMESETUP_DM_SHOWPLAYERWEAPONS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.config.ShowOpponentWeapons );
static MenuEntry_t ME_GAMESETUP_DM_SHOWPLAYERWEAPONS = MAKE_MENUENTRY( &MF_BluefontRed, "DM: Show player weapons", Option, &MEO_GAMESETUP_DM_SHOWPLAYERWEAPONS );
static MenuOption_t MEO_GAMESETUP_DM_SHOWPLAYEROBITUARIES = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.obituaries );
static MenuEntry_t ME_GAMESETUP_DM_SHOWPLAYEROBITUARIES = MAKE_MENUENTRY( &MF_BluefontRed, "DM: Show player obituaries", Option, &MEO_GAMESETUP_DM_SHOWPLAYEROBITUARIES );
static char *MEOSN_GAMESETUP_CONSOLETEXT[] = { "Sprites", "Basic", };
static MenuOptionSet_t MEOS_GAMESETUP_CONSOLETEXT = MAKE_MENUOPTIONSET( 0x3, MEOSN_GAMESETUP_CONSOLETEXT, NULL );
static MenuOption_t MEO_GAMESETUP_CONSOLETEXT = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_GAMESETUP_CONSOLETEXT, NULL );
static MenuEntry_t ME_GAMESETUP_CONSOLETEXT = MAKE_MENUENTRY( &MF_BluefontRed, "Console text style", Option, &MEO_GAMESETUP_CONSOLETEXT );
#ifdef _WIN32
static MenuOption_t MEO_GAMESETUP_UPDATES = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.config.CheckForUpdates );
static MenuEntry_t ME_GAMESETUP_UPDATES = MAKE_MENUENTRY( &MF_BluefontRed, "Check for updates at start", Option, &MEO_GAMESETUP_UPDATES );
#endif

static MenuEntry_t *MEL_GAMESETUP1[] = {
    &ME_GAMESETUP_STARTWIN,
    &ME_GAMESETUP_CROSSHAIR,
    &ME_GAMESETUP_CROSSHAIRSIZE,
};
static MenuEntry_t *MEL_GAMESETUP2[] = {
    &ME_GAMESETUP_SCREENSIZE,
    &ME_GAMESETUP_SBARSIZE,
    &ME_GAMESETUP_TEXTSIZE,
    &ME_GAMESETUP_LEVELSTATS,
};
static MenuEntry_t *MEL_GAMESETUP3[] = {
    &ME_GAMESETUP_WALKWITHAUTORUN,
};
static MenuEntry_t *MEL_GAMESETUP4[] = {
    &ME_GAMESETUP_SHADOWS,
    &ME_GAMESETUP_SCREENTILTING,
};
static MenuEntry_t *MEL_GAMESETUP5[] = {
    &ME_GAMESETUP_FRAMERATE,
    &ME_GAMESETUP_DEMOREC,
};
static MenuEntry_t *MEL_GAMESETUP6[] = {
    &ME_GAMESETUP_PARENTALLOCK,
};
static MenuEntry_t *MEL_GAMESETUP7[] = {
    &ME_GAMESETUP_SHOWPICKUPMESSAGES,
    &ME_GAMESETUP_DISPCURRWEAPON,
    &ME_GAMESETUP_NEWSTATUSBAR,
    &ME_GAMESETUP_CAMERAVIEWINDEMOS,
};
static MenuEntry_t *MEL_GAMESETUP8[] = {
    &ME_GAMESETUP_DM_IGNOREMAPVOTES,
    &ME_GAMESETUP_DM_USEPRIVATEMESSAGES,
    &ME_GAMESETUP_DM_SHOWPLAYERNAMES,
    &ME_GAMESETUP_DM_SHOWPLAYERWEAPONS,
    &ME_GAMESETUP_DM_SHOWPLAYEROBITUARIES,
};
static MenuEntry_t *MEL_GAMESETUP9[] = {
    &ME_GAMESETUP_CONSOLETEXT,
};
#ifdef _WIN32
static MenuEntry_t *MEL_GAMESETUP_UPDATES[] = {
    &ME_GAMESETUP_UPDATES,
};
#endif

MAKE_MENU_TOP_ENTRYLINK( OPTIONS_GAMESETUP, MENU_GAMESETUP, "Game Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_SOUNDSETUP, MENU_SOUND, "Sound Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_VIDEOSETUP, MENU_VIDEOSETUP, "Video Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_PLAYERSETUP, MENU_PLAYER, "Player Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_KEYBOARDSETUP, MENU_KEYBOARDSETUP, "Keyboard Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_MOUSESETUP, MENU_MOUSESETUP, "Mouse Setup" );
MAKE_MENU_TOP_ENTRYLINK( OPTIONS_JOYSTICKSETUP, MENU_JOYSTICKSETUP, "Joystick Setup" );


static int32_t newresolution, newrendermode, newfullscreen;

enum resflags_t {
    RES_FS  = 0x1,
    RES_WIN = 0x2,
};

#define MAXRESOLUTIONSTRINGLENGTH 16

typedef struct resolution_t {
    int32_t xdim, ydim;
    int32_t flags;
    int32_t bppmax;
    char name[MAXRESOLUTIONSTRINGLENGTH];
} resolution_t;

resolution_t resolution[MAXVALIDMODES];

static char *MEOSN_VIDEOSETUP_RESOLUTION[MAXVALIDMODES];
static MenuOptionSet_t MEOS_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTIONSETDYN( 0x0, MEOSN_VIDEOSETUP_RESOLUTION, NULL, 0 );
static MenuOption_t MEO_VIDEOSETUP_RESOLUTION = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_VIDEOSETUP_RESOLUTION, &newresolution );
static MenuEntry_t ME_VIDEOSETUP_RESOLUTION = MAKE_MENUENTRY( &MF_Redfont, "Resolution", Option, &MEO_VIDEOSETUP_RESOLUTION );

#ifdef USE_OPENGL
#ifdef POLYMER
static char *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "Polymost", "Polymer", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, REND_POLYMER, };
#else
static char *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", "Polymost", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, REND_POLYMOST, };
#endif
#else
static char *MEOSN_VIDEOSETUP_RENDERER[] = { "Classic", };
static int32_t MEOSV_VIDEOSETUP_RENDERER[] = { REND_CLASSIC, };
#endif

static MenuOptionSet_t MEOS_VIDEOSETUP_RENDERER = MAKE_MENUOPTIONSET( 0x2, MEOSN_VIDEOSETUP_RENDERER, MEOSV_VIDEOSETUP_RENDERER );

static MenuOption_t MEO_VIDEOSETUP_RENDERER = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_VIDEOSETUP_RENDERER, &newrendermode );
static MenuEntry_t ME_VIDEOSETUP_RENDERER = MAKE_MENUENTRY( &MF_Redfont, "Renderer", Option, &MEO_VIDEOSETUP_RENDERER );
static MenuOption_t MEO_VIDEOSETUP_FULLSCREEN = MAKE_MENUOPTION( &MF_Redfont, &MEOS_NoYes, &newfullscreen );
static MenuEntry_t ME_VIDEOSETUP_FULLSCREEN = MAKE_MENUENTRY( &MF_Redfont, "Fullscreen", Option, &MEO_VIDEOSETUP_FULLSCREEN );
static MenuEntry_t ME_VIDEOSETUP_APPLY = MAKE_MENUENTRY( &MF_Redfont, "Apply Changes", Link, &MEO_NULL );
static MenuLink_t MEO_VIDEOSETUP_COLORCORR =  { MENU_COLCORR, };
static MenuEntry_t ME_VIDEOSETUP_COLORCORR = MAKE_MENUENTRY( &MF_Redfont, "Color Correction", Link, &MEO_VIDEOSETUP_COLORCORR );
static MenuOption_t MEO_VIDEOSETUP_PIXELDOUBLING = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OnOff, &ud.detail );
static MenuEntry_t ME_VIDEOSETUP_PIXELDOUBLING = MAKE_MENUENTRY( &MF_Redfont, "Pixel Doubling", Option, &MEO_VIDEOSETUP_PIXELDOUBLING );
#ifdef USE_OPENGL
static char *MEOSN_VIDEOSETUP_TEXFILTER[] = { "Nearest", "Linear", "Near_MM_Near", "Bilinear", "Near_MM_Lin", "Trilinear", };
static MenuOptionSet_t MEOS_VIDEOSETUP_TEXFILTER = MAKE_MENUOPTIONSET( 0x2, MEOSN_VIDEOSETUP_TEXFILTER, NULL );
static MenuOption_t MEO_VIDEOSETUP_TEXFILTER = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_VIDEOSETUP_TEXFILTER, &gltexfiltermode );
static MenuEntry_t ME_VIDEOSETUP_TEXFILTER = MAKE_MENUENTRY( &MF_Redfont, "Texture Filter", Option, &MEO_VIDEOSETUP_TEXFILTER );
#endif
static MenuLink_t MEO_VIDEOSETUP_RENDERERSETUP =  { MENU_RENDERERSETUP, };
static MenuEntry_t ME_VIDEOSETUP_RENDERERSETUP = MAKE_MENUENTRY( &MF_Redfont, "Renderer Setup", Link, &MEO_VIDEOSETUP_RENDERERSETUP );

static MenuEntry_t *MEL_OPTIONS[] = {
    &ME_OPTIONS_GAMESETUP,
    &ME_OPTIONS_SOUNDSETUP,
    &ME_OPTIONS_VIDEOSETUP,
    &ME_OPTIONS_PLAYERSETUP,
    &ME_OPTIONS_KEYBOARDSETUP,
    &ME_OPTIONS_MOUSESETUP,
    &ME_OPTIONS_JOYSTICKSETUP,
};

static MenuEntry_t *MEL_VIDEOSETUP1[] = {
    &ME_VIDEOSETUP_RESOLUTION,
    &ME_VIDEOSETUP_RENDERER,
    &ME_VIDEOSETUP_FULLSCREEN,
};
static MenuEntry_t *MEL_VIDEOSETUP_APPLY[] = {
    &ME_VIDEOSETUP_APPLY,
};
static MenuEntry_t *MEL_VIDEOSETUP2[] = {
    &ME_VIDEOSETUP_COLORCORR,
    &ME_VIDEOSETUP_PIXELDOUBLING,
    &ME_VIDEOSETUP_RENDERERSETUP,
};
#ifdef USE_OPENGL
static MenuEntry_t *MEL_VIDEOSETUP2_GL[] = {
    &ME_VIDEOSETUP_COLORCORR,
    &ME_VIDEOSETUP_TEXFILTER,
    &ME_VIDEOSETUP_RENDERERSETUP,
};
#endif

static char *MenuKeyNone = "  -";
static char *MEOSN_Keys[NUMKEYS];

static MenuCustom2Col_t MEO_KEYBOARDSETUPFUNCS_TEMPLATE = { &MF_MinifontRed, 54<<16, MEOSN_Keys, NUMKEYS, { NULL, NULL, }, 0, };
static MenuCustom2Col_t MEO_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];
static MenuEntry_t ME_KEYBOARDSETUPFUNCS_TEMPLATE = MAKE_MENUENTRY( &MF_Minifont, NULL, Custom2Col, &MEO_KEYBOARDSETUPFUNCS_TEMPLATE );
static MenuEntry_t ME_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];
static MenuEntry_t *MEL_KEYBOARDSETUPFUNCS[NUMGAMEFUNCTIONS];

static MenuLink_t MEO_KEYBOARDSETUP_KEYS =  { MENU_KEYBOARDKEYS, };
static MenuEntry_t ME_KEYBOARDSETUP_KEYS = MAKE_MENUENTRY( &MF_BluefontRed, "Configure Keys", Link, &MEO_KEYBOARDSETUP_KEYS );
static MenuEntry_t ME_KEYBOARDSETUP_RESET = MAKE_MENUENTRY( &MF_BluefontRed, "Reset Keys To Defaults", Link, &MEO_NULL );
static MenuEntry_t ME_KEYBOARDSETUP_RESETCLASSIC = MAKE_MENUENTRY( &MF_BluefontRed, "Set Classic Key Layout", Link, &MEO_NULL );

static MenuEntry_t *MEL_KEYBOARDSETUP[] = {
    &ME_KEYBOARDSETUP_KEYS,
    &ME_KEYBOARDSETUP_RESET,
    &ME_KEYBOARDSETUP_RESETCLASSIC,
};


// There is no better way to do this than manually.

#define MENUMOUSEFUNCTIONS 12

static char *MenuMouseNames[MENUMOUSEFUNCTIONS] = {
    "Button 1",
    "Double Button 1",
    "Button 2",
    "Double Button 2",
    "Button 3",
    "Double Button 3",

    "Wheel Up",
    "Wheel Down",

    "Button 4",
    "Double Button 4",
    "Button 5",
    "Double Button 5",
};
static int32_t MenuMouseDataIndex[MENUMOUSEFUNCTIONS][2] = {
    { 0, 0, },
    { 0, 1, },
    { 1, 0, },
    { 1, 1, },
    { 2, 0, },
    { 2, 1, },

    // note the mouse wheel
    { 4, 0, },
    { 5, 0, },

    { 3, 0, },
    { 3, 1, },
    { 6, 0, },
    { 6, 1, },
};

static MenuOption_t MEO_MOUSEJOYSETUPBTNS_TEMPLATE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuOption_t MEO_MOUSESETUPBTNS[MENUMOUSEFUNCTIONS];
static MenuEntry_t ME_MOUSEJOYSETUPBTNS_TEMPLATE = MAKE_MENUENTRY( &MF_Minifont, NULL, Option, NULL );
static MenuEntry_t ME_MOUSESETUPBTNS[MENUMOUSEFUNCTIONS];
static MenuEntry_t *MEL_MOUSESETUPBTNS[MENUMOUSEFUNCTIONS];

static MenuLink_t MEO_MOUSESETUP_BTNS =  { MENU_MOUSEBTNS, };
static MenuEntry_t ME_MOUSESETUP_BTNS = MAKE_MENUENTRY( &MF_BluefontRed, "Configure Buttons", Link, &MEO_MOUSESETUP_BTNS );
static MenuRangeFloat_t MEO_MOUSESETUP_SENSITIVITY = MAKE_MENURANGE( &MF_Bluefont, 0, 0.f, 16.f, 33, 0.f, &CONTROL_MouseSensitivity );
static MenuEntry_t ME_MOUSESETUP_SENSITIVITY = MAKE_MENUENTRY( &MF_BluefontRed, "Base mouse sensitivity", RangeFloat, &MEO_MOUSESETUP_SENSITIVITY );
static MenuOption_t MEO_MOUSESETUP_MOUSEAIMING = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &g_myAimMode );
static MenuEntry_t ME_MOUSESETUP_MOUSEAIMING = MAKE_MENUENTRY( &MF_BluefontRed, "Use mouse aiming", Option, &MEO_MOUSESETUP_MOUSEAIMING );
static MenuOption_t MEO_MOUSESETUP_INVERT = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_YesNo, &ud.mouseflip );
static MenuEntry_t ME_MOUSESETUP_INVERT = MAKE_MENUENTRY( &MF_BluefontRed, "Invert mouse", Option, &MEO_MOUSESETUP_INVERT );
static MenuOption_t MEO_MOUSESETUP_SMOOTH = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.config.SmoothInput );
static MenuEntry_t ME_MOUSESETUP_SMOOTH = MAKE_MENUENTRY( &MF_BluefontRed, "Smooth mouse movement", Option, &MEO_MOUSESETUP_SMOOTH );
static MenuLink_t MEO_MOUSESETUP_ADVANCED =  { MENU_MOUSEADVANCED, };
static MenuEntry_t ME_MOUSESETUP_ADVANCED = MAKE_MENUENTRY( &MF_BluefontRed, "Advanced mouse setup", Link, &MEO_MOUSESETUP_ADVANCED );

static MenuEntry_t *MEL_MOUSESETUP[] = {
    &ME_MOUSESETUP_BTNS,
    &ME_MOUSESETUP_SENSITIVITY,
    &ME_MOUSESETUP_MOUSEAIMING,
    &ME_MOUSESETUP_INVERT,
    &ME_MOUSESETUP_SMOOTH,
    &ME_MOUSESETUP_ADVANCED,
};

MAKE_MENU_TOP_ENTRYLINK( JOYSTICK_EDITBUTTONS, MENU_JOYSTICKBTNS, "Edit Buttons" );
MAKE_MENU_TOP_ENTRYLINK( JOYSTICK_EDITAXES, MENU_JOYSTICKAXES, "Edit Axes" );

static MenuEntry_t *MEL_JOYSTICKSETUP[] = {
    &ME_JOYSTICK_EDITBUTTONS,
    &ME_JOYSTICK_EDITAXES,
};

#define MAXJOYBUTTONSTRINGLENGTH 32

static char MenuJoystickNames[MAXJOYBUTTONS<<1][MAXJOYBUTTONSTRINGLENGTH];

static MenuOption_t MEO_JOYSTICKBTNS[MAXJOYBUTTONS<<1];
static MenuEntry_t ME_JOYSTICKBTNS[MAXJOYBUTTONS<<1];
static MenuEntry_t *MEL_JOYSTICKBTNS[MAXJOYBUTTONS<<1];

static MenuLink_t MEO_JOYSTICKAXES = { MENU_JOYSTICKAXIS, };
static MenuEntry_t ME_JOYSTICKAXES_TEMPLATE = MAKE_MENUENTRY( &MF_Redfont, NULL, Link, &MEO_JOYSTICKAXES );
static MenuEntry_t ME_JOYSTICKAXES[MAXJOYAXES];
static char MenuJoystickAxes[MAXJOYAXES][MAXJOYBUTTONSTRINGLENGTH];

static MenuEntry_t *MEL_JOYSTICKAXES[MAXJOYAXES];

static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEX = MAKE_MENURANGE( &MF_Bluefont, 3, -262144, 262144, 65, 65536, &ud.config.MouseAnalogueScale[0] );
static MenuEntry_t ME_MOUSEADVANCED_SCALEX = MAKE_MENUENTRY( &MF_Redfont, "X-Axis Scale", RangeInt32, &MEO_MOUSEADVANCED_SCALEX );
static MenuRangeInt32_t MEO_MOUSEADVANCED_SCALEY = MAKE_MENURANGE( &MF_Bluefont, 3, -262144, 262144, 65, 65536, &ud.config.MouseAnalogueScale[1] );
static MenuEntry_t ME_MOUSEADVANCED_SCALEY = MAKE_MENUENTRY( &MF_Redfont, "Y-Axis Scale", RangeInt32, &MEO_MOUSEADVANCED_SCALEY );
static MenuRangeInt32_t MEO_MOUSEADVANCED_DEADZONE = MAKE_MENURANGE( &MF_Bluefont, 2, 0, 128, 33, 0, &ud.config.MouseDeadZone );
static MenuEntry_t ME_MOUSEADVANCED_DEADZONE = MAKE_MENUENTRY( &MF_Redfont, "Dead Zone", RangeInt32, &MEO_MOUSEADVANCED_DEADZONE );

static MenuEntry_t *MEL_MOUSEADVANCED[] = {
    &ME_MOUSEADVANCED_SCALEX,
    &ME_MOUSEADVANCED_SCALEY,
    &ME_MOUSEADVANCED_DEADZONE,
};

static MenuOption_t MEO_MOUSEADVANCED_DAXES_UP = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[1][0] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_UP = MAKE_MENUENTRY( &MF_BluefontRed, "Digital Up", Option, &MEO_MOUSEADVANCED_DAXES_UP );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_DOWN = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[1][1] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_DOWN = MAKE_MENUENTRY( &MF_BluefontRed, "Digital Down", Option, &MEO_MOUSEADVANCED_DAXES_DOWN );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_LEFT = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[0][0] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_LEFT = MAKE_MENUENTRY( &MF_BluefontRed, "Digital Left", Option, &MEO_MOUSEADVANCED_DAXES_LEFT );
static MenuOption_t MEO_MOUSEADVANCED_DAXES_RIGHT = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, &ud.config.MouseDigitalFunctions[0][1] );
static MenuEntry_t ME_MOUSEADVANCED_DAXES_RIGHT = MAKE_MENUENTRY( &MF_BluefontRed, "Digital Right", Option, &MEO_MOUSEADVANCED_DAXES_RIGHT );

static MenuEntry_t *MEL_MOUSEADVANCED_DAXES[] = {
    &ME_MOUSEADVANCED_DAXES_UP,
    &ME_MOUSEADVANCED_DAXES_DOWN,
    &ME_MOUSEADVANCED_DAXES_LEFT,
    &ME_MOUSEADVANCED_DAXES_RIGHT,
};

static const char *MenuJoystickHatDirections[] = { "Up", "Right", "Down", "Left", };

static char *MEOSN_JOYSTICKAXIS_ANALOG[] = { "  -None-", "Turning", "Strafing", "Looking Up/Down", "Moving", };
static int32_t MEOSV_JOYSTICKAXIS_ANALOG[] = { -1, analog_turning, analog_strafing, analog_lookingupanddown, analog_moving, };
static MenuOptionSet_t MEOS_JOYSTICKAXIS_ANALOG = MAKE_MENUOPTIONSET( 0x0, MEOSN_JOYSTICKAXIS_ANALOG, MEOSV_JOYSTICKAXIS_ANALOG );
static MenuOption_t MEO_JOYSTICKAXIS_ANALOG = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_JOYSTICKAXIS_ANALOG, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_ANALOG = MAKE_MENUENTRY( &MF_Redfont, "Analog", Option, &MEO_JOYSTICKAXIS_ANALOG );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_SCALE = MAKE_MENURANGE( &MF_Bluefont, 3, -262144, 262144, 65, 65536, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_SCALE = MAKE_MENUENTRY( &MF_Redfont, "Scale", RangeInt32, &MEO_JOYSTICKAXIS_SCALE );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_DEAD = MAKE_MENURANGE( &MF_Bluefont, 2, 0, 1000000, 33, 0, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DEAD = MAKE_MENUENTRY( &MF_Redfont, "Dead Zone", RangeInt32, &MEO_JOYSTICKAXIS_DEAD );
static MenuRangeInt32_t MEO_JOYSTICKAXIS_SATU = MAKE_MENURANGE( &MF_Bluefont, 2, 0, 1000000, 33, 0, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_SATU = MAKE_MENUENTRY( &MF_Redfont, "Saturation", RangeInt32, &MEO_JOYSTICKAXIS_SATU );

static MenuOption_t MEO_JOYSTICKAXIS_DIGITALNEGATIVE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DIGITALNEGATIVE = MAKE_MENUENTRY( &MF_BluefontRed, "Digital -", Option, &MEO_JOYSTICKAXIS_DIGITALNEGATIVE );
static MenuOption_t MEO_JOYSTICKAXIS_DIGITALPOSITIVE = MAKE_MENUOPTION( &MF_Minifont, &MEOS_Gamefuncs, NULL );
static MenuEntry_t ME_JOYSTICKAXIS_DIGITALPOSITIVE = MAKE_MENUENTRY( &MF_BluefontRed, "Digital +", Option, &MEO_JOYSTICKAXIS_DIGITALPOSITIVE );

static MenuEntry_t *MEL_JOYSTICKAXIS[] = {
    &ME_JOYSTICKAXIS_ANALOG,
    &ME_JOYSTICKAXIS_SCALE,
    &ME_JOYSTICKAXIS_DEAD,
    &ME_JOYSTICKAXIS_SATU,
};

static MenuEntry_t *MEL_JOYSTICKAXIS_DIGITAL[] = {
    &ME_JOYSTICKAXIS_DIGITALNEGATIVE,
    &ME_JOYSTICKAXIS_DIGITALPOSITIVE,
};

#ifdef USE_OPENGL
static char *MEOSN_RENDERERSETUP_ASPECTRATIO[] = { "Old reg.", "Old wide", "Auto", };
static MenuOptionSet_t MEOS_RENDERERSETUP_ASPECTRATIO = MAKE_MENUOPTIONSET( 0x3, MEOSN_RENDERERSETUP_ASPECTRATIO, NULL );
static MenuOption_t MEO_RENDERERSETUP_ASPECTRATIO = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_ASPECTRATIO, NULL );
#else
static char *MEOSN_RENDERERSETUP_ASPECTRATIO[] = { "Old reg.", "Auto", };
static MenuOptionSet_t MEOS_RENDERERSETUP_ASPECTRATIO = MAKE_MENUOPTIONSET( 0x3, MEOSN_RENDERERSETUP_ASPECTRATIO, NULL );
static MenuOption_t MEO_RENDERERSETUP_ASPECTRATIO = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_ASPECTRATIO, &r_usenewaspect );
#endif
static MenuEntry_t ME_RENDERERSETUP_ASPECTRATIO = MAKE_MENUENTRY( &MF_BluefontRed, "Aspect ratio", Option, &MEO_RENDERERSETUP_ASPECTRATIO );
#ifdef POLYMER
static char *MEOSN_RENDERERSETUP_ASPECTRATIO_POLYMER[] = { "Auto", "1.33:1", "1.60:1", "1.66:1", "1.78:1", "1.85:1", "2.35:1", "2.39:1", };
static double MEOSV_RENDERERSETUP_ASPECTRATIO_POLYMER[] = { 0., 1.33, 1.6, 1.66, 1.78, 1.85, 2.35, 2.39, };
static MenuOptionSet_t MEOS_RENDERERSETUP_ASPECTRATIO_POLYMER = MAKE_MENUOPTIONSET( 0x2, MEOSN_RENDERERSETUP_ASPECTRATIO_POLYMER, NULL );
static MenuOption_t MEO_RENDERERSETUP_ASPECTRATIO_POLYMER = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_ASPECTRATIO_POLYMER, NULL );
static MenuEntry_t ME_RENDERERSETUP_ASPECTRATIO_POLYMER = MAKE_MENUENTRY( &MF_BluefontRed, "Aspect ratio", Option, &MEO_RENDERERSETUP_ASPECTRATIO_POLYMER );
#endif
static MenuRangeFloat_t MEO_RENDERERSETUP_AMBIENT = MAKE_MENURANGE( &MF_Bluefont, 1, 0.125f, 4.f, 32, 0.f, &r_ambientlight );
static MenuEntry_t ME_RENDERERSETUP_AMBIENT = MAKE_MENUENTRY( &MF_BluefontRed, "Ambient light level", RangeFloat, &MEO_RENDERERSETUP_AMBIENT );
#ifdef USE_OPENGL
static char *MEOSN_RENDERERSETUP_ANISOTROPY[] = { "None", "2x", "4x", "8x", "16x", };
static int32_t MEOSV_RENDERERSETUP_ANISOTROPY[] = { 0, 2, 4, 8, 16, };
static MenuOptionSet_t MEOS_RENDERERSETUP_ANISOTROPY = MAKE_MENUOPTIONSET( 0x2, MEOSN_RENDERERSETUP_ANISOTROPY, MEOSV_RENDERERSETUP_ANISOTROPY );
static MenuOption_t MEO_RENDERERSETUP_ANISOTROPY = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_ANISOTROPY, &glanisotropy );
static MenuEntry_t ME_RENDERERSETUP_ANISOTROPY = MAKE_MENUENTRY( &MF_BluefontRed, "Anisotropic filtering", Option, &MEO_RENDERERSETUP_ANISOTROPY );
static char *MEOSN_RENDERERSETUP_VSYNC[] = { "Adaptive", "No", "Yes", };
static int32_t MEOSV_RENDERERSETUP_VSYNC[] = { -1, 0, 1, };
static MenuOptionSet_t MEOS_RENDERERSETUP_VSYNC = MAKE_MENUOPTIONSET( 0x2, MEOSN_RENDERERSETUP_VSYNC, MEOSV_RENDERERSETUP_VSYNC );
static MenuOption_t MEO_RENDERERSETUP_VSYNC = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_VSYNC, &vsync );
static MenuEntry_t ME_RENDERERSETUP_VSYNC = MAKE_MENUENTRY( &MF_BluefontRed, "Use VSync", Option, &MEO_RENDERERSETUP_VSYNC );
static MenuOption_t MEO_RENDERERSETUP_HIGHTILE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &usehightile );
static MenuEntry_t ME_RENDERERSETUP_HIGHTILE = MAKE_MENUENTRY( &MF_BluefontRed, "Enable hires textures", Option, &MEO_RENDERERSETUP_HIGHTILE );
static MenuRangeInt32_t MEO_RENDERERSETUP_TEXQUALITY = MAKE_MENURANGE( &MF_Bluefont, 0, 2, 0, 3, 0, &r_downsize );
static MenuEntry_t ME_RENDERERSETUP_TEXQUALITY = MAKE_MENUENTRY( &MF_BluefontRed, "Hires texture quality", RangeInt32, &MEO_RENDERERSETUP_TEXQUALITY );
static MenuOption_t MEO_RENDERERSETUP_PRECACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.config.useprecache );
static MenuEntry_t ME_RENDERERSETUP_PRECACHE = MAKE_MENUENTRY( &MF_BluefontRed, "Pre-load map textures", Option, &MEO_RENDERERSETUP_PRECACHE );
static char *MEOSN_RENDERERSETUP_TEXCACHE[] = { "Off", "On", "Compress", };
static MenuOptionSet_t MEOS_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTIONSET( 0x2, MEOSN_RENDERERSETUP_TEXCACHE, NULL );
static MenuOption_t MEO_RENDERERSETUP_TEXCACHE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_RENDERERSETUP_TEXCACHE, &glusetexcache );
static MenuEntry_t ME_RENDERERSETUP_TEXCACHE = MAKE_MENUENTRY( &MF_BluefontRed, "On disk texture cache", Option, &MEO_RENDERERSETUP_TEXCACHE );
static MenuOption_t MEO_RENDERERSETUP_DETAILTEX = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &r_detailmapping );
static MenuEntry_t ME_RENDERERSETUP_DETAILTEX = MAKE_MENUENTRY( &MF_BluefontRed, "Use detail textures", Option, &MEO_RENDERERSETUP_DETAILTEX );
static MenuOption_t MEO_RENDERERSETUP_MODELS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &usemodels );
static MenuEntry_t ME_RENDERERSETUP_MODELS = MAKE_MENUENTRY( &MF_BluefontRed, "Use models", Option, &MEO_RENDERERSETUP_MODELS );
#endif

static MenuEntry_t *MEL_RENDERERSETUP[] = {
    &ME_RENDERERSETUP_ASPECTRATIO,
    &ME_RENDERERSETUP_AMBIENT,
};
#ifdef USE_OPENGL
static MenuEntry_t *MEL_RENDERERSETUP_GL1[] = {
    &ME_RENDERERSETUP_ASPECTRATIO,
    &ME_RENDERERSETUP_AMBIENT,
    &ME_RENDERERSETUP_ANISOTROPY,
    &ME_RENDERERSETUP_VSYNC,
};
#ifdef POLYMER
static MenuEntry_t *MEL_RENDERERSETUP_GL1_POLYMER[] = {
    &ME_RENDERERSETUP_ASPECTRATIO_POLYMER,
    &ME_RENDERERSETUP_AMBIENT,
    &ME_RENDERERSETUP_ANISOTROPY,
    &ME_RENDERERSETUP_VSYNC,
};
#endif
static MenuEntry_t *MEL_RENDERERSETUP_GL2[] = {
    &ME_RENDERERSETUP_HIGHTILE,
    &ME_RENDERERSETUP_TEXQUALITY,
    &ME_RENDERERSETUP_PRECACHE,
    &ME_RENDERERSETUP_TEXCACHE,
    &ME_RENDERERSETUP_DETAILTEX,
};
static MenuEntry_t *MEL_RENDERERSETUP_GL3[] = {
    &ME_RENDERERSETUP_MODELS,
};
#endif

static MenuRangeDouble_t MEO_COLCORR_GAMMA = MAKE_MENURANGE( &MF_Bluefont, 1, 0.2f, 4.f, 39, 0.f, &vid_gamma );
static MenuEntry_t ME_COLCORR_GAMMA = MAKE_MENUENTRY( &MF_Redfont, "Gamma", RangeDouble, &MEO_COLCORR_GAMMA );
static MenuRangeDouble_t MEO_COLCORR_CONTRAST = MAKE_MENURANGE( &MF_Bluefont, 1, 0.1f, 2.7f, 53, 0.f, &vid_contrast );
static MenuEntry_t ME_COLCORR_CONTRAST = MAKE_MENUENTRY( &MF_Redfont, "Contrast", RangeDouble, &MEO_COLCORR_CONTRAST );
static MenuRangeDouble_t MEO_COLCORR_BRIGHTNESS = MAKE_MENURANGE( &MF_Bluefont, 1, -0.8f, 0.8f, 33, 0.f, &vid_brightness );
static MenuEntry_t ME_COLCORR_BRIGHTNESS = MAKE_MENUENTRY( &MF_Redfont, "Brightness", RangeDouble, &MEO_COLCORR_BRIGHTNESS );
static MenuEntry_t ME_COLCORR_RESET = MAKE_MENUENTRY( &MF_Redfont, "Reset To Defaults", Link, &MEO_NULL );

static MenuEntry_t *MEL_COLCORR[] = {
    &ME_COLCORR_GAMMA,
    &ME_COLCORR_CONTRAST,
    &ME_COLCORR_BRIGHTNESS,
};
static MenuEntry_t *MEL_COLCORR_RESET[] = {
    &ME_COLCORR_RESET,
};

// Save and load will be filled in before every viewing of the save/load screen.
static MenuLink_t MEO_LOAD = { MENU_NULL, };
static MenuLink_t MEO_LOAD_VALID = { MENU_LOADVERIFY, };
static MenuEntry_t ME_LOAD_TEMPLATE = MAKE_MENUENTRY( &MF_MinifontRed, NULL, Link, &MEO_LOAD );
static MenuEntry_t ME_LOAD[MAXSAVEGAMES];
static MenuEntry_t *MEL_LOAD[MAXSAVEGAMES];

static MenuString_t MEO_SAVE_TEMPLATE = MAKE_MENUSTRING( &MF_MinifontRed, NULL, MAXSAVEGAMENAME, 0 );
static MenuString_t MEO_SAVE[MAXSAVEGAMES];
static MenuEntry_t ME_SAVE_TEMPLATE = MAKE_MENUENTRY( &MF_MinifontRed, NULL, String, &MEO_SAVE_TEMPLATE );
static MenuEntry_t ME_SAVE[MAXSAVEGAMES];
static MenuEntry_t *MEL_SAVE[MAXSAVEGAMES];

static int32_t soundrate, soundbits, soundvoices;
static MenuOption_t MEO_SOUND = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.config.SoundToggle );
static MenuEntry_t ME_SOUND = MAKE_MENUENTRY( &MF_BluefontRed, "Sound", Option, &MEO_SOUND );
static MenuOption_t MEO_SOUND_MUSIC = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.config.MusicToggle );
static MenuEntry_t ME_SOUND_MUSIC = MAKE_MENUENTRY( &MF_BluefontRed, "Music", Option, &MEO_SOUND_MUSIC );
static MenuRangeInt32_t MEO_SOUND_VOLUME_MASTER = MAKE_MENURANGE( &MF_Bluefont, 2, 0, 255, 17, 0, &ud.config.MasterVolume );
static MenuEntry_t ME_SOUND_VOLUME_MASTER = MAKE_MENUENTRY( &MF_BluefontRed, "Master volume", RangeInt32, &MEO_SOUND_VOLUME_MASTER );
static MenuRangeInt32_t MEO_SOUND_VOLUME_EFFECTS = MAKE_MENURANGE( &MF_Bluefont, 2, 0, 255, 17, 0, &ud.config.FXVolume );
static MenuEntry_t ME_SOUND_VOLUME_EFFECTS = MAKE_MENUENTRY( &MF_BluefontRed, "Effects volume", RangeInt32, &MEO_SOUND_VOLUME_EFFECTS );
static MenuRangeInt32_t MEO_SOUND_VOLUME_MUSIC = MAKE_MENURANGE( &MF_Bluefont, 2, 0, 255, 17, 0, &ud.config.MusicVolume );
static MenuEntry_t ME_SOUND_VOLUME_MUSIC = MAKE_MENUENTRY( &MF_BluefontRed, "Music volume", RangeInt32, &MEO_SOUND_VOLUME_MUSIC );
static char *MEOSN_SOUND_SAMPLINGRATE[] = { "22050 Hz", "24000 Hz", "32000 Hz", "44100 Hz", "48000 Hz", };
static int32_t MEOSV_SOUND_SAMPLINGRATE[] = { 22050, 24000, 32000, 44100, 48000, };
static MenuOptionSet_t MEOS_SOUND_SAMPLINGRATE = MAKE_MENUOPTIONSET( 0x0, MEOSN_SOUND_SAMPLINGRATE, MEOSV_SOUND_SAMPLINGRATE );
static MenuOption_t MEO_SOUND_SAMPLINGRATE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_SOUND_SAMPLINGRATE, &soundrate );
static MenuEntry_t ME_SOUND_SAMPLINGRATE = MAKE_MENUENTRY( &MF_BluefontRed, "Playback sampling rate", Option, &MEO_SOUND_SAMPLINGRATE );
static char *MEOSN_SOUND_SAMPLESIZE[] = { "8-bit", "16-bit", };
static int32_t MEOSV_SOUND_SAMPLESIZE[] = { 8, 16, };
static MenuOptionSet_t MEOS_SOUND_SAMPLESIZE = MAKE_MENUOPTIONSET( 0x3, MEOSN_SOUND_SAMPLESIZE, MEOSV_SOUND_SAMPLESIZE );
static MenuOption_t MEO_SOUND_SAMPLESIZE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_SOUND_SAMPLESIZE, &soundbits );
static MenuEntry_t ME_SOUND_SAMPLESIZE = MAKE_MENUENTRY( &MF_BluefontRed, "Sample size", Option, &MEO_SOUND_SAMPLESIZE );
static MenuRangeInt32_t MEO_SOUND_NUMVOICES = MAKE_MENURANGE( &MF_Bluefont, 1, 4, 256, 64, 0, &soundvoices );
static MenuEntry_t ME_SOUND_NUMVOICES = MAKE_MENUENTRY( &MF_BluefontRed, "Number of voices", RangeInt32, &MEO_SOUND_NUMVOICES );
static MenuEntry_t ME_SOUND_RESTART = MAKE_MENUENTRY( &MF_BluefontRed, "Restart sound system", Link, &MEO_NULL );
static MenuOption_t MEO_SOUND_DUKETALK = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, NULL );
static MenuEntry_t ME_SOUND_DUKETALK = MAKE_MENUENTRY( &MF_BluefontRed, "Duke talk", Option, &MEO_SOUND_DUKETALK );
static MenuOption_t MEO_SOUND_DUKEMATCHPLAYER = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, NULL );
static MenuEntry_t ME_SOUND_DUKEMATCHPLAYER = MAKE_MENUENTRY( &MF_BluefontRed, "Dukematch player sounds", Option, &MEO_SOUND_DUKEMATCHPLAYER );
static MenuOption_t MEO_SOUND_AMBIENT = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.config.AmbienceToggle );
static MenuEntry_t ME_SOUND_AMBIENT = MAKE_MENUENTRY( &MF_BluefontRed, "Ambient sounds", Option, &MEO_SOUND_AMBIENT );
static MenuOption_t MEO_SOUND_REVERSESTEREO = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NoYes, &ud.config.ReverseStereo );
static MenuEntry_t ME_SOUND_REVERSESTEREO = MAKE_MENUENTRY( &MF_BluefontRed, "Reverse stereo channels", Option, &MEO_SOUND_REVERSESTEREO );

static MenuEntry_t *MEL_SOUND1[] = {
    &ME_SOUND,
    &ME_SOUND_MUSIC,
    &ME_SOUND_VOLUME_MASTER,
    &ME_SOUND_VOLUME_EFFECTS,
    &ME_SOUND_VOLUME_MUSIC,
};
static MenuEntry_t *MEL_SOUND2[] = {
    &ME_SOUND_SAMPLINGRATE,
    &ME_SOUND_SAMPLESIZE,
    &ME_SOUND_NUMVOICES,
};
static MenuEntry_t *MEL_SOUND3[] = {
    &ME_SOUND_RESTART,
};
static MenuEntry_t *MEL_SOUND4[] = {
    &ME_SOUND_DUKETALK,
    &ME_SOUND_DUKEMATCHPLAYER,
    &ME_SOUND_AMBIENT,
    &ME_SOUND_REVERSESTEREO,
};

static MenuOption_t MEO_ADULTMODE = MAKE_MENUOPTION( &MF_Redfont, &MEOS_OnOff, &ud.lockout );
static MenuEntry_t ME_ADULTMODE = MAKE_MENUENTRY( &MF_Redfont, "Adult Mode", Option, &MEO_ADULTMODE );
static MenuLink_t MEO_ADULTMODE_PASSWORD = { MENU_ADULTPASSWORD, };
static MenuEntry_t ME_ADULTMODE_PASSWORD = MAKE_MENUENTRY( &MF_Redfont, "Enter Password", Link, &MEO_ADULTMODE_PASSWORD );

static MenuEntry_t *MEL_ADULTMODE[] = {
    &ME_ADULTMODE,
    &ME_ADULTMODE_PASSWORD,
};

MAKE_MENU_TOP_ENTRYLINK( NETWORK_PLAYERSETUP, MENU_PLAYER, "Player Setup" );
MAKE_MENU_TOP_ENTRYLINK( NETWORK_JOINGAME, MENU_NETJOIN, "Join Game" );
MAKE_MENU_TOP_ENTRYLINK( NETWORK_HOSTGAME, MENU_NETHOST, "Host Game" );

static MenuEntry_t *MEL_NETWORK[] = {
    &ME_NETWORK_PLAYERSETUP,
    &ME_NETWORK_JOINGAME,
    &ME_NETWORK_HOSTGAME,
};

static MenuString_t MEO_PLAYER_NAME = MAKE_MENUSTRING( &MF_Bluefont, szPlayerName, MAXPLAYERNAME, 0 );
static MenuEntry_t ME_PLAYER_NAME = MAKE_MENUENTRY( &MF_BluefontRed, "Name", String, &MEO_PLAYER_NAME );
static char *MEOSN_PLAYER_COLOR[] = { "Auto", "Blue", "Red", "Green", "Gray", "Dark gray", "Dark green", "Brown", "Dark blue", "Bright red", "Yellow", };
static int32_t MEOSV_PLAYER_COLOR[] = { 0, 9, 10, 11, 12, 13, 14, 15, 16, 22, 23, };
static MenuOptionSet_t MEOS_PLAYER_COLOR = MAKE_MENUOPTIONSET( 0x2, MEOSN_PLAYER_COLOR, MEOSV_PLAYER_COLOR );
static MenuOption_t MEO_PLAYER_COLOR = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_COLOR, &ud.color );
static MenuEntry_t ME_PLAYER_COLOR = MAKE_MENUENTRY( &MF_BluefontRed, "Color", Option, &MEO_PLAYER_COLOR );
static char *MEOSN_PLAYER_TEAM[] = { "Blue", "Red", "Green", "Gray", };
static MenuOptionSet_t MEOS_PLAYER_TEAM = MAKE_MENUOPTIONSET( 0x2, MEOSN_PLAYER_TEAM, NULL );
static MenuOption_t MEO_PLAYER_TEAM = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_TEAM, &ud.team );
static MenuEntry_t ME_PLAYER_TEAM = MAKE_MENUENTRY( &MF_BluefontRed, "Team", Option, &MEO_PLAYER_TEAM );
static char *MEOSN_PLAYER_AIM_AUTO[] = { "Off", "All weapons", "Bullets only", };
static MenuOptionSet_t MEOS_PLAYER_AIM_AUTO = MAKE_MENUOPTIONSET( 0x2, MEOSN_PLAYER_AIM_AUTO, NULL );
static MenuOption_t MEO_PLAYER_AIM_AUTO = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_AIM_AUTO, &ud.config.AutoAim );
static MenuEntry_t ME_PLAYER_AIM_AUTO = MAKE_MENUENTRY( &MF_BluefontRed, "Auto aim", Option, &MEO_PLAYER_AIM_AUTO );
static char *MEOSN_PLAYER_AIM_MOUSE[] = { "Toggle on/off", "Hold button", };
static MenuOptionSet_t MEOS_PLAYER_AIM_MOUSE = MAKE_MENUOPTIONSET( 0x2, MEOSN_PLAYER_AIM_MOUSE, NULL );
static MenuOption_t MEO_PLAYER_AIM_MOUSE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_AIM_MOUSE, &ud.mouseaiming );
static MenuEntry_t ME_PLAYER_AIM_MOUSE = MAKE_MENUENTRY( &MF_BluefontRed, "Mouse aim", Option, &MEO_PLAYER_AIM_MOUSE );
static char *MEOSN_PLAYER_WEAPSWITCH_PICKUP[] = { "Off", "All weapons", "Fav priority", };
static MenuOptionSet_t MEOS_PLAYER_WEAPSWITCH_PICKUP = MAKE_MENUOPTIONSET( 0x2, MEOSN_PLAYER_WEAPSWITCH_PICKUP, NULL );
static MenuOption_t MEO_PLAYER_WEAPSWITCH_PICKUP = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_PLAYER_WEAPSWITCH_PICKUP, NULL );
static MenuEntry_t ME_PLAYER_WEAPSWITCH_PICKUP = MAKE_MENUENTRY( &MF_BluefontRed, "Switch weapons on pickup", Option, &MEO_PLAYER_WEAPSWITCH_PICKUP );
static MenuOption_t MEO_PLAYER_WEAPSWITCH_EMPTY = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, NULL );
static MenuEntry_t ME_PLAYER_WEAPSWITCH_EMPTY = MAKE_MENUENTRY( &MF_BluefontRed, "Switch weapons when empty", Option, &MEO_PLAYER_WEAPSWITCH_EMPTY );
static MenuLink_t MEO_PLAYER_MACROS = { MENU_MACROS, };
static MenuEntry_t ME_PLAYER_MACROS = MAKE_MENUENTRY( &MF_BluefontRed, "Multiplayer macros", Link, &MEO_PLAYER_MACROS );

static MenuEntry_t *MEL_PLAYER_NAME[] = {
    &ME_PLAYER_NAME,
};
static MenuEntry_t *MEL_PLAYER_COLOR[] = {
    &ME_PLAYER_COLOR,
};
static MenuEntry_t *MEL_PLAYER_TEAM[] = {
    &ME_PLAYER_TEAM,
};
static MenuEntry_t *MEL_PLAYER_AIM[] = {
    &ME_PLAYER_AIM_AUTO,
    &ME_PLAYER_AIM_MOUSE,
};
static MenuEntry_t *MEL_PLAYER_WEAPSWITCH[] = {
    &ME_PLAYER_WEAPSWITCH_PICKUP,
    &ME_PLAYER_WEAPSWITCH_EMPTY,
};
static MenuEntry_t *MEL_PLAYER_MACROS[] = {
    &ME_PLAYER_MACROS,
};

static MenuString_t MEO_MACROS_TEMPLATE = MAKE_MENUSTRING( &MF_Bluefont, NULL, MAXRIDECULELENGTH, 0 );
static MenuString_t MEO_MACROS[MAXSAVEGAMES];
static MenuEntry_t ME_MACROS_TEMPLATE = MAKE_MENUENTRY( &MF_Bluefont, NULL, String, &MEO_MACROS_TEMPLATE );
static MenuEntry_t ME_MACROS[MAXRIDECULE];
static MenuEntry_t *MEL_MACROS[MAXRIDECULE];

static char *MenuUserMap = "User Map";
static char *MenuSkillNone = "None";

static char *MEOSN_NetGametypes[MAXGAMETYPES];
static char *MEOSN_NetEpisodes[MAXVOLUMES+1];
static char *MEOSN_NetLevels[MAXVOLUMES][MAXLEVELS];
static char *MEOSN_NetSkills[MAXSKILLS+1];

static MenuLink_t MEO_NETHOST_OPTIONS =  { MENU_NETOPTIONS, };
static MenuEntry_t ME_NETHOST_OPTIONS = MAKE_MENUENTRY( &MF_Redfont, "Game Options", Link, &MEO_NETHOST_OPTIONS );
static MenuEntry_t ME_NETHOST_LAUNCH = MAKE_MENUENTRY( &MF_Redfont, "Launch Game", Link, &MEO_NULL );

static MenuEntry_t *MEL_NETHOST[] = {
    &ME_NETHOST_OPTIONS,
    &ME_NETHOST_LAUNCH,
};

static MenuOptionSet_t MEOS_NETOPTIONS_GAMETYPE = MAKE_MENUOPTIONSET( 0x0, MEOSN_NetGametypes, NULL );
static MenuOption_t MEO_NETOPTIONS_GAMETYPE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_GAMETYPE, &ud.m_coop );
static MenuEntry_t ME_NETOPTIONS_GAMETYPE = MAKE_MENUENTRY( &MF_Redfont, "Game Type", Option, &MEO_NETOPTIONS_GAMETYPE );
static MenuOptionSet_t MEOS_NETOPTIONS_EPISODE = MAKE_MENUOPTIONSET( 0x0, MEOSN_NetEpisodes, NULL );
static MenuOption_t MEO_NETOPTIONS_EPISODE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_EPISODE, NULL );
static MenuEntry_t ME_NETOPTIONS_EPISODE = MAKE_MENUENTRY( &MF_Redfont, "Episode", Option, &MEO_NETOPTIONS_EPISODE );
static MenuOptionSet_t MEOS_NETOPTIONS_LEVEL[MAXVOLUMES];
static MenuOption_t MEO_NETOPTIONS_LEVEL = MAKE_MENUOPTION( &MF_Bluefont, NULL, &ud.m_level_number );
static MenuEntry_t ME_NETOPTIONS_LEVEL = MAKE_MENUENTRY( &MF_Redfont, "Level", Option, &MEO_NETOPTIONS_LEVEL );
static MenuLink_t MEO_NETOPTIONS_USERMAP =  { MENU_NETUSERMAP, };
static MenuEntry_t ME_NETOPTIONS_USERMAP = MAKE_MENUENTRY( &MF_Redfont, "User Map", Link, &MEO_NETOPTIONS_USERMAP );
static MenuOptionSet_t MEOS_NETOPTIONS_MONSTERS = MAKE_MENUOPTIONSET( 0x0, MEOSN_NetSkills, NULL );
static MenuOption_t MEO_NETOPTIONS_MONSTERS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_NETOPTIONS_MONSTERS, NULL );
static MenuEntry_t ME_NETOPTIONS_MONSTERS = MAKE_MENUENTRY( &MF_Redfont, "Monsters", Option, &MEO_NETOPTIONS_MONSTERS );
static MenuOption_t MEO_NETOPTIONS_MARKERS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.m_marker );
static MenuEntry_t ME_NETOPTIONS_MARKERS = MAKE_MENUENTRY( &MF_Redfont, "Markers", Option, &MEO_NETOPTIONS_MARKERS );
static MenuEntry_t ME_NETOPTIONS_MARKERS_DISABLED = MAKE_MENUENTRY( &MF_RedfontBlue, "Markers", Dummy, NULL );
static MenuOption_t MEO_NETOPTIONS_MAPEXITS = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OnOff, &ud.m_noexits );
static MenuEntry_t ME_NETOPTIONS_MAPEXITS = MAKE_MENUENTRY( &MF_Redfont, "Map Exits", Option, &MEO_NETOPTIONS_MAPEXITS );
static MenuOption_t MEO_NETOPTIONS_FRFIRE = MAKE_MENUOPTION( &MF_Bluefont, &MEOS_OffOn, &ud.m_ffire );
static MenuEntry_t ME_NETOPTIONS_FRFIRE = MAKE_MENUENTRY( &MF_Redfont, "Fr. Fire", Option, &MEO_NETOPTIONS_FRFIRE );
static MenuEntry_t ME_NETOPTIONS_ACCEPT = MAKE_MENUENTRY( &MF_RedfontGreen, "Accept", Link, &MEO_NULL );

static MenuEntry_t *MEL_NETOPTIONS[] = {
    &ME_NETOPTIONS_GAMETYPE,
    &ME_NETOPTIONS_EPISODE,
    &ME_NETOPTIONS_LEVEL,
    &ME_NETOPTIONS_MONSTERS,
    &ME_NETOPTIONS_MARKERS,
    &ME_NETOPTIONS_MAPEXITS,
    &ME_NETOPTIONS_ACCEPT,
};

static char MenuServer[BMAX_PATH] = "localhost";
static MenuString_t MEO_NETJOIN_SERVER = MAKE_MENUSTRING( &MF_Bluefont, MenuServer, BMAX_PATH, 0 );
static MenuEntry_t ME_NETJOIN_SERVER = MAKE_MENUENTRY( &MF_Redfont, "Server", String, &MEO_NETJOIN_SERVER );
#define MAXPORTSTRINGLENGTH 6 // unsigned 16-bit integer
static char MenuPort[MAXPORTSTRINGLENGTH] = "19014";
static MenuString_t MEO_NETJOIN_PORT = MAKE_MENUSTRING( &MF_Bluefont, MenuPort, MAXPORTSTRINGLENGTH, INPUT_NUMERIC );
static MenuEntry_t ME_NETJOIN_PORT = MAKE_MENUENTRY( &MF_Redfont, "Port", String, &MEO_NETJOIN_PORT );
static MenuEntry_t ME_NETJOIN_CONNECT = MAKE_MENUENTRY( &MF_RedfontGreen, "Connect", Link, &MEO_NULL );

static MenuEntry_t *MEL_NETJOIN[] = {
    &ME_NETJOIN_SERVER,
    &ME_NETJOIN_PORT,
    &ME_NETJOIN_CONNECT,
};


#define MAKE_MENUGROUP(Entries, ...) { Entries, ARRAY_SIZE(Entries), __VA_ARGS__, 0, 0, }

static MenuGroup_t MG_MAIN = MAKE_MENUGROUP( MEL_MAIN, &MP_TOP_MAIN );
static MenuGroup_t *MGL_MAIN[] = {
    &MG_MAIN,
};

static MenuGroup_t MG_MAIN_INGAME = MAKE_MENUGROUP( MEL_MAIN_INGAME, &MP_TOP_MAIN );
static MenuGroup_t *MGL_MAIN_INGAME[] = {
    &MG_MAIN_INGAME,
};

static MenuGroup_t MG_EPISODE = MAKE_MENUGROUP( MEL_EPISODE, &MP_TOP_EPISODE );
static MenuGroup_t MG_EPISODE_USERMAP = MAKE_MENUGROUP( MEL_EPISODE_USERMAP, &MP_TOP_EPISODE );
static MenuGroup_t *MGL_EPISODE[] = {
    &MG_EPISODE,
    &MG_EPISODE_USERMAP,
};
static MenuGroup_t *MGL_EPISODE_SHAREWARE[] = {
    &MG_EPISODE,
};

static MenuGroup_t MG_SKILL = MAKE_MENUGROUP( MEL_SKILL, &MP_TOP_SKILL );
static MenuGroup_t *MGL_SKILL[] = {
    &MG_SKILL,
};

static MenuGroup_t MG_GAMESETUP1 = MAKE_MENUGROUP( MEL_GAMESETUP1, &MP_OPTIONS );
static MenuGroup_t MG_GAMESETUP2 = MAKE_MENUGROUP( MEL_GAMESETUP2, &MP_OPTIONS );
static MenuGroup_t MG_GAMESETUP3 = MAKE_MENUGROUP( MEL_GAMESETUP3, &MP_OPTIONS );
static MenuGroup_t MG_GAMESETUP4 = MAKE_MENUGROUP( MEL_GAMESETUP4, &MP_OPTIONS );
static MenuGroup_t MG_GAMESETUP5 = MAKE_MENUGROUP( MEL_GAMESETUP5, &MP_OPTIONS );
static MenuGroup_t MG_GAMESETUP6 = MAKE_MENUGROUP( MEL_GAMESETUP6, &MP_OPTIONS );
static MenuGroup_t MG_GAMESETUP7 = MAKE_MENUGROUP( MEL_GAMESETUP7, &MP_OPTIONS );
static MenuGroup_t MG_GAMESETUP8 = MAKE_MENUGROUP( MEL_GAMESETUP8, &MP_OPTIONS );
static MenuGroup_t MG_GAMESETUP9 = MAKE_MENUGROUP( MEL_GAMESETUP9, &MP_OPTIONS );
#ifdef _WIN32
static MenuGroup_t MG_GAMESETUP_UPDATES = MAKE_MENUGROUP( MEL_GAMESETUP_UPDATES, &MP_OPTIONS );
#endif
static MenuGroup_t *MGL_GAMESETUP[] = {
    &MG_GAMESETUP1,
    &MG_GAMESETUP2,
    &MG_GAMESETUP3,
    &MG_GAMESETUP4,
    &MG_GAMESETUP5,
    &MG_GAMESETUP6,
    &MG_GAMESETUP7,
    &MG_GAMESETUP8,
    &MG_GAMESETUP9,
#ifdef _WIN32
    &MG_GAMESETUP_UPDATES,
#endif
};

static MenuGroup_t MG_OPTIONS = MAKE_MENUGROUP( MEL_OPTIONS, &MP_TOP_OPTIONS );
static MenuGroup_t *MGL_OPTIONS[] = {
    &MG_OPTIONS,
};

static MenuGroup_t MG_VIDEOSETUP1 = MAKE_MENUGROUP( MEL_VIDEOSETUP1, &MP_VIDEOSETUP );
static MenuGroup_t MG_VIDEOSETUP_APPLY = MAKE_MENUGROUP( MEL_VIDEOSETUP_APPLY, &MP_VIDEOSETUP_APPLY );
static MenuGroup_t MG_VIDEOSETUP2 = MAKE_MENUGROUP( MEL_VIDEOSETUP2, &MP_VIDEOSETUP );
#ifdef USE_OPENGL
static MenuGroup_t MG_VIDEOSETUP2_GL = MAKE_MENUGROUP( MEL_VIDEOSETUP2_GL, &MP_VIDEOSETUP );
#endif
static MenuGroup_t *MGL_VIDEOSETUP[] = {
    &MG_VIDEOSETUP1,
    &MG_VIDEOSETUP_APPLY,
    &MG_VIDEOSETUP2,
};
#ifdef USE_OPENGL
static MenuGroup_t *MGL_VIDEOSETUP_GL[] = {
    &MG_VIDEOSETUP1,
    &MG_VIDEOSETUP_APPLY,
    &MG_VIDEOSETUP2_GL,
};
#endif

static MenuGroup_t MG_KEYBOARDSETUP = MAKE_MENUGROUP( MEL_KEYBOARDSETUP, &MP_KEYBOARDSETUP );
static MenuGroup_t *MGL_KEYBOARDSETUP[] = {
    &MG_KEYBOARDSETUP,
};

static MenuGroup_t MG_MOUSESETUP = MAKE_MENUGROUP( MEL_MOUSESETUP, &MP_MOUSESETUP );
static MenuGroup_t *MGL_MOUSESETUP[] = {
    &MG_MOUSESETUP,
};

static MenuGroup_t MG_JOYSTICKSETUP = MAKE_MENUGROUP( MEL_JOYSTICKSETUP, &MP_TOP_JOYSTICK_NETWORK );
static MenuGroup_t *MGL_JOYSTICKSETUP[] = {
    &MG_JOYSTICKSETUP,
};

static MenuGroup_t MG_JOYSTICKBTNS = MAKE_MENUGROUP( MEL_JOYSTICKBTNS, &MP_MOUSEJOYSETUPBTNS );
static MenuGroup_t *MGL_JOYSTICKBTNS[] = {
    &MG_JOYSTICKBTNS,
};

static MenuGroup_t MG_KEYBOARDSETUPFUNCS = MAKE_MENUGROUP( MEL_KEYBOARDSETUPFUNCS, &MP_KEYBOARDSETUPFUNCS );
static MenuGroup_t *MGL_KEYBOARDKEYS[] = {
    &MG_KEYBOARDSETUPFUNCS,
};

static MenuGroup_t MG_MOUSESETUPBTNS = MAKE_MENUGROUP( MEL_MOUSESETUPBTNS, &MP_MOUSEJOYSETUPBTNS );
static MenuGroup_t *MGL_MOUSEBTNS[] = {
    &MG_MOUSESETUPBTNS,
};

static MenuGroup_t MG_JOYSTICKAXES = MAKE_MENUGROUP( MEL_JOYSTICKAXES, &MP_REDSLIDE );
static MenuGroup_t *MGL_JOYSTICKAXES[] = {
    &MG_JOYSTICKAXES,
};

static MenuGroup_t MG_MOUSEADVANCED = MAKE_MENUGROUP( MEL_MOUSEADVANCED, &MP_REDSLIDE );
static MenuGroup_t MG_MOUSEADVANCED_DAXES = MAKE_MENUGROUP( MEL_MOUSEADVANCED_DAXES, &MP_REDSLIDE );
static MenuGroup_t *MGL_MOUSEADVANCED[] = {
    &MG_MOUSEADVANCED,
    &MG_MOUSEADVANCED_DAXES,
};

static MenuGroup_t MG_JOYSTICKAXIS = MAKE_MENUGROUP( MEL_JOYSTICKAXIS, &MP_REDSLIDE );
static MenuGroup_t MG_JOYSTICKAXIS_DIGITAL = MAKE_MENUGROUP( MEL_JOYSTICKAXIS_DIGITAL, &MP_REDSLIDE );
static MenuGroup_t *MGL_JOYSTICKAXIS[] = {
    &MG_JOYSTICKAXIS,
    &MG_JOYSTICKAXIS_DIGITAL,
};

static MenuGroup_t MG_RENDERERSETUP = MAKE_MENUGROUP( MEL_RENDERERSETUP, &MP_OPTIONS );
static MenuGroup_t *MGL_RENDERERSETUP[] = {
    &MG_RENDERERSETUP,
};

#ifdef USE_OPENGL
static MenuGroup_t MG_RENDERERSETUP_GL1 = MAKE_MENUGROUP( MEL_RENDERERSETUP_GL1, &MP_OPTIONS );
#ifdef POLYMER
static MenuGroup_t MG_RENDERERSETUP_GL1_POLYMER = MAKE_MENUGROUP( MEL_RENDERERSETUP_GL1_POLYMER, &MP_OPTIONS );
#endif
static MenuGroup_t MG_RENDERERSETUP_GL2 = MAKE_MENUGROUP( MEL_RENDERERSETUP_GL2, &MP_OPTIONS );
static MenuGroup_t MG_RENDERERSETUP_GL3 = MAKE_MENUGROUP( MEL_RENDERERSETUP_GL3, &MP_OPTIONS );
static MenuGroup_t *MGL_RENDERERSETUP_GL[] = {
    &MG_RENDERERSETUP_GL1,
    &MG_RENDERERSETUP_GL2,
    &MG_RENDERERSETUP_GL3,
};
#ifdef POLYMER
static MenuGroup_t *MGL_RENDERERSETUP_GL_POLYMER[] = {
    &MG_RENDERERSETUP_GL1_POLYMER,
    &MG_RENDERERSETUP_GL2,
    &MG_RENDERERSETUP_GL3,
};
#endif
#endif

static MenuGroup_t MG_COLCORR = MAKE_MENUGROUP( MEL_COLCORR, &MP_COLCORR );
static MenuGroup_t MG_COLCORR_RESET = MAKE_MENUGROUP( MEL_COLCORR_RESET, &MP_COLCORR );
static MenuGroup_t *MGL_COLCORR[] = {
    &MG_COLCORR,
    &MG_COLCORR_RESET,
};

static MenuGroup_t MG_LOAD = MAKE_MENUGROUP( MEL_LOAD, &MP_LOADSAVE );
static MenuGroup_t *MGL_LOAD[] = {
    &MG_LOAD,
};

static MenuGroup_t MG_SAVE = MAKE_MENUGROUP( MEL_SAVE, &MP_LOADSAVE );
static MenuGroup_t *MGL_SAVE[] = {
    &MG_SAVE,
};

static MenuGroup_t MG_SOUND1 = MAKE_MENUGROUP( MEL_SOUND1, &MP_OPTIONS );
static MenuGroup_t MG_SOUND2 = MAKE_MENUGROUP( MEL_SOUND2, &MP_OPTIONS );
static MenuGroup_t MG_SOUND3 = MAKE_MENUGROUP( MEL_SOUND3, &MP_OPTIONS );
static MenuGroup_t MG_SOUND4 = MAKE_MENUGROUP( MEL_SOUND4, &MP_OPTIONS );
static MenuGroup_t *MGL_SOUND[] = {
    &MG_SOUND1,
    &MG_SOUND2,
    &MG_SOUND3,
    &MG_SOUND4,
};

static MenuGroup_t MG_ADULTMODE = MAKE_MENUGROUP( MEL_ADULTMODE, &MP_ADULTMODE );
static MenuGroup_t *MGL_ADULTMODE[] = {
    &MG_ADULTMODE,
};

static MenuGroup_t MG_NETWORK = MAKE_MENUGROUP( MEL_NETWORK, &MP_TOP_JOYSTICK_NETWORK );
static MenuGroup_t *MGL_NETWORK[] = {
    &MG_NETWORK,
};

static MenuGroup_t MG_PLAYER_NAME = MAKE_MENUGROUP( MEL_PLAYER_NAME, &MP_PLAYER_1 );
static MenuGroup_t MG_PLAYER_COLOR = MAKE_MENUGROUP( MEL_PLAYER_COLOR, &MP_PLAYER_1 );
static MenuGroup_t MG_PLAYER_TEAM = MAKE_MENUGROUP( MEL_PLAYER_TEAM, &MP_PLAYER_1 );
static MenuGroup_t MG_PLAYER_AIM = MAKE_MENUGROUP( MEL_PLAYER_AIM, &MP_PLAYER_2 );
static MenuGroup_t MG_PLAYER_WEAPSWITCH = MAKE_MENUGROUP( MEL_PLAYER_WEAPSWITCH, &MP_PLAYER_3 );
static MenuGroup_t MG_PLAYER_MACROS = MAKE_MENUGROUP( MEL_PLAYER_MACROS, &MP_PLAYER_3 );
static MenuGroup_t *MGL_PLAYER[] = {
    &MG_PLAYER_NAME,
    &MG_PLAYER_COLOR,
    &MG_PLAYER_TEAM,
    &MG_PLAYER_AIM,
    &MG_PLAYER_WEAPSWITCH,
    &MG_PLAYER_MACROS,
};

static MenuGroup_t MG_MACROS = MAKE_MENUGROUP( MEL_MACROS, &MP_MACROS );
static MenuGroup_t *MGL_MACROS[] = {
    &MG_MACROS,
};

static MenuGroup_t MG_NETHOST = MAKE_MENUGROUP( MEL_NETHOST, &MP_VIDEOSETUP );
static MenuGroup_t *MGL_NETHOST[] = {
    &MG_NETHOST,
};

static MenuGroup_t MG_NETOPTIONS = MAKE_MENUGROUP( MEL_NETOPTIONS, &MP_NETSETUP );
static MenuGroup_t *MGL_NETOPTIONS[] = {
    &MG_NETOPTIONS,
};

static MenuGroup_t MG_NETJOIN = MAKE_MENUGROUP( MEL_NETJOIN, &MP_VIDEOSETUP );
static MenuGroup_t *MGL_NETJOIN[] = {
    &MG_NETJOIN,
};



#define NoTitle NULL

#define MAKE_MENUMENU(Groups, ...) { Groups, ARRAY_SIZE(Groups), __VA_ARGS__, 0, INT32_MAX, INT32_MIN, 0, 0, }

static MenuMenu_t M_MAIN = MAKE_MENUMENU( MGL_MAIN, NoTitle );
static MenuMenu_t M_MAIN_INGAME = MAKE_MENUMENU( MGL_MAIN_INGAME, NoTitle );
static MenuMenu_t M_EPISODE = MAKE_MENUMENU( MGL_EPISODE, "Select An Episode" );
static MenuMenu_t M_SKILL = MAKE_MENUMENU( MGL_SKILL, "Select Skill" );
static MenuMenu_t M_GAMESETUP = MAKE_MENUMENU( MGL_GAMESETUP, "Game Setup" );
static MenuMenu_t M_OPTIONS = MAKE_MENUMENU( MGL_OPTIONS, "Options" );
static MenuMenu_t M_VIDEOSETUP = MAKE_MENUMENU( MGL_VIDEOSETUP, "Video Setup" );
static MenuMenu_t M_KEYBOARDSETUP = MAKE_MENUMENU( MGL_KEYBOARDSETUP, "Keyboard Setup" );
static MenuMenu_t M_MOUSESETUP = MAKE_MENUMENU( MGL_MOUSESETUP, "Mouse Setup" );
static MenuMenu_t M_JOYSTICKSETUP = MAKE_MENUMENU( MGL_JOYSTICKSETUP, "Joystick Setup" );
static MenuMenu_t M_JOYSTICKBTNS = MAKE_MENUMENU( MGL_JOYSTICKBTNS, "Joystick Buttons" );
static MenuMenu_t M_JOYSTICKAXES = MAKE_MENUMENU( MGL_JOYSTICKAXES, "Joystick Axes" );
static MenuMenu_t M_KEYBOARDKEYS = MAKE_MENUMENU( MGL_KEYBOARDKEYS, "Keyboard Keys" );
static MenuMenu_t M_MOUSEBTNS = MAKE_MENUMENU( MGL_MOUSEBTNS, "Mouse Buttons" );
static MenuMenu_t M_MOUSEADVANCED = MAKE_MENUMENU( MGL_MOUSEADVANCED, "Advanced Mouse" );
static MenuMenu_t M_JOYSTICKAXIS = MAKE_MENUMENU( MGL_JOYSTICKAXIS, NULL );
static MenuMenu_t M_RENDERERSETUP = MAKE_MENUMENU( MGL_RENDERERSETUP, "Renderer Setup" );
static MenuMenu_t M_COLCORR = MAKE_MENUMENU( MGL_COLCORR, "Color Correction" );
static MenuMenu_t M_LOAD = MAKE_MENUMENU( MGL_LOAD, "Load Game" );
static MenuMenu_t M_SAVE = MAKE_MENUMENU( MGL_SAVE, "Save Game" );
static MenuMenu_t M_SOUND = MAKE_MENUMENU( MGL_SOUND, "Sound Setup" );
static MenuMenu_t M_ADULTMODE = MAKE_MENUMENU( MGL_ADULTMODE, "Adult Mode" );
static MenuMenu_t M_NETWORK = MAKE_MENUMENU( MGL_NETWORK, "Network Game" );
static MenuMenu_t M_PLAYER = MAKE_MENUMENU( MGL_PLAYER, "Player Setup" );
static MenuMenu_t M_MACROS = MAKE_MENUMENU( MGL_MACROS, "Multiplayer Macros" );
static MenuMenu_t M_NETHOST = MAKE_MENUMENU( MGL_NETHOST, "Host Network Game" );
static MenuMenu_t M_NETOPTIONS = MAKE_MENUMENU( MGL_NETOPTIONS, "Net Game Options" );
static MenuMenu_t M_NETJOIN = MAKE_MENUMENU( MGL_NETJOIN, "Join Network Game" );

static MenuPanel_t M_STORY = { NoTitle, MENU_F1HELP, MENU_F1HELP, };
static MenuPanel_t M_F1HELP = { NoTitle, MENU_STORY, MENU_STORY, };
static MenuPanel_t M_ORDERING = { NoTitle, MENU_ORDERING4, MENU_ORDERING2, };
static MenuPanel_t M_ORDERING2 = { NoTitle, MENU_ORDERING, MENU_ORDERING3, };
static MenuPanel_t M_ORDERING3 = { NoTitle, MENU_ORDERING2, MENU_ORDERING4, };
static MenuPanel_t M_ORDERING4 = { NoTitle, MENU_ORDERING3, MENU_ORDERING, };
static const char* MenuCredits = "Credits";
static MenuPanel_t M_CREDITS = { NoTitle, MENU_CREDITS5, MENU_CREDITS2, };
static MenuPanel_t M_CREDITS2 = { NoTitle, MENU_CREDITS, MENU_CREDITS3, };
static MenuPanel_t M_CREDITS3 = { NoTitle, MENU_CREDITS2, MENU_CREDITS4, };
static MenuPanel_t M_CREDITS4 = { "About EDuke32", MENU_CREDITS3, MENU_CREDITS5, };
static MenuPanel_t M_CREDITS5 = { "About EDuke32", MENU_CREDITS4, MENU_CREDITS, };

#define CURSOR_CENTER_2LINE { 160<<16, 120<<16, }
#define CURSOR_CENTER_3LINE { 160<<16, 129<<16, }
#define CURSOR_BOTTOMRIGHT { 304<<16, 186<<16, }

static MenuVerify_t M_QUIT = { CURSOR_CENTER_2LINE, MENU_CLOSE, };
static MenuVerify_t M_QUITTOTITLE = { CURSOR_CENTER_2LINE, MENU_CLOSE, };
static MenuVerify_t M_LOADVERIFY = { CURSOR_CENTER_3LINE, MENU_CLOSE, };
static MenuVerify_t M_NEWVERIFY = { CURSOR_CENTER_2LINE, MENU_EPISODE, };
static MenuVerify_t M_SAVEVERIFY = { CURSOR_CENTER_2LINE, MENU_SAVE, };
static MenuVerify_t M_RESETPLAYER = { CURSOR_CENTER_3LINE, MENU_CLOSE, };

static MenuMessage_t M_NETWAITMASTER = { CURSOR_BOTTOMRIGHT, MENU_NULL, };
static MenuMessage_t M_NETWAITVOTES = { CURSOR_BOTTOMRIGHT, MENU_NULL, };
static MenuMessage_t M_BUYDUKE = { CURSOR_BOTTOMRIGHT, MENU_EPISODE, };

static MenuPassword_t M_ADULTPASSWORD = { MAXPWLOCKOUT, NULL, };

#define MAKE_MENUFILESELECT(...) { __VA_ARGS__, FNLIST_INITIALIZER, NULL, NULL, NULL, 0, 0, };

static MenuFileSelect_t M_USERMAP = MAKE_MENUFILESELECT( "Select A User Map", &MF_Minifont, &MF_MinifontRed, "*.map", boardfilename );

// MUST be in ascending order of MenuID enum values due to binary search
static Menu_t Menus[] = {
    { MENU_MAIN, MENU_CLOSE, Menu, &M_MAIN, },
    { MENU_MAIN_INGAME, MENU_CLOSE, Menu, &M_MAIN_INGAME, },
    { MENU_EPISODE, MENU_MAIN, Menu, &M_EPISODE, },
    { MENU_USERMAP, MENU_EPISODE, FileSelect, &M_USERMAP, },
    { MENU_SKILL, MENU_EPISODE, Menu, &M_SKILL, },
    { MENU_GAMESETUP, MENU_OPTIONS, Menu, &M_GAMESETUP, },
    { MENU_OPTIONS, MENU_MAIN, Menu, &M_OPTIONS, },
    { MENU_VIDEOSETUP, MENU_OPTIONS, Menu, &M_VIDEOSETUP, },
    { MENU_KEYBOARDSETUP, MENU_OPTIONS, Menu, &M_KEYBOARDSETUP, },
    { MENU_MOUSESETUP, MENU_OPTIONS, Menu, &M_MOUSESETUP, },
    { MENU_JOYSTICKSETUP, MENU_OPTIONS, Menu, &M_JOYSTICKSETUP, },
    { MENU_JOYSTICKBTNS, MENU_JOYSTICKSETUP, Menu, &M_JOYSTICKBTNS, },
    { MENU_JOYSTICKAXES, MENU_JOYSTICKSETUP, Menu, &M_JOYSTICKAXES, },
    { MENU_KEYBOARDKEYS, MENU_KEYBOARDSETUP, Menu, &M_KEYBOARDKEYS, },
    { MENU_MOUSEBTNS, MENU_MOUSESETUP, Menu, &M_MOUSEBTNS, },
    { MENU_MOUSEADVANCED, MENU_MOUSESETUP, Menu, &M_MOUSEADVANCED, },
    { MENU_JOYSTICKAXIS, MENU_JOYSTICKAXES, Menu, &M_JOYSTICKAXIS, },
    { MENU_RENDERERSETUP, MENU_VIDEOSETUP, Menu, &M_RENDERERSETUP, },
    { MENU_COLCORR, MENU_VIDEOSETUP, Menu, &M_COLCORR, },
    { MENU_COLCORR_INGAME, MENU_CLOSE, Menu, &M_COLCORR, },
    { MENU_LOAD, MENU_MAIN, Menu, &M_LOAD, },
    { MENU_SAVE, MENU_MAIN, Menu, &M_SAVE, },
    { MENU_STORY, MENU_MAIN, Panel, &M_STORY, },
    { MENU_F1HELP, MENU_MAIN, Panel, &M_F1HELP, },
    { MENU_ORDERING, MENU_MAIN, Panel, &M_ORDERING, },
    { MENU_ORDERING2, MENU_MAIN, Panel, &M_ORDERING2, },
    { MENU_ORDERING3, MENU_MAIN, Panel, &M_ORDERING3, },
    { MENU_ORDERING4, MENU_MAIN, Panel, &M_ORDERING4, },
    { MENU_QUIT, MENU_PREVIOUS, Verify, &M_QUIT, },
    { MENU_QUITTOTITLE, MENU_PREVIOUS, Verify, &M_QUITTOTITLE, },
    { MENU_QUIT_INGAME, MENU_CLOSE, Verify, &M_QUIT, },
    { MENU_NETSETUP, MENU_MAIN, Menu, &M_NETHOST, },
    { MENU_NETWAITMASTER, MENU_MAIN, Message, &M_NETWAITMASTER, },
    { MENU_NETWAITVOTES, MENU_MAIN, Message, &M_NETWAITVOTES, },
    { MENU_SOUND, MENU_OPTIONS, Menu, &M_SOUND, },
    { MENU_SOUND_INGAME, MENU_CLOSE, Menu, &M_SOUND, },
    { MENU_CREDITS, MENU_MAIN, Panel, &M_CREDITS, },
    { MENU_CREDITS2, MENU_MAIN, Panel, &M_CREDITS2, },
    { MENU_CREDITS3, MENU_MAIN, Panel, &M_CREDITS3, },
    { MENU_CREDITS4, MENU_MAIN, Panel, &M_CREDITS4, },
    { MENU_CREDITS5, MENU_MAIN, Panel, &M_CREDITS5, },
    { MENU_LOADVERIFY, MENU_LOAD, Verify, &M_LOADVERIFY, },
    { MENU_NEWVERIFY, MENU_MAIN, Verify, &M_NEWVERIFY, },
    { MENU_SAVEVERIFY, MENU_SAVE, Verify, &M_SAVEVERIFY, },
    { MENU_ADULTMODE, MENU_GAMESETUP, Menu, &M_ADULTMODE, },
    { MENU_ADULTPASSWORD, MENU_ADULTMODE, Password, &M_ADULTPASSWORD, },
    { MENU_RESETPLAYER, MENU_CLOSE, Verify, &M_RESETPLAYER, },
    { MENU_BUYDUKE, MENU_EPISODE, Message, &M_BUYDUKE, },
    { MENU_NETWORK, MENU_MAIN, Menu, &M_NETWORK, },
    { MENU_PLAYER, MENU_OPTIONS, Menu, &M_PLAYER, },
    { MENU_MACROS, MENU_PLAYER, Menu, &M_MACROS, },
    { MENU_NETHOST, MENU_NETWORK, Menu, &M_NETHOST, },
    { MENU_NETOPTIONS, MENU_NETWORK, Menu, &M_NETOPTIONS, },
    { MENU_NETUSERMAP, MENU_NETOPTIONS, FileSelect, &M_USERMAP, },
    { MENU_NETJOIN, MENU_NETWORK, Menu, &M_NETJOIN, },
};

static const size_t numMenus = ARRAY_SIZE(Menus);

Menu_t *m_currentMenu = &Menus[0];
static Menu_t *m_previousMenu = &Menus[0];

MenuID_t g_currentMenu;
static MenuID_t g_previousMenu;


#define MenuMenu_ChangeGroupList(m, gl)\
    do {\
        m.grouplist = gl;\
        m.numGroups = ARRAY_SIZE(gl);\
    } while (0)

#define MenuGroup_ChangeEntryList(g, el)\
    do {\
        g.entrylist = el;\
        g.numEntries = ARRAY_SIZE(el);\
    } while (0)


/*
This function prepares data after ART and CON have been processed.
It also initializes some data in loops rather than statically at compile time.
*/
void M_Init(void)
{
    int32_t i, j, k;

    // prepare menu fonts
    MF_Redfont.tilenum = MF_RedfontBlue.tilenum = MF_RedfontGreen.tilenum = BIGALPHANUM;
    MF_Bluefont.tilenum = MF_BluefontRed.tilenum = STARTALPHANUM;
    MF_Minifont.tilenum = MF_MinifontRed.tilenum = MF_MinifontDarkGray.tilenum = MINIFONT;
    if (!minitext_lowercase)
    {
        MF_Minifont.textflags |= TEXT_UPPERCASE;
        MF_MinifontRed.textflags |= TEXT_UPPERCASE;
        MF_MinifontDarkGray.textflags |= TEXT_UPPERCASE;
    }

    // prepare gamefuncs and keys
    for (i = 0; i < NUMGAMEFUNCTIONS; ++i)
    {
        Bstrcpy(MenuGameFuncs[i], gamefunctions[i]);

        for (j = 0; j < MAXGAMEFUNCLEN; ++j)
            if (MenuGameFuncs[i][j] == '_')
                MenuGameFuncs[i][j] = ' ';

        MEOSN_Gamefuncs[i] = MenuGameFuncs[i];
    }
    MEOSN_Gamefuncs[NUMGAMEFUNCTIONS] = MenuGameFuncNone;
    for (i = 0; i < NUMKEYS; ++i)
        MEOSN_Keys[i] = key_names[i];
    MEOSN_Keys[NUMKEYS-1] = MenuKeyNone;


    // prepare episodes
    k = -1;
    for (i = 0; i < MAXVOLUMES; ++i)
    {
        if (EpisodeNames[i][0])
        {
            MEL_EPISODE[i] = &ME_EPISODE[i];
            ME_EPISODE[i] = ME_EPISODE_TEMPLATE;
            ME_EPISODE[i].name = EpisodeNames[i];

            MEOSN_NetEpisodes[i] = EpisodeNames[i];

            k = i;
        }

        // prepare levels
        for (j = 0; j < MAXLEVELS; ++j)
        {
            MEOSN_NetLevels[i][j] = MapInfo[MAXLEVELS*i+j].name;
            if (MapInfo[i*MAXLEVELS+j].filename != NULL)
                MEOS_NETOPTIONS_LEVEL[i].numOptions = j+1;
        }
        MEOS_NETOPTIONS_LEVEL[i].optionNames = MEOSN_NetLevels[i];
    }
    ++k;
    MG_EPISODE.numEntries = g_numVolumes; // k;
    MEOS_NETOPTIONS_EPISODE.numOptions = g_numVolumes + 1; // k+1;
    MEOSN_NetEpisodes[g_numVolumes] = MenuUserMap;
    MP_TOP_EPISODE.pos.y = (48-(g_numVolumes*2))<<16;

    // prepare skills
    k = -1;
    for (i = 0; i < g_numSkills && SkillNames[i][0]; ++i)
    {
        MEL_SKILL[i] = &ME_SKILL[i];
        ME_SKILL[i] = ME_SKILL_TEMPLATE;
        ME_SKILL[i].name = SkillNames[i];

        MEOSN_NetSkills[i] = SkillNames[i];

        k = i;
    }
    ++k;
    MG_SKILL.numEntries = g_numSkills; // k;
    MEOS_NETOPTIONS_MONSTERS.numOptions = g_numSkills + 1; // k+1;
    MEOSN_NetSkills[g_numSkills] = MenuSkillNone;
    MP_TOP_SKILL.pos.y = (58 + (4-g_numSkills)*6)<<16;
    MG_SKILL.currentEntry = 1;

    // prepare multiplayer gametypes
    k = -1;
    for (i = 0; i < MAXGAMETYPES; ++i)
        if (GametypeNames[i][0])
        {
            MEOSN_NetGametypes[i] = GametypeNames[i];
            k = i;
        }
    ++k;
    MEOS_NETOPTIONS_GAMETYPE.numOptions = k;

    // prepare savegames
    for (i = 0; i < MAXSAVEGAMES; ++i)
    {
        MEL_LOAD[i] = &ME_LOAD[i];
        MEL_SAVE[i] = &ME_SAVE[i];
        ME_LOAD[i] = ME_LOAD_TEMPLATE;
        ME_SAVE[i] = ME_SAVE_TEMPLATE;
        ME_SAVE[i].entry = &MEO_SAVE[i];
        MEO_SAVE[i] = MEO_SAVE_TEMPLATE;

        ME_LOAD[i].name = ud.savegame[i];
        MEO_SAVE[i].variable = ud.savegame[i];
    }

    // prepare text chat macros
    for (i = 0; i < MAXRIDECULE; ++i)
    {
        MEL_MACROS[i] = &ME_MACROS[i];
        ME_MACROS[i] = ME_MACROS_TEMPLATE;
        ME_MACROS[i].entry = &MEO_MACROS[i];
        MEO_MACROS[i] = MEO_MACROS_TEMPLATE;

        MEO_MACROS[i].variable = ud.ridecule[i];
    }

    // prepare input
    for (i = 0; i < NUMGAMEFUNCTIONS; ++i)
    {
        MEL_KEYBOARDSETUPFUNCS[i] = &ME_KEYBOARDSETUPFUNCS[i];
        ME_KEYBOARDSETUPFUNCS[i] = ME_KEYBOARDSETUPFUNCS_TEMPLATE;
        ME_KEYBOARDSETUPFUNCS[i].name = MenuGameFuncs[i];
        ME_KEYBOARDSETUPFUNCS[i].entry = &MEO_KEYBOARDSETUPFUNCS[i];
        MEO_KEYBOARDSETUPFUNCS[i] = MEO_KEYBOARDSETUPFUNCS_TEMPLATE;
        MEO_KEYBOARDSETUPFUNCS[i].column[0] = &ud.config.KeyboardKeys[i][0];
        MEO_KEYBOARDSETUPFUNCS[i].column[1] = &ud.config.KeyboardKeys[i][1];
    }
    for (i = 0; i < MENUMOUSEFUNCTIONS; ++i)
    {
        MEL_MOUSESETUPBTNS[i] = &ME_MOUSESETUPBTNS[i];
        ME_MOUSESETUPBTNS[i] = ME_MOUSEJOYSETUPBTNS_TEMPLATE;
        ME_MOUSESETUPBTNS[i].name = MenuMouseNames[i];
        ME_MOUSESETUPBTNS[i].entry = &MEO_MOUSESETUPBTNS[i];
        MEO_MOUSESETUPBTNS[i] = MEO_MOUSEJOYSETUPBTNS_TEMPLATE;
        MEO_MOUSESETUPBTNS[i].data = &ud.config.MouseFunctions[MenuMouseDataIndex[i][0]][MenuMouseDataIndex[i][1]];
    }
    for (i = 0; i < 2*joynumbuttons + 2*joynumhats; ++i)
    {
        if (i < 2*joynumbuttons)
        {
            if (i & 1)
                Bsnprintf(MenuJoystickNames[i], MAXJOYBUTTONSTRINGLENGTH, "Double %s", getjoyname(1, i>>1));
            else
                Bstrncpy(MenuJoystickNames[i], getjoyname(1, i>>1), MAXJOYBUTTONSTRINGLENGTH);
        }
        else
        {
            Bsnprintf(MenuJoystickNames[i], MAXJOYBUTTONSTRINGLENGTH, (i & 1) ? "Double Hat %d %s" : "Hat %d %s", ((i - 2*joynumbuttons)>>3), MenuJoystickHatDirections[((i - 2*joynumbuttons)>>1) % 4]);
        }

        MEL_JOYSTICKBTNS[i] = &ME_JOYSTICKBTNS[i];
        ME_JOYSTICKBTNS[i] = ME_MOUSEJOYSETUPBTNS_TEMPLATE;
        ME_JOYSTICKBTNS[i].name = MenuJoystickNames[i];
        ME_JOYSTICKBTNS[i].entry = &MEO_JOYSTICKBTNS[i];
        MEO_JOYSTICKBTNS[i] = MEO_MOUSEJOYSETUPBTNS_TEMPLATE;
        MEO_JOYSTICKBTNS[i].data = &ud.config.JoystickFunctions[i>>1][i&1];
    }
    MG_JOYSTICKBTNS.numEntries = 2*joynumbuttons + 2*joynumhats;
    for (i = 0; i < joynumaxes; ++i)
    {
        ME_JOYSTICKAXES[i] = ME_JOYSTICKAXES_TEMPLATE;
        Bstrncpy(MenuJoystickAxes[i], getjoyname(0, i), MAXJOYBUTTONSTRINGLENGTH);
        ME_JOYSTICKAXES[i].name = MenuJoystickAxes[i];
        MEL_JOYSTICKAXES[i] = &ME_JOYSTICKAXES[i];
    }
    MG_JOYSTICKAXES.numEntries = joynumaxes;

    // prepare video setup
    for (i = 0; i < validmodecnt; ++i)
    {
        int32_t *const r = &MEOS_VIDEOSETUP_RESOLUTION.numOptions;

        for (j = 0; j < *r; ++j)
        {
            if (validmode[i].xdim == resolution[j].xdim && validmode[i].ydim == resolution[j].ydim)
            {
                resolution[j].flags |= validmode[i].fs ? RES_FS : RES_WIN;
                if (validmode[i].bpp > resolution[j].bppmax)
                    resolution[j].bppmax = validmode[i].bpp;
                break;
            }
        }

        if (j == *r) // no match found
        {
            resolution[j].xdim = validmode[i].xdim;
            resolution[j].ydim = validmode[i].ydim;
            resolution[j].bppmax = validmode[i].bpp;
            resolution[j].flags = validmode[i].fs ? RES_FS : RES_WIN;
            Bsnprintf(resolution[j].name, MAXRESOLUTIONSTRINGLENGTH, "%d x %d", resolution[j].xdim, resolution[j].ydim);
            MEOSN_VIDEOSETUP_RESOLUTION[j] = resolution[j].name;
            ++*r;
        }
    }

    // prepare shareware
    if (VOLUMEONE)
    {
        // change main menu
        MenuGroup_ChangeEntryList(MG_MAIN, MEL_MAIN_SHAREWARE);
        MenuGroup_ChangeEntryList(MG_MAIN_INGAME, MEL_MAIN_INGAME_SHAREWARE);

        // remove User Map
        MenuMenu_ChangeGroupList(M_EPISODE, MGL_EPISODE_SHAREWARE);

        // blue out episodes beyond the first
        for (i = 1; i < MAXVOLUMES; ++i)
        {
            if (MEL_EPISODE[i])
            {
                ME_EPISODE[i].font = &MF_RedfontBlue;
                ME_EPISODE[i].entry = &MEO_EPISODE_SHAREWARE;
            }
        }
        MEOS_NETOPTIONS_EPISODE.numOptions = 1;
        ME_NETOPTIONS_EPISODE.disabled = 1;
    }

    // prepare pre-Atomic
    if (!VOLUMEALL || !PLUTOPAK)
    {
        // prepare credits
        M_CREDITS.title = M_CREDITS2.title = M_CREDITS3.title = MenuCredits;
    }
}

static void M_RunMenu(Menu_t *cm);



/*
At present, no true difference is planned between M_PreMenu() and M_PreMenuDraw().
They are separate for purposes of organization.
*/

static void M_PreMenu(MenuID_t cm)
{
    int32_t i;
    DukePlayer_t *ps = g_player[myconnectindex].ps;

    switch (cm)
    {
    case MENU_MAIN_INGAME:
        ME_MAIN_SAVEGAME.disabled = (ud.recstat == 2);
        ME_MAIN_QUITTOTITLE.disabled = (g_netServer || numplayers > 1);
    case MENU_MAIN:
        if ((g_netServer || ud.multimode > 1) && ud.recstat != 2)
        {
            ME_MAIN_NEWGAME.entry = &MEO_MAIN_NEWGAME_NETWORK;
            ME_MAIN_NEWGAME_INGAME.entry = &MEO_MAIN_NEWGAME_NETWORK;
        }
        else
        {
            ME_MAIN_NEWGAME.entry = &MEO_MAIN_NEWGAME;
            ME_MAIN_NEWGAME_INGAME.entry = &MEO_MAIN_NEWGAME_INGAME;
        }
        break;

    case MENU_GAMESETUP:
        MEO_GAMESETUP_DEMOREC.options = (ps->gm&MODE_GAME) ? &MEOS_DemoRec : &MEOS_OffOn;
        ME_GAMESETUP_DEMOREC.disabled = ((ps->gm&MODE_GAME) && ud.m_recstat != 1);
        break;

#ifdef USE_OPENGL
    case MENU_RENDERERSETUP:
        if (getrendermode() == REND_CLASSIC)
            MenuMenu_ChangeGroupList(M_RENDERERSETUP, MGL_RENDERERSETUP);
#ifdef POLYMER
        else if (getrendermode() == REND_POLYMER)
            MenuMenu_ChangeGroupList(M_RENDERERSETUP, MGL_RENDERERSETUP_GL_POLYMER);
#endif
        else
            MenuMenu_ChangeGroupList(M_RENDERERSETUP, MGL_RENDERERSETUP_GL);

        if (getrendermode() != REND_CLASSIC)
            for (i = (int32_t) ARRAY_SIZE(MEOSV_RENDERERSETUP_ANISOTROPY) - 1; i >= 0; --i)
            {
                if (MEOSV_RENDERERSETUP_ANISOTROPY[i] <= glinfo.maxanisotropy)
                {
                    MEOS_RENDERERSETUP_ANISOTROPY.numOptions = i + 1;
                    break;
                }
            }

        ME_RENDERERSETUP_TEXQUALITY.disabled = !usehightile;
        ME_RENDERERSETUP_PRECACHE.disabled = !usehightile;
        ME_RENDERERSETUP_TEXCACHE.disabled = !(glusetexcompr && usehightile);
        ME_RENDERERSETUP_DETAILTEX.disabled = !usehightile;
        break;
#endif

    case MENU_VIDEOSETUP:
#ifdef USE_OPENGL
        if (getrendermode() == REND_CLASSIC)
            MenuMenu_ChangeGroupList(M_VIDEOSETUP, MGL_VIDEOSETUP);
        else
            MenuMenu_ChangeGroupList(M_VIDEOSETUP, MGL_VIDEOSETUP_GL);
#endif
        ME_VIDEOSETUP_APPLY.disabled = ((xdim == resolution[newresolution].xdim && ydim == resolution[newresolution].ydim && getrendermode() == newrendermode && fullscreen == newfullscreen) || (newfullscreen ? !(resolution[newresolution].flags & RES_FS) : !(resolution[newresolution].flags & RES_WIN)) || (newrendermode != REND_CLASSIC && resolution[newresolution].bppmax <= 8));
        break;

    case MENU_SOUND:
        ME_SOUND.disabled = (ud.config.FXDevice < 0);
        ME_SOUND_MUSIC.disabled = (ud.config.MusicDevice < 0);
        ME_SOUND_VOLUME_MASTER.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0) && (!ud.config.MusicToggle || ud.config.MusicDevice < 0);
        ME_SOUND_VOLUME_EFFECTS.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        ME_SOUND_VOLUME_MUSIC.disabled = (!ud.config.MusicToggle || ud.config.MusicDevice < 0);
        ME_SOUND_SAMPLINGRATE.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0) && (!ud.config.MusicToggle || ud.config.MusicDevice < 0);
        ME_SOUND_SAMPLESIZE.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0) && (!ud.config.MusicToggle || ud.config.MusicDevice < 0);
        ME_SOUND_NUMVOICES.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        ME_SOUND_RESTART.disabled = (soundrate == ud.config.MixRate && soundvoices == ud.config.NumVoices && soundbits == ud.config.NumBits);
        ME_SOUND_DUKETALK.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        ME_SOUND_DUKEMATCHPLAYER.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        ME_SOUND_AMBIENT.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        ME_SOUND_REVERSESTEREO.disabled = (!ud.config.SoundToggle || ud.config.FXDevice < 0);
        break;

    case MENU_MOUSESETUP:
        ME_MOUSESETUP_MOUSEAIMING.disabled = ud.mouseaiming;
        break;

    case MENU_NETOPTIONS:
        if (MEO_NETOPTIONS_EPISODE.currentOption == MEOS_NETOPTIONS_EPISODE.numOptions-1)
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_USERMAP;
        else
        {
            MEL_NETOPTIONS[2] = &ME_NETOPTIONS_LEVEL;
            MEO_NETOPTIONS_LEVEL.options = &MEOS_NETOPTIONS_LEVEL[MEO_NETOPTIONS_EPISODE.currentOption];
        }
        MEL_NETOPTIONS[4] = (GametypeFlags[ud.m_coop] & GAMETYPE_MARKEROPTION) ? &ME_NETOPTIONS_MARKERS : &ME_NETOPTIONS_MARKERS_DISABLED;
        MEL_NETOPTIONS[5] = (GametypeFlags[ud.m_coop] & (GAMETYPE_PLAYERSFRIENDLY|GAMETYPE_TDM)) ? &ME_NETOPTIONS_FRFIRE : &ME_NETOPTIONS_MAPEXITS;
        break;

    case MENU_OPTIONS:
        ME_OPTIONS_PLAYERSETUP.disabled = ud.recstat == 1;
        ME_OPTIONS_JOYSTICKSETUP.disabled = CONTROL_JoyPresent == 0;
        break;

    case MENU_LOAD:
        for (i = 0; i < MAXSAVEGAMES; ++i)
        {
            ME_LOAD[i].entry = (ud.savegame[i][0] && !g_oldverSavegame[i]) ? &MEO_LOAD_VALID : &MEO_LOAD;
            ME_LOAD[i].font = g_oldverSavegame[i] ? &MF_MinifontDarkGray : &MF_MinifontRed;
        }
        break;

    case MENU_SAVE:
        for (i = 0; i < MAXSAVEGAMES; ++i)
            MEO_SAVE[i].font = (g_oldverSavegame[i] && MEO_SAVE[i].editfield == NULL) ? &MF_MinifontDarkGray : &MF_MinifontRed;
        break;

    case MENU_LOADVERIFY:
    case MENU_SAVEVERIFY:
    case MENU_ADULTPASSWORD:
        M_RunMenu(m_previousMenu);
        break;

    default:
        break;
    }
}

static void M_PreMenuDraw(MenuID_t cm, MenuGroup_t *group, MenuEntry_t *entry)
{
    int32_t i, j, l = 0, m;

    switch (cm)
    {
    case MENU_MAIN_INGAME:
        l += 4;
    case MENU_MAIN:
        rotatesprite_fs(MENU_MARGIN_CENTER<<16,(28+l)<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite_fs((MENU_MARGIN_CENTER+100)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,(sintable[(totalclock<<4)&2047]>>11),0,2+8);
        break;

    case MENU_PLAYER:
        rotatesprite_fs((260)<<16,(24+(tilesizy[APLAYER]>>1))<<16,49152L,0,1441-((((4-(totalclock>>4)))&3)*5),0,entry == &ME_PLAYER_TEAM ? G_GetTeamPalette(ud.team) : ud.color,10);
        break;

    case MENU_MACROS:
        mgametext(160,144,"Activate in-game with Shift-F#",0,2+8+16);
        break;

    case MENU_COLCORR:
    case MENU_COLCORR_INGAME:
        rotatesprite(40<<16,24<<16,24576,0,BONUSSCREEN,0,0,2+8+16,0,scale(ydim,35,200),xdim-1,scale(ydim,80,200)-1);
        rotatesprite(160<<16,27<<16,24576,0,3290,0,0,2+8+16,0,scale(ydim,35,200),xdim-1,scale(ydim,80,200)-1);
        break;

    case MENU_NETSETUP:
    case MENU_NETHOST:
        minitext(90,90,            "Game Type"    ,2,26);
        minitext(90,90+8,          "Episode"      ,2,26);
        minitext(90,90+8+8,        "Level"        ,2,26);
        minitext(90,90+8+8+8,      "Monsters"     ,2,26);
        if (ud.m_coop == 0)
            minitext(90,90+8+8+8+8,    "Markers"      ,2,26);
        else if (ud.m_coop == 1)
            minitext(90,90+8+8+8+8,    "Friendly Fire",2,26);
        minitext(90,90+8+8+8+8+8,  "User Map"     ,2,26);

        minitext(90+60,90,GametypeNames[ud.m_coop],0,26);

        minitext(90+60,90+8,      EpisodeNames[ud.m_volume_number],0,26);
        minitext(90+60,90+8+8,    MapInfo[MAXLEVELS*ud.m_volume_number+ud.m_level_number].name,0,26);
        if (ud.m_monsters_off == 0 || ud.m_player_skill > 0)
            minitext(90+60,90+8+8+8,  SkillNames[ud.m_player_skill],0,26);
        else minitext(90+60,90+8+8+8,  "None",0,28);
        if (ud.m_coop == 0)
        {
            if (ud.m_marker) minitext(90+60,90+8+8+8+8,"On",0,26);
            else minitext(90+60,90+8+8+8+8,"Off",0,26);
        }
        else if (ud.m_coop == 1)
        {
            if (ud.m_ffire) minitext(90+60,90+8+8+8+8,"On",0,26);
            else minitext(90+60,90+8+8+8+8,"Off",0,26);
        }
        break;

    case MENU_KEYBOARDKEYS:
        if (group == &MG_KEYBOARDSETUPFUNCS)
        {
            mgametext(160,144+9+3,"Up/Down = Select Action",0,2+8+16);
            mgametext(160,144+9+9+3,"Left/Right = Select List",0,2+8+16);
            mgametext(160,144+9+9+9+3,"Enter = Modify   Delete = Clear",0,2+8+16);
        }
        break;

    case MENU_MOUSESETUP:
        if (entry == &ME_MOUSESETUP_MOUSEAIMING)
        {
            if (entry->disabled)
            {
                mgametext(160,140+9+9+9,"Set mouse aim type to toggle on/off",0,2+8+16);
                mgametext(160,140+9+9+9+9,"in the Player Setup menu to enable",0,2+8+16);
            }
        }
        break;

    case MENU_MOUSEBTNS:
        if (group == &MG_MOUSESETUPBTNS)
        {
            mgametext(160,160+9,"Up/Down = Select Button",0,2+8+16);
            mgametext(160,160+9+9,"Enter = Modify",0,2+8+16);
        }
        break;

    case MENU_MOUSEADVANCED:
        if (group == &MG_MOUSEADVANCED_DAXES)
        {
            mgametext(160,144+9+9,"Digital axes are not for mouse look",0,2+8+16);
            mgametext(160,144+9+9+9,"or for aiming up and down",0,2+8+16);
        }
        break;

    case MENU_JOYSTICKBTNS:
        if (group == &MG_JOYSTICKBTNS)
        {
            mgametext(160,149,"Up/Down = Select Button",0,2+8+16);
            mgametext(160,149+9,"Enter = Modify",0,2+8+16);
        }
        break;

    case MENU_RESETPLAYER:
        mgametext(160,90,"Load last game:",0,2+8+16);

        Bsprintf(tempbuf,"\"%s\"",ud.savegame[g_lastSaveSlot]);
        mgametext(160,99,tempbuf,0,2+8+16);

        mgametext(160,99+9,"(Y/N)",0,2+8+16);
        break;

    case MENU_LOAD:
        M_DrawBackground();

        for (i = 0; i <= 108; i += 12)
            rotatesprite_fs((160+64+91-64)<<16,(i+56)<<16,65536L,0,TEXTBOX,24,0,10);

        rotatesprite_fs(22<<16,97<<16,65536L,0,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(180<<16,97<<16,65536L,1024,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(99<<16,50<<16,65536L,512,WINDOWBORDER1,24,0,10);
        rotatesprite_fs(103<<16,144<<16,65536L,1024+512,WINDOWBORDER1,24,0,10);

        if (ud.savegame[group->currentEntry][0])
        {
            rotatesprite_fs(101<<16,97<<16,65536>>1,512,TILE_LOADSHOT,-32,0,4+10+64);

            if (g_oldverSavegame[group->currentEntry])
            {
                menutext(53,70,0,0,"Version");
                menutext(48,90,0,0,"Mismatch");

                Bsprintf(tempbuf,"Saved: %d.%d.%d %d-bit", savehead.majorver, savehead.minorver,
                         savehead.bytever, 8*savehead.ptrsize);
                mgametext(31,104,tempbuf,0,2+8+16);
                Bsprintf(tempbuf,"Our: %d.%d.%d %d-bit", SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                         (int32_t)(8*sizeof(intptr_t)));
                mgametext(31+16,QUOTE_SAVE_BAD_VERSION,tempbuf,0,2+8+16);
            }

            Bsprintf(tempbuf,"Players: %-2d                      ", savehead.numplayers);
            mgametext(160,156,tempbuf,0,2+8+16);
            Bsprintf(tempbuf,"Episode: %-2d / Level: %-2d / Skill: %-2d",
                     1+savehead.volnum, 1+savehead.levnum, savehead.skill);
            mgametext(160,168,tempbuf,0,2+8+16);
            if (savehead.volnum == 0 && savehead.levnum == 7)
                mgametext(160,180,savehead.boardfn,0,2+8+16);
        }
        else
            menutext(69,70,0,0,"Empty");
        break;

    case MENU_SAVE:
        M_DrawBackground();

        for (i = 0; i <= 108; i += 12)
            rotatesprite_fs((160+64+91-64)<<16,(i+56)<<16,65536L,0,TEXTBOX,24,0,10);

        rotatesprite_fs(22<<16,97<<16,65536L,0,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(180<<16,97<<16,65536L,1024,WINDOWBORDER2,24,0,10);
        rotatesprite_fs(99<<16,50<<16,65536L,512,WINDOWBORDER1,24,0,10);
        rotatesprite_fs(103<<16,144<<16,65536L,1024+512,WINDOWBORDER1,24,0,10);

        if (ud.savegame[group->currentEntry][0])
        {
            j = 0;
            for (i = 0; i < 10; ++i)
                if (((MenuString_t*)group->entrylist[i]->entry)->editfield)
                    j |= 1;

            rotatesprite_fs(101<<16,97<<16,65536L>>1,512,j?TILE_SAVESHOT:TILE_LOADSHOT,-32,0,4+10+64);

            if (g_oldverSavegame[group->currentEntry])
            {
                menutext(53,70,0,0,"Version");
                menutext(48,90,0,0,"Mismatch");

                Bsprintf(tempbuf,"Saved: %d.%d.%d %d-bit", savehead.majorver, savehead.minorver,
                         savehead.bytever, 8*savehead.ptrsize);
                mgametext(31,104,tempbuf,0,2+8+16);
                Bsprintf(tempbuf,"Our: %d.%d.%d %d-bit", SV_MAJOR_VER, SV_MINOR_VER, BYTEVERSION,
                         (int32_t)(8*sizeof(intptr_t)));
                mgametext(31+16,QUOTE_SAVE_BAD_VERSION,tempbuf,0,2+8+16);
            }
        }
        else
            menutext(69,70,0,0,"Empty");

        Bsprintf(tempbuf,"Players: %-2d                      ",ud.multimode);
        mgametext(160,156,tempbuf,0,2+8+16);
        Bsprintf(tempbuf,"Episode: %-2d / Level: %-2d / Skill: %-2d",1+ud.volume_number,1+ud.level_number,ud.player_skill);
        mgametext(160,168,tempbuf,0,2+8+16);
        if (ud.volume_number == 0 && ud.level_number == 7)
            mgametext(160,180,currentboardfilename,0,2+8+16);
        break;

    case MENU_LOADVERIFY:
        mgametext(160,90,"Load game:",0,2+8+16);
        Bsprintf(tempbuf,"\"%s\"",ud.savegame[MG_LOAD.currentEntry]);
        mgametext(160,99,tempbuf,0,2+8+16);
        mgametext(160,99+9,"(Y/N)",0,2+8+16);
        break;

    case MENU_SAVEVERIFY:
        mgametext(160,90,"Overwrite previous saved game?",0,2+8+16);
        mgametext(160,90+9,"(Y/N)",0,2+8+16);
        break;

    case MENU_NEWVERIFY:
        mgametext(160,90,"Abort this game?",0,2+8+16);
        mgametext(160,90+9,"(Y/N)",0,2+8+16);
        break;

    case MENU_QUIT:
    case MENU_QUIT_INGAME:
        mgametext(MENU_MARGIN_CENTER,90,"Are you sure you want to quit?",0,2+8+16);
        mgametext(MENU_MARGIN_CENTER,99,"(Y/N)",0,2+8+16);
        break;

    case MENU_QUITTOTITLE:
        mgametext(MENU_MARGIN_CENTER,90,"Quit to Title?",0,2+8+16);
        mgametext(MENU_MARGIN_CENTER,99,"(Y/N)",0,2+8+16);
        break;

    case MENU_NETWAITMASTER:
        G_DrawFrags();
        mgametext(160,50,"Waiting for master",0,2+8+16);
        mgametext(160,59,"to select level",0,2+8+16);
        break;

    case MENU_NETWAITVOTES:
        G_DrawFrags();
        mgametext(160,90,"Waiting for votes",0,2);
        break;

    case MENU_ORDERING:
    case MENU_ORDERING2:
    case MENU_ORDERING3:
    case MENU_ORDERING4:
        rotatesprite_fs(0,0,65536L,0,ORDERING+g_currentMenu-MENU_ORDERING,0,0,10+16+64);
        break;

    case MENU_STORY:
        rotatesprite_fs(0,0,65536L,0,TEXTSTORY,0,0,10+16+64);
        break;

    case MENU_F1HELP:
        rotatesprite_fs(0,0,65536L,0,F1HELP,0,0,10+16+64);
        break;

    case MENU_BUYDUKE:
        mgametext(160,41-8,"You are playing the shareware",0,2+8+16);
        mgametext(160,50-8,"version of Duke Nukem 3D.  While",0,2+8+16);
        mgametext(160,59-8,"this version is really cool, you",0,2+8+16);
        mgametext(160,68-8,"are missing over 75% of the total",0,2+8+16);
        mgametext(160,77-8,"game, along with other great extras",0,2+8+16);
        mgametext(160,86-8,"and games, which you'll get when",0,2+8+16);
        mgametext(160,95-8,"you order the complete version and",0,2+8+16);
        mgametext(160,104-8,"get the final three episodes.",0,2+8+16);

        mgametext(160,104+8,"Please read the 'How To Order' item",0,2+8+16);
        mgametext(160,113+8,"on the main menu or visit",0,2+8+16);
        mgametext(160,122+8,"Steam or the Google Play Store",0,2+8+16);
        mgametext(160,131+8,"to upgrade to the full registered",0,2+8+16);
        mgametext(160,139+8,"version of Duke Nukem 3D.",0,2+8+16);

        mgametext(160,148+16,"Press any key...",0,2+8+16);
        break;

    case MENU_CREDITS:
    case MENU_CREDITS2:
    case MENU_CREDITS3:
        if (!VOLUMEALL || !PLUTOPAK)
        {
            // pre-Atomic credits where there are not tiles devoted to them

            M_DrawBackground();

            switch (g_currentMenu)
            {
            case MENU_CREDITS:
                m = 20;
                l = 33;

                shadowminitext(m, l, "Original Concept", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Todd Replogle and Allen H. Blum III", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Produced & Directed By", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Greg Malone", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Executive Producer", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "George Broussard", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "BUILD Engine", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Ken Silverman", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Game Programming", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Todd Replogle", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "3D Engine/Tools/Net", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Ken Silverman", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Network Layer/Setup Program", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Mark Dochtermann", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Map Design", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Allen H. Blum III", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Richard Gray", 12, 10+16+128); l += 7;

                m = 180;
                l = 33;

                shadowminitext(m, l, "3D Modeling", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Chuck Jones", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Sapphire Corporation", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Artwork", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Dirk Jones, Stephen Hornback", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "James Storey, David Demaret", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Douglas R. Wood", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Sound Engine", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Jim Dose", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Sound & Music Development", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Robert Prince", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Lee Jackson", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Voice Talent", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Lani Minella - Voice Producer", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Jon St. John as \"Duke Nukem\"", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Graphic Design", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Packaging, Manual, Ads", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Robert M. Atkins", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Michael Hadwin", 12, 10+16+128); l += 7;
                break;

            case MENU_CREDITS2:
                m = 20;
                l = 33;

                shadowminitext(m, l, "Special Thanks To", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Steven Blackburn, Tom Hall", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Scott Miller, Joe Siegler", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Terry Nagy, Colleen Compton", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "HASH, Inc., FormGen, Inc.", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "The 3D Realms Beta Testers", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Nathan Anderson, Wayne Benner", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Glenn Brensinger, Rob Brown", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Erik Harris, Ken Heckbert", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Terry Herrin, Greg Hively", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Hank Leukart, Eric Baker", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Jeff Rausch, Kelly Rogers", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Mike Duncan, Doug Howell", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "Bill Blair", 12, 10+16+128); l += 7;

                m = 160;
                l = 33;

                shadowminitext(m, l, "Company Product Support", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "The following companies were cool", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "enough to give us lots of stuff", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "during the making of Duke Nukem 3D.", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Altec Lansing Multimedia", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "for tons of speakers and the", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "THX-licensed sound system.", 12, 10+16+128); l += 7;
                shadowminitext(m, l, "For info call 1-800-548-0620", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Creative Labs, Inc.", 12, 10+16+128); l += 7;
                l += 3;
                shadowminitext(m, l, "Thanks for the hardware, guys.", 12, 10+16+128); l += 7;
                break;

            case MENU_CREDITS3:
                m = MENU_MARGIN_CENTER;
                mgametext(m,50,        "Duke Nukem is a trademark of",0,2+8+16);
                mgametext(m,50+9,      "3D Realms Entertainment",0,2+8+16);

                mgametext(m,50+9+9+9,  "Duke Nukem",0,2+8+16);
                mgametext(m,50+9+9+9+9,"(C) 1996 3D Realms Entertainment",0,2+8+16);

                if (VOLUMEONE)
                {
                    mgametext(m,106,   "Please read LICENSE.DOC for shareware",0,2+8+16);
                    mgametext(m,106+9, "distribution grants and restrictions.",0,2+8+16);
                }

                mgametext(m,VOLUMEONE?134:115,       "Made in Dallas, Texas USA",0,2+8+16);
                break;
            }
        }
        else
            rotatesprite_fs(160<<16,200<<15,65536L,0,2504+g_currentMenu-MENU_CREDITS,0,0,10+64);
        break;
    case MENU_CREDITS4:   // JBF 20031220
        M_DrawBackground();

        l = 7;

        mgametext(160,38-l,"Management, Design and Production",0,2+8+16);
        creditsminitext(160, 38+10-l, "Richard \"TerminX\" Gobeille", 8, 10+16+128);

#ifndef POLYMER
        mgametext(160,58-l,"Rendering and Support Programming",0,2+8+16);
#else
        mgametext(160,58-l,"Polymer Rendering System by",0,2+8+16);
#endif
        creditsminitext(160, 58+10-l, "Pierre-Loup \"Plagman\" Griffais", 8, 10+16+128);

        mgametext(160,78-l,"Engine and Game Programming",0,2+8+16);
        creditsminitext(160, 78+10-l, "Philipp \"Helixhorned\" Kutin", 8, 10+16+128);
        creditsminitext(160, 78+7+10-l, "Evan \"Hendricks266\" Ramos", 8, 10+16+128);

        mgametext(160,98+7-l,"Based on \"JFDuke3D\" by",0,2+8+16);
        creditsminitext(160, 98+7+10-l, "Jonathon \"JonoF\" Fowler", 8, 10+16+128);

        mgametext(160,118+7-l,"Original \"EDuke\" Concept",0,2+8+16);
        creditsminitext(160, 118+7+10-l, "Matt \"Matteus\" Saettler", 8, 10+16+128);

        mgametext(160,138+7-l,"BUILD Engine by",0,2+8+16);
        creditsminitext(160, 138+7+10-l, "Ken \"Awesoken\" Silverman", 8, 10+16+128);

#ifdef __ANDROID__
        mgametext(160, 158+7-l, "Android version by", 0, 2+8+16);
        creditsminitext(160, 158+7+10-l, "Emile Belanger/Beloko Games", 8, 10+16+128);
#endif

        creditsminitext(160, 138+10+10+10+10+4-l, "Visit www.eduke32.com for news and updates", 8, 10+16+128);
        break;

    case MENU_CREDITS5:
        M_DrawBackground();

        l = 7;

        mgametext(160,38-l,"License and Other Contributors",0,2+8+16);
        {
            const char *header[] =
            {
                "This program is distributed under the terms of the",
                "GNU General Public License version 2 as published by the",
                "Free Software Foundation. See GNU.TXT for details.",
                " ",
                "Thanks to the following people for their contributions:",
                " ",
            };
            const char *body[] =
            {
                "Adam Fazakerley",   // netcode NAT traversal
                "Alan Ondra",        // testing
                "Bioman",            // GTK work, APT repository and package upkeep
                "Brandon Bergren",   // "Bdragon" - tiles.cfg
                "Charlie Honig",     // "CONAN" - showview command
                "Dan Gaskill",       // "DeeperThought" - testing
                "David Koenig",      // "Bargle" - Merged a couple of things from duke3d_w32
                "Ed Coolidge",       // Mapster32 improvements
                "Ferry Landzaat",    // ? (listed on the wiki page)
                "Hunter_rus",        // tons of stuff
                "James Bentler",     // Mapster32 improvements
                "Jasper Foreman",    // netcode contributions
                "Javier Martinez",   // "Malone3D" - EDuke 2.1.1 components
                "Jeff Hart",         // website graphics
                "Jonathan Smith",    // "Mblackwell" - testing
                "Jose del Castillo", // "Renegado" - EDuke 2.1.1 components
                "Lachlan McDonald",  // official EDuke32 icon
                "LSDNinja",          // OS X help and testing
                "Marcus Herbert",    // "rhoenie" - OS X compatibility work
                "Matthew Palmer",    // "Usurper" - testing and eduke32.com domain
                "Ozkan Sezer",       // SDL/GTK version checking improvements
                "Peter Green",       // "Plugwash" - dynamic remapping, custom gametypes
                "Peter Veenstra",    // "Qbix" - port to 64-bit
                "Randy Heit",        // random snippets of ZDoom here and there
                "Robin Green",       // CON array support
                "Ryan Gordon",       // "icculus" - icculus.org Duke3D port sound code
                "Stephen Anthony",   // early 64-bit porting work
                "Thijs Leenders",    // Android icon work
                "tueidj",            // Wii port
                " ",
            };
            const char *footer[] =
            {
                " ",
                "BUILD engine technology available under BUILDLIC.",
            };

            const int32_t header_numlines = sizeof(header)/sizeof(char *);
            const int32_t body_numlines = sizeof(body)/sizeof(char *);
            const int32_t footer_numlines = sizeof(footer)/sizeof(char *);

            i = 0;
            for (m=0; m<header_numlines; m++)
                creditsminitext(160, 17+10+10+8+4+(m*7)-l, header[m], 8, 10+16+128);
            i += m;
#define CCOLUMNS 3
#define CCOLXBUF 20
            for (m=0; m<body_numlines; m++)
                creditsminitext(CCOLXBUF+((320-CCOLXBUF*2)/(CCOLUMNS*2))  +((320-CCOLXBUF*2)/CCOLUMNS)*(m/(body_numlines/CCOLUMNS)), 17+10+10+8+4+((m%(body_numlines/CCOLUMNS))*7)+(i*7)-l, body[m], 8, 10+16+128);
            i += m/CCOLUMNS;
            for (m=0; m<footer_numlines; m++)
                creditsminitext(160, 17+10+10+8+4+(m*7)+(i*7)-l, footer[m], 8, 10+16+128);
        }

        creditsminitext(160, 138+10+10+10+10+4-l, "Visit www.eduke32.com for news and updates", 8, 10+16+128);
        break;

    default:
        break;
    }
}



static void M_PreMenuInput(MenuGroup_t *group, MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {

    case MENU_KEYBOARDKEYS:
        if (group == &MG_KEYBOARDSETUPFUNCS)
        {
            if (KB_KeyPressed(sc_Delete))
            {
                MenuCustom2Col_t *column = (MenuCustom2Col_t*)entry->entry;
                char key[2];
                key[0] = ud.config.KeyboardKeys[group->currentEntry][0];
                key[1] = ud.config.KeyboardKeys[group->currentEntry][1];
                *column->column[group->currentColumn] = 0xff;
                CONFIG_MapKey(group->currentEntry, ud.config.KeyboardKeys[group->currentEntry][0], key[0], ud.config.KeyboardKeys[group->currentEntry][1], key[1]);
                S_PlaySound(KICK_HIT);
                KB_ClearKeyDown(sc_Delete);
            }
        }
        break;

    default:
        break;
    }
}

static void M_PreMenuOptionListDraw(/*MenuGroup_t *group,*/ MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {
    case MENU_MOUSEBTNS:
    case MENU_MOUSEADVANCED:
    case MENU_JOYSTICKBTNS:
    case MENU_JOYSTICKAXIS:
        mgametext(320>>1,31,"Select a function to assign",0,2+8+16);

        Bsprintf(tempbuf, "to %s", entry->name);

        mgametext(320>>1,31+9,tempbuf,0,2+8+16);

        mgametext(320>>1,161,"Press \"Escape\" To Cancel",0,2+8+16);
        break;
    }
}

static int32_t M_PreMenuCustom2ColScreen(MenuGroup_t *group, MenuEntry_t *entry)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        MenuCustom2Col_t *column = (MenuCustom2Col_t*)entry->entry;

        int32_t sc = KB_GetLastScanCode();
        if (sc != sc_None)
        {
            char key[2];
            key[0] = ud.config.KeyboardKeys[group->currentEntry][0];
            key[1] = ud.config.KeyboardKeys[group->currentEntry][1];

            S_PlaySound(PISTOL_BODYHIT);

            *column->column[group->currentColumn] = KB_GetLastScanCode();

            CONFIG_MapKey(group->currentEntry, ud.config.KeyboardKeys[group->currentEntry][0], key[0], ud.config.KeyboardKeys[group->currentEntry][1], key[1]);

            KB_ClearKeyDown(sc);

            return 1;
        }
    }

    return 0;
}

static void M_PreMenuCustom2ColScreenDraw(MenuGroup_t *group, MenuEntry_t *entry)
{
    if (g_currentMenu == MENU_KEYBOARDKEYS)
    {
        mgametext(320>>1,90,"Press the key to assign as",0,2+8+16);
        Bsprintf(tempbuf,"%s for \"%s\"", group->currentColumn?"secondary":"primary", entry->name);
        mgametext(320>>1,90+9,tempbuf,0,2+8+16);
        mgametext(320>>1,90+9+9+9,"Press \"Escape\" To Cancel",0,2+8+16);
    }
}

static void M_MenuEntryFocus(MenuGroup_t *group/*, MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_LOAD:
    case MENU_SAVE:
        G_LoadSaveHeaderNew(group->currentEntry, &savehead);
        break;

    default:
        break;
    }
}

/*
Functions where a "newValue" or similar is passed are run *before* the linked variable is actually changed.
That way you can compare the new and old values and potentially block the change.
*/
static void M_MenuEntryLinkActivate(MenuGroup_t *group, MenuEntry_t *entry)
{
    switch (g_currentMenu)
    {
    case MENU_EPISODE:
        if (group == &MG_EPISODE)
        {
            ud.m_volume_number = group->currentEntry;
            ud.m_level_number = 0;
        }
        break;

    case MENU_SKILL:
    {
        int32_t skillsound = 0;

        switch (group->currentEntry)
        {
        case 0:
            skillsound = JIBBED_ACTOR6;
            break;
        case 1:
            skillsound = BONUS_SPEECH1;
            break;
        case 2:
            skillsound = DUKE_GETWEAPON2;
            break;
        case 3:
            skillsound = JIBBED_ACTOR5;
            break;
        }

        g_skillSoundVoice = S_PlaySound(skillsound);

        ud.m_player_skill = group->currentEntry+1;
        if (group->currentEntry == 3) ud.m_respawn_monsters = 1;
        else ud.m_respawn_monsters = 0;

        ud.m_monsters_off = ud.monsters_off = 0;

        ud.m_respawn_items = 0;
        ud.m_respawn_inventory = 0;

        ud.multimode = 1;

        if (ud.m_volume_number == 3 && (G_GetLogoFlags() & LOGO_NOE4CUTSCENE)==0)
        {
            flushperms();
            setview(0,0,xdim-1,ydim-1);
            clearview(0L);
            nextpage();
        }

        G_NewGame_EnterLevel();
        break;
    }

    case MENU_JOYSTICKAXES:
        M_JOYSTICKAXIS.title = getjoyname(0, group->currentEntry);
        MEO_JOYSTICKAXIS_ANALOG.data = &ud.config.JoystickAnalogueAxes[group->currentEntry];
        MEO_JOYSTICKAXIS_SCALE.variable = &ud.config.JoystickAnalogueScale[group->currentEntry];
        MEO_JOYSTICKAXIS_DEAD.variable = &ud.config.JoystickAnalogueDead[group->currentEntry];
        MEO_JOYSTICKAXIS_SATU.variable = &ud.config.JoystickAnalogueSaturate[group->currentEntry];
        MEO_JOYSTICKAXIS_DIGITALNEGATIVE.data = &ud.config.JoystickDigitalFunctions[group->currentEntry][0];
        MEO_JOYSTICKAXIS_DIGITALPOSITIVE.data = &ud.config.JoystickDigitalFunctions[group->currentEntry][1];
        break;

    default:
        break;
    }

    if (entry == &ME_VIDEOSETUP_APPLY)
    {
        int32_t pxdim, pydim, pfs, pbpp, prend;
        int32_t nxdim, nydim, nfs, nbpp, nrend;

        pxdim = xdim;
        pydim = ydim;
        pbpp  = bpp;
        pfs   = fullscreen;
        prend = getrendermode();
        nxdim = resolution[newresolution].xdim;
        nydim = resolution[newresolution].ydim;
        nfs   = newfullscreen;

        nbpp  = (newrendermode == REND_CLASSIC) ? 8 : resolution[newresolution].bppmax;
        nrend = newrendermode;

        if (setgamemode(nfs, nxdim, nydim, nbpp) < 0)
        {
            if (setgamemode(pfs, pxdim, pydim, pbpp) < 0)
            {
                setrendermode(prend);
                G_GameExit("Failed restoring old video mode.");
            }
            else onvideomodechange(pbpp > 8);
        }
        else onvideomodechange(nbpp > 8);

        g_restorePalette = -1;
        G_UpdateScreenArea();
        setrendermode(nrend);

        ud.config.ScreenMode = fullscreen;
        ud.config.ScreenWidth = xdim;
        ud.config.ScreenHeight = ydim;
        ud.config.ScreenBPP = bpp;
    }
    else if (entry == &ME_SOUND_RESTART)
    {
        ud.config.MixRate = soundrate;
        ud.config.NumVoices = soundvoices;
        ud.config.NumBits = soundbits;

        S_SoundShutdown();
        S_MusicShutdown();

        S_MusicStartup();
        S_SoundStartup();

        FX_StopAllSounds();
        S_ClearSoundLocks();

        if (ud.config.MusicToggle == 1)
        {
            if (ud.recstat != 2 && g_player[myconnectindex].ps->gm&MODE_GAME)
            {
                if (MapInfo[g_musicIndex].musicfn != NULL)
                    S_PlayMusic(&MapInfo[g_musicIndex].musicfn[0],g_musicIndex);
            }
            else S_PlayMusic(&EnvMusicFilename[0][0],MAXVOLUMES*MAXLEVELS);
        }
    }
    else if (entry == &ME_COLCORR_RESET)
    {
        vid_gamma = DEFAULT_GAMMA;
        vid_contrast = DEFAULT_CONTRAST;
        vid_brightness = DEFAULT_BRIGHTNESS;
        ud.brightness = 0;
        setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
    }
    else if (entry == &ME_KEYBOARDSETUP_RESET)
        CONFIG_SetDefaultKeys((const char (*)[MAXGAMEFUNCLEN])keydefaults);
    else if (entry == &ME_KEYBOARDSETUP_RESETCLASSIC)
        CONFIG_SetDefaultKeys(oldkeydefaults);
    else if (entry == &ME_NETHOST_LAUNCH)
    {
        // master does whatever it wants
        if (g_netServer)
        {
            Net_FillNewGame(&pendingnewgame, 1);
            Net_StartNewGame();
            Net_SendNewGame(1, NULL);
        }
        else if (voting == -1)
        {
            Net_SendMapVoteInitiate();
            M_ChangeMenu(MENU_NETWAITVOTES);
        }
    }
}

static int32_t M_MenuEntryOptionModify(MenuGroup_t* group, MenuEntry_t *entry, int32_t newOption)
{
    int32_t x;
    DukePlayer_t *ps = g_player[myconnectindex].ps;

    if (entry == &ME_GAMESETUP_DEMOREC)
    {
        if ((ps->gm&MODE_GAME))
            G_CloseDemoWrite();
    }
    else if (entry == &ME_GAMESETUP_DM_SHOWPLAYERWEAPONS)
        ud.showweapons = newOption;
    else if (entry == &ME_GAMESETUP_CONSOLETEXT)
        OSD_SetTextMode(newOption);
#ifdef _WIN32
    else if (entry == &ME_GAMESETUP_UPDATES)
        ud.config.LastUpdateCheck = 0;
#endif
    else if (entry == &ME_PLAYER_WEAPSWITCH_PICKUP)
    {
        ud.weaponswitch &= ~(1|4);
        switch (newOption)
        {
        case 2:
            ud.weaponswitch |= 4;
        case 1:
            ud.weaponswitch |= 1;
            break;
        default:
            break;
        }
    }
    else if (entry == &ME_PLAYER_WEAPSWITCH_EMPTY)
        ud.weaponswitch = (ud.weaponswitch&~2) | (newOption ? 2 : 0);
#ifdef USE_OPENGL
    else if (entry == &ME_VIDEOSETUP_TEXFILTER)
    {
        gltexfiltermode = newOption;
        gltexapplyprops();
    }
    else if (entry == &ME_RENDERERSETUP_ASPECTRATIO)
    {
        glwidescreen = newOption & 1;
        r_usenewaspect = newOption & 2;
    }
#ifdef POLYMER
    else if (entry == &ME_RENDERERSETUP_ASPECTRATIO_POLYMER)
    {
        pr_customaspect = MEOSV_RENDERERSETUP_ASPECTRATIO_POLYMER[newOption];
    }
#endif
    else if (entry == &ME_RENDERERSETUP_ANISOTROPY)
    {
        glanisotropy = newOption;
        gltexapplyprops();
    }
    else if (entry == &ME_RENDERERSETUP_VSYNC)
        setvsync(newOption);
    else if (entry == &ME_RENDERERSETUP_TEXQUALITY)
    {
        r_downsize = newOption;
        texcache_invalidate();
        resetvideomode();
        if (setgamemode(fullscreen,xdim,ydim,bpp))
            OSD_Printf("restartvid: Reset failed...\n");
        r_downsizevar = r_downsize;
    }
#endif
    else if (entry == &ME_SOUND)
    {
        if (newOption == 0)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }
    }
    else if (entry == &ME_SOUND_MUSIC)
    {
        ud.config.MusicToggle = newOption;

        if (newOption == 0)
            S_PauseMusic(1);
        else
        {
            if (ud.recstat != 2 && g_player[myconnectindex].ps->gm&MODE_GAME)
            {
                if (MapInfo[g_musicIndex].musicfn != NULL)
                    S_PlayMusic(&MapInfo[g_musicIndex].musicfn[0], g_musicIndex);
            }
            else S_PlayMusic(&EnvMusicFilename[0][0], MAXVOLUMES*MAXLEVELS);

            S_PauseMusic(0);
        }
    }
    else if (entry == &ME_SOUND_DUKETALK)
        ud.config.VoiceToggle = (ud.config.VoiceToggle&~1) | newOption;
    else if (entry == &ME_SOUND_DUKEMATCHPLAYER)
        ud.config.VoiceToggle = (ud.config.VoiceToggle&~4) | (newOption ? 4 : 0);
    else if (entry == &ME_MOUSESETUP_SMOOTH)
        CONTROL_SmoothMouse = ud.config.SmoothInput;
    else if (entry == &ME_JOYSTICKAXIS_ANALOG)
        CONTROL_MapAnalogAxis(MG_JOYSTICKAXES.currentEntry, newOption, controldevice_joystick);
    else if (entry == &ME_NETOPTIONS_EPISODE)
    {
        if (newOption < MAXVOLUMES)
            ud.m_volume_number = newOption;
    }
    else if (entry == &ME_NETOPTIONS_MONSTERS)
    {
        ud.m_monsters_off = (newOption == MAXSKILLS);
        if (newOption < MAXSKILLS)
            ud.m_player_skill = newOption;
    }
    else if (entry == &ME_ADULTMODE)
    {
        if (newOption)
        {
            for (x=0; x<g_numAnimWalls; x++)
                switch (DYNAMICTILEMAP(wall[animwall[x].wallnum].picnum))
                {
                case FEMPIC1__STATIC:
                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                    break;
                case FEMPIC2__STATIC:
                case FEMPIC3__STATIC:
                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                    break;
                }
        }
        else
        {
            if (ud.pwlockout[0] == 0)
            {
                ud.lockout = 0;
#if 0
                for (x=0; x<g_numAnimWalls; x++)
                    if (wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                            wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                            wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                        if (wall[animwall[x].wallnum].extra >= 0)
                            wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
#endif
            }
            else
            {
                M_ChangeMenu(MENU_ADULTPASSWORD);
                return 1;
            }
        }
    }

    if (g_currentMenu == MENU_PLAYER && entry != &ME_PLAYER_MACROS)
        G_UpdatePlayerFromMenu();

    switch (g_currentMenu)
    {
    case MENU_MOUSEBTNS:
    case MENU_MOUSEADVANCED:
    case MENU_JOYSTICKBTNS:
    case MENU_JOYSTICKAXIS:
        if (group == &MG_MOUSESETUPBTNS)
        {
            CONTROL_MapButton(newOption, MenuMouseDataIndex[group->currentEntry][0], MenuMouseDataIndex[group->currentEntry][1], controldevice_mouse);
            CONTROL_FreeMouseBind(MenuMouseDataIndex[group->currentEntry][0]);
        }
        else if (group == &MG_MOUSEADVANCED_DAXES)
        {
            CONTROL_MapDigitalAxis(group->currentEntry>>1, newOption, group->currentEntry&1, controldevice_mouse);
        }
        else if (group == &MG_JOYSTICKBTNS)
        {
            CONTROL_MapButton(newOption, group->currentEntry>>1, group->currentEntry&1, controldevice_joystick);
        }
        else if (group == &MG_JOYSTICKAXIS_DIGITAL)
        {
            CONTROL_MapDigitalAxis(group->currentEntry>>1, newOption, group->currentEntry&1, controldevice_joystick);
        }
        break;
    }

    return 0;
}

static void M_MenuCustom2ColScreen(MenuGroup_t *group/*, MenuEntry_t *entry*/)
{
    if (group == &MG_KEYBOARDSETUPFUNCS)
    {
        KB_FlushKeyboardQueue();
        KB_ClearLastScanCode();
    }
}

static int32_t M_MenuEntryRangeInt32Modify(MenuEntry_t *entry, int32_t newValue)
{
    if (entry == &ME_GAMESETUP_SCREENSIZE)
        G_SetViewportShrink(newValue - vpsize);
    else if (entry == &ME_GAMESETUP_SBARSIZE)
        G_SetStatusBarScale(newValue);
    else if (entry == &ME_SOUND_VOLUME_MASTER)
    {
        FX_SetVolume(newValue);
        S_MusicVolume(MASTER_VOLUME(ud.config.MusicVolume));
    }
    else if (entry == &ME_SOUND_VOLUME_MUSIC)
        S_MusicVolume(MASTER_VOLUME(newValue));
    else if (entry == &ME_MOUSEADVANCED_SCALEX)
        CONTROL_SetAnalogAxisScale(0, newValue, controldevice_mouse);
    else if (entry == &ME_MOUSEADVANCED_SCALEY)
        CONTROL_SetAnalogAxisScale(1, newValue, controldevice_mouse);
    else if (entry == &ME_JOYSTICKAXIS_SCALE)
        CONTROL_SetAnalogAxisScale(MG_JOYSTICKAXES.currentEntry, newValue, controldevice_joystick);
    else if (entry == &ME_JOYSTICKAXIS_DEAD)
        setjoydeadzone(MG_JOYSTICKAXES.currentEntry, newValue, *MEO_JOYSTICKAXIS_SATU.variable);
    else if (entry == &ME_JOYSTICKAXIS_SATU)
        setjoydeadzone(MG_JOYSTICKAXES.currentEntry, *MEO_JOYSTICKAXIS_DEAD.variable, newValue);

    return 0;
}

static int32_t M_MenuEntryRangeFloatModify(MenuEntry_t *entry, float newValue)
{
    if (entry == &ME_RENDERERSETUP_AMBIENT)
        r_ambientlightrecip = 1.f/newValue;

    return 0;
}

static int32_t M_MenuEntryRangeDoubleModify(MenuEntry_t *entry/*, double newValue*/)
{
    if (entry == &ME_COLCORR_GAMMA)
    {
        ud.brightness = GAMMA_CALC<<2;
        setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);
    }
    else if (entry == &ME_COLCORR_CONTRAST || entry == &ME_COLCORR_BRIGHTNESS)
        setbrightness(ud.brightness>>2,g_player[myconnectindex].ps->palette,0);

    return 0;
}

static uint32_t save_xxh = 0;

static void M_MenuEntryStringActivate(MenuGroup_t *group/*, MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_SAVE:
        if (!save_xxh)
            save_xxh = XXH32((uint8_t *)&ud.savegame[group->currentEntry][0], 19, 0xDEADBEEF);
        if (ud.savegame[group->currentEntry][0])
            M_ChangeMenu(MENU_SAVEVERIFY);
        break;

    default:
        break;
    }
}

static void M_MenuEntryStringSubmit(/*MenuGroup_t *group, MenuEntry_t *entry, char *input*/)
{
    switch (g_currentMenu)
    {
    case MENU_SAVE:
        // dirty hack... char 127 in last position indicates an auto-filled name
        if (ud.savegame[MG_SAVE.currentEntry][0] == 0 || (ud.savegame[MG_SAVE.currentEntry][20] == 127 &&
            save_xxh == XXH32((uint8_t *)&ud.savegame[MG_SAVE.currentEntry][0], 19, 0xDEADBEEF)))
        {
            Bstrncpy(&ud.savegame[MG_SAVE.currentEntry][0], MapInfo[ud.volume_number * MAXLEVELS + ud.level_number].name, 19);
            ud.savegame[MG_SAVE.currentEntry][20] = 127;
        }

        G_SavePlayerMaybeMulti(MG_SAVE.currentEntry);

        g_lastSaveSlot = MG_SAVE.currentEntry;
        g_player[myconnectindex].ps->gm = MODE_GAME;

        if ((!g_netServer && ud.multimode < 2)  && ud.recstat != 2)
        {
            ready2send = 1;
            totalclock = ototalclock;
        }
        save_xxh = 0;
        break;

    default:
        break;
    }
}

static void M_MenuEntryStringCancel(/*MenuGroup_t *group, MenuEntry_t *entry*/)
{
    switch (g_currentMenu)
    {
    case MENU_SAVE:
        save_xxh = 0;
        ReadSaveGameHeaders();
        break;

    default:
        break;
    }
}

/*
This is polled when the menu code is populating the screen but for some reason doesn't have the data.
*/
static int32_t M_MenuEntryOptionSource(MenuEntry_t *entry, int32_t currentValue)
{
    if (entry == &ME_GAMESETUP_CONSOLETEXT)
        return OSD_GetTextMode();
    else if (entry == &ME_PLAYER_WEAPSWITCH_PICKUP)
        return (ud.weaponswitch & 1) ? ((ud.weaponswitch & 4) ? 2 : 1) : 0;
    else if (entry == &ME_PLAYER_WEAPSWITCH_EMPTY)
        return (ud.weaponswitch & 2) ? 1 : 0;
#ifdef USE_OPENGL
    else if (entry == &ME_RENDERERSETUP_ASPECTRATIO)
        return r_usenewaspect ? 2 : glwidescreen;
#ifdef POLYMER
    else if (entry == &ME_RENDERERSETUP_ASPECTRATIO_POLYMER)
        return clamp(currentValue, 0, ARRAY_SIZE(MEOSV_RENDERERSETUP_ASPECTRATIO_POLYMER)-1);
#endif
#endif
    else if (entry == &ME_SOUND_DUKETALK)
        return ud.config.VoiceToggle & 1;
    else if (entry == &ME_SOUND_DUKEMATCHPLAYER)
        return (ud.config.VoiceToggle & 4) ? 1 : 0;
    else if (entry == &ME_NETOPTIONS_EPISODE)
        return (currentValue < MAXVOLUMES ? ud.m_volume_number : MAXVOLUMES);
    else if (entry == &ME_NETOPTIONS_MONSTERS)
        return (ud.m_monsters_off ? MAXSKILLS : ud.m_player_skill);

    return currentValue;
}

static void M_MenuVerify(int32_t input)
{
    switch (g_currentMenu)
    {
    case MENU_RESETPLAYER:
        if (input)
        {
            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();
            FX_StopAllSounds();
            S_ClearSoundLocks();

            G_LoadPlayerMaybeMulti(g_lastSaveSlot);
        }
        else
        {
            if (sprite[g_player[myconnectindex].ps->i].extra <= 0)
            {
                if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
                return;
            }

            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
        }
        break;

    case MENU_LOADVERIFY:
        if (input)
        {
            g_lastSaveSlot = MG_LOAD.currentEntry;

            KB_FlushKeyboardQueue();
            KB_ClearKeysDown();
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }

            G_LoadPlayerMaybeMulti(g_lastSaveSlot);
        }
        else
        {
            if (g_player[myconnectindex].ps->gm&MODE_GAME)
            {
                g_player[myconnectindex].ps->gm &= ~MODE_MENU;
                if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
                {
                    ready2send = 1;
                    totalclock = ototalclock;
                }
            }
        }
        break;

    case MENU_SAVEVERIFY:
        if (!input)
        {
            save_xxh = 0;
            ReadSaveGameHeaders();

            ((MenuString_t*)MG_SAVE.entrylist[MG_SAVE.currentEntry]->entry)->editfield = NULL;
        }
        break;

    case MENU_NEWVERIFY:
        if (!input)
        {
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
        }
        break;

    case MENU_QUIT:
    case MENU_QUIT_INGAME:
        if (input)
            G_GameQuit();
        else
            g_quitDeadline = 0;
        break;

    case MENU_QUITTOTITLE:
        if (input)
        {
            g_player[myconnectindex].ps->gm = MODE_DEMO;
            if (ud.recstat == 1)
                G_CloseDemoWrite();
            E_MapArt_Clear();
        }
        break;

    case MENU_NETWAITVOTES:
        if (!input)
			Net_SendMapVoteCancel(0);
        break;

    default:
        break;
    }
}

static void M_MenuPasswordSubmit(char *input)
{
    switch (g_currentMenu)
    {
    case MENU_ADULTPASSWORD:
        if (ud.pwlockout[0] == 0 || ud.lockout == 0)
            Bstrcpy(&ud.pwlockout[0], input);
        else if (Bstrcmp(input, &ud.pwlockout[0]) == 0)
        {
            ud.lockout = 0;
#if 0
            for (x=0; x<g_numAnimWalls; x++)
                if (wall[animwall[x].wallnum].picnum != W_SCREENBREAK &&
                        wall[animwall[x].wallnum].picnum != W_SCREENBREAK+1 &&
                        wall[animwall[x].wallnum].picnum != W_SCREENBREAK+2)
                    if (wall[animwall[x].wallnum].extra >= 0)
                        wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
#endif
        }
        M_ChangeMenu(MENU_ADULTMODE);
        break;

    default:
        break;
    }
}

static void M_MenuFileSelectInit(MenuFileSelect_t *object)
{
    if (object->destination[0] == 0) Bstrcpy(object->destination, "./");
    Bcorrectfilename(object->destination, 1);

    fnlist_getnames(&object->fnlist, object->destination, object->pattern, 0, 0);
    object->finddirshigh = object->fnlist.finddirs;
    object->findfileshigh = object->fnlist.findfiles;
    object->currentlist = 0;
    if (object->findfileshigh)
        object->currentlist = 1;

    KB_FlushKeyboardQueue();
}

static void M_MenuFileSelect(int32_t input)
{
    switch (g_currentMenu)
    {
    case MENU_NETUSERMAP:
        if ((g_netServer || ud.multimode > 1))
            Net_SendUserMapName();
    case MENU_USERMAP:
        if (input)
        {
            ud.m_volume_number = 0;
            ud.m_level_number = 7;
            M_ChangeMenu(MENU_SKILL);
        }
        break;

    default:
        break;
    }
}





static Menu_t* M_FindMenuBinarySearch(MenuID_t query, size_t searchstart, size_t searchend)
{
    const size_t thissearch = (searchstart + searchend) / 2;
    const MenuID_t difference = query - Menus[thissearch].menuID;

    if (difference == 0)
        return &Menus[thissearch];
    else if (searchstart == searchend)
        return NULL;
    else if (difference > 0)
    {
        if (thissearch == searchend)
            return NULL;
        searchstart = thissearch + 1;
    }
    else if (difference < 0)
    {
        if (thissearch == searchstart)
            return NULL;
        searchend = thissearch - 1;
    }

    return M_FindMenuBinarySearch(query, searchstart, searchend);
}

static Menu_t* M_FindMenu(MenuID_t query)
{
    if ((unsigned) query > (unsigned) Menus[numMenus-1].menuID)
        return NULL;

    return M_FindMenuBinarySearch(query, 0, numMenus-1);
}

void M_ChangeMenu(MenuID_t cm)
{
    Menu_t *search;
    int32_t i;

    if (G_HaveEvent(EVENT_CHANGEMENU))
        cm = VM_OnEvent(EVENT_CHANGEMENU, g_player[myconnectindex].ps->i, myconnectindex, -1, cm);

    if (cm == MENU_PREVIOUS)
    {
        m_currentMenu = m_previousMenu;
        g_currentMenu = g_previousMenu;
    }
    else if (cm == MENU_CLOSE)
    {
        if (g_player[myconnectindex].ps->gm & MODE_GAME)
        {
            g_player[myconnectindex].ps->gm &= ~MODE_MENU;
            if ((!g_netServer && ud.multimode < 2) && ud.recstat != 2)
            {
                ready2send = 1;
                totalclock = ototalclock;
            }
        }
    }
    else if (cm >= 0)
    {
        if ((g_player[myconnectindex].ps->gm&MODE_GAME) && cm == MENU_MAIN)
            cm = MENU_MAIN_INGAME;

        search = M_FindMenu(cm);

        if (search == NULL)
            return;

        m_previousMenu = m_currentMenu;
        m_currentMenu = search;
        g_currentMenu = cm;
    }
    else
        return;

    switch (g_currentMenu)
    {
    case MENU_SAVE:
        if (g_player[myconnectindex].ps->gm&MODE_GAME)
        {
            g_screenCapture = 1;
            G_DrawRooms(myconnectindex,65536);
            g_screenCapture = 0;
        }
        break;

    case MENU_VIDEOSETUP:
        newresolution = 0;
        for (i = 0; i < MAXVALIDMODES; ++i)
        {
            if (resolution[i].xdim == xdim && resolution[i].ydim == ydim)
            {
                newresolution = i;
                break;
            }
        }
        newrendermode = getrendermode();
        newfullscreen = fullscreen;
        break;

    case MENU_SOUND:
    case MENU_SOUND_INGAME:
        soundrate = ud.config.MixRate;
        soundvoices = ud.config.NumVoices;
        soundbits = ud.config.NumBits;
        break;

    default:
        break;
    }

    if (m_currentMenu->type == Password)
    {
        typebuf[0] = 0;
        ((MenuPassword_t*)m_currentMenu->object)->input = typebuf;
    }
    else if (m_currentMenu->type == FileSelect)
        M_MenuFileSelectInit((MenuFileSelect_t*)m_currentMenu->object);
    else if (m_currentMenu->type == Menu)
    {
        MenuMenu_t *menu = (MenuMenu_t*)m_currentMenu->object;
        MenuGroup_t* currgroup = menu->grouplist[menu->currentGroup];
        // MenuEntry_t* currentry = currgroup->entrylist[currgroup->currentEntry];

        M_MenuEntryFocus(currgroup/*, currentry*/);
    }
}










void G_CheckPlayerColor(int32_t *color, int32_t prev_color)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(MEOSV_PLAYER_COLOR); ++i)
        if (*color == MEOSV_PLAYER_COLOR[i])
            return;

    *color = prev_color;
}


int32_t M_DetermineMenuSpecialState(MenuEntry_t *entry)
{
    if (entry == NULL)
        return 0;

    if (entry->type == String)
    {
        if (((MenuString_t*)entry->entry)->editfield)
            return 1;
    }
    else if (entry->type == Option)
    {
        if (((MenuOption_t*)entry->entry)->options->currentEntry >= 0)
            return 2;
    }
    else if (entry->type == Custom2Col)
    {
        if (((MenuCustom2Col_t*)entry->entry)->screenOpen)
            return 2;
    }

    return 0;
}

int32_t M_IsTextInput(Menu_t *cm)
{
    switch (m_currentMenu->type)
    {
        case Verify:
        case Password:
        case FileSelect:
        case Message:
            return 1;
            break;
        case Panel:
            return 0;
            break;
        case Menu:
        {
            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuGroup_t *group = menu->grouplist[menu->currentGroup];
            MenuEntry_t *entry = group->entrylist[group->currentEntry];
            return M_DetermineMenuSpecialState(entry);
        }
            break;
    }

    return 0;
}

static inline int32_t M_BlackTranslucentBackgroundOK(MenuID_t cm)
{
    switch (cm)
    {
        case MENU_COLCORR:
        case MENU_COLCORR_INGAME:
            return 0;
            break;
        default:
            return 1;
            break;
    }

    return 1;
}

static inline int32_t M_UpdateScreenOK(MenuID_t cm)
{
    switch (cm)
    {
        case MENU_LOAD:
        case MENU_SAVE:
        case MENU_LOADVERIFY:
        case MENU_SAVEVERIFY:
            return 0;
            break;
        default:
            return 1;
            break;
    }

    return 1;
}


/*
    Code below this point is entirely general,
    so if you want to change or add a menu,
    chances are you should scroll up.
*/

static void M_ShadePal(MenuTextType_t *font, uint8_t status, int32_t *s, int32_t *p)
{
    *s = (status & (1<<0)) ? (sintable[(totalclock<<5)&2047]>>12) : font->shade_deselected;
    *p = (status & (1<<1)) ? font->pal_disabled : font->pal;
}

static vec2_t M_MenuText(int32_t x, int32_t y, MenuTextType_t *font, const char *t, uint8_t status)
{
    int32_t s, p, ybetween = font->ybetween;
    int32_t f = font->textflags;
    if (status & (1<<2))
        f |= TEXT_XCENTER;
    if (status & (1<<3))
        f |= TEXT_XRIGHT;
    if (status & (1<<4))
    {
        f |= TEXT_YCENTER | TEXT_YOFFSETZERO;
        ybetween = font->yline; // <^ the battle against 'Q'
    }
    if (status & (1<<5))
        f |= TEXT_LITERALESCAPE;

    M_ShadePal(font, status, &s, &p);

    return G_ScreenText(font->tilenum, x, y, 65536, 0, 0, t, s, p, 2|8|16|ROTATESPRITE_FULL16, 0, font->xspace, font->yline, font->xbetween, ybetween, f, 0, 0, xdim-1, ydim-1);
}

#if 0
static vec2_t M_MenuTextSize(int32_t x, int32_t y, MenuTextType_t *font, const char *t, uint8_t status)
{
    int32_t f = font->textflags;
    if (status & (1<<5))
        f |= TEXT_LITERALESCAPE;

    return G_ScreenTextSize(font->tilenum, x, y, 65536, 0, t, 2|8|16|ROTATESPRITE_FULL16, font->xspace, font->yline, font->xbetween, font->ybetween, f, 0, 0, xdim-1, ydim-1);
}
#endif

static int32_t M_FindOptionBinarySearch(MenuOption_t *object, const int32_t query, size_t searchstart, size_t searchend)
{
    const size_t thissearch = (searchstart + searchend) / 2;
    const int32_t difference = ((object->options->optionValues == NULL && query < 0) ? object->options->numOptions-1 : query) - ((object->options->optionValues == NULL) ? (int32_t) thissearch : object->options->optionValues[thissearch]);

    if (difference == 0)
        return thissearch;
    else if (searchstart == searchend)
        return -1;
    else if (difference > 0)
    {
        if (thissearch == searchend)
            return -1;
        searchstart = thissearch + 1;
    }
    else if (difference < 0)
    {
        if (thissearch == searchstart)
            return -1;
        searchend = thissearch - 1;
    }

    return M_FindOptionBinarySearch(object, query, searchstart, searchend);
}

static int32_t M_RunMenu_MenuMenu(MenuMenu_t *menu, MenuEntry_t *currentry, int32_t state)
{
    const int32_t cursorShade = 4-(sintable[(totalclock<<4)&2047]>>11);
    int32_t g, rely = INT32_MIN, srcy = INT32_MIN, totalextent = 0;

    menu->ytop = INT32_MAX;
    menu->bottomcutoff = INT32_MIN;

    // find min/max
    for (g = 0; g < menu->numGroups; ++g)
    {
        MenuGroup_t *group = menu->grouplist[g];

        if (group == NULL || group->numEntries == 0)
            continue;

        if (group->position->pos.y < menu->ytop)
            menu->ytop = group->position->pos.y;

        if (group->position->bottomcutoff > menu->bottomcutoff)
            menu->bottomcutoff = group->position->bottomcutoff;
    }

    // iterate through menu
    for (g = 0; g < menu->numGroups; ++g)
    {
        MenuGroup_t *group = menu->grouplist[g];
        int32_t e, y;

        if (group == NULL || group->numEntries == 0)
            continue;

        y = group->position->pos.y;

        if (y == srcy)
            y = rely + group->position->groupspacing;
        else
            srcy = y;

        for (e = 0; e < group->numEntries; ++e)
        {
            MenuEntry_t *entry = group->entrylist[e];
            uint8_t status = 0;
            int32_t height, x;
            // vec2_t textsize;
            int32_t dodraw = 1;

            if (entry == NULL)
                continue;

            x = group->position->pos.x;

            status |= (g == menu->currentGroup && e == group->currentEntry)<<0;
            status |= (entry->disabled)<<1;
            status |= (group->position->width == 0)<<2;

            dodraw &= menu->ytop <= y - menu->scrollPos && y - menu->scrollPos + entry->font->yline <= menu->bottomcutoff;

            if (dodraw)
            /*textsize =*/ M_MenuText(x, y - menu->scrollPos, entry->font, entry->name, status);

            height = entry->font->yline; // max(textsize.y, entry->font->yline); // bluefont Q ruins this

            status |= (group->position->width < 0)<<3 | (1<<4);

            if (dodraw && (status & (1<<0)) && state != 1)
            {
                if (status & (1<<2))
                {
                    rotatesprite_fs((160<<16) + group->position->cursorPosition, y + (height>>1) - menu->scrollPos, group->position->cursorScale, 0, SPINNINGNUKEICON+6-((6+(totalclock>>3))%7), cursorShade, 0, 10);
                    rotatesprite_fs((160<<16) - group->position->cursorPosition, y + (height>>1) - menu->scrollPos, group->position->cursorScale, 0, SPINNINGNUKEICON+((totalclock>>3)%7), cursorShade, 0, 10);
                }
                else
                    rotatesprite_fs(x - group->position->cursorPosition,
                    y + (height>>1) - menu->scrollPos, group->position->cursorScale, 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);
            }

            x += klabs(group->position->width);

            if (dodraw)
            switch (entry->type)
            {
                case Dummy:
                    break;
                case Link:
                    break;
                case Option:
                {
                    MenuOption_t *object = (MenuOption_t*)entry->entry;
                    int32_t currentOption = M_FindOptionBinarySearch(object, object->data == NULL ? M_MenuEntryOptionSource(entry, object->currentOption) : *object->data, 0, object->options->numOptions);

                    if (currentOption >= 0)
                        object->currentOption = currentOption;

                    M_MenuText(x, y + (height>>1) - menu->scrollPos, object->font, currentOption < 0 ? MenuCustom : object->options->optionNames[currentOption], status);
                    break;
                }
                case Custom2Col:
                {
                    MenuCustom2Col_t *object = (MenuCustom2Col_t*)entry->entry;
                    M_MenuText(x - ((status & (1<<3)) ? object->columnWidth : 0), y + (height>>1) - menu->scrollPos, object->font, object->key[*object->column[0]], status & ~((group->currentColumn != 0)<<0));
                    M_MenuText(x + ((status & (1<<3)) ? 0 : object->columnWidth), y + (height>>1) - menu->scrollPos, object->font, object->key[*object->column[1]], status & ~((group->currentColumn != 1)<<0));
                    break;
                }
                case RangeInt32:
                {
                    MenuRangeInt32_t *object = (MenuRangeInt32_t*)entry->entry;

                    int32_t s, p;
                    const int32_t z = scale(65536, height, tilesizy[SLIDEBAR]<<16);
                    M_ShadePal(object->font, status, &s, &p);

                    if (status & (1<<3))
                        x -= scale(tilesizx[SLIDEBAR]<<16, height, tilesizy[SLIDEBAR]<<16);

                    rotatesprite_fs(x, y - menu->scrollPos, z, 0, SLIDEBAR, s, entry->disabled ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16);

                    rotatesprite_fs(
                    x + (1<<16) + scale(scale((tilesizx[SLIDEBAR]-2-tilesizx[SLIDEBAR+1])<<16, height, tilesizy[SLIDEBAR]<<16), *object->variable - object->min, object->max - object->min),
                    y + scale((tilesizy[SLIDEBAR]-tilesizy[SLIDEBAR+1])<<15, height, tilesizy[SLIDEBAR]<<16) - menu->scrollPos,
                    z, 0, SLIDEBAR+1, s, entry->disabled ? 1 : 0, 2|8|16|ROTATESPRITE_FULL16);

                    if (object->displaytype > 0)
                    {
                        status |= (1<<3);

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = object->max;

                        switch (object->displaytype)
                        {
                            case 1:
                                Bsprintf(tempbuf, "%d", *object->variable);
                                break;
                            case 2:
                            {
                                int32_t v;
                                ftol(((float) *object->variable * 100.) / (float) object->onehundredpercent + 0.5, &v);
                                Bsprintf(tempbuf, "%d%%", v);
                                break;
                            }
                            case 3:
                                Bsprintf(tempbuf, "%.2f", (double) *object->variable / (double) object->onehundredpercent);
                                break;
                        }

                        M_MenuText(x - (4<<16), y + (height>>1) - menu->scrollPos, object->font, tempbuf, status);
                    }
                    break;
                }
                case RangeFloat:
                {
                    MenuRangeFloat_t *object = (MenuRangeFloat_t*)entry->entry;

                    int32_t s, p;
                    const int32_t z = scale(65536, height, tilesizy[SLIDEBAR]<<16);
                    M_ShadePal(object->font, status, &s, &p);

                    if (status & (1<<3))
                        x -= scale(tilesizx[SLIDEBAR]<<16, height, tilesizy[SLIDEBAR]<<16);

                    rotatesprite_fs(x, y - menu->scrollPos, z, 0, SLIDEBAR, s, p, 2|8|16|ROTATESPRITE_FULL16);

                    rotatesprite_fs(
                    x + (1<<16) + ((float) scale((tilesizx[SLIDEBAR]-2-tilesizx[SLIDEBAR+1])<<16, height, tilesizy[SLIDEBAR]<<16) * (*object->variable - object->min) / (object->max - object->min)),
                    y + scale((tilesizy[SLIDEBAR]-tilesizy[SLIDEBAR+1])<<15, height, tilesizy[SLIDEBAR]<<16) - menu->scrollPos,
                    z, 0, SLIDEBAR+1, s, p, 2|8|16|ROTATESPRITE_FULL16);

                    if (object->displaytype > 0)
                    {
                        status |= (1<<3);

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = 1.;

                        switch (object->displaytype)
                        {
                            case 1:
                                Bsprintf(tempbuf, "%.2f", *object->variable);
                                break;
                            case 2:
                            {
                                int32_t v;
                                ftol((*object->variable * 100.) / object->onehundredpercent + 0.5, &v);
                                Bsprintf(tempbuf, "%d%%", v);
                                break;
                            }
                            case 3:
                                Bsprintf(tempbuf, "%.2f", *object->variable / object->onehundredpercent);
                                break;
                        }

                        M_MenuText(x - (4<<16), y + (height>>1) - menu->scrollPos, object->font, tempbuf, status);
                    }
                    break;
                }
                case RangeDouble:
                {
                    MenuRangeDouble_t *object = (MenuRangeDouble_t*)entry->entry;

                    int32_t s, p;
                    const int32_t z = scale(65536, height, tilesizy[SLIDEBAR]<<16);
                    M_ShadePal(object->font, status, &s, &p);

                    if (status & (1<<3))
                        x -= scale(tilesizx[SLIDEBAR]<<16, height, tilesizy[SLIDEBAR]<<16);

                    rotatesprite_fs(x, y - menu->scrollPos, z, 0, SLIDEBAR, s, p, 2|8|16|ROTATESPRITE_FULL16);

                    rotatesprite_fs(
                    x + (1<<16) + ((double) scale((tilesizx[SLIDEBAR]-2-tilesizx[SLIDEBAR+1])<<16, height, tilesizy[SLIDEBAR]<<16) * (*object->variable - object->min) / (object->max - object->min)),
                    y + scale((tilesizy[SLIDEBAR]-tilesizy[SLIDEBAR+1])<<15, height, tilesizy[SLIDEBAR]<<16) - menu->scrollPos,
                    z, 0, SLIDEBAR+1, s, p, 2|8|16|ROTATESPRITE_FULL16);

                    if (object->displaytype > 0)
                    {
                        status |= (1<<3);

                        if (object->onehundredpercent == 0)
                            object->onehundredpercent = 1.;

                        switch (object->displaytype)
                        {
                            case 1:
                                Bsprintf(tempbuf, "%.2f", *object->variable);
                                break;
                            case 2:
                            {
                                int32_t v;
                                dtol((*object->variable * 100.) / object->onehundredpercent + 0.5, &v);
                                Bsprintf(tempbuf, "%d%%", v);
                                break;
                            }
                            case 3:
                                Bsprintf(tempbuf, "%.2f", *object->variable / object->onehundredpercent);
                                break;
                        }

                        M_MenuText(x - (4<<16), y + (height>>1) - menu->scrollPos, object->font, tempbuf, status);
                    }
                    break;
                }
                case String:
                {
                    MenuString_t *object = (MenuString_t*)entry->entry;

                    if (entry == currentry && object->editfield != NULL)
                    {
                        const vec2_t dim = M_MenuText(x, y + (height>>1), object->font, object->editfield, status | (1<<5));
                        const int32_t h = max(dim.y, entry->font->yline);

                        rotatesprite_fs(x + dim.x + (1<<16) + scale(tilesizx[SPINNINGNUKEICON]<<15, h, tilesizy[SPINNINGNUKEICON]<<16), y + (height>>1) - menu->scrollPos, scale(65536, h, tilesizy[SPINNINGNUKEICON]<<16), 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);
                    }
                    else
                        M_MenuText(x, y + (height>>1) - menu->scrollPos, object->font, object->variable, status);
                    break;
                }
            }

            // prepare for the next line
            entry->ytop = y;

            y += height;
            entry->ybottom = y;
            totalextent = y;

            y += group->position->entryspacing;
            rely = y;
        }
    }

    menu->totalHeight = totalextent - menu->ytop;

    // draw indicators if applicable
    if (totalextent > menu->bottomcutoff)
    {
        rotatesprite((320 - tilesizx[SELECTDIR])<<16, menu->ytop, 65536, 0, SELECTDIR, menu->scrollPos > 0 ? 0 : 20, 0, 26, 0, 0, xdim-1, scale(ydim, menu->ytop + (tilesizy[SELECTDIR]<<15), 200<<16) - 1);
        rotatesprite((320 - tilesizx[SELECTDIR])<<16, menu->bottomcutoff - (tilesizy[SELECTDIR]<<16), 65536, 0, SELECTDIR, menu->ytop + menu->totalHeight - menu->scrollPos > menu->bottomcutoff ? 0 : 20, 0, 26, 0, scale(ydim, menu->bottomcutoff - (tilesizy[SELECTDIR]<<15), 200<<16), xdim-1, ydim-1);
    }

    return menu->totalHeight;
}

static void M_RunMenu_MenuOptionList(MenuOption_t *object)
{
    const int32_t cursorShade = 4-(sintable[(totalclock<<4)&2047]>>11);
    int32_t e, y = object->options->list->pos.y;

    for (e = 0; e < object->options->numOptions; ++e)
    {
        uint8_t status = 0;
        int32_t height, x;
        // vec2_t textsize;
        int32_t dodraw = 1;

        x = object->options->list->pos.x;

        status |= (e == object->options->currentEntry)<<0;
        status |= (object->options->list->width == 0)<<2;

        dodraw &= object->options->list->pos.y <= y - object->options->scrollPos && y - object->options->scrollPos + object->font->yline <= object->options->list->bottomcutoff;

        if (dodraw)
        /*textsize =*/ M_MenuText(x, y - object->options->scrollPos, object->font, object->options->optionNames[e], status);

        height = object->font->yline; // max(textsize.y, object->font->yline);

        status |= (object->options->list->width < 0)<<3 | (1<<4);

        if (dodraw && (status & (1<<0)))
        {
            if (status & (1<<2))
            {
                rotatesprite_fs((160<<16) + object->options->list->cursorPosition, y + (height>>1) - object->options->scrollPos, object->options->list->cursorScale, 0, SPINNINGNUKEICON+6-((6+(totalclock>>3))%7), cursorShade, 0, 10);
                rotatesprite_fs((160<<16) - object->options->list->cursorPosition, y + (height>>1) - object->options->scrollPos, object->options->list->cursorScale, 0, SPINNINGNUKEICON+((totalclock>>3)%7), cursorShade, 0, 10);
            }
            else
                rotatesprite_fs(x - object->options->list->cursorPosition,
                y + (height>>1) - object->options->scrollPos, object->options->list->cursorScale, 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);
        }

        // prepare for the next line
        y += height;
        y += object->options->list->entryspacing;
    }

    y -= object->options->list->entryspacing;

    // draw indicators if applicable
    if (y > object->options->list->bottomcutoff)
    {
        rotatesprite((320 - tilesizx[SELECTDIR])<<16, object->options->list->pos.y, 65536, 0, SELECTDIR, object->options->scrollPos > 0 ? 0 : 20, 0, 26, 0, 0, xdim-1, scale(ydim, object->options->list->pos.y + (tilesizy[SELECTDIR]<<15), 200<<16) - 1);
        rotatesprite((320 - tilesizx[SELECTDIR])<<16, object->options->list->bottomcutoff - (tilesizy[SELECTDIR]<<16), 65536, 0, SELECTDIR, y - object->options->scrollPos > object->options->list->bottomcutoff ? 0 : 20, 0, 26, 0, scale(ydim, object->options->list->bottomcutoff - (tilesizy[SELECTDIR]<<15), 200<<16), xdim-1, ydim-1);
    }
}

static void M_RunMenu(Menu_t *cm)
{
    const int32_t cursorShade = 4-(sintable[(totalclock<<4)&2047]>>11);

    switch (cm->type)
    {
        case Verify:
        {
            MenuVerify_t *object = (MenuVerify_t*)cm->object;

            M_PreMenu(cm->menuID);

            rotatesprite_fs(object->cursorpos.x, object->cursorpos.y, 65536, 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);

            M_PreMenuDraw(cm->menuID, NULL, NULL);
            break;
        }

        case Message:
        {
            MenuMessage_t *object = (MenuMessage_t*)cm->object;

            rotatesprite_fs(object->cursorpos.x, object->cursorpos.y, 65536, 0, SPINNINGNUKEICON+(((totalclock>>3))%7), cursorShade, 0, 10);

            M_PreMenu(cm->menuID);
            M_PreMenuDraw(cm->menuID, NULL, NULL);
            break;
        }

        case Password:
        {
            MenuPassword_t *object = (MenuPassword_t*)cm->object;
            size_t x;

            M_PreMenu(cm->menuID);

            mgametext(160,50+16+16+16+16-12,"Enter Password",0,2|8|16);

            for (x=0; x < Bstrlen(object->input); ++x)
                tempbuf[x] = '*';
            tempbuf[x] = 0;

            x = gametext(160,50+16+16+16+16,tempbuf,998,2|8|16);

            x = 160 + ((x - 160)>>1);

            rotatesprite_fs((x+8)<<16,(50+16+16+16+16+4)<<16,32768,0,SPINNINGNUKEICON+((totalclock>>3)%7),cursorShade,0,2|8);

            M_PreMenuDraw(cm->menuID, NULL, NULL);
            break;
        }

        case FileSelect:
        {
            MenuFileSelect_t *object = (MenuFileSelect_t*)cm->object;

            int32_t i, width = 160 - (40-4);

            M_PreMenu(cm->menuID);

            // black translucent background underneath file lists
            rotatesprite(0<<16, 0<<16, 65536<<5, 0, /*tile*/ 0, numshades, 0, 10+16+1+32,
                         xdim/2-scale(width,(ydim*4)/3,320),scale(12+32-2,ydim,200),
                         xdim/2+scale(width,(ydim*4)/3,320)-1,scale(12+32+112+4,ydim,200)-1);

            // path
            minitext(38,45,object->destination,16,26);

            mgametext(40+4,32,"Directories",0,2+8+16);

            if (object->finddirshigh)
            {
                int32_t len;

                object->dir = object->finddirshigh;
                for (i=0; i<5; i++) if (!object->dir->prev) break;
                    else object->dir=object->dir->prev;
                for (i=5; i>-8 && object->dir; i--, object->dir=object->dir->next)
                {
                    uint8_t status = 0;

                    len = Bstrlen(object->dir->name);
                    Bstrncpy(tempbuf,object->dir->name,len);
                    if (len > USERMAPENTRYLENGTH)
                    {
                        len = USERMAPENTRYLENGTH-3;
                        tempbuf[len] = 0;
                        while (len < USERMAPENTRYLENGTH)
                            tempbuf[len++] = '.';
                    }
                    tempbuf[len] = 0;

                    status |= (object->dir == object->finddirshigh && object->currentlist == 0)<<0;

                    M_MenuText(40<<16, (1+12+32+8*(6-i))<<16, object->dirfont, tempbuf, status);
                }
            }

            mgametext(180+4,32,"Files",0,2+8+16);

            if (object->findfileshigh)
            {
                int32_t len;

                object->dir = object->findfileshigh;
                for (i=0; i<6; i++) if (!object->dir->prev) break;
                    else object->dir=object->dir->prev;
                for (i=6; i>-8 && object->dir; i--, object->dir=object->dir->next)
                {
                    uint8_t status = 0;

                    len = Bstrlen(object->dir->name);
                    Bstrncpy(tempbuf,object->dir->name,len);
                    if (len > USERMAPENTRYLENGTH)
                    {
                        len = USERMAPENTRYLENGTH-3;
                        tempbuf[len] = 0;
                        while (len < USERMAPENTRYLENGTH)
                            tempbuf[len++] = '.';
                    }
                    tempbuf[len] = 0;

                    status |= (object->dir == object->findfileshigh && object->currentlist == 1)<<0;

                    M_MenuText(180<<16, (1+12+32+8*(6-i))<<16, object->filefont, tempbuf, status);

                    // object->dir->source==CACHE1D_SOURCE_ZIP ? 8 : 2
                }
            }

            rotatesprite_fs(((object->currentlist == 0 ? 45 : 185)<<16)-(21<<15),(32+4+1-2)<<16,32768,0,SPINNINGNUKEICON+(((totalclock>>3))%7),cursorShade,0,10);

            M_PreMenuDraw(cm->menuID, NULL, NULL);

            if (object->title != NoTitle)
                M_DrawTopBar(object->title);
            break;
        }

        case Panel:
        {
            MenuPanel_t *object = (MenuPanel_t*)cm->object;
            M_PreMenu(cm->menuID);
            M_PreMenuDraw(cm->menuID, NULL, NULL);
            if (object->title != NoTitle)
                M_DrawTopBar(object->title);
            break;
        }

        case Menu:
        {
            int32_t state;

            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuGroup_t *currgroup = menu->grouplist[menu->currentGroup];
            MenuEntry_t *currentry = currgroup->entrylist[currgroup->currentEntry];

            state = M_DetermineMenuSpecialState(currentry);

            if (state != 2)
            {
                M_PreMenu(cm->menuID);
                M_PreMenuDraw(cm->menuID, currgroup, currentry);

                M_RunMenu_MenuMenu(menu, currentry, state);
            }
            else
            {
                if (currentry->type == Option)
                {
                    M_PreMenuOptionListDraw(/*currgroup,*/ currentry);

                    M_RunMenu_MenuOptionList((MenuOption_t*)currentry->entry);
                }
                else if (currentry->type == Custom2Col)
                {
                    M_PreMenuCustom2ColScreenDraw(currgroup, currentry);
                }
            }

            if (menu->title != NoTitle)
                M_DrawTopBar(menu->title);
            break;
        }
    }
}

static void M_RunMenuInput(Menu_t *cm)
{
    switch (cm->type)
    {
        case Panel:
        {
            MenuPanel_t *panel = (MenuPanel_t*)cm->object;

            if (I_ReturnTrigger())
            {
                I_ReturnTriggerClear();

                S_PlaySound(EXITMENUSOUND);

                M_ChangeMenu(cm->parentID);
            }
            else if (I_PanelUp())
            {
                I_PanelUpClear();

                S_PlaySound(KICK_HIT);
                M_ChangeMenu(panel->previousID);
            }
            else if (I_PanelDown())
            {
                I_PanelDownClear();

                S_PlaySound(KICK_HIT);
                M_ChangeMenu(panel->nextID);
            }
        }

        case Password:
        {
            MenuPassword_t *object = (MenuPassword_t*)cm->object;
            int32_t hitstate = I_EnterText(object->input, object->maxlength, 0);

            if (hitstate == 1)
            {
                S_PlaySound(PISTOL_BODYHIT);

                M_MenuPasswordSubmit(object->input);

                object->input = NULL;
            }
            else if (hitstate == -1)
            {
                S_PlaySound(EXITMENUSOUND);

                object->input = NULL;

                M_ChangeMenu(cm->parentID);
            }
            break;
        }

        case FileSelect:
        {
            MenuFileSelect_t *object = (MenuFileSelect_t*)cm->object;

            // JBF 20040208: seek to first name matching pressed character
            CACHE1D_FIND_REC *seeker = object->currentlist ? object->fnlist.findfiles : object->fnlist.finddirs;

            if (I_ReturnTrigger())
            {
                I_ReturnTriggerClear();

                S_PlaySound(EXITMENUSOUND);

                fnlist_clearnames(&object->fnlist);

                object->destination[0] = 0;

                M_MenuFileSelect(0);

                M_ChangeMenu(cm->parentID);
            }
            else if (I_AdvanceTrigger())
            {
                I_AdvanceTriggerClear();

                S_PlaySound(PISTOL_BODYHIT);

                if (object->currentlist == 0)
                {
                    if (!object->finddirshigh) break;
                    Bstrcat(object->destination, object->finddirshigh->name);
                    Bstrcat(object->destination, "/");
                    Bcorrectfilename(object->destination, 1);

                    fnlist_clearnames(&object->fnlist);

                    M_MenuFileSelectInit(object);
                }
                else
                {
                    if (!object->findfileshigh) break;
                    Bstrcat(object->destination, object->findfileshigh->name);

                    fnlist_clearnames(&object->fnlist);

                    M_MenuFileSelect(1);
                }
            }
            else if ((KB_KeyPressed(sc_Home)|KB_KeyPressed(sc_End)) > 0)
            {
                while (seeker && (KB_KeyPressed(sc_End)?seeker->next:seeker->prev))
                    seeker = KB_KeyPressed(sc_End)?seeker->next:seeker->prev;
                if (seeker)
                {
                    if (object->currentlist) object->findfileshigh = seeker;
                    else object->finddirshigh = seeker;
                    // clear keys, don't play the kick sound a dozen times!
                    KB_ClearKeyDown(sc_End);
                    KB_ClearKeyDown(sc_Home);
                    S_PlaySound(KICK_HIT);
                }
            }
            else if ((KB_KeyPressed(sc_PgUp)|KB_KeyPressed(sc_PgDn)) > 0)
            {
                int32_t i = 6;
                seeker = object->currentlist?object->findfileshigh:object->finddirshigh;
                while (i>0)
                {
                    if (seeker && (KB_KeyPressed(sc_PgDn)?seeker->next:seeker->prev))
                        seeker = KB_KeyPressed(sc_PgDn)?seeker->next:seeker->prev;
                    i--;
                }
                if (seeker)
                {
                    if (object->currentlist) object->findfileshigh = seeker;
                    else object->finddirshigh = seeker;
                    // clear keys, don't play the kick sound a dozen times!
                    KB_ClearKeyDown(sc_PgDn);
                    KB_ClearKeyDown(sc_PgUp);
                    S_PlaySound(KICK_HIT);
                }
            }
            else if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) ||
                    KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) ||
                    KB_KeyPressed(sc_Tab) || (MOUSE_GetButtons()&MIDDLE_MOUSE))
            {
                KB_ClearKeyDown(sc_LeftArrow);
                KB_ClearKeyDown(sc_kpad_4);
                KB_ClearKeyDown(sc_RightArrow);
                KB_ClearKeyDown(sc_kpad_6);
                KB_ClearKeyDown(sc_Tab);
                MOUSE_ClearButton(MIDDLE_MOUSE);
                object->currentlist = !object->currentlist;
                S_PlaySound(KICK_HIT);
            }
            else if (KB_KeyPressed(sc_UpArrow) || KB_KeyPressed(sc_kpad_8) || (MOUSE_GetButtons()&WHEELUP_MOUSE) || BUTTON(gamefunc_Move_Forward) || (JOYSTICK_GetHat(0)&HAT_UP))
            {
                KB_ClearKeyDown(sc_UpArrow);
                KB_ClearKeyDown(sc_kpad_8);
                MOUSE_ClearButton(WHEELUP_MOUSE);
                CONTROL_ClearButton(gamefunc_Move_Forward);
                JOYSTICK_ClearHat(0);

                S_PlaySound(KICK_HIT);

                if (object->currentlist == 0)
                {
                    if (object->finddirshigh)
                        if (object->finddirshigh->prev) object->finddirshigh = object->finddirshigh->prev;
                }
                else
                {
                    if (object->findfileshigh)
                        if (object->findfileshigh->prev) object->findfileshigh = object->findfileshigh->prev;
                }
            }
            else if (KB_KeyPressed(sc_DownArrow) || KB_KeyPressed(sc_kpad_2) || (MOUSE_GetButtons()&WHEELDOWN_MOUSE) || BUTTON(gamefunc_Move_Backward) || (JOYSTICK_GetHat(0)&HAT_DOWN))
            {
                KB_ClearKeyDown(sc_DownArrow);
                KB_ClearKeyDown(sc_kpad_2);
                KB_ClearKeyDown(sc_PgDn);
                MOUSE_ClearButton(WHEELDOWN_MOUSE);
                CONTROL_ClearButton(gamefunc_Move_Backward);
                JOYSTICK_ClearHat(0);

                S_PlaySound(KICK_HIT);

                if (object->currentlist == 0)
                {
                    if (object->finddirshigh)
                        if (object->finddirshigh->next) object->finddirshigh = object->finddirshigh->next;
                }
                else
                {
                    if (object->findfileshigh)
                        if (object->findfileshigh->next) object->findfileshigh = object->findfileshigh->next;
                }
            }
            else
            {
                char ch2, ch;
                ch = KB_GetCh();
                if (ch > 0 && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')))
                {
                    if (ch >= 'a') ch -= ('a'-'A');
                    while (seeker)
                    {
                        ch2 = seeker->name[0];
                        if (ch2 >= 'a' && ch2 <= 'z') ch2 -= ('a'-'A');
                        if (ch2 == ch) break;
                        seeker = seeker->next;
                    }
                    if (seeker)
                    {
                        if (object->currentlist) object->findfileshigh = seeker;
                        else object->finddirshigh = seeker;
                        S_PlaySound(KICK_HIT);
                    }
                }
            }

            M_PreMenuInput(NULL, NULL);
            break;
        }

        case Message:
            if (I_ReturnTrigger())
            {
                I_ReturnTriggerClear();

                S_PlaySound(EXITMENUSOUND);

                M_ChangeMenu(cm->parentID);
            }

            if (I_CheckAllInput())
            {
                I_ClearAllInput();

                S_PlaySound(EXITMENUSOUND);

                M_ChangeMenu(((MenuMessage_t*)cm->object)->linkID);
            }

            M_PreMenuInput(NULL, NULL);
            break;

        case Verify:
            if (I_ReturnTrigger() || KB_KeyPressed(sc_N))
            {
                I_ReturnTriggerClear();
                KB_ClearKeyDown(sc_N);

                M_MenuVerify(0);

                M_ChangeMenu(cm->parentID);
            }

            if (I_AdvanceTrigger() || KB_KeyPressed(sc_Y))
            {
                I_AdvanceTriggerClear();
                KB_ClearKeyDown(sc_Y);

                M_MenuVerify(1);

                M_ChangeMenu(((MenuVerify_t*)cm->object)->linkID);
            }

            M_PreMenuInput(NULL, NULL);
            break;

        case Menu:
        {
            int32_t state, movement = 0;

            MenuMenu_t *menu = (MenuMenu_t*)cm->object;
            MenuGroup_t *currgroup = menu->grouplist[menu->currentGroup];
            MenuEntry_t *currentry = currgroup->entrylist[currgroup->currentEntry];

            state = M_DetermineMenuSpecialState(currentry);

            if (state == 0)
            {
                if (currentry != NULL)
                switch (currentry->type)
                {
                    case Dummy:
                        break;
                    case Link:
                        if (currentry->disabled)
                            break;
                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            M_MenuEntryLinkActivate(currgroup, currentry);

                            if (g_currentMenu != MENU_SKILL)
                                S_PlaySound(PISTOL_BODYHIT);

                            M_ChangeMenu(((MenuLink_t*)currentry->entry)->linkID);
                        }
                        break;
                    case Option:
                    {
                        MenuOption_t *object = (MenuOption_t*)currentry->entry;
                        int32_t modification = -1;

                        if (currentry->disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            S_PlaySound(PISTOL_BODYHIT);

                            if (object->options->features & 2)
                            {
                                modification = object->currentOption + 1;
                                if (modification >= object->options->numOptions)
                                    modification = 0;
                            }
                            else
                            {
                                object->options->currentEntry = object->currentOption;
                            }
                        }
                        else if (KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6))
                        {
                            modification = object->currentOption + 1;
                            if (modification >= object->options->numOptions)
                                modification = 0;

                            KB_ClearKeyDown(sc_RightArrow);
                            KB_ClearKeyDown(sc_kpad_6);
                            S_PlaySound(PISTOL_BODYHIT);
                        }
                        else if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4))
                        {
                            modification = object->currentOption - 1;
                            if (modification < 0)
                                modification = object->options->numOptions - 1;

                            KB_ClearKeyDown(sc_LeftArrow);
                            KB_ClearKeyDown(sc_kpad_4);
                            S_PlaySound(PISTOL_BODYHIT);
                        }

                        if (modification >= 0)
                        {
                            int32_t temp = (object->options->optionValues == NULL) ? modification : object->options->optionValues[modification];
                            if (!M_MenuEntryOptionModify(currgroup, currentry, temp))
                            {
                                object->currentOption = modification;
                                if ((int32_t*)object->data != NULL)
                                    *((int32_t*)object->data) = temp;
                            }
                        }
                    }
                        break;
                    case Custom2Col:
                        if (KB_KeyPressed(sc_LeftArrow) || KB_KeyPressed(sc_kpad_4) ||
                                 KB_KeyPressed(sc_RightArrow) || KB_KeyPressed(sc_kpad_6) ||
                                 KB_KeyPressed(sc_Tab) || (MOUSE_GetButtons()&MIDDLE_MOUSE))
                        {
                            currgroup->currentColumn = !currgroup->currentColumn;
                            KB_ClearKeyDown(sc_LeftArrow);
                            KB_ClearKeyDown(sc_RightArrow);
                            KB_ClearKeyDown(sc_kpad_4);
                            KB_ClearKeyDown(sc_kpad_6);
                            KB_ClearKeyDown(sc_Tab);
                            MOUSE_ClearButton(MIDDLE_MOUSE);
                            S_PlaySound(KICK_HIT);
                        }

                        if (currentry->disabled)
                            break;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            S_PlaySound(PISTOL_BODYHIT);

                            M_MenuCustom2ColScreen(currgroup/*, currentry*/);

                            ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 1;
                        }
                        break;
                    case RangeInt32:
                    {
                        MenuRangeInt32_t *object = (MenuRangeInt32_t*)currentry->entry;
                        const double interval = (double) (object->max - object->min) / (object->steps - 1);
                        int32_t step;
                        int32_t modification = 0;

                        if (currentry->disabled)
                            break;

                        dtol((double) (*object->variable - object->min) / interval + 0.5, &step);

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            modification = -1;
                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            modification = 1;
                            S_PlaySound(KICK_HIT);
                        }

                        if (modification != 0)
                        {
                            int32_t temp;

                            step += modification;

                            if (step < 0)
                                step = 0;
                            else if (step >= object->steps)
                                step = object->steps - 1;

                            dtol(interval * step + object->min + 0.5, &temp);

                            if (!M_MenuEntryRangeInt32Modify(currentry, temp))
                                *object->variable = temp;
                        }

                        break;
                    }
                    case RangeFloat:
                    {
                        MenuRangeFloat_t *object = (MenuRangeFloat_t*)currentry->entry;
                        const float interval = (object->max - object->min) / (object->steps - 1);
                        int32_t step;
                        int32_t modification = 0;

                        if (currentry->disabled)
                            break;

                        ftol((*object->variable - object->min) / interval + 0.5, &step);

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            modification = -1;
                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            modification = 1;
                            S_PlaySound(KICK_HIT);
                        }

                        if (modification != 0)
                        {
                            float temp;

                            step += modification;

                            if (step < 0)
                                step = 0;
                            else if (step >= object->steps)
                                step = object->steps - 1;

                            temp = interval * step + object->min;

                            if (!M_MenuEntryRangeFloatModify(currentry, temp))
                                *object->variable = temp;
                        }

                        break;
                    }
                    case RangeDouble:
                    {
                        MenuRangeDouble_t *object = (MenuRangeDouble_t*)currentry->entry;
                        const double interval = (object->max - object->min) / (object->steps - 1);
                        int32_t step;
                        int32_t modification = 0;

                        if (currentry->disabled)
                            break;

                        dtol((*object->variable - object->min) / interval + 0.5, &step);

                        if (I_SliderLeft())
                        {
                            I_SliderLeftClear();

                            modification = -1;
                            S_PlaySound(KICK_HIT);
                        }
                        else if (I_SliderRight())
                        {
                            I_SliderRightClear();

                            modification = 1;
                            S_PlaySound(KICK_HIT);
                        }

                        if (modification != 0)
                        {
                            double temp;

                            step += modification;

                            if (step < 0)
                                step = 0;
                            else if (step >= object->steps)
                                step = object->steps - 1;

                            temp = interval * step + object->min;

                            if (!M_MenuEntryRangeDoubleModify(currentry/*, temp*/))
                                *object->variable = temp;
                        }

                        break;
                    }

                    case String:
                    {
                        MenuString_t *object = (MenuString_t*)currentry->entry;

                        if (I_AdvanceTrigger())
                        {
                            I_AdvanceTriggerClear();

                            S_PlaySound(PISTOL_BODYHIT);

                            Bstrncpy(typebuf, object->variable, TYPEBUFSIZE);
                            object->editfield = typebuf;

                            // this limitation is an arbitrary implementation detail
                            if (object->maxlength > TYPEBUFSIZE)
                                object->maxlength = TYPEBUFSIZE;

                            M_MenuEntryStringActivate(currgroup/*, currentry*/);
                        }

                        break;
                    }
                }

                if (I_ReturnTrigger() || I_EscapeTrigger())
                {
                    I_ReturnTriggerClear();
                    I_EscapeTriggerClear();

                    S_PlaySound(EXITMENUSOUND);

                    M_ChangeMenu(cm->parentID);
                }
                else if (KB_KeyPressed(sc_Home))
                {
                    movement = 1;

                    KB_ClearKeyDown(sc_Home);

                    S_PlaySound(KICK_HIT);

                    menu->currentGroup = 0;
                    menu->grouplist[menu->currentGroup]->currentEntry = 0;

                    currgroup = menu->grouplist[menu->currentGroup];
                    currentry = currgroup->entrylist[currgroup->currentEntry];

                    M_MenuEntryFocus(currgroup/*, currentry*/);
                }
                else if (KB_KeyPressed(sc_End))
                {
                    movement = 1;

                    KB_ClearKeyDown(sc_End);

                    S_PlaySound(KICK_HIT);

                    menu->currentGroup = menu->numGroups-1;
                    menu->grouplist[menu->currentGroup]->currentEntry = menu->grouplist[menu->currentGroup]->numEntries-1;

                    currgroup = menu->grouplist[menu->currentGroup];
                    currentry = currgroup->entrylist[currgroup->currentEntry];

                    M_MenuEntryFocus(currgroup/*, currentry*/);
                }
                else if (KB_KeyPressed(sc_UpArrow) || KB_KeyPressed(sc_kpad_8) || (MOUSE_GetButtons()&WHEELUP_MOUSE) || BUTTON(gamefunc_Move_Forward) || (JOYSTICK_GetHat(0)&HAT_UP))
                {
                    movement = 1;

                    KB_ClearKeyDown(sc_UpArrow);
                    KB_ClearKeyDown(sc_kpad_8);
                    MOUSE_ClearButton(WHEELUP_MOUSE);
                    CONTROL_ClearButton(gamefunc_Move_Forward);
                    JOYSTICK_ClearHat(0);

                    S_PlaySound(KICK_HIT);

                    --currgroup->currentEntry;
                    if (currgroup->currentEntry < 0)
                    {
                        currgroup->currentEntry = 0;
                        --menu->currentGroup;

                        if (menu->currentGroup < 0)
                            menu->currentGroup = menu->numGroups-1;

                        currgroup = menu->grouplist[menu->currentGroup];
                        currgroup->currentEntry = currgroup->numEntries-1;
                    }
                    currentry = currgroup->entrylist[currgroup->currentEntry];

                    M_MenuEntryFocus(currgroup/*, currentry*/);
                }
                else if (KB_KeyPressed(sc_DownArrow) || KB_KeyPressed(sc_kpad_2) || (MOUSE_GetButtons()&WHEELDOWN_MOUSE) || BUTTON(gamefunc_Move_Backward) || (JOYSTICK_GetHat(0)&HAT_DOWN))
                {
                    movement = 1;

                    KB_ClearKeyDown(sc_DownArrow);
                    KB_ClearKeyDown(sc_kpad_2);
                    KB_ClearKeyDown(sc_PgDn);
                    MOUSE_ClearButton(WHEELDOWN_MOUSE);
                    CONTROL_ClearButton(gamefunc_Move_Backward);
                    JOYSTICK_ClearHat(0);

                    S_PlaySound(KICK_HIT);

                    ++currgroup->currentEntry;
                    if (currgroup->currentEntry >= currgroup->numEntries)
                    {
                        currgroup->currentEntry = currgroup->numEntries-1;
                        ++menu->currentGroup;

                        if (menu->currentGroup >= menu->numGroups)
                            menu->currentGroup = 0;

                        currgroup = menu->grouplist[menu->currentGroup];
                        currgroup->currentEntry = 0;
                    }
                    currentry = currgroup->entrylist[currgroup->currentEntry];

                    M_MenuEntryFocus(currgroup/*, currentry*/);
                }

                if (movement)
                {
                    if (currentry->ybottom - menu->scrollPos > menu->bottomcutoff)
                        menu->scrollPos = currentry->ybottom - menu->bottomcutoff;
                    else if (currentry->ytop - menu->scrollPos < menu->ytop)
                        menu->scrollPos = currentry->ytop - menu->ytop;
                }

                if (currentry != NULL && !currentry->disabled)
                    M_PreMenuInput(currgroup, currentry);
            }
            else if (state == 1)
            {
                if (currentry->type == String)
                {
                    MenuString_t *object = (MenuString_t*)currentry->entry;

                    int32_t hitstate = I_EnterText(object->editfield, object->maxlength, object->flags);

                    if (hitstate == 1)
                    {
                        S_PlaySound(PISTOL_BODYHIT);

                        M_MenuEntryStringSubmit(/*currgroup, currentry, object->editfield*/);

                        Bstrncpy(object->variable, object->editfield, object->maxlength);

                        object->editfield = NULL;
                    }
                    else if (hitstate == -1)
                    {
                        S_PlaySound(EXITMENUSOUND);

                        M_MenuEntryStringCancel(/*currgroup, currentry*/);

                        object->editfield = NULL;
                    }
                }
            }
            else if (state == 2)
            {
                if (currentry->type == Option)
                {
                    MenuOption_t *object = (MenuOption_t*)currentry->entry;
                    int32_t movement = 0;

                    if (I_ReturnTrigger())
                    {
                        I_ReturnTriggerClear();

                        S_PlaySound(EXITMENUSOUND);

                        object->options->currentEntry = -1;
                    }
                    else if (I_AdvanceTrigger())
                    {
                        int32_t temp = (object->options->optionValues == NULL) ? object->options->currentEntry : object->options->optionValues[object->options->currentEntry];

                        I_AdvanceTriggerClear();

                        S_PlaySound(PISTOL_BODYHIT);

                        if (!M_MenuEntryOptionModify(currgroup, currentry, temp))
                        {
                            object->currentOption = object->options->currentEntry;
                            if ((int32_t*)object->data != NULL)
                                *((int32_t*)object->data) = temp;
                        }

                        object->options->currentEntry = -1;
                    }
                    else if (KB_KeyPressed(sc_Home))
                    {
                        movement = 1;

                        KB_ClearKeyDown(sc_Home);

                        S_PlaySound(KICK_HIT);

                        object->options->currentEntry = 0;
                    }
                    else if (KB_KeyPressed(sc_End))
                    {
                        movement = 1;

                        KB_ClearKeyDown(sc_End);

                        S_PlaySound(KICK_HIT);

                        object->options->currentEntry = object->options->numOptions-1;
                    }
                    else if (KB_KeyPressed(sc_UpArrow) || KB_KeyPressed(sc_kpad_8) || (MOUSE_GetButtons()&WHEELUP_MOUSE) || BUTTON(gamefunc_Move_Forward) || (JOYSTICK_GetHat(0)&HAT_UP))
                    {
                        movement = 1;

                        KB_ClearKeyDown(sc_UpArrow);
                        KB_ClearKeyDown(sc_kpad_8);
                        MOUSE_ClearButton(WHEELUP_MOUSE);
                        CONTROL_ClearButton(gamefunc_Move_Forward);
                        JOYSTICK_ClearHat(0);

                        S_PlaySound(KICK_HIT);

                        --object->options->currentEntry;

                        if (object->options->currentEntry < 0)
                            object->options->currentEntry = object->options->numOptions-1;
                    }
                    else if (KB_KeyPressed(sc_DownArrow) || KB_KeyPressed(sc_kpad_2) || (MOUSE_GetButtons()&WHEELDOWN_MOUSE) || BUTTON(gamefunc_Move_Backward) || (JOYSTICK_GetHat(0)&HAT_DOWN))
                    {
                        movement = 1;

                        KB_ClearKeyDown(sc_DownArrow);
                        KB_ClearKeyDown(sc_kpad_2);
                        KB_ClearKeyDown(sc_PgDn);
                        MOUSE_ClearButton(WHEELDOWN_MOUSE);
                        CONTROL_ClearButton(gamefunc_Move_Backward);
                        JOYSTICK_ClearHat(0);

                        S_PlaySound(KICK_HIT);

                        ++object->options->currentEntry;

                        if (object->options->currentEntry >= object->options->numOptions)
                            object->options->currentEntry = 0;
                    }

                    if (movement)
                    {
                        int32_t listytop = object->options->list->pos.y;
                        // assumes height == font->yline!
                        int32_t ytop = listytop + object->options->currentEntry * (object->font->yline + object->options->list->entryspacing);
                        int32_t ybottom = ytop + object->font->yline;

                        if (ybottom - object->options->scrollPos > object->options->list->bottomcutoff)
                            object->options->scrollPos = ybottom - object->options->list->bottomcutoff;
                        else if (ytop - object->options->scrollPos < listytop)
                            object->options->scrollPos = ytop - listytop;
                    }
                }
                else if (currentry->type == Custom2Col)
                {
                    if (I_EscapeTrigger())
                    {
                        I_EscapeTriggerClear();

                        S_PlaySound(EXITMENUSOUND);

                        ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 0;
                    }
                    else if (M_PreMenuCustom2ColScreen(currgroup, currentry))
                        ((MenuCustom2Col_t*)currentry->entry)->screenOpen = 0;
                }
            }

            break;
        }
    }
}

// This function MUST NOT RECURSE. That is why M_RunMenu is separate.
void M_DisplayMenus(void)
{
    Net_GetPackets();

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
    {
        walock[TILE_LOADSHOT] = 1;
        return;
    }
    if (G_HaveEvent(EVENT_DISPLAYMENU))
        VM_OnEvent(EVENT_DISPLAYMENU, g_player[screenpeek].ps->i, screenpeek, -1, 0);

    g_player[myconnectindex].ps->gm &= (0xff-MODE_TYPE);
    g_player[myconnectindex].ps->fta = 0;

    if (((g_player[myconnectindex].ps->gm&MODE_GAME) || ud.recstat==2) && M_BlackTranslucentBackgroundOK(g_currentMenu))
        fade_screen_black(1);

    if (M_UpdateScreenOK(g_currentMenu))
        G_UpdateScreenArea();

    if (!M_IsTextInput(m_currentMenu) && KB_KeyPressed(sc_Q))
    {
        g_previousMenu = g_currentMenu;
        M_ChangeMenu(MENU_QUIT);
    }


    M_RunMenu(m_currentMenu);
    M_RunMenuInput(m_currentMenu);


    if (G_HaveEvent(EVENT_DISPLAYMENUREST))
        VM_OnEvent(EVENT_DISPLAYMENUREST, g_player[screenpeek].ps->i, screenpeek, -1, 0);

    if ((g_player[myconnectindex].ps->gm&MODE_MENU) != MODE_MENU)
    {
        G_UpdateScreenArea();
        CAMERACLOCK = totalclock;
        CAMERADIST = 65536;
    }
}
