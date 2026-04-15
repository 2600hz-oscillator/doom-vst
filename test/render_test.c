// render_test.c — Standalone test harness for the DoomViz renderer library
//
// Initializes the Doom renderer, loads E1M1, renders a single frame,
// and dumps the framebuffer as a PPM image file.
//
// Usage: ./render_test <path_to_DOOM1.WAD> [output.ppm]

#include "doom_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Doom mobjtype_t values we need (from info.h)
#define MT_TROOP 3  // Imp

static void write_ppm(const char* filename, const doom_frame_t* frame)
{
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Cannot open %s for writing\n", filename);
        return;
    }

    fprintf(f, "P6\n%d %d\n255\n", frame->width, frame->height);

    for (int i = 0; i < frame->width * frame->height; i++) {
        uint8_t idx = frame->framebuffer[i];
        fputc(frame->palette[idx * 3 + 0], f);  // R
        fputc(frame->palette[idx * 3 + 1], f);  // G
        fputc(frame->palette[idx * 3 + 2], f);  // B
    }

    fclose(f);
    printf("Wrote %s (%dx%d)\n", filename, frame->width, frame->height);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <DOOM1.WAD> [output.ppm]\n", argv[0]);
        return 1;
    }

    const char* wad_path = argv[1];
    const char* output_path = argc > 2 ? argv[2] : "e1m1_test.ppm";

    printf("=== DoomViz Renderer Test ===\n\n");

    printf("Initializing Doom renderer with WAD: %s\n", wad_path);
    if (doom_init(wad_path) != 0) {
        fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }
    printf("Renderer initialized.\n");

    printf("Loading E1M1...\n");
    if (doom_load_map(1, 1) != 0) {
        fprintf(stderr, "Failed to load E1M1\n");
        doom_shutdown();
        return 1;
    }

    doom_map_info_t info = doom_get_map_info();
    printf("Map loaded: %d sectors, %d lines\n", info.num_sectors, info.num_lines);

    // Get player spawn position
    int32_t px, py, pz;
    uint32_t pangle;
    doom_get_player_pos(&px, &py, &pz, &pangle);
    printf("Player spawn: x=%d y=%d z=%d angle=0x%08x\n", px, py, pz, pangle);

    // Check outdoor sectors
    printf("Outdoor sectors: ");
    int outdoor_count = 0;
    for (int i = 0; i < info.num_sectors; i++) {
        if (doom_sector_is_outdoor(i)) {
            if (outdoor_count < 10)
                printf("%d ", i);
            outdoor_count++;
        }
    }
    if (outdoor_count > 10) printf("... ");
    printf("(%d total)\n", outdoor_count);

    // Render the initial frame
    printf("Rendering frame 1 (initial view)...\n");
    doom_frame_t* frame = doom_render_frame();
    if (frame == NULL) {
        fprintf(stderr, "Failed to render frame\n");
        doom_shutdown();
        return 1;
    }

    write_ppm(output_path, frame);

    // Spawn an imp in front of the player and render again
    printf("\nSpawning an imp 512 units ahead...\n");
    // Calculate position in front of player
    // In Doom fixed-point: 512 map units = 512 << 16
    int32_t imp_x = px + 512 * DOOM_FRACUNIT;
    int32_t imp_y = py;
    int imp = doom_spawn_thing(imp_x, imp_y, MT_TROOP);
    if (imp >= 0) {
        printf("Imp spawned (handle=%d)\n", imp);

        // Advance a tick so the imp's state initializes
        doom_tick();

        // Render with imp visible
        frame = doom_render_frame();
        if (frame != NULL) {
            char imp_path[256];
            snprintf(imp_path, sizeof(imp_path), "%s", "e1m1_with_imp.ppm");
            write_ppm(imp_path, frame);
        }

        // Clean up imp
        doom_remove_thing(imp);
    } else {
        printf("Failed to spawn imp\n");
    }

    doom_shutdown();
    printf("\nDone. Check the PPM output files.\n");
    return 0;
}
