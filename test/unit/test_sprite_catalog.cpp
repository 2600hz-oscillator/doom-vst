#include "patch/SpriteCatalog.h"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <string>

TEST_CASE("SpriteCatalog: findSpriteById returns the entry for known IDs", "[sprite_catalog]")
{
    using namespace patch;
    REQUIRE(findSpriteById(28) != nullptr);
    REQUIRE(std::string(findSpriteById(28)->code) == "PLAY");
    REQUIRE(findSpriteById(0)->category == SpriteCategory::Character);
    REQUIRE(findSpriteById(92)->category == SpriteCategory::Gun);
    REQUIRE(findSpriteById(70)->category == SpriteCategory::Powerup);
    REQUIRE(findSpriteById(55)->category == SpriteCategory::Armor);
}

TEST_CASE("SpriteCatalog: findSpriteById returns nullptr for unknown IDs", "[sprite_catalog]")
{
    using namespace patch;
    REQUIRE(findSpriteById(-1) == nullptr);
    REQUIRE(findSpriteById(9999) == nullptr);
}

TEST_CASE("SpriteCatalog: findSpriteByCode lookups", "[sprite_catalog]")
{
    using namespace patch;
    REQUIRE(findSpriteByCode("TROO") != nullptr);
    REQUIRE(findSpriteByCode("TROO")->id == 0);
    REQUIRE(findSpriteByCode("PLAY")->id == 28);
    REQUIRE(findSpriteByCode("XXXX") == nullptr);
    REQUIRE(findSpriteByCode(nullptr) == nullptr);
}

TEST_CASE("SpriteCatalog: every entry has a unique ID and 4-character code",
          "[sprite_catalog]")
{
    using namespace patch;
    std::set<int> ids;
    for (const auto& e : kSpriteCatalog)
    {
        REQUIRE(ids.insert(e.id).second); // duplicate IDs would collide in lookups
        REQUIRE(std::string(e.code).length() == 4);
        REQUIRE(e.niceName != nullptr);
        REQUIRE(*e.niceName != '\0');
    }
}
