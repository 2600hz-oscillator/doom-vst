// doom_renderer.c — Implementation of the DoomViz public C API
//
// Wraps the stripped linuxdoom-1.10 renderer into a clean interface.
// NOT thread-safe — call all functions from a single thread.

#include "doom_renderer.h"

// Doom headers
#include "doomdef.h"
#include "doomtype.h"
#include "doomstat.h"
#include "d_player.h"
#include "r_local.h"
#include "r_main.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_things.h"
#include "p_setup.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "p_local.h"
#include "w_wad.h"
#include "z_zone.h"
#include "v_video.h"
#include "i_video.h"
#include "tables.h"
#include "info.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_swap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External functions defined in our stubs
extern void D_CheckNetGame(void);
extern void dviz_set_tick(int tick);
extern void dviz_advance_tick(void);
extern unsigned char* dviz_get_palette(void);
extern int dviz_palette_was_changed(void);

// Globals needed by the Doom engine
int screenblocks = 10;   // Full screen rendering (no status bar)
int detailLevel = 0;     // Normal detail

// m_misc.c needs these
int usemouse = 0;
int usejoystick = 0;

// Extern globals that the renderer references
extern int numvertexes;
extern int numsegs;
extern int numsectors;
extern int numsubsectors;
extern int numnodes;
extern int numlines;
extern int numsides;

// Track spawned things for handle management
#define MAX_TRACKED_THINGS 256
static mobj_t* tracked_things[MAX_TRACKED_THINGS];
static int num_tracked = 0;

// Static frame data
static doom_frame_t frame_data;
static int initialized = 0;
static int map_loaded = 0;

// Fake argv for m_argv
static char* fake_argv[] = { "doomviz", NULL };
static int fake_argc = 1;

int doom_init(const char* wad_path)
{
    if (initialized)
        return 0;

    // Set up fake command line (m_argv uses this)
    myargc = fake_argc;
    myargv = fake_argv;

    // Initialize zone memory
    Z_Init();

    // Set game mode to shareware
    gamemode = shareware;
    gamemission = doom;
    language = english;

    // No monsters mode off, normal gameplay
    nomonsters = false;
    respawnparm = false;
    fastparm = false;
    devparm = false;

    // Load the WAD file
    char* wad_files[2];
    wad_files[0] = (char*)wad_path;
    wad_files[1] = NULL;
    W_InitMultipleFiles(wad_files);

    // Initialize video buffers
    V_Init();

    // Initialize renderer (loads textures, colormaps, sprites, etc.)
    R_Init();

    // Initialize game data (switch textures, animated flats, sprites)
    P_Init();

    // Set up rendering viewport (full screen, normal detail)
    R_SetViewSize(screenblocks, detailLevel);
    R_ExecuteSetViewSize();

    // Set up networking defaults (single player)
    D_CheckNetGame();

    // Initialize player
    memset(&players[0], 0, sizeof(player_t));
    players[0].playerstate = PST_LIVE;
    players[0].health = 100;
    players[0].readyweapon = wp_pistol;
    players[0].weaponowned[wp_fist] = true;
    players[0].weaponowned[wp_pistol] = true;
    players[0].ammo[am_clip] = 9999;
    players[0].ammo[am_shell] = 9999;
    players[0].ammo[am_cell] = 9999;
    players[0].ammo[am_misl] = 9999;
    players[0].maxammo[am_clip] = 9999;
    players[0].maxammo[am_shell] = 9999;
    players[0].maxammo[am_cell] = 9999;
    players[0].maxammo[am_misl] = 50;

    // Load the initial palette from PLAYPAL
    {
        byte* playpal = W_CacheLumpName("PLAYPAL", PU_CACHE);
        I_SetPalette(playpal);
    }

    // Set up frame data
    frame_data.width = SCREENWIDTH;
    frame_data.height = SCREENHEIGHT;
    frame_data.framebuffer = screens[0];
    // Copy initial palette from the just-loaded PLAYPAL
    memcpy(frame_data.palette, dviz_get_palette(), 768);
    frame_data.palette_changed = 1;

    // Reset thing tracking
    memset(tracked_things, 0, sizeof(tracked_things));
    num_tracked = 0;

    initialized = 1;
    return 0;
}

void doom_shutdown(void)
{
    if (!initialized)
        return;

    // Zone memory handles cleanup
    initialized = 0;
    map_loaded = 0;
}

int doom_load_map(int episode, int map)
{
    if (!initialized)
        return -1;

    // Set game state for map loading
    gameepisode = episode;
    gamemap = map;
    gameskill = sk_medium;
    gamestate = GS_LEVEL;

    // Load the map
    P_SetupLevel(episode, map, 0, sk_medium);

    // The player mobj should have been spawned by P_LoadThings
    if (players[0].mo == NULL) {
        fprintf(stderr, "doom_load_map: player mobj not spawned\n");
        return -1;
    }

    // Set default view height
    players[0].viewz = players[0].mo->z + 41 * FRACUNIT;
    players[0].viewheight = 41 * FRACUNIT;
    players[0].deltaviewheight = 0;

    // Reset thing tracking
    memset(tracked_things, 0, sizeof(tracked_things));
    num_tracked = 0;

    map_loaded = 1;
    return 0;
}

doom_map_info_t doom_get_map_info(void)
{
    doom_map_info_t info;
    info.num_sectors = numsectors;
    info.num_things = num_tracked;
    info.num_lines = numlines;
    return info;
}

void doom_set_camera(int32_t x, int32_t y, int32_t z, uint32_t angle)
{
    if (!map_loaded || !players[0].mo)
        return;

    players[0].mo->x = x;
    players[0].mo->y = y;
    players[0].mo->z = z - 41 * FRACUNIT; // viewz = mo->z + viewheight
    players[0].mo->angle = angle;
    players[0].viewz = z;

    // Update subsector link
    P_UnsetThingPosition(players[0].mo);
    P_SetThingPosition(players[0].mo);
}

int doom_spawn_thing(int32_t x, int32_t y, int type_id)
{
    if (!map_loaded)
        return -1;

    if (type_id < 0 || type_id >= NUMMOBJTYPES)
        return -1;

    // Find a free handle
    int handle = -1;
    for (int i = 0; i < MAX_TRACKED_THINGS; i++) {
        if (tracked_things[i] == NULL) {
            handle = i;
            break;
        }
    }
    if (handle < 0)
        return -1;

    mobj_t* mo = P_SpawnMobj(x, y, ONFLOORZ, (mobjtype_t)type_id);
    if (mo == NULL)
        return -1;

    // Remove MF_SOLID so spawned things don't block player movement.
    // They're visual props for the visualizer, not gameplay obstacles.
    mo->flags &= ~MF_SOLID;

    tracked_things[handle] = mo;
    if (handle >= num_tracked)
        num_tracked = handle + 1;

    return handle;
}

void doom_remove_thing(int handle)
{
    if (handle < 0 || handle >= MAX_TRACKED_THINGS)
        return;
    if (tracked_things[handle] == NULL)
        return;

    P_RemoveMobj(tracked_things[handle]);
    tracked_things[handle] = NULL;
}

void doom_set_thing_state(int handle, int state_id)
{
    if (handle < 0 || handle >= MAX_TRACKED_THINGS)
        return;
    if (tracked_things[handle] == NULL)
        return;
    if (state_id < 0 || state_id >= NUMSTATES)
        return;

    P_SetMobjState(tracked_things[handle], (statenum_t)state_id);
}

void doom_set_thing_position(int handle, int32_t x, int32_t y)
{
    if (handle < 0 || handle >= MAX_TRACKED_THINGS)
        return;
    mobj_t* mo = tracked_things[handle];
    if (mo == NULL)
        return;

    P_UnsetThingPosition(mo);
    mo->x = x;
    mo->y = y;
    P_SetThingPosition(mo);
}

void doom_set_thing_angle(int handle, uint32_t angle)
{
    if (handle < 0 || handle >= MAX_TRACKED_THINGS)
        return;
    if (tracked_things[handle] == NULL)
        return;

    tracked_things[handle]->angle = angle;
}

void doom_get_player_pos(int32_t* x, int32_t* y, int32_t* z, uint32_t* angle)
{
    if (!map_loaded || !players[0].mo) {
        if (x) *x = 0;
        if (y) *y = 0;
        if (z) *z = 0;
        if (angle) *angle = 0;
        return;
    }
    if (x) *x = players[0].mo->x;
    if (y) *y = players[0].mo->y;
    if (z) *z = players[0].viewz;
    if (angle) *angle = players[0].mo->angle;
}

void doom_set_sector_light(int sector_id, int level)
{
    if (!map_loaded)
        return;
    if (sector_id < 0 || sector_id >= numsectors)
        return;
    if (level < 0) level = 0;
    if (level > 255) level = 255;

    sectors[sector_id].lightlevel = level;
}

int doom_get_sector_light(int sector_id)
{
    if (!map_loaded)
        return 0;
    if (sector_id < 0 || sector_id >= numsectors)
        return 0;

    return sectors[sector_id].lightlevel;
}

void doom_set_sector_floor_height(int sector_id, int32_t height)
{
    if (!map_loaded)
        return;
    if (sector_id < 0 || sector_id >= numsectors)
        return;

    sectors[sector_id].floorheight = height;
}

void doom_set_sector_ceiling_height(int sector_id, int32_t height)
{
    if (!map_loaded)
        return;
    if (sector_id < 0 || sector_id >= numsectors)
        return;

    sectors[sector_id].ceilingheight = height;
}

int doom_sector_is_outdoor(int sector_id)
{
    if (!map_loaded)
        return 0;
    if (sector_id < 0 || sector_id >= numsectors)
        return 0;

    // In Doom, outdoor sectors use F_SKY1 as ceiling texture
    // The sky flat number is stored in skyflatnum
    extern int skyflatnum;
    return sectors[sector_id].ceilingpic == skyflatnum;
}

void doom_fire_weapon(void)
{
    if (!map_loaded || !players[0].mo)
        return;

    extern void P_FireWeapon(player_t* player);
    P_FireWeapon(&players[0]);
}

int doom_get_thing_position(int handle, int32_t* x, int32_t* y)
{
    if (handle < 0 || handle >= MAX_TRACKED_THINGS)
        return 0;
    if (tracked_things[handle] == NULL)
        return 0;

    if (x) *x = tracked_things[handle]->x;
    if (y) *y = tracked_things[handle]->y;
    return 1;
}

int doom_move_player(int32_t dx, int32_t dy)
{
    if (!map_loaded || !players[0].mo)
        return 0;

    mobj_t* mo = players[0].mo;
    fixed_t newx = mo->x + dx;
    fixed_t newy = mo->y + dy;

    // Use Doom's collision-checked movement
    extern boolean P_TryMove(mobj_t* thing, fixed_t x, fixed_t y);
    if (P_TryMove(mo, newx, newy))
    {
        // Update viewz after successful move
        players[0].viewz = mo->z + players[0].viewheight;
        return 1;
    }
    return 0;
}

void doom_set_god_mode(int enabled)
{
    if (!map_loaded)
        return;

    if (enabled)
        players[0].cheats |= CF_GODMODE;
    else
        players[0].cheats &= ~CF_GODMODE;
}

int doom_is_player_dead(void)
{
    if (!map_loaded || !players[0].mo)
        return 0;

    return players[0].playerstate == PST_DEAD;
}

void doom_respawn_player(void)
{
    if (!map_loaded)
        return;

    // Reset player state
    players[0].playerstate = PST_LIVE;
    players[0].health = 100;
    players[0].mo->health = 100;

    // Reset view height (gets messed up on death)
    players[0].viewheight = 41 * FRACUNIT;
    players[0].deltaviewheight = 0;
    players[0].viewz = players[0].mo->z + 41 * FRACUNIT;
}

void doom_tick(void)
{
    if (!map_loaded)
        return;

    dviz_advance_tick();
    P_Ticker();
}

doom_frame_t* doom_render_frame(void)
{
    if (!map_loaded)
        return NULL;

    // Render the player's view
    R_RenderPlayerView(&players[0]);

    // Update frame data
    frame_data.framebuffer = screens[0];

    // Copy palette if changed
    unsigned char* pal = dviz_get_palette();
    if (dviz_palette_was_changed()) {
        memcpy(frame_data.palette, pal, 768);
        frame_data.palette_changed = 1;
    } else {
        frame_data.palette_changed = 0;
    }

    return &frame_data;
}

doom_sprite_t doom_get_sprite(int sprite_id, int frame, int rotation)
{
    doom_sprite_t result;
    memset(&result, 0, sizeof(result));

    if (!initialized)
        return result;
    if (sprite_id < 0 || sprite_id >= NUMSPRITES)
        return result;

    spritedef_t* sprdef = &sprites[sprite_id];
    if (frame < 0 || frame >= sprdef->numframes)
        return result;

    spriteframe_t* sprframe = &sprdef->spriteframes[frame];

    // Get the lump for this rotation
    int lump;
    if (sprframe->rotate) {
        if (rotation < 0 || rotation > 7)
            rotation = 0;
        lump = sprframe->lump[rotation];
    } else {
        lump = sprframe->lump[0];
    }

    // Load the patch
    patch_t* patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);
    if (patch == NULL)
        return result;

    result.patch_data = (const uint8_t*)patch;
    result.width = SHORT(patch->width);
    result.height = SHORT(patch->height);
    result.left_offset = SHORT(patch->leftoffset);
    result.top_offset = SHORT(patch->topoffset);

    return result;
}

void doom_set_flat_data(const char* flat_name, const uint8_t* pixels)
{
    if (!initialized || flat_name == NULL || pixels == NULL)
        return;

    int lump = W_CheckNumForName(flat_name);
    if (lump < 0)
        return;

    byte* flat = W_CacheLumpNum(lump, PU_STATIC);
    if (flat != NULL) {
        memcpy(flat, pixels, 64 * 64);
    }
}

// Externs from r_data.c
extern byte**   texturecomposite;
extern int*     texturecompositesize;
extern short**  texturecolumnlump;
extern unsigned short** texturecolumnofs;
extern int      numtextures;
extern int      R_CheckTextureNumForName(char* name);

// Forward declare texture_t (defined in r_data.c)
typedef struct {
    char  name[8];
    short width;
    short height;
    short patchcount;
    // patches follow but we only need width/height
} dviz_texture_t;
extern dviz_texture_t** textures;

void doom_set_wall_texture_data(const char* tex_name, const uint8_t* pixels,
                                 int* out_width, int* out_height)
{
    if (!initialized || tex_name == NULL)
        return;

    int texnum = R_CheckTextureNumForName((char*)tex_name);
    if (texnum < 0 || texnum >= numtextures)
        return;

    dviz_texture_t* tex = textures[texnum];
    int w = tex->width;
    int h = tex->height;

    if (out_width) *out_width = w;
    if (out_height) *out_height = h;

    // Query-only mode (pixels == NULL)
    if (pixels == NULL)
        return;

    // Ensure composite is allocated
    if (!texturecomposite[texnum])
    {
        // Allocate and set up column offsets for column-major layout
        texturecompositesize[texnum] = w * h;
        texturecomposite[texnum] = Z_Malloc(w * h, PU_STATIC, &texturecomposite[texnum]);
    }

    // Copy column-major pixel data
    memcpy(texturecomposite[texnum], pixels, w * h);

    // Force all columns to use composite (not individual lumps)
    for (int col = 0; col < w; col++)
    {
        texturecolumnlump[texnum][col] = 0;  // 0 = use composite
        texturecolumnofs[texnum][col] = col * h;
    }
}

void doom_give_weapon(int weapon_id)
{
    if (!map_loaded)
        return;
    if (weapon_id < 0 || weapon_id >= NUMWEAPONS)
        return;

    players[0].weaponowned[weapon_id] = true;
    players[0].readyweapon = (weapontype_t)weapon_id;
    players[0].pendingweapon = (weapontype_t)weapon_id;

    // Ensure ammo
    players[0].ammo[am_clip] = 9999;
    players[0].ammo[am_shell] = 9999;
    players[0].ammo[am_cell] = 9999;
    players[0].ammo[am_misl] = 9999;
}
