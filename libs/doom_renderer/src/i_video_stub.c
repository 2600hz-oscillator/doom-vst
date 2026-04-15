// i_video_stub.c — Replaces X11 i_video.c with in-memory framebuffer
// DoomViz: renders to a byte buffer instead of an X11 window

#include "doomdef.h"
#include "doomtype.h"
#include "i_video.h"
#include "v_video.h"
#include "d_event.h"

#include <stdlib.h>
#include <string.h>

// The palette set by the engine — we expose this for RGBA conversion
static unsigned char dviz_palette[768]; // 256 * 3 (RGB)
static int dviz_palette_changed = 0;

// Accessor for the public API
unsigned char* dviz_get_palette(void) { return dviz_palette; }
int dviz_palette_was_changed(void) {
    int c = dviz_palette_changed;
    dviz_palette_changed = 0;
    return c;
}

void I_InitGraphics(void)
{
    // Allocate screen buffers — screens[0] is the primary framebuffer
    // v_video.c V_Init allocates these, but we need screens[0] at minimum
}

void I_ShutdownGraphics(void)
{
}

void I_SetPalette(byte* palette)
{
    memcpy(dviz_palette, palette, 768);
    dviz_palette_changed = 1;
}

void I_UpdateNoBlit(void)
{
}

void I_FinishUpdate(void)
{
    // No-op: the caller reads screens[0] directly
}

void I_WaitVBL(int count)
{
    (void)count;
}

void I_ReadScreen(byte* scr)
{
    memcpy(scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

// Input stubs — no keyboard/mouse in visualizer mode
void I_StartFrame(void)
{
}

void I_StartTic(void)
{
}
