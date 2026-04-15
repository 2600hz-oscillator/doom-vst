// i_system_stub.c — Minimal system interface for DoomViz renderer library

#include "doomdef.h"
#include "doomtype.h"
#include "d_ticcmd.h"
#include "i_system.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

// Controllable tick counter — the VST drives this
static int dviz_tick_count = 0;

void dviz_set_tick(int tick)
{
    dviz_tick_count = tick;
}

void dviz_advance_tick(void)
{
    dviz_tick_count++;
}

int I_GetTime(void)
{
    return dviz_tick_count;
}

void I_Init(void)
{
}

byte* I_ZoneBase(int* size)
{
    // Allocate 32MB for the zone heap
    *size = 32 * 1024 * 1024;
    return (byte*)malloc(*size);
}

byte* I_AllocLow(int length)
{
    return (byte*)calloc(1, length);
}

static ticcmd_t emptycmd;

ticcmd_t* I_BaseTiccmd(void)
{
    return &emptycmd;
}

void I_Quit(void)
{
    exit(0);
}

void I_Error(char* error, ...)
{
    va_list argptr;
    fprintf(stderr, "DoomViz I_Error: ");
    va_start(argptr, error);
    vfprintf(stderr, error, argptr);
    va_end(argptr);
    fprintf(stderr, "\n");
    // Don't call exit() — let the VST host handle this gracefully
    // For the standalone test harness this is fine
}

void I_Tactile(int on, int off, int total)
{
    (void)on; (void)off; (void)total;
}
