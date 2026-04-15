// game_stubs.c — Stubs for game systems not needed by the renderer
// Covers: g_game, d_main, menus, HUD, status bar, automap,
//         intermission, finale, wipe, save/load

#include "doomdef.h"
#include "doomtype.h"
#include "doomstat.h"
#include "d_event.h"
#include "d_player.h"

// Global variables originally defined in g_game.c
gamestate_t  gamestate;
skill_t      gameskill;
boolean      respawnmonsters;
int          gameepisode;
int          gamemap;
boolean      deathmatch;
boolean      netgame;
int          consoleplayer;
int          displayplayer;
int          gametic;
int          totalkills, totalitems, totalsecret;
boolean      playeringame[MAXPLAYERS];
player_t     players[MAXPLAYERS];
wbstartstruct_t wminfo;

// Globals originally defined in d_main.c
boolean      devparm;
boolean      nomonsters;
boolean      respawnparm;
boolean      fastparm;

// autostart/singletics needed by various modules
boolean      autostart;
boolean      singletics;
skill_t      startskill;
int          startepisode;
int          startmap;

// Input globals from g_game.c / m_misc.c
int key_right = 0xae;
int key_left = 0xac;
int key_up = 0xad;
int key_down = 0xaf;
int key_strafeleft = ',';
int key_straferight = '.';
int key_fire = 0x9d;
int key_use = ' ';
int key_strafe = 0xb8;
int key_speed = 0xb6;
int mousebfire = 0;
int mousebstrafe = 1;
int mousebforward = 2;
int joybfire = 0;
int joybstrafe = 1;
int joybuse = 3;
int joybspeed = 2;
int numChannels = 3;
char* chat_macros[10] = { "", "", "", "", "", "", "", "", "", "" };

// m_misc.h function stubs
#include <stdio.h>
int M_WriteFile(char* name, void* source, int length)
{
    (void)name; (void)source; (void)length;
    return 0;
}
int M_ReadFile(char* name, byte** buffer)
{
    (void)name; (void)buffer;
    return 0;
}
void M_ScreenShot(void) {}
void M_SaveDefaults(void) {}
void M_LoadDefaults(void) {}

// Missing globals from g_game.c / d_main.c
boolean      automapactive = false;
boolean      menuactive = false;
boolean      paused = false;
boolean      demoplayback = false;
boolean      precache = true;
int          bodyqueslot;
int          bodyque[32];  // BODYQUESIZE

void G_PlayerReborn(int player)
{
    // Reset player state for respawn — minimal version
    player_t* p = &players[player];
    int killcount = p->killcount;
    int itemcount = p->itemcount;
    int secretcount = p->secretcount;
    memset(p, 0, sizeof(*p));
    p->killcount = killcount;
    p->itemcount = itemcount;
    p->secretcount = secretcount;
    p->playerstate = PST_LIVE;
    p->health = 100;
    p->readyweapon = wp_pistol;
    p->pendingweapon = wp_pistol;
    p->weaponowned[wp_fist] = true;
    p->weaponowned[wp_pistol] = true;
    p->ammo[am_clip] = 50;
    p->maxammo[am_clip] = 200;
    p->maxammo[am_shell] = 50;
    p->maxammo[am_cell] = 300;
    p->maxammo[am_misl] = 50;
}

// g_game.h stubs
void G_ExitLevel(void) {}
void G_SecretExitLevel(void) {}
void G_DeathMatchSpawnPlayer(int playernum) { (void)playernum; }
void G_InitNew(skill_t skill, int episode, int map)
{
    (void)skill; (void)episode; (void)map;
}
void G_DeferedInitNew(skill_t skill, int episode, int map)
{
    (void)skill; (void)episode; (void)map;
}
void G_DeferedPlayDemo(char* demo) { (void)demo; }
void G_LoadGame(char* name) { (void)name; }
void G_DoLoadGame(void) {}
void G_SaveGame(int slot, char* description) { (void)slot; (void)description; }
void G_RecordDemo(char* name) { (void)name; }
void G_BeginRecording(void) {}
void G_PlayDemo(char* name) { (void)name; }
void G_TimeDemo(char* name) { (void)name; }
boolean G_CheckDemoStatus(void) { return false; }
void G_WorldDone(void) {}
void G_Ticker(void) {}
boolean G_Responder(event_t* ev) { (void)ev; return false; }
void G_ScreenShot(void) {}

// d_main.h stubs
void D_AdvanceDemo(void) {}
void D_PageTicker(void) {}
void D_PageDrawer(void) {}
void D_StartTitle(void) {}

// m_menu.h stubs
boolean M_Responder(event_t* ev) { (void)ev; return false; }
void M_Ticker(void) {}
void M_Drawer(void) {}
void M_Init(void) {}
void M_StartControlPanel(void) {}

// hu_stuff.h stubs
void HU_Init(void) {}
void HU_Start(void) {}
boolean HU_Responder(event_t* ev) { (void)ev; return false; }
void HU_Ticker(void) {}
void HU_Drawer(void) {}
char HU_dequeueChatChar(void) { return 0; }
void HU_Erase(void) {}

// st_stuff.h stubs
boolean ST_Responder(event_t* ev) { (void)ev; return false; }
void ST_Ticker(void) {}
void ST_Drawer(boolean fullscreen, boolean refresh)
{
    (void)fullscreen; (void)refresh;
}
void ST_Start(void) {}
void ST_Init(void) {}

// am_map.h stubs
boolean AM_Responder(event_t* ev) { (void)ev; return false; }
void AM_Ticker(void) {}
void AM_Drawer(void) {}
void AM_Stop(void) {}

// wi_stuff.h stubs
void WI_Ticker(void) {}
void WI_Drawer(void) {}
void WI_Start(wbstartstruct_t* wbstartstruct) { (void)wbstartstruct; }

// f_finale.h stubs
boolean F_Responder(event_t* ev) { (void)ev; return false; }
void F_Ticker(void) {}
void F_Drawer(void) {}
void F_StartFinale(void) {}

// f_wipe.h stubs
int wipe_StartScreen(int x, int y, int width, int height)
{
    (void)x; (void)y; (void)width; (void)height; return 0;
}
int wipe_EndScreen(int x, int y, int width, int height)
{
    (void)x; (void)y; (void)width; (void)height; return 0;
}
int wipe_ScreenWipe(int wipeno, int x, int y, int width, int height, int ticks)
{
    (void)wipeno; (void)x; (void)y; (void)width; (void)height; (void)ticks;
    return 0;
}

// p_saveg.h stubs
void P_ArchivePlayers(void) {}
void P_UnArchivePlayers(void) {}
void P_ArchiveWorld(void) {}
void P_UnArchiveWorld(void) {}
void P_ArchiveThinkers(void) {}
void P_UnArchiveThinkers(void) {}
void P_ArchiveSpecials(void) {}
void P_UnArchiveSpecials(void) {}
