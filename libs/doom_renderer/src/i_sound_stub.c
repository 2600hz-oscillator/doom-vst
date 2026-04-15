// i_sound_stub.c — Empty sound stubs for DoomViz renderer library

#include "doomdef.h"
#include "doomtype.h"
#include "sounds.h"
#include "i_sound.h"
#include "s_sound.h"
#include "w_wad.h"

// Globals from i_sound/m_misc
FILE* sndserver = NULL;
char* sndserver_filename = "sndserver";
int mb_used = 2;

// I_Sound stubs
void I_InitSound(void) {}
void I_UpdateSound(void) {}
void I_SubmitSound(void) {}
void I_ShutdownSound(void) {}
void I_SetChannels(void) {}
int I_GetSfxLumpNum(sfxinfo_t* sfxinfo)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfxinfo->name);
    return W_CheckNumForName(namebuf);
}
int I_StartSound(int id, int vol, int sep, int pitch, int priority)
{
    (void)id; (void)vol; (void)sep; (void)pitch; (void)priority;
    return 0;
}
void I_StopSound(int handle) { (void)handle; }
int I_SoundIsPlaying(int handle) { (void)handle; return 0; }
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
    (void)handle; (void)vol; (void)sep; (void)pitch;
}

// I_Music stubs
void I_InitMusic(void) {}
void I_ShutdownMusic(void) {}
void I_SetMusicVolume(int volume) { (void)volume; }
void I_PauseSong(int handle) { (void)handle; }
void I_ResumeSong(int handle) { (void)handle; }
int I_RegisterSong(void* data) { (void)data; return 0; }
void I_PlaySong(int handle, int looping) { (void)handle; (void)looping; }
void I_StopSong(int handle) { (void)handle; }
void I_UnRegisterSong(int handle) { (void)handle; }

// S_Sound stubs
void S_Init(int sfxVolume, int musicVolume) { (void)sfxVolume; (void)musicVolume; }
void S_Start(void) {}
void S_StartSound(void* origin, int sound_id) { (void)origin; (void)sound_id; }
void S_StartSoundAtVolume(void* origin, int sound_id, int volume)
{
    (void)origin; (void)sound_id; (void)volume;
}
void S_StopSound(void* origin) { (void)origin; }
void S_StartMusic(int music_id) { (void)music_id; }
void S_ChangeMusic(int music_id, int looping) { (void)music_id; (void)looping; }
void S_StopMusic(void) {}
void S_PauseSound(void) {}
void S_ResumeSound(void) {}
void S_UpdateSounds(void* listener) { (void)listener; }
void S_SetMusicVolume(int volume) { (void)volume; }
void S_SetSfxVolume(int volume) { (void)volume; }
