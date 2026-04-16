// doom_renderer.h — Public C API for the DoomViz renderer library
//
// This library wraps the Doom (1994) software renderer, stripped from
// linuxdoom-1.10, into a clean API for embedding in other applications.
// The renderer is NOT thread-safe — call all functions from a single thread.

#ifndef DOOM_RENDERER_H
#define DOOM_RENDERER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Frame data returned by doom_render_frame
typedef struct {
    uint8_t*  framebuffer;    // 320x200 indexed pixels (8-bit palette indices)
    uint8_t   palette[768];   // 256 entries * 3 bytes (RGB)
    int       width;          // Always 320
    int       height;         // Always 200
    int       palette_changed; // 1 if palette changed since last frame
} doom_frame_t;

// Sprite data for direct sprite access (Scene B)
typedef struct {
    const uint8_t* patch_data;  // Raw patch_t data from WAD
    int       width;
    int       height;
    int       left_offset;
    int       top_offset;
} doom_sprite_t;

// Map info
typedef struct {
    int num_sectors;
    int num_things;
    int num_lines;
} doom_map_info_t;

// --- Lifecycle ---

// Initialize the renderer and load a WAD file.
// Returns 0 on success, -1 on failure.
int doom_init(const char* wad_path);

// Shut down the renderer and free all resources.
void doom_shutdown(void);

// --- Map Loading ---

// Load a map by episode and mission number (e.g., 1,1 for E1M1).
// Returns 0 on success, -1 on failure.
int doom_load_map(int episode, int map);

// Get info about the currently loaded map.
doom_map_info_t doom_get_map_info(void);

// --- Camera / Player ---

// Set the camera position and angle.
// x, y, z are in Doom fixed-point (16.16).
// angle is in Doom BAM (Binary Angle Measurement): 0=East, 0x40000000=North, etc.
void doom_set_camera(int32_t x, int32_t y, int32_t z, uint32_t angle);

// Set only the camera angle (does not move the player or touch the blockmap).
void doom_set_camera_angle(uint32_t angle);

// --- Objects (Things) ---

// Spawn a thing at the given position. type_id is a mobjtype_t value.
// Returns a handle (>= 0) on success, -1 on failure.
int doom_spawn_thing(int32_t x, int32_t y, int type_id);

// Remove a previously spawned thing.
void doom_remove_thing(int handle);

// Set a thing's animation state. state_id is a statenum_t value.
void doom_set_thing_state(int handle, int state_id);

// Move a thing to a new position.
void doom_set_thing_position(int handle, int32_t x, int32_t y);

// Set a thing's facing angle.
void doom_set_thing_angle(int handle, uint32_t angle);

// --- World Manipulation ---

// Set a sector's light level (0-255).
void doom_set_sector_light(int sector_id, int level);

// Get a sector's light level.
int doom_get_sector_light(int sector_id);

// Set a sector's floor height (fixed-point 16.16).
void doom_set_sector_floor_height(int sector_id, int32_t height);

// Set a sector's ceiling height (fixed-point 16.16).
void doom_set_sector_ceiling_height(int sector_id, int32_t height);

// Check if a sector has sky ceiling (outdoor area detection).
int doom_sector_is_outdoor(int sector_id);

// --- Rendering ---

// Advance the game tick (updates animations, thinkers).
void doom_tick(void);

// Render a frame and return the framebuffer data.
// The returned pointer is valid until the next call to doom_render_frame or doom_shutdown.
doom_frame_t* doom_render_frame(void);

// --- Sprite Access (for 2D scene modes) ---

// Get sprite data by sprite number, frame, and rotation.
// sprite_id is a spritenum_t value, frame is 0-based, rotation is 0-7.
// Returns zero-initialized struct if sprite not found.
doom_sprite_t doom_get_sprite(int sprite_id, int frame, int rotation);

// --- Dynamic Textures ---

// Replace a flat (floor/ceiling texture) with custom pixel data.
// pixels must be 64x64 indexed-color (4096 bytes).
// flat_name is the lump name (e.g., "FLAT1").
void doom_set_flat_data(const char* flat_name, const uint8_t* pixels);

// Replace a wall texture's composite data with custom pixel data.
// pixels is column-major indexed-color (width * height bytes).
// tex_name is the texture name (e.g., "STARTAN3").
// Returns the texture dimensions via out params (can be NULL).
void doom_set_wall_texture_data(const char* tex_name, const uint8_t* pixels,
                                 int* out_width, int* out_height);

// Give the player a specific weapon. weapon_id is a weapontype_t value.
void doom_give_weapon(int weapon_id);

// --- Query ---

// Get player spawn position (set after doom_load_map).
void doom_get_player_pos(int32_t* x, int32_t* y, int32_t* z, uint32_t* angle);

// Move the player with collision detection. dx, dy are fixed-point deltas.
// Returns 1 if the move succeeded, 0 if blocked by a wall.
int doom_move_player(int32_t dx, int32_t dy);

// Fire the player's current weapon.
void doom_fire_weapon(void);

// Get the position of a tracked thing. Returns 0 if invalid handle.
int doom_get_thing_position(int handle, int32_t* x, int32_t* y);

// --- Player State ---

// Enable/disable god mode (invulnerability). Call after doom_load_map.
void doom_set_god_mode(int enabled);

// Check if player is dead.
int doom_is_player_dead(void);

// Respawn the player at the start position. Call when player dies.
void doom_respawn_player(void);

// --- Utility ---

// Convert Doom fixed-point to float and back.
#define DOOM_FRACBITS 16
#define DOOM_FRACUNIT (1 << DOOM_FRACBITS)
#define DOOM_TO_FIXED(f) ((int32_t)((f) * DOOM_FRACUNIT))
#define DOOM_FROM_FIXED(x) ((float)(x) / DOOM_FRACUNIT)

// Convert degrees to Doom BAM angle.
#define DOOM_DEG_TO_ANGLE(deg) ((uint32_t)((deg) / 360.0 * 4294967296.0))

#ifdef __cplusplus
}
#endif

#endif // DOOM_RENDERER_H
