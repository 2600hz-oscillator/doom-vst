// d_net_stub.c — Empty networking stubs for DoomViz renderer library

#include "doomdef.h"
#include "doomtype.h"
#include "d_net.h"
#include "i_net.h"
#include "doomstat.h"

// d_net stubs
void NetUpdate(void) {}
void D_QuitNetGame(void) {}
void TryRunTics(void) {}
void D_CheckNetGame(void)
{
    // Set up single-player defaults
    consoleplayer = 0;
    displayplayer = 0;
    playeringame[0] = true;
    for (int i = 1; i < MAXPLAYERS; i++)
        playeringame[i] = false;
}

// i_net stubs
void I_InitNetwork(void) {}
void I_NetCmd(void) {}
