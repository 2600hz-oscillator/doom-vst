#pragma once

#include <array>
#include <cstddef>

namespace patch
{

enum class SpriteCategory
{
    Character,
    Gun,
    Powerup,
    Armor,
};

struct SpriteEntry
{
    int id;                 // matches the SPR_* enum order in linuxdoom info.h
    const char* code;       // 4-letter Doom sprite code
    const char* niceName;   // user-facing label
    SpriteCategory category;
};

// Sprites known to be present in the shareware DOOM1.WAD (Episode 1 only).
// Doom 2 monsters/items, plasma rifle and BFG are excluded — those lumps
// don't exist in the shareware WAD and would render nothing.
constexpr std::array<SpriteEntry, 23> kSpriteCatalog = { {
    // Characters
    { 28, "PLAY", "Marine",          SpriteCategory::Character },
    {  0, "TROO", "Imp",             SpriteCategory::Character },
    { 29, "POSS", "Zombieman",       SpriteCategory::Character },
    { 30, "SPOS", "Shotgun Guy",     SpriteCategory::Character },
    { 39, "SARG", "Demon",           SpriteCategory::Character },
    { 40, "HEAD", "Cacodemon",       SpriteCategory::Character },
    { 42, "BOSS", "Baron of Hell",   SpriteCategory::Character },
    { 44, "SKUL", "Lost Soul",       SpriteCategory::Character },

    // Guns (pickup sprites)
    { 92, "SHOT", "Shotgun",         SpriteCategory::Gun },
    { 89, "CSAW", "Chainsaw",        SpriteCategory::Gun },
    { 88, "MGUN", "Chaingun",        SpriteCategory::Gun },
    { 90, "LAUN", "Rocket Launcher", SpriteCategory::Gun },

    // Powerups
    { 70, "SOUL", "Soulsphere",      SpriteCategory::Powerup },
    { 71, "PINV", "Invulnerability", SpriteCategory::Powerup },
    { 72, "PSTR", "Berserk",         SpriteCategory::Powerup },
    { 73, "PINS", "Blursphere",      SpriteCategory::Powerup },
    { 75, "SUIT", "Radiation Suit",  SpriteCategory::Powerup },
    { 76, "PMAP", "Computer Map",    SpriteCategory::Powerup },
    { 77, "PVIS", "Light Amp Visor", SpriteCategory::Powerup },

    // Armor
    { 55, "ARM1", "Green Armor",     SpriteCategory::Armor },
    { 56, "ARM2", "Blue Armor",      SpriteCategory::Armor },
    { 60, "BON1", "Health Bonus",    SpriteCategory::Armor },
    { 61, "BON2", "Armor Bonus",     SpriteCategory::Armor },
} };

inline const SpriteEntry* findSpriteById(int id)
{
    for (const auto& e : kSpriteCatalog)
        if (e.id == id) return &e;
    return nullptr;
}

inline const SpriteEntry* findSpriteByCode(const char* code)
{
    if (code == nullptr) return nullptr;
    for (const auto& e : kSpriteCatalog)
    {
        const char* a = e.code;
        const char* b = code;
        while (*a && *b && *a == *b) { ++a; ++b; }
        if (*a == '\0' && *b == '\0') return &e;
    }
    return nullptr;
}

} // namespace patch
