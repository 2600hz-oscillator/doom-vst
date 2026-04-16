#pragma once

#include "doom_renderer.h"
#include <cstdint>
#include <string>
#include <vector>

// C++ RAII wrapper around the Doom renderer C API.
// NOT thread-safe — call all methods from a single thread (the render thread).
class DoomEngine
{
public:
    DoomEngine();
    ~DoomEngine();

    // Initialize with path to WAD file. Returns true on success.
    bool init(const std::string& wadPath);

    // Load a map by episode and mission (e.g., 1,1 for E1M1).
    bool loadMap(int episode, int map);

    // Advance one game tick (animations, thinkers).
    void tick();

    // Render a frame. Returns pointer to RGBA pixel buffer (320x200x4).
    // The returned pointer is valid until the next call to renderFrame() or destruction.
    const uint8_t* renderFrame();

    // Camera control
    void setCamera(int32_t x, int32_t y, int32_t z, uint32_t angle);
    void setCameraAngle(uint32_t angle);
    void getPlayerPos(int32_t& x, int32_t& y, int32_t& z, uint32_t& angle);

    // Collision-checked movement. Returns true if the move succeeded.
    bool movePlayer(int32_t dx, int32_t dy);

    // Fire the player's weapon.
    void fireWeapon();

    // Get position of a tracked thing. Returns false if invalid.
    bool getThingPosition(int handle, int32_t& x, int32_t& y);

    // Thing management
    int spawnThing(int32_t x, int32_t y, int typeId);
    void removeThing(int handle);
    void setThingPosition(int handle, int32_t x, int32_t y);
    void setThingAngle(int handle, uint32_t angle);

    // Sector manipulation
    void setSectorLight(int sectorId, int level);
    int getSectorLight(int sectorId);

    // Texture manipulation
    void setWallTextureData(const char* texName, const uint8_t* pixels,
                            int* outWidth = nullptr, int* outHeight = nullptr);

    // Give player a weapon (weapontype_t value)
    void giveWeapon(int weaponId);

    // Player state
    void setGodMode(bool enabled);
    bool isPlayerDead();
    void respawnPlayer();

    // Query
    bool isInitialized() const { return initialized; }
    bool isMapLoaded() const { return mapLoaded; }
    int getWidth() const { return 320; }
    int getHeight() const { return 200; }

private:
    bool initialized = false;
    bool mapLoaded = false;
    std::vector<uint8_t> rgbaBuffer; // 320*200*4 RGBA output
    uint8_t currentPalette[768] = {};

    void paletteConvert(const doom_frame_t* frame);

    DoomEngine(const DoomEngine&) = delete;
    DoomEngine& operator=(const DoomEngine&) = delete;
};
