#include "DoomEngine.h"
#include <cstring>

DoomEngine::DoomEngine()
{
    rgbaBuffer.resize(320 * 200 * 4, 0);
}

DoomEngine::~DoomEngine()
{
    if (initialized)
        doom_shutdown();
}

bool DoomEngine::init(const std::string& wadPath)
{
    if (initialized)
        return true;

    if (doom_init(wadPath.c_str()) != 0)
        return false;

    initialized = true;
    return true;
}

bool DoomEngine::loadMap(int episode, int map)
{
    if (!initialized)
        return false;

    if (doom_load_map(episode, map) != 0)
        return false;

    mapLoaded = true;
    return true;
}

void DoomEngine::tick()
{
    if (mapLoaded)
        doom_tick();
}

const uint8_t* DoomEngine::renderFrame()
{
    if (!mapLoaded)
        return rgbaBuffer.data();

    doom_frame_t* frame = doom_render_frame();
    if (frame == nullptr)
        return rgbaBuffer.data();

    paletteConvert(frame);
    return rgbaBuffer.data();
}

void DoomEngine::paletteConvert(const doom_frame_t* frame)
{
    if (frame->palette_changed)
        std::memcpy(currentPalette, frame->palette, 768);

    const uint8_t* fb = frame->framebuffer;
    const int numPixels = 320 * 200;

    for (int i = 0; i < numPixels; ++i)
    {
        uint8_t idx = fb[i];
        int palOfs = idx * 3;
        int rgbaOfs = i * 4;
        rgbaBuffer[rgbaOfs + 0] = currentPalette[palOfs + 0];
        rgbaBuffer[rgbaOfs + 1] = currentPalette[palOfs + 1];
        rgbaBuffer[rgbaOfs + 2] = currentPalette[palOfs + 2];
        rgbaBuffer[rgbaOfs + 3] = 255;
    }
}

void DoomEngine::setCamera(int32_t x, int32_t y, int32_t z, uint32_t angle)
{
    doom_set_camera(x, y, z, angle);
}

void DoomEngine::getPlayerPos(int32_t& x, int32_t& y, int32_t& z, uint32_t& angle)
{
    doom_get_player_pos(&x, &y, &z, &angle);
}

int DoomEngine::spawnThing(int32_t x, int32_t y, int typeId)
{
    return doom_spawn_thing(x, y, typeId);
}

void DoomEngine::removeThing(int handle)
{
    doom_remove_thing(handle);
}

void DoomEngine::setThingPosition(int handle, int32_t x, int32_t y)
{
    doom_set_thing_position(handle, x, y);
}

void DoomEngine::setThingAngle(int handle, uint32_t angle)
{
    doom_set_thing_angle(handle, angle);
}

void DoomEngine::setSectorLight(int sectorId, int level)
{
    doom_set_sector_light(sectorId, level);
}

int DoomEngine::getSectorLight(int sectorId)
{
    return doom_get_sector_light(sectorId);
}
